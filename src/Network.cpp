#include "Network.h"
#include "Logger.h"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
typedef int socklen_t;
#undef ERROR
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

    bool Network::initialize()
    {
#ifdef _WIN32
        WSADATA wsaData;
        return WSAStartup(MAKEWORD(2, 2), &wsaData) == 0;
#else
        return true;
#endif
    }

    void Network::cleanup()
    {
#ifdef _WIN32
        WSACleanup();
#endif
    }

    void Network::Connection::disconnect()
    {
        if (socketDescriptor != INVALID_SOCKET)
        {
#ifdef _WIN32
            closesocket(socketDescriptor);
#else
            close(socketDescriptor);
#endif
            socketDescriptor = INVALID_SOCKET;
        }
    }

    bool Network::Connection::connect()
    {
        if (socketDescriptor != INVALID_SOCKET)
        {
            lastError = "Already connected";
            return false;
        }

        socketDescriptor = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (socketDescriptor == INVALID_SOCKET)
        {
            lastError = "Failed to create socket";
            return false;
        }

        // Set keep-alive to detect disconnections
        BOOL keepAlive = TRUE;
        if (setsockopt(socketDescriptor, SOL_SOCKET, SO_KEEPALIVE, (char*)&keepAlive, sizeof(keepAlive)) == SOCKET_ERROR)
        {
            lastError = "Failed to set keep-alive";
            closesocket(socketDescriptor);
            socketDescriptor = INVALID_SOCKET;
            return false;
        }

        // Set receive timeout to 10 seconds (increased from 5)
        DWORD timeout = 10000; // 10 seconds
        if (setsockopt(socketDescriptor, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout)) == SOCKET_ERROR)
        {
            lastError = "Failed to set receive timeout";
            closesocket(socketDescriptor);
            socketDescriptor = INVALID_SOCKET;
            return false;
        }

        // Set send timeout to 10 seconds
        if (setsockopt(socketDescriptor, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeout, sizeof(timeout)) == SOCKET_ERROR)
        {
            lastError = "Failed to set send timeout";
            closesocket(socketDescriptor);
            socketDescriptor = INVALID_SOCKET;
            return false;
        }

        // Disable Nagle's algorithm for better responsiveness
        BOOL noDelay = TRUE;
        if (setsockopt(socketDescriptor, IPPROTO_TCP, TCP_NODELAY, (char*)&noDelay, sizeof(noDelay)) == SOCKET_ERROR)
        {
            lastError = "Failed to disable Nagle's algorithm";
            closesocket(socketDescriptor);
            socketDescriptor = INVALID_SOCKET;
            return false;
        }

        sockaddr_in addr;
        ZeroMemory(&addr, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(serverPort);
        addr.sin_addr.s_addr = inet_addr(serverAddress.c_str());

        if (::connect(socketDescriptor, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR)
        {
            lastError = "Connect failed: " + std::to_string(WSAGetLastError());
            closesocket(socketDescriptor);
            socketDescriptor = INVALID_SOCKET;
            return false;
        }

        return true;
    }

    bool Network::Connection::send(const char* data, int size)
    {
        int totalSent = 0;
        int remainingBytes = size;
        int maxRetries = 3;
        int retries = 0;

        while (totalSent < size)
        {
            int sent = ::send(socketDescriptor, data + totalSent, remainingBytes, 0);
            if (sent <= 0)
            {
                // Check if we should retry
                if (retries < maxRetries)
                {
                    retries++;
                    // Small delay before retry
                    Sleep(100);
                    continue;
                }
                
                lastError = "Send failed: " + std::to_string(WSAGetLastError());
                return false;
            }
            
            totalSent += sent;
            remainingBytes -= sent;
            retries = 0; // Reset retry counter on successful send
        }
        return true;
    }

    bool Network::Connection::recvAll(char* buffer, int size)
    {
        int totalReceived = 0;
        int remainingBytes = size;
        int maxRetries = 3;
        int retries = 0;

        while (totalReceived < size)
        {
            int received = ::recv(socketDescriptor, buffer + totalReceived, remainingBytes, 0);
            if (received <= 0)
            {
                // Check if we should retry
                if (retries < maxRetries && received == 0)
                {
                    retries++;
                    // Small delay before retry
                    Sleep(100);
                    continue;
                }
                
                lastError = "Receive failed or connection closed: " + std::to_string(WSAGetLastError());
                return false;
            }
            
            totalReceived += received;
            remainingBytes -= received;
            retries = 0; // Reset retry counter on successful receive
        }
        return true;
    }

    bool Network::Connection::sendMessage(MessageType type, const std::string& payload)
    {
        // First send the message type
        if (!send((const char*)&type, sizeof(type)))
        {
            return false;
        }

        // Then send the payload size
        int size = static_cast<int>(payload.size());
        if (!send((const char*)&size, sizeof(size)))
        {
            return false;
        }

        // Finally send the payload if there is one
        if (size > 0)
        {
            if (!send(payload.c_str(), size))
            {
                return false;
            }
        }

        return true;
    }

    bool Network::Connection::receiveMessage(MessageType& type, std::string& payload)
    {
        // First receive the message type
        if (!recvAll((char*)&type, sizeof(type)))
        {
            return false;
        }

        // Then receive the payload size
        int size;
        if (!recvAll((char*)&size, sizeof(size)))
        {
            return false;
        }

        // Finally receive the payload if there is one
        if (size > 0)
        {
            std::vector<char> buffer(size);
            if (!recvAll(buffer.data(), size))
            {
                return false;
            }
            payload.assign(buffer.begin(), buffer.end());
        }
        else
        {
            payload.clear();
        }

        return true;
    }

} // namespace dtq
