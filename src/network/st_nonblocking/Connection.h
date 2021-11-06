#ifndef AFINA_NETWORK_ST_NONBLOCKING_CONNECTION_H
#define AFINA_NETWORK_ST_NONBLOCKING_CONNECTION_H

#include <cstring>
#include <string>
#include <deque>

#include <memory>
#include <spdlog/logger.h>
#include <sys/epoll.h>

#include <queue>

#include <spdlog/logger.h>

#include <afina/Storage.h>
#include <afina/execute/Command.h>
#include <afina/logging/Service.h>

#include "protocol/Parser.h"


namespace Afina {
namespace Network {
namespace STnonblock {

class Connection {
public:
    Connection(int s, std::shared_ptr<spdlog::logger> my_logger, std::shared_ptr<Afina::Storage> my_pstorage, int _max_size = 1000)
        : _socket(s),_max_size(_max_size) {
        std::memset(&_event, 0, sizeof(struct epoll_event));
        _event.data.ptr = this;
        _logger = my_logger;
        pStorage = my_pstorage;
    }

    bool isAlive = true;

    void Start();

protected:
    void OnError();
    void OnClose();
    void DoRead();
    void DoWrite();

private:
    friend class ServerImpl;

    std::shared_ptr<Afina::Storage> pStorage;
    std::shared_ptr<spdlog::logger> _logger;
    std::size_t arg_remains;
    Protocol::Parser parser;
    std::string argument_for_command;
    std::unique_ptr<Execute::Command> command_to_execute;


    int _socket;
    struct epoll_event _event;

    int readed_bytes = 0;
    char client_buffer[4096];


    std::deque<std::string> write_queue;
    int _max_size;
    size_t head_offset = 0;

};

} // namespace STnonblock
} // namespace Network
} // namespace Afina

#endif // AFINA_NETWORK_ST_NONBLOCKING_CONNECTION_H
