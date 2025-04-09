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
        int res = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (res != 0)
        {
            Logger::getInstance().log(LogLevel::ERR,
                                      "WSAStartup failed with error: " + std::to_string(res));
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

// ------------------------------------------------------------------
#ifdef _WIN32
    Network::Connection::Connection(SOCKET acceptedSocket)
        : socketDescriptor(acceptedSocket), serverAddress(""), serverPort(0), lastError("")
    {
    }
#else
    Network::Connection::Connection(int acceptedSocket)
        : socketDescriptor(acceptedSocket), serverAddress(""), serverPort(0), lastError("")
    {
    }
#endif

    Network::Connection::Connection(const std::string &address, int port)
        : serverAddress(address), serverPort(port), lastError("")
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
        return sendAll(data.data(), static_cast<int>(data.size()));
    }

    bool Network::Connection::receive(std::vector<char> &buffer)
    {
        const int buffSize = 1024;
        char temp[buffSize];
        int recvd = ::recv(socketDescriptor, temp, buffSize, 0);
        if (recvd <= 0)
        {
            lastError = "Receive failed or connection closed.";
            return false;
        }
        buffer.assign(temp, temp + recvd);
        return true;
    }

    bool Network::Connection::sendMessage(MessageType type, const std::string &payload)
    {
        int msgType = static_cast<int>(type);
        int len = static_cast<int>(payload.size());

        // [4 bytes: msgType][4 bytes: length][payload bytes]
        std::vector<char> buffer(8 + len);
        std::memcpy(buffer.data(), &msgType, 4);
        std::memcpy(buffer.data() + 4, &len, 4);
        std::memcpy(buffer.data() + 8, payload.data(), len);

        return sendAll(buffer.data(), static_cast<int>(buffer.size()));
    }

    bool Network::Connection::receiveMessage(MessageType &outType, std::string &outPayload)
    {
        // first read 8 bytes
        char header[8];
        if (!recvAll(header, 8))
        {
            return false;
        }
        int msgTypeInt = 0;
        int payloadLen = 0;
        std::memcpy(&msgTypeInt, header, 4);
        std::memcpy(&payloadLen, header + 4, 4);

        outType = static_cast<MessageType>(msgTypeInt);

        if (payloadLen < 0)
        {
            lastError = "Negative payload size??";
            return false;
        }
        if (payloadLen == 0)
        {
            outPayload.clear();
            return true;
        }

        std::vector<char> payloadBuf(payloadLen);
        if (!recvAll(payloadBuf.data(), payloadLen))
        {
            return false;
        }
        outPayload.assign(payloadBuf.begin(), payloadBuf.end());
        return true;
    }

    bool Network::Connection::sendAll(const char *data, int size)
    {
        int totalSent = 0;
        while (totalSent < size)
        {
            int s = ::send(socketDescriptor, data + totalSent, size - totalSent, 0);
            if (s <= 0)
            {
                lastError = "Send failed.";
                return false;
            }
            totalSent += s;
        }
        return true;
    }

    bool Network::Connection::recvAll(char *data, int size)
    {
        int totalRecv = 0;
        while (totalRecv < size)
        {
            int r = ::recv(socketDescriptor, data + totalRecv, size - totalRecv, 0);
            if (r <= 0)
            {
                lastError = "Receive failed or connection closed.";
                return false;
            }
            totalRecv += r;
        }
        return true;
    }

    std::string Network::Connection::getLastError() const
    {
        return lastError;
    }

} // namespace dtq
