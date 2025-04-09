// File: src/Network.cpp
#include "Network.h"
#include "Logger.h"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
typedef int socklen_t;
#undef ERROR // Avoid conflict with Windows macro
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

#include <cstring>
#include <string>
#include <vector>

namespace dtq
{

    //------------------------------------------------------------------------------
    // Network-level functions
    //------------------------------------------------------------------------------
#ifdef _WIN32
    Network::Connection::Connection(SOCKET acceptedSocket)
    {
        socketDescriptor = acceptedSocket;
        serverAddress = "";
        serverPort = 0;
        lastError = "";
    }
#else
    Network::Connection::Connection(int acceptedSocket)
    {
        socketDescriptor = acceptedSocket;
        serverAddress = "";
        serverPort = 0;
        lastError = "";
    }
#endif
    bool Network::initialize()
    {
#ifdef _WIN32
        WSADATA wsaData;
        int res = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (res != 0)
        {
            Logger::getInstance().log(LogLevel::ERR, "WSAStartup failed with error: " + std::to_string(res));
            return false;
        }
#endif
        return true;
    }

    void Network::cleanup()
    {
#ifdef _WIN32
        WSACleanup();
#endif
    }

    //------------------------------------------------------------------------------
    // Network::Connection implementations
    //------------------------------------------------------------------------------

    Network::Connection::Connection(const std::string &address, int port)
        : serverAddress(address), serverPort(port)
    {
#ifdef _WIN32
        socketDescriptor = INVALID_SOCKET;
#else
        socketDescriptor = -1;
#endif
    }

    Network::Connection::~Connection()
    {
        disconnect();
    }

    bool Network::Connection::connect()
    {
#ifdef _WIN32
        socketDescriptor = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (socketDescriptor == INVALID_SOCKET)
        {
            lastError = "Socket creation failed.";
            return false;
        }
#else
        socketDescriptor = ::socket(AF_INET, SOCK_STREAM, 0);
        if (socketDescriptor < 0)
        {
            lastError = "Socket creation failed.";
            return false;
        }
#endif

        sockaddr_in serverInfo;
        std::memset(&serverInfo, 0, sizeof(serverInfo));
        serverInfo.sin_family = AF_INET;
        serverInfo.sin_port = htons(serverPort);
        serverInfo.sin_addr.s_addr = inet_addr(serverAddress.c_str());

#ifdef _WIN32
        if (::connect(socketDescriptor, reinterpret_cast<sockaddr *>(&serverInfo), sizeof(serverInfo)) == SOCKET_ERROR)
        {
            lastError = "Connect failed.";
            closesocket(socketDescriptor);
            socketDescriptor = INVALID_SOCKET;
            return false;
        }
#else
        if (::connect(socketDescriptor, reinterpret_cast<sockaddr *>(&serverInfo), sizeof(serverInfo)) < 0)
        {
            lastError = "Connect failed.";
            close(socketDescriptor);
            socketDescriptor = -1;
            return false;
        }
#endif
        return true;
    }

    void Network::Connection::disconnect()
    {
#ifdef _WIN32
        if (socketDescriptor != INVALID_SOCKET)
        {
            closesocket(socketDescriptor);
            socketDescriptor = INVALID_SOCKET;
        }
#else
        if (socketDescriptor >= 0)
        {
            close(socketDescriptor);
            socketDescriptor = -1;
        }
#endif
    }

    bool Network::Connection::send(const std::vector<char> &data)
    {
        int totalSent = 0;
        int dataSize = static_cast<int>(data.size());
        while (totalSent < dataSize)
        {
            int sent = ::send(socketDescriptor, data.data() + totalSent, dataSize - totalSent, 0);
            if (sent <= 0)
            {
                lastError = "Send failed.";
                return false;
            }
            totalSent += sent;
        }
        return true;
    }

    bool Network::Connection::receive(std::vector<char> &buffer)
    {
        const int bufferSize = 1024;
        char tempBuffer[bufferSize];
        int received = ::recv(socketDescriptor, tempBuffer, bufferSize, 0);
        if (received <= 0)
        {
            lastError = "Receive failed or connection closed.";
            return false;
        }
        buffer.assign(tempBuffer, tempBuffer + received);
        return true;
    }

    std::string Network::Connection::getLastError() const
    {
        return lastError;
    }

} // namespace dtq
