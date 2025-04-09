#ifndef TASK_H
#define TASK_H

#include <string>
#include <sstream>

namespace dtq {

// Enum representing the state of a task.
enum class TaskStatus {
    PENDING,
    IN_PROGRESS,
    COMPLETED,
    FAILED
};

// Structure representing a task in the queue.
struct Task {
    int taskId;
    std::string payload;  // Task data; this can be modified to a more complex type if needed.
    TaskStatus status;
    std::string result;   // Result after processing the task.
    int retryCount;       // Number of times the task has been retried.

    Task() : taskId(0), status(TaskStatus::PENDING), retryCount(0) {}

    // Serialize the task to a string (for network transfer).
    std::string serialize() const {
        std::ostringstream oss;
        oss << taskId << "|" << payload << "|" << static_cast<int>(status) << "|" << result << "|" << retryCount;
        return oss.str();
    }

    // Deserialize a task from a string.
    static Task deserialize(const std::string& data) {
        Task task;
        std::istringstream iss(data);
        std::string token;
        if (std::getline(iss, token, '|'))
            task.taskId = std::stoi(token);
        if (std::getline(iss, token, '|'))
            task.payload = token;
        if (std::getline(iss, token, '|'))
            task.status = static_cast<TaskStatus>(std::stoi(token));
        if (std::getline(iss, token, '|'))
            task.result = token;
        if (std::getline(iss, token, '|'))
            task.retryCount = std::stoi(token);
        return task;
    }
};

} // namespace dtq

#endif // TASK_H
