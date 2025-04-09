#include "Network.h"
#include "Task.h"
#include "TaskQueue.h"
#include "Logger.h"
#include <iostream>
#include <thread>
#include <chrono>

using namespace dtq;

int main()
{
    Logger::getInstance().setLogFile("worker.log");
    Logger::getInstance().log(LogLevel::INFO, "Starting Worker...");

    if (!Network::initialize())
    {
        Logger::getInstance().log(LogLevel::ERR, "Network init failed.");
        return -1;
    }

    // Connect once
    Network::Connection conn("127.0.0.1", 5555); // Changed from 5555 to match server
    if (!conn.connect())
    {
        Logger::getInstance().log(LogLevel::ERR,
                                  "Worker failed to connect: " + conn.getLastError());
        return -1;
    }

    while (true)
    {
        // Ask server for a task
        if (!conn.sendMessage(MessageType::WORKER_GET_TASK, ""))
        {
            Logger::getInstance().log(LogLevel::ERR, "Failed to request task from server.");
            break;
        }

        // Receive server response
        MessageType msgType;
        std::string payload;
        if (!conn.receiveMessage(msgType, payload))
        {
            Logger::getInstance().log(LogLevel::ERR, "Failed to get task from server.");
            break;
        }

        if (msgType != MessageType::SERVER_SEND_TASK)
        {
            Logger::getInstance().log(LogLevel::WARN, "Unexpected message from server.");
            break;
        }

        if (payload.empty())
        {
            // no tasks in queue
            Logger::getInstance().log(LogLevel::INFO,
                                      "No tasks available. Will retry in 1 second...");
            std::this_thread::sleep_for(std::chrono::seconds(1));
            continue;
        }

        // We have a real task
        Task t = Task::deserialize(payload);
        Logger::getInstance().log(LogLevel::INFO,
                                  "Got task ID=" + std::to_string(t.taskId));

        // Simulate processing
        t.status = TaskStatus::IN_PROGRESS;
        std::this_thread::sleep_for(std::chrono::milliseconds(500)); // e.g. do real work
        t.result = "Processed by worker";
        t.status = TaskStatus::COMPLETED;

        // Send result back
        if (!conn.sendMessage(MessageType::WORKER_SEND_RESULT, t.serialize()))
        {
            Logger::getInstance().log(LogLevel::ERR, "Failed to send result to server.");
            break;
        }
        Logger::getInstance().log(LogLevel::INFO,
                                  "Sent result for task ID=" + std::to_string(t.taskId));
    }

    conn.disconnect();
    Network::cleanup();
    return 0;
}