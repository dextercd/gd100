#ifndef GDTERM_PROGRAM_TERMINAL_MANAGER_HPP
#define GDTERM_PROGRAM_TERMINAL_MANAGER_HPP

#include <thread>
#include <mutex>
#include <unordered_map>
#include <atomic>
#include <memory>

#include "program.hpp"

namespace gd100 {

class program_terminal_manager {
public:
    program_terminal_manager();
    program_terminal_manager(program_terminal_manager&&)=delete;

    program* register_program(int fid, std::unique_ptr<program> prg);

    ~program_terminal_manager();

private:
    program* get_program(int fid);
    void controller_loop();
    void unregister_program(int fid);

private:
    std::thread controller;
    std::mutex mutex;

    int controller_write;
    int controller_read;

    int epoll_handle;

    std::unordered_map<int, std::unique_ptr<program>> registered;
    std::atomic<bool> stopping = false;
    std::unique_ptr<char[]> read_buffer;
};

} // gd100::

#endif // header guard
