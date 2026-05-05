#include "sys_net.h"
#include <cstring>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

namespace com {
namespace arenax3 {

// Socket class implementation
Socket::Socket() : m_socket(-1), m_isConnected(false) {}

Socket::~Socket() {
    close();
}

bool Socket::create(SocketType type, SocketProtocol protocol) {
    int sockType = (type == SocketType::TCP) ? SOCK_STREAM : SOCK_DGRAM;
    int sockProto = (protocol == SocketProtocol::IPv4) ? AF_INET : AF_INET6;
    
    m_socket = ::socket(sockProto, sockType, 0);
    if (m_socket < 0) {
        m_lastError = errno;
        return false;
    }
    
    int opt = 1;
    setsockopt(m_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    return true;
}

bool Socket::bind(uint16_t port, const char* address) {
    if (m_socket < 0) return false;
    
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    
    if (address == nullptr || strcmp(address, "0.0.0.0") == 0) {
        addr.sin_addr.s_addr = INADDR_ANY;
    } else {
        if (inet_pton(AF_INET, address, &addr.sin_addr) <= 0) {
            m_lastError = errno;
            return false;
        }
    }
    
    if (::bind(m_socket, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        m_lastError = errno;
        return false;
    }
    
    return true;
}

bool Socket::listen(int backlog) {
    if (m_socket < 0) return false;
    
    if (::listen(m_socket, backlog) < 0) {
        m_lastError = errno;
        return false;
    }
    
    return true;
}

Socket* Socket::accept() {
    if (m_socket < 0) return nullptr;
    
    struct sockaddr_in clientAddr;
    socklen_t addrLen = sizeof(clientAddr);
    
    int clientSocket = ::accept(m_socket, (struct sockaddr*)&clientAddr, &addrLen);
    if (clientSocket < 0) {
        m_lastError = errno;
        return nullptr;
    }
    
    Socket* client = new Socket();
    client->m_socket = clientSocket;
    client->m_isConnected = true;
    
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &clientAddr.sin_addr, ip, INET_ADDRSTRLEN);
    client->m_remoteAddress = ip;
    client->m_remotePort = ntohs(clientAddr.sin_port);
    
    return client;
}

bool Socket::connect(const char* address, uint16_t port) {
    if (m_socket < 0) return false;
    
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    
    if (inet_pton(AF_INET, address, &addr.sin_addr) <= 0) {
        struct hostent* host = gethostbyname(address);
        if (host == nullptr) {
            m_lastError = errno;
            return false;
        }
        memcpy(&addr.sin_addr, host->h_addr, host->h_length);
    }
    
    if (::connect(m_socket, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        m_lastError = errno;
        return false;
    }
    
    m_isConnected = true;
    m_remoteAddress = address;
    m_remotePort = port;
    return true;
}

int Socket::send(const void* data, size_t size, int flags) {
    if (m_socket < 0 || !m_isConnected) return -1;
    
    int result = ::send(m_socket, data, size, flags);
    if (result < 0) {
        m_lastError = errno;
    }
    return result;
}

int Socket::receive(void* buffer, size_t size, int flags) {
    if (m_socket < 0) return -1;
    
    int result = ::recv(m_socket, buffer, size, flags);
    if (result < 0) {
        m_lastError = errno;
    }
    return result;
}

int Socket::sendTo(const void* data, size_t size, const char* address, uint16_t port) {
    if (m_socket < 0) return -1;
    
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    
    if (inet_pton(AF_INET, address, &addr.sin_addr) <= 0) {
        m_lastError = errno;
        return -1;
    }
    
    int result = ::sendto(m_socket, data, size, 0, (struct sockaddr*)&addr, sizeof(addr));
    if (result < 0) {
        m_lastError = errno;
    }
    return result;
}

int Socket::receiveFrom(void* buffer, size_t size, std::string& address, uint16_t& port) {
    if (m_socket < 0) return -1;
    
    struct sockaddr_in addr;
    socklen_t addrLen = sizeof(addr);
    
    int result = ::recvfrom(m_socket, buffer, size, 0, (struct sockaddr*)&addr, &addrLen);
    if (result < 0) {
        m_lastError = errno;
        return -1;
    }
    
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &addr.sin_addr, ip, INET_ADDRSTRLEN);
    address = ip;
    port = ntohs(addr.sin_port);
    
    return result;
}

void Socket::close() {
    if (m_socket >= 0) {
        ::close(m_socket);
        m_socket = -1;
        m_isConnected = false;
    }
}

void Socket::setNonBlocking(bool nonBlocking) {
    if (m_socket < 0) return;
    
    int flags = fcntl(m_socket, F_GETFL, 0);
    if (nonBlocking) {
        fcntl(m_socket, F_SETFL, flags | O_NONBLOCK);
    } else {
        fcntl(m_socket, F_SETFL, flags & ~O_NONBLOCK);
    }
}

int Socket::getLastError() const {
    return m_lastError;
}

// NetworkManager class implementation
NetworkManager::NetworkManager() : m_initialized(false) {}

NetworkManager::~NetworkManager() {
    shutdown();
}

bool NetworkManager::initialize() {
    m_initialized = true;
    return true;
}

void NetworkManager::shutdown() {
    for (auto& socket : m_sockets) {
        if (socket) {
            socket->close();
            delete socket;
        }
    }
    m_sockets.clear();
    m_initialized = false;
}

Socket* NetworkManager::createSocket(SocketType type, SocketProtocol protocol) {
    if (!m_initialized) return nullptr;
    
    Socket* socket = new Socket();
    if (!socket->create(type, protocol)) {
        delete socket;
        return nullptr;
    }
    
    m_sockets.push_back(socket);
    return socket;
}

void NetworkManager::destroySocket(Socket* socket) {
    if (!socket) return;
    
    auto it = std::find(m_sockets.begin(), m_sockets.end(), socket);
    if (it != m_sockets.end()) {
        (*it)->close();
        delete *it;
        m_sockets.erase(it);
    }
}

bool NetworkManager::sendPacket(Socket* socket, const NetworkPacket& packet) {
    if (!socket || !socket->isConnected()) return false;
    
    // Serialize packet: [type(1)][size(4)][data]
    size_t totalSize = 1 + sizeof(uint32_t) + packet.data.size();
    std::vector<uint8_t> buffer(totalSize);
    
    buffer[0] = static_cast<uint8_t>(packet.type);
    uint32_t dataSize = htonl(static_cast<uint32_t>(packet.data.size()));
    memcpy(buffer.data() + 1, &dataSize, sizeof(uint32_t));
    memcpy(buffer.data() + 5, packet.data.data(), packet.data.size());
    
    int result = socket->send(buffer.data(), buffer.size());
    return result == static_cast<int>(totalSize);
}

bool NetworkManager::receivePacket(Socket* socket, NetworkPacket& packet) {
    if (!socket) return false;
    
    // Read packet type
    uint8_t type;
    int result = socket->receive(&type, 1);
    if (result != 1) return false;
    
    // Read data size
    uint32_t dataSize;
    result = socket->receive(&dataSize, sizeof(uint32_t));
    if (result != sizeof(uint32_t)) return false;
    dataSize = ntohl(dataSize);
    
    if (dataSize > 65536) return false; // Sanity check
    
    // Read data
    packet.data.resize(dataSize);
    size_t received = 0;
    while (received < dataSize) {
        result = socket->receive(packet.data.data() + received, dataSize - received);
        if (result <= 0) return false;
        received += result;
    }
    
    packet.type = static_cast<PacketType>(type);
    return true;
}

bool NetworkManager::sendToAddress(const char* address, uint16_t port, const NetworkPacket& packet) {
    // For UDP broadcast/client usage
    Socket tempSocket;
    if (!tempSocket.create(SocketType::UDP, SocketProtocol::IPv4)) {
        return false;
    }
    
    size_t totalSize = 1 + sizeof(uint32_t) + packet.data.size();
    std::vector<uint8_t> buffer(totalSize);
    
    buffer[0] = static_cast<uint8_t>(packet.type);
    uint32_t dataSize = htonl(static_cast<uint32_t>(packet.data.size()));
    memcpy(buffer.data() + 1, &dataSize, sizeof(uint32_t));
    memcpy(buffer.data() + 5, packet.data.data(), packet.data.size());
    
    int result = tempSocket.sendTo(buffer.data(), buffer.size(), address, port);
    tempSocket.close();
    
    return result == static_cast<int>(totalSize);
}

} // namespace arenax3
} // namespace com