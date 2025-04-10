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
#include <mutex>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#endif

using namespace dtq;

TaskQueue globalTaskQueue;
std::mutex queueMutex;

// Counters
std::atomic<long long> tasksCompleted{0};
std::atomic<long long> totalLatencyMs{0};
std::atomic<bool> stopServer{false};
std::atomic<long long> tasksReceived{0};
std::mutex metricsMutex;

// For throughput
static auto serverStartTime = std::chrono::steady_clock::now();

// Current time in ms
static long long nowMs()
{
    auto tp = std::chrono::steady_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(tp.time_since_epoch()).count();
    return (long long)ms;
}

// For throughput - using a sliding window
static auto lastReportTime = std::chrono::steady_clock::now();
static std::atomic<long long> tasksSinceLastReport{0};

// Handle exactly one message from the client/worker, then close
static void handleClientConnection(SOCKET clientSock)
{
    Network::Connection conn(clientSock);
    
    // Receive the message
    MessageType msgType;
    std::string payload;
    
    if (!conn.receiveMessage(msgType, payload))
    {
        Logger::getInstance().log(LogLevel::ERR, "Failed to receive message: " + conn.getLastError());
        return;
    }
    
    // Process the message based on its type
    if (msgType == MessageType::CLIENT_ADD_TASK)
    {
        // Deserialize the task
        Task task = Task::deserialize(payload);
        
        // Add the task to the queue
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            globalTaskQueue.enqueue(task);
            Logger::getInstance().log(LogLevel::INFO, "Task added to queue: ID=" + std::to_string(task.taskId));
        }
        
        // Send acknowledgment to the client
        if (!conn.sendMessage(MessageType::SERVER_TASK_ACCEPTED, ""))
        {
            Logger::getInstance().log(LogLevel::ERR, "Failed to send acknowledgment: " + conn.getLastError());
            return;
        }
        
        // Update metrics
        {
            std::lock_guard<std::mutex> lock(metricsMutex);
            tasksReceived++;
        }
    }
    else if (msgType == MessageType::WORKER_REQUEST_TASK)
    {
        // Try to get a task from the queue
        Task task;
        bool hasTask = false;
        
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            auto optTask = globalTaskQueue.dequeue();
            if (optTask.has_value())
            {
                task = optTask.value();
                hasTask = true;
            }
        }
        
        if (hasTask)
        {
            // Serialize the task and send it to the worker
            std::string serializedTask = task.serialize();
            if (!conn.sendMessage(MessageType::SERVER_ASSIGN_TASK, serializedTask))
            {
                Logger::getInstance().log(LogLevel::ERR, "Failed to send task to worker: " + conn.getLastError());
                
                // Put the task back in the queue
                std::lock_guard<std::mutex> lock(queueMutex);
                globalTaskQueue.enqueue(task);
                return;
            }
            
            // Wait for acknowledgment from worker
            MessageType ackType;
            std::string ackPayload;
            
            if (!conn.receiveMessage(ackType, ackPayload))
            {
                Logger::getInstance().log(LogLevel::ERR, "Failed to receive worker acknowledgment: " + conn.getLastError());
                
                // Put the task back in the queue
                std::lock_guard<std::mutex> lock(queueMutex);
                globalTaskQueue.enqueue(task);
                return;
            }
            
            if (ackType != MessageType::WORKER_TASK_RECEIVED)
            {
                Logger::getInstance().log(LogLevel::ERR, "Unexpected acknowledgment type from worker");
                
                // Put the task back in the queue
                std::lock_guard<std::mutex> lock(queueMutex);
                globalTaskQueue.enqueue(task);
                return;
            }
            
            Logger::getInstance().log(LogLevel::INFO, "Task assigned to worker: ID=" + std::to_string(task.taskId));
        }
        else
        {
            // No tasks available, send empty response
            if (!conn.sendMessage(MessageType::SERVER_ASSIGN_TASK, ""))
            {
                Logger::getInstance().log(LogLevel::ERR, "Failed to send empty task response: " + conn.getLastError());
                return;
            }
        }
    }
    else if (msgType == MessageType::WORKER_SUBMIT_RESULT)
    {
        // Deserialize the task result
        Task completedTask = Task::deserialize(payload);
        
        // Process the completed task
        Logger::getInstance().log(LogLevel::INFO, "Task completed: ID=" + std::to_string(completedTask.taskId) + 
                                  ", Result=" + completedTask.result);
        
        // Update metrics
        {
            std::lock_guard<std::mutex> lock(metricsMutex);
            tasksCompleted++;
        }
        
        // Send acknowledgment to the worker
        if (!conn.sendMessage(MessageType::SERVER_RESULT_CONFIRMED, ""))
        {
            Logger::getInstance().log(LogLevel::ERR, "Failed to send result confirmation: " + conn.getLastError());
            return;
        }
    }
    else
    {
        Logger::getInstance().log(LogLevel::ERR, "Received unknown message type: " + std::to_string(static_cast<int>(msgType)));
    }
}

// Thread that logs tasks/s every 5 seconds using a sliding window
static void throughputReporter()
{
    while (!stopServer.load())
    {
        std::this_thread::sleep_for(std::chrono::seconds(5));

        auto now = std::chrono::steady_clock::now();
        auto elapsedSec = std::chrono::duration_cast<std::chrono::seconds>(now - lastReportTime).count();
        
        long long tasksDone = tasksSinceLastReport.exchange(0);  // Reset counter
        lastReportTime = now;

        if (elapsedSec > 0)
        {
            double tps = static_cast<double>(tasksDone) / elapsedSec;
            long long totalDone = tasksCompleted.load();
            
            Logger::getInstance().log(LogLevel::INFO,
                                    "[THROUGHPUT REPORT] Recent tasks/sec=" + std::to_string(tps) +
                                    " totalCompleted=" + std::to_string(totalDone));
        }
    }
}

int main()
{
    Logger::getInstance().setLogFile("server.log");
    Logger::getInstance().log(LogLevel::INFO, "Starting Dist. Task Queue Server (One msg/connection).");

    if (!Network::initialize())
    {
        Logger::getInstance().log(LogLevel::ERR, "Network init failed.");
        return -1;
    }

#ifdef _WIN32
    SOCKET serverSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSock == INVALID_SOCKET)
    {
        Logger::getInstance().log(LogLevel::ERR, "Server socket creation failed.");
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

    // Launch stats thread
    std::thread statsThread(throughputReporter);

    std::vector<std::thread> connectionThreads;
    connectionThreads.reserve(128);

    std::cout << "Server running. Press Enter to stop..." << std::endl;
    
    // Run server in a separate thread so we can handle shutdown
    std::thread serverThread([&]() {
        while (!stopServer.load())
        {
            sockaddr_in clientAddr;
            int clientSize = sizeof(clientAddr);
            SOCKET clientSock = accept(serverSock, reinterpret_cast<sockaddr *>(&clientAddr), &clientSize);
            if (clientSock == INVALID_SOCKET)
            {
                if (!stopServer.load()) {
                    Logger::getInstance().log(LogLevel::ERR, "Accept failed.");
                }
                continue;
            }
            connectionThreads.emplace_back(std::thread(handleClientConnection, clientSock));
        }
    });

    // Wait for user input to stop
    std::cin.get();
    stopServer.store(true);
    
    // Force close the server socket to unblock accept
    closesocket(serverSock);
    
    // Wait for server thread
    if (serverThread.joinable())
        serverThread.join();

    // Wait for stats thread
    if (statsThread.joinable())
        statsThread.join();

    // Wait for connection threads
    for (auto &t : connectionThreads)
    {
        if (t.joinable())
            t.join();
    }

    Network::cleanup();
#endif

    return 0;
}
