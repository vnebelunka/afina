#include "Connection.h"

#include <sys/uio.h>
#include <iostream>

namespace Afina {
namespace Network {
namespace MTnonblock {

// See Connection.h
void Connection::Start() {
    _event.events |= EPOLLIN;
    _event.events |= EPOLLET;
}

// See Connection.h
void Connection::OnError() { isAlive = false; }

// See Connection.h
void Connection::OnClose() {
    isAlive = false;
    _logger->debug("Connection closed: {}", _socket);
}

// See Connection.h
void Connection::DoRead() {
    std::unique_lock<std::mutex> lock(connection_mutex);
    try {
        while ((readed_bytes = read(_socket, client_buffer + readed_bytes, sizeof(client_buffer))) > 0) { // Edge triggered
            _logger->warn("Got {} bytes from socket", readed_bytes);

            // Single block of data readed from the socket could trigger inside actions a multiple times,
            // for example:
            // - read#0: [<command1 start>]
            // - read#1: [<command1 end> <argument> <command2> <argument for command 2> <command3> ... ]
            while (readed_bytes > 0) {
                _logger->debug("Process {} bytes", readed_bytes);
                // There is no command yet
                if (!command_to_execute) {
                    std::size_t parsed = 0;
                    if (parser.Parse(client_buffer, readed_bytes, parsed)) {
                        // There is no command to be launched, continue to parse input stream
                        // Here we are, current chunk finished some command, process it
                        _logger->debug("Found new command: {} in {} bytes", parser.Name(), parsed);
                        command_to_execute = parser.Build(arg_remains);
                        if (arg_remains > 0) {
                            arg_remains += 2;
                        }
                    }

                    // Parsed might fails to consume any bytes from input stream. In real life that could happens,
                    // for example, because we are working with UTF-16 chars and only 1 byte left in stream
                    if (parsed == 0) {
                        break;
                    } else {
                        std::memmove(client_buffer, client_buffer + parsed, readed_bytes - parsed);
                        readed_bytes -= parsed;
                    }
                }

                // There is command, but we still wait for argument to arrive...
                if (command_to_execute && arg_remains > 0) {
                    _logger->debug("Fill argument: {} bytes of {}", readed_bytes, arg_remains);
                    // There is some parsed command, and now we are reading argument
                    std::size_t to_read = std::min(arg_remains, std::size_t(readed_bytes));
                    argument_for_command.append(client_buffer, to_read);

                    std::memmove(client_buffer, client_buffer + to_read, readed_bytes - to_read);
                    arg_remains -= to_read;
                    readed_bytes -= to_read;
                }

                // Thre is command & argument - RUN!
                if (command_to_execute && arg_remains == 0) {
                    _logger->debug("Start command execution");

                    std::string result;
                    if (argument_for_command.size()) {
                        argument_for_command.resize(argument_for_command.size() - 2);
                    }
                    command_to_execute->Execute(*pStorage, argument_for_command, result);

                    // Send response
                    result += "\r\n";
                    if(write_queue.size() < queue_size) {
                        write_queue.push_back(result);
                    } else {
                        _logger->error("queue is full on descriptor {}", _socket);
                    }

                    // Prepare for the next command
                    command_to_execute.reset();
                    argument_for_command.resize(0);
                    parser.Reset();
                }
            } // while (readed_bytes)

        } // while(read)
        if(!write_queue.empty()) {
            _event.events |= EPOLLOUT;
        }
        _event.events &= ~ EPOLLONESHOT;
        if(readed_bytes < 0 && errno != EWOULDBLOCK){
            readed_bytes = 0;
            _logger->error("failed to read: {}", errno);
        }
    } catch(std::runtime_error &ex) {
        _logger->error("Failed to process connection on descriptor {}: {}", _socket, ex.what());
        OnError();
    }
}

// See Connection.h
void Connection::DoWrite() {
    _logger->debug("Start write on descriptor {}", _socket);
    const int iovec_size = 1000;
    iovec data[iovec_size] = {};
    int i = 0;
    {
        auto it = write_queue.begin();
        data[i].iov_base = &((*it)[0]) + head_offset;
        data[i].iov_len = it->size() - head_offset;
        ++i, ++it;
        for(; it != write_queue.end() && i < iovec_size; ++it){
            data[i].iov_base = &((*it)[0]);
            data[i].iov_len = it->size();
            ++i;
        }
    }
    int writen = 0;
    if((writen = writev(_socket, data, i)) > 0) {
        head_offset += writen;
        auto it = write_queue.begin();
        for(; it->size() <= head_offset; ++i){
            head_offset -= it->size();
        }
        write_queue.erase(write_queue.begin(), ++it);
    }else if(writen == -1){
        if(errno != EWOULDBLOCK){
            _logger->error("Error writing descriptor {}: {}", _socket, errno);
            OnError();
        }
        _logger->debug("Stop write on descriptor {}", _socket);
        return;
    }
    if(write_queue.empty()) {
        _event.events &= ~EPOLLOUT;
    }
    _event.events &= ~ EPOLLONESHOT;
}

} // namespace MTnonblock
} // namespace Network
} // namespace Afina
