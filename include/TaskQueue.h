#ifndef TASKQUEUE_H
#define TASKQUEUE_H

#include "Task.h"
#include <queue>
#include <mutex>
#include <condition_variable>
#include <optional>

namespace dtq
{

    class TaskQueue
    {
    public:
        TaskQueue();
        ~TaskQueue();

        bool enqueue(const Task &task);
        std::optional<Task> dequeue();
        bool updateTaskResult(int taskId, const std::string &result, TaskStatus status);
        size_t size();

    private:
        std::queue<Task> queue;
        std::mutex queueMutex;
        std::condition_variable condition;
    };

} // namespace dtq

#endif // TASKQUEUE_H
