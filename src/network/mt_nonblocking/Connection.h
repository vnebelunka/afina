#ifndef AFINA_NETWORK_MT_NONBLOCKING_CONNECTION_H
#define AFINA_NETWORK_MT_NONBLOCKING_CONNECTION_H

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

#include <sys/epoll.h>

namespace Afina {
namespace Network {
namespace MTnonblock {

class Connection {
public:
    Connection(int s, std::shared_ptr<spdlog::logger> my_logger, std::shared_ptr<Afina::Storage> my_pstorage, int queue_size = 1000)
        : _socket(s), queue_size(queue_size){
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
    friend class Worker;
    friend class ServerImpl;

    std::shared_ptr<Afina::Storage> pStorage;
    std::shared_ptr<spdlog::logger> _logger;
    std::size_t arg_remains;
    Protocol::Parser parser;
    std::string argument_for_command;
    std::unique_ptr<Execute::Command> command_to_execute;

    int _socket;
    struct epoll_event _event;

    std::deque<std::string> write_queue;
    const int queue_size = 1000;
    std::mutex connection_mutex;
    size_t head_offset = 0;

    char client_buffer[4096];
    int readed_bytes = 0;
};

} // namespace MTnonblock
} // namespace Network
} // namespace Afina

#endif // AFINA_NETWORK_MT_NONBLOCKING_CONNECTION_H
