#ifndef CONFIG_H
#define CONFIG_H

#include <chrono>
#include <string>

namespace dtq
{

    // Hyperparameters and configuration settings for the Distributed Task Queue.
    class Config
    {
    public:
        // Maximum number of tasks allowed in the queue.
        static const int MaxQueueSize;

        // Number of threads or any concurrency setting, if you want to implement a thread pool in server/worker.
        static const int ThreadPoolSize;

        // Network timeout duration (in milliseconds).
        static const std::chrono::milliseconds NetworkTimeout;

        // Maximum number of times to retry a failed task.
        static const int TaskRetryLimit;

        // Interval (in milliseconds) for workers to send heartbeat messages, etc.
        static const std::chrono::milliseconds HeartbeatInterval;

        // Optionally load configuration from file.
        static bool loadConfig(const std::string &filename);
    };

} // namespace dtq

#endif // CONFIG_H
