#include "TaskQueue.h"
#include "Config.h"
#include "Logger.h"
#include <algorithm>

namespace dtq
{

    TaskQueue::TaskQueue() {}
    TaskQueue::~TaskQueue() {}

    bool TaskQueue::enqueue(const Task &task)
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        if (queue.size() >= Config::MaxQueueSize)
        {
            Logger::getInstance().log(LogLevel::WARN,
                                      "Queue is full. Task " + std::to_string(task.taskId) + " rejected.");
            return false;
        }
        queue.push(task);
        condition.notify_one();
        Logger::getInstance().log(LogLevel::INFO,
                                  "Task " + std::to_string(task.taskId) + " enqueued. Queue size=" + std::to_string(queue.size()));
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
        Logger::getInstance().log(LogLevel::INFO,
                                  "Task " + std::to_string(task.taskId) + " dequeued. Queue size=" + std::to_string(queue.size()));
        return task;
    }

    bool TaskQueue::updateTaskResult(int taskId, const std::string &result, TaskStatus status)
    {
        Logger::getInstance().log(LogLevel::INFO,
                                  "Task " + std::to_string(taskId) + " updated with result: " + result);
        return true;
    }

    size_t TaskQueue::size()
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        return queue.size();
    }

} // namespace dtq
