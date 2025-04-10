#ifndef TASK_H
#define TASK_H

#include <string>
#include <sstream>

namespace dtq
{

    enum class TaskStatus
    {
        PENDING,
        IN_PROGRESS,
        COMPLETED,
        FAILED
    };

    struct Task
    {
        int taskId;
        std::string payload;
        TaskStatus status;
        std::string result;
        int retryCount;

        long long enqueueTimeMs; // for measuring latency

        Task() : taskId(0), status(TaskStatus::PENDING), retryCount(0), enqueueTimeMs(0) {}

        std::string serialize() const
        {
            std::ostringstream oss;
            oss << taskId << "|" << payload << "|" << (int)status << "|" << result << "|"
                << retryCount << "|" << enqueueTimeMs;
            return oss.str();
        }

        static Task deserialize(const std::string &data)
        {
            Task task;
            std::istringstream iss(data);
            std::string token;
            if (std::getline(iss, token, '|'))
                task.taskId = std::stoi(token);
            if (std::getline(iss, token, '|'))
                task.payload = token;
            if (std::getline(iss, token, '|'))
                task.status = (TaskStatus)std::stoi(token);
            if (std::getline(iss, token, '|'))
                task.result = token;
            if (std::getline(iss, token, '|'))
                task.retryCount = std::stoi(token);
            if (std::getline(iss, token, '|'))
                task.enqueueTimeMs = std::stoll(token);
            return task;
        }
    };

} // namespace dtq

#endif // TASK_H
