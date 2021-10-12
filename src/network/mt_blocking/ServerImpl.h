#ifndef AFINA_NETWORK_MT_BLOCKING_SERVER_H
#define AFINA_NETWORK_MT_BLOCKING_SERVER_H

#include <atomic>
#include <thread>

#include <afina/network/Server.h>
#include <mutex>
#include <unordered_set>
#include <condition_variable>

namespace spdlog {
class logger;
}

namespace Afina {
namespace Network {
namespace MTblocking {

/**
 * # Network resource manager implementation
 * Server that is spawning a separate thread for each connection
 */
class ServerImpl : public Server {
public:
    ServerImpl(std::shared_ptr<Afina::Storage> ps, std::shared_ptr<Logging::Service> pl);
    ~ServerImpl();

    // See Server.h
    void Start(uint16_t port, uint32_t, uint32_t) override;

    // See Server.h
    void Stop() override;

    // See Server.h
    void Join() override;

protected:
    /**
     * Method is running in the connection acceptor thread
     */
    void OnRun();

    // Worker with client
    void Worker(int client_socket);

private:
    // Logger instance
    std::shared_ptr<spdlog::logger> _logger;

    // Atomic flag to notify threads when it is time to stop. Note that
    // flag must be atomic in order to safely publisj changes cross thread
    // bounds
    std::atomic<bool> running;

    // lock for onRun
    std::mutex _lock_serv;

    // checks if all client sockets are closed
    std::condition_variable sockets_closed;

    // Server socket to accept connections on
    int _server_socket;

    //set of working sockets.
    std::unordered_set<int> socket_set;

    // num of max workers on server
    uint32_t _n_workers;

};

} // namespace MTblocking
} // namespace Network
} // namespace Afina

#endif // AFINA_NETWORK_MT_BLOCKING_SERVER_H
