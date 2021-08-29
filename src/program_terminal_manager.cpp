#include <stdexcept>
#include <iostream>
#include <cstdio>
#include <chrono>

#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <poll.h>

#include "program_terminal_manager.hpp"

using namespace std::chrono_literals;

namespace gd100 {

constexpr std::size_t read_buffer_size = BUFSIZ;

program_terminal_manager::program_terminal_manager()
    : read_buffer{new char[read_buffer_size]}
{
    int pipe_descriptors[2];
    if (pipe2(pipe_descriptors, O_CLOEXEC))
        throw std::runtime_error{"Couldn't create controller communication pipe."};

    controller_read = pipe_descriptors[0];
    controller_write = pipe_descriptors[1];

    epoll_handle = epoll_create(EPOLL_CLOEXEC);
    if (epoll_handle < 0)
        throw std::runtime_error{"Couldn't create epoll handle."};

    epoll_data data;
    data.fd = controller_read;

    epoll_event controller_event_spec{
        EPOLLIN,
        data
    };

    if (epoll_ctl(epoll_handle, EPOLL_CTL_ADD, controller_read, &controller_event_spec))
        throw std::runtime_error{"Couldn't add controller read to epoll."};

    controller = std::thread{[this] { controller_loop(); }};
}

program* program_terminal_manager::get_program(int fid)
{
    auto lock = std::scoped_lock{mutex};

    auto it = registered.find(fid);
    if (it == registered.end())
        return nullptr;

    return it->second.get();
}

program* program_terminal_manager::register_program(int fid, std::unique_ptr<program> prg)
{
    auto ret = prg.get();

    {
        auto lock = std::scoped_lock{mutex};
        registered[fid] = std::move(prg);
    }

    epoll_data data;
    data.fd = fid;

    epoll_event program_event_spec{
        EPOLLIN,
        data
    };

    if (epoll_ctl(epoll_handle, EPOLL_CTL_ADD, fid, &program_event_spec))
        throw std::runtime_error{"Couldn't add program read to epoll."};

    return ret;
}

void program_terminal_manager::unregister_program(int fid)
{
    epoll_data data;
    data.fd = fid;
    epoll_event program_event_spec{{}, data};

    if (epoll_ctl(epoll_handle, EPOLL_CTL_DEL, fid, &program_event_spec))
        throw std::runtime_error{"Couldn't remove program read to epoll."};
}

void program_terminal_manager::controller_loop()
{
    while(!stopping) {
        epoll_event event;
        auto const poll_result = epoll_wait(epoll_handle, &event, 1, 1000);

        if (poll_result == -1)
            throw std::runtime_error{"epoll_wait failed."};

        if (poll_result == 1) {
            if (event.data.fd == controller_read)
                continue;

            if (event.events & EPOLLIN) {
                auto program = get_program(event.data.fd);
                if (!program)
                    continue;

                // As long as there's input we keep reading for ~1 frame
                constexpr auto parse_max_duration = 16.66ms;
                auto const parse_start = std::chrono::steady_clock::now();

                // We read some input and indicate to the processor whether more
                // input is expected.  This way the processor can wait before
                // displaying the data or doing some other expensive operation.
                auto has_input = true;
                for (int i = 0; has_input; ++i) {
                    auto const read_count = read(event.data.fd, read_buffer.get(), read_buffer_size);
                    if (read_count == -1)
                        break;

                    if (i == 0) {
                        // There's a large likelyhood we'll get more input,
                        // so we sleep for a little bit before doing the 'more input' check.
                        std::this_thread::sleep_for(std::chrono::milliseconds(2));
                    }

                    pollfd poll_has_input;
                    poll_has_input.fd = event.data.fd;
                    poll_has_input.events = POLLIN;
                    auto const pollres = poll(&poll_has_input, 1, 0);
                    has_input = pollres == 1;

                    program->handle_bytes(read_buffer.get(), read_count, has_input);

                    auto const parse_now = std::chrono::steady_clock::now();
                    if ((parse_now - parse_start) > parse_max_duration)
                        break;
                }

                // Even though more data is in the file descriptor we're not
                // going to extract it immediately.  This call gives the
                // processor an opportunity to flush.
                if (has_input)
                    program->handle_bytes(nullptr, 0, false);
            }

            if (event.events & EPOLLHUP) {
                unregister_program(event.data.fd);
            }
        }

    }
}

program_terminal_manager::~program_terminal_manager()
{
    stopping = true;
    write(controller_write, "w", 1); // wake up the controller thread
    controller.join();
}

} // gd100::
