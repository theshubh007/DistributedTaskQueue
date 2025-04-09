#include "TaskQueue.h"
#include "Logger.h"
#include <algorithm>
#include "Config.h"

namespace dtq
{

    TaskQueue::TaskQueue()
    {
    }

    TaskQueue::~TaskQueue()
    {
    }

    bool TaskQueue::enqueue(const Task &task)
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        if (queue.size() >= Config::MaxQueueSize)
        {
            Logger::getInstance().log(LogLevel::WARN, "Queue is full. Task " + std::to_string(task.taskId) + " rejected.");
            return false;
        }
        queue.push(task);
        condition.notify_one();
        Logger::getInstance().log(LogLevel::INFO, "Task " + std::to_string(task.taskId) + " enqueued.");
        return true;
    }

    std::optional<Task> TaskQueue::dequeue()
    {
        std::unique_lock<std::mutex> lock(queueMutex);
        if (queue.empty())
        {
            return std::nullopt;
        }
        Task task = queue.front();
        queue.pop();
        Logger::getInstance().log(LogLevel::INFO, "Task " + std::to_string(task.taskId) + " dequeued.");
        return task;
    }

    bool TaskQueue::updateTaskResult(int taskId, const std::string &result, TaskStatus status)
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        // For demonstration, we will only log the update.
        // In a real-world scenario, you might maintain a separate mapping for taskId to task.
        Logger::getInstance().log(LogLevel::INFO, "Task " + std::to_string(taskId) + " updated with result: " + result);
        return true;
    }

    size_t TaskQueue::size()
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        return queue.size();
    }

} // namespace dtq
