#ifndef CONFIG_H
#define CONFIG_H

#include <chrono>
#include <string>

namespace dtq {

// Hyperparameters and configuration settings for the Distributed Task Queue.
class Config {
public:
    // Maximum number of tasks allowed in the queue.
    static const int MaxQueueSize = 1000;
    
    // Number of threads available for processing tasks.
    static const int ThreadPoolSize = 8;
    
    // Network timeout duration (in milliseconds).
    static const std::chrono::milliseconds NetworkTimeout;

    // Maximum number of times to retry a failed task.
    static const int TaskRetryLimit = 3;
    
    // Interval (in milliseconds) for workers to send heartbeat messages.
    static const std::chrono::milliseconds HeartbeatInterval;
    
    // Batch size if tasks are processed in batches.
    static const int BatchSize = 10;

    // Additional configuration parameters can be added here.
    
    // Optional: Function to load configuration from a file.
    static bool loadConfig(const std::string& filename);
};

} // namespace dtq

#endif // CONFIG_H
