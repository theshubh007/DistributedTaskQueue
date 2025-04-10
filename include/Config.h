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
        static const int MaxQueueSize;
        static const int ThreadPoolSize;
        static const std::chrono::milliseconds NetworkTimeout;
        static const int TaskRetryLimit;
        static const std::chrono::milliseconds HeartbeatInterval;

        static bool loadConfig(const std::string &filename);
    };

} // namespace dtq

#endif // CONFIG_H
