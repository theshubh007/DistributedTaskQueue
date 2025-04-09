#include "Network.h"
#include "TaskQueue.h"
#include "Logger.h"
#include "Config.h"
#include "Task.h"

#include <thread>
#include <atomic>
#include <vector>
#include <iostream>
#include <chrono>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#endif

using namespace dtq;

// Global queue
TaskQueue globalTaskQueue;

// Simple counters for throughput measurement
std::atomic<long long> tasksCompleted{0};
std::atomic<long long> totalLatencyMs{0};

// Helper: current time in ms
static long long nowMs()
{
    auto tp = std::chrono::steady_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(tp.time_since_epoch()).count();
    return static_cast<long long>(ms);
}

void handleOneMessage(MessageType msgType, const std::string &payload, Network::Connection &conn)
{
    switch (msgType)
    {
    case MessageType::CLIENT_ADD_TASK:
    {
        Task task = Task::deserialize(payload);
        // Record enqueue time in ms
        task.enqueueTimeMs = nowMs();

        // Enqueue
        if (!globalTaskQueue.enqueue(task))
        {
            Logger::getInstance().log(LogLevel::ERR,
                                      "Failed to enqueue task " + std::to_string(task.taskId));
        }
        else
        {
            Logger::getInstance().log(LogLevel::INFO,
                                      "Enqueued task ID=" + std::to_string(task.taskId));
        }
        break;
    }
    case MessageType::WORKER_GET_TASK:
    {
        // Worker requests a task
        auto optTask = globalTaskQueue.dequeue();
        if (optTask.has_value())
        {
            Task t = optTask.value();
            conn.sendMessage(MessageType::SERVER_SEND_TASK, t.serialize());
            Logger::getInstance().log(LogLevel::INFO,
                                      "Sent task ID=" + std::to_string(t.taskId) + " to worker.");
        }
        else
        {
            conn.sendMessage(MessageType::SERVER_SEND_TASK, "");
            Logger::getInstance().log(LogLevel::INFO,
                                      "No tasks in queue; sent empty response to worker.");
        }
        break;
    }
    case MessageType::WORKER_SEND_RESULT:
    {
        // Worker finishing a task
        Task t = Task::deserialize(payload);

        // Measure total latency
        long long finishMs = nowMs();
        long long latency = finishMs - t.enqueueTimeMs;
        totalLatencyMs.fetch_add(latency);

        // Mark completed
        tasksCompleted.fetch_add(1);
        globalTaskQueue.updateTaskResult(t.taskId, t.result, t.status);

        // Approx tasks/second => tasksCompleted / (time from start), or just log average
        long long completed = tasksCompleted.load();
        long long sumLatency = totalLatencyMs.load();
        long long avgLatency = (completed == 0) ? 0 : (sumLatency / completed);

        Logger::getInstance().log(LogLevel::INFO,
                                  "Worker completed task ID=" + std::to_string(t.taskId) +
                                      " latency=" + std::to_string(latency) + "ms" +
                                      " avgLatency=" + std::to_string(avgLatency) + "ms" +
                                      " totalCompleted=" + std::to_string(completed));
        break;
    }
    default:
        Logger::getInstance().log(LogLevel::WARN, "Received invalid or unexpected message type.");
        break;
    }
}

void connectionHandler(Network::Connection conn)
{
    Logger::getInstance().log(LogLevel::INFO, "New connection accepted.");

    // Instead of reading just once, we loop until the client disconnects.
    while (true)
    {
        MessageType msgType;
        std::string payload;

        // If receiveMessage fails, check if it's a normal disconnect (0 bytes) or real error.
        if (!conn.receiveMessage(msgType, payload))
        {
            std::string err = conn.getLastError();
            // If the server receives 0 bytes, some implementations return
            // "Receive failed or connection closed." We detect that case:
            if (err.find("connection closed") != std::string::npos ||
                err.find("Receive failed") != std::string::npos)
            {
                Logger::getInstance().log(LogLevel::INFO, "Client disconnected normally.");
            }
            else
            {
                Logger::getInstance().log(LogLevel::ERR,
                                          "Failed to receive message: " + err);
            }
            break; // exit the while loop
        }

        // We got a valid message
        handleOneMessage(msgType, payload, conn);
    }

    conn.disconnect();
}

int main()
{
    Logger::getInstance().setLogFile("server.log");
    Logger::getInstance().log(LogLevel::INFO, "Starting Distributed Task Queue Server...");

    if (!Network::initialize())
    {
        Logger::getInstance().log(LogLevel::ERR, "Network initialization failed.");
        return -1;
    }

#ifdef _WIN32
    SOCKET serverSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSock == INVALID_SOCKET)
    {
        Logger::getInstance().log(LogLevel::ERR, "Failed to create server socket.");
        return -1;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(5555);

    if (bind(serverSock, reinterpret_cast<sockaddr *>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR)
    {
        Logger::getInstance().log(LogLevel::ERR, "Bind failed.");
        return -1;
    }

    if (listen(serverSock, SOMAXCONN) == SOCKET_ERROR)
    {
        Logger::getInstance().log(LogLevel::ERR, "Listen failed.");
        return -1;
    }

    Logger::getInstance().log(LogLevel::INFO, "Server listening on port 5555...");

    std::vector<std::thread> threads;

    // Accept loop
    while (true)
    {
        sockaddr_in clientAddr;
        int clientSize = sizeof(clientAddr);
        SOCKET clientSock = accept(serverSock, reinterpret_cast<sockaddr *>(&clientAddr), &clientSize);
        if (clientSock == INVALID_SOCKET)
        {
            Logger::getInstance().log(LogLevel::ERR, "Accept failed.");
            continue;
        }

        // Construct a Connection from the accepted socket
        Network::Connection conn(clientSock);

        // Launch a thread that loops reading messages until client disconnect
        threads.emplace_back(std::thread(connectionHandler, std::move(conn)));
    }

    // Never reached in this infinite loop, but for completeness:
    for (auto &t : threads)
    {
        if (t.joinable())
        {
            t.join();
        }
    }
    closesocket(serverSock);
    Network::cleanup();
#endif

    return 0;
}
