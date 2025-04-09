#include "Network.h"
#include "TaskQueue.h"
#include "Logger.h"
#include "Config.h"

#include <thread>
#include <vector>
#include <iostream>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#endif

using namespace dtq;

// Global task queue instance
TaskQueue globalTaskQueue;

// Same client handler function as before
void clientHandler(Network::Connection conn)
{
    std::vector<char> buffer;
    if (!conn.receive(buffer))
    {
        Logger::getInstance().log(LogLevel::ERR,
                                  "Failed to receive data from client: " + conn.getLastError());
        return;
    }
    std::string data(buffer.begin(), buffer.end());
    Task task = Task::deserialize(data);

    if (!globalTaskQueue.enqueue(task))
    {
        Logger::getInstance().log(LogLevel::ERR,
                                  "Failed to enqueue task " + std::to_string(task.taskId));
    }
    conn.disconnect();
}

int main()
{
    Logger::getInstance().setLogFile("server.log");
    Logger::getInstance().log(LogLevel::INFO, "Starting Task Queue Server...");

    // 1) Initialize Winsock (on Windows)
    if (!Network::initialize())
    {
        Logger::getInstance().log(LogLevel::ERR, "Network initialization failed.");
        return -1;
    }

#ifdef _WIN32
    // 2) Create a server socket
    SOCKET serverSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSock == INVALID_SOCKET)
    {
        Logger::getInstance().log(LogLevel::ERR, "Failed to create server socket.");
        Network::cleanup();
        return -1;
    }

    // 3) Bind to port 5555
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(5555);

    if (bind(serverSock, reinterpret_cast<sockaddr *>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR)
    {
        Logger::getInstance().log(LogLevel::ERR, "Bind failed.");
        closesocket(serverSock);
        Network::cleanup();
        return -1;
    }

    // 4) Listen
    if (listen(serverSock, SOMAXCONN) == SOCKET_ERROR)
    {
        Logger::getInstance().log(LogLevel::ERR, "Listen failed.");
        closesocket(serverSock);
        Network::cleanup();
        return -1;
    }
    Logger::getInstance().log(LogLevel::INFO, "Server listening on port 5555...");

    // 5) Accept connections in a loop
    std::vector<std::thread> clientThreads;
    while (true)
    {
        sockaddr_in clientAddr;
        int clientLen = sizeof(clientAddr);
        SOCKET clientSock = accept(serverSock, reinterpret_cast<sockaddr *>(&clientAddr), &clientLen);
        if (clientSock == INVALID_SOCKET)
        {
            Logger::getInstance().log(LogLevel::ERR, "Accept failed.");
            break; // Or continue, your choice
        }

        // Wrap the accepted socket in a Connection object using the new constructor
        Network::Connection conn(clientSock);

        // Spawn a thread to handle the client
        clientThreads.emplace_back(std::thread(clientHandler, std::move(conn)));
    }

    // Clean up
    for (auto &t : clientThreads)
    {
        if (t.joinable())
        {
            t.join();
        }
    }
    closesocket(serverSock);
#endif

    // 6) Shutdown
    Network::cleanup();
    Logger::getInstance().log(LogLevel::INFO, "Server shutting down.");
    return 0;
}
