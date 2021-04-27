#include <stdexcept>
#include <iostream>

#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/epoll.h>

#include <gd100/program_terminal_manager.hpp>

namespace gd100 {

program_terminal_manager::program_terminal_manager()
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

                char read_buffer[1024];
                auto read_count = read(event.data.fd, read_buffer, sizeof(read_buffer));

                program->handle_bytes(read_buffer, read_count);
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
