#include "Network.h"
#include "Task.h"
#include "TaskQueue.h"
#include "Logger.h"

#include <iostream>
#include <thread>
#include <chrono>

using namespace dtq;

// In a realistic scenario, workers would request tasks from the server.
// For demonstration, this worker will simulate polling a local task queue.
extern TaskQueue globalTaskQueue;

void processTask(Task task) {
    Logger::getInstance().log(LogLevel::INFO, "Worker processing task " + std::to_string(task.taskId));
    // Simulate task processing delay
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    // Set task result
    task.result = "Processed Result for task " + std::to_string(task.taskId);
    task.status = TaskStatus::COMPLETED;
    // In a real implementation, the worker would send the result back to the server.
    // For this demo, we log the update.
    Logger::getInstance().log(LogLevel::INFO, "Worker completed task " + std::to_string(task.taskId));
}

int main() {
    Logger::getInstance().setLogFile("worker.log");
    Logger::getInstance().log(LogLevel::INFO, "Starting Task Queue Worker...");

    // Initialize network (if needed for communicating with server)
    if (!Network::initialize()) {
        Logger::getInstance().log(LogLevel::ERROR, "Network initialization failed.");
        return -1;
    }

    // Simulate polling tasks from the server/local queue
    while (true) {
        auto taskOpt = globalTaskQueue.dequeue();
        if (taskOpt.has_value()) {
            Task task = taskOpt.value();
            task.status = TaskStatus::IN_PROGRESS;
            processTask(task);
            // Update the task result back to the server (here, simulated by logging)
            globalTaskQueue.updateTaskResult(task.taskId, task.result, task.status);
        } else {
            // No task available, wait before polling again
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    }

    Network::cleanup();
    return 0;
}
