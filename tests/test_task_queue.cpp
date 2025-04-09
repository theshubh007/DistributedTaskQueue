#include "TaskQueue.h"
#include "Task.h"
#include "Logger.h"
#include <iostream>
#include <cassert>

int main() {
    // Create a TaskQueue instance.
    dtq::TaskQueue queue;

    // Test: The initial size should be 0.
    assert(queue.size() == 0);

    // Test: Enqueue a task.
    dtq::Task task;
    task.taskId = 1;
    task.payload = "Test payload";
    task.status = dtq::TaskStatus::PENDING;
    
    bool enqueueSuccess = queue.enqueue(task);
    assert(enqueueSuccess);
    assert(queue.size() == 1);

    // Test: Dequeue the task.
    auto dequeuedTaskOpt = queue.dequeue();
    assert(dequeuedTaskOpt.has_value());
    dtq::Task dequeuedTask = dequeuedTaskOpt.value();
    assert(dequeuedTask.taskId == task.taskId);
    assert(queue.size() == 0);

    // Test: Enqueue again and update task result.
    queue.enqueue(task);
    bool updateSuccess = queue.updateTaskResult(task.taskId, "Test result", dtq::TaskStatus::COMPLETED);
    assert(updateSuccess);

    std::cout << "All TaskQueue tests passed." << std::endl;
    return 0;
}
