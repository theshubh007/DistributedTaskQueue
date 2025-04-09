#ifndef TASKQUEUE_H
#define TASKQUEUE_H

#include "Task.h"
#include <queue>
#include <mutex>
#include <condition_variable>
#include <optional>

namespace dtq {

// Thread-safe task queue interface.
class TaskQueue {
public:
    TaskQueue();
    ~TaskQueue();

    // Enqueue a new task. Returns true if the task is added successfully.
    bool enqueue(const Task& task);

    // Dequeue a task if available; returns an optional task.
    std::optional<Task> dequeue();

    // Update the result and status of a task identified by taskId.
    bool updateTaskResult(int taskId, const std::string& result, TaskStatus status);

    // Check the current size of the queue.
    size_t size();

private:
    std::queue<Task> queue;
    std::mutex queueMutex;
    std::condition_variable condition;
};

} // namespace dtq

#endif // TASKQUEUE_H
