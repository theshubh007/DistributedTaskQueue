#include "Config.h"
#include <fstream>
#include <sstream>

namespace dtq
{

    const int Config::MaxQueueSize = 1000;
    const int Config::ThreadPoolSize = 4;
    const std::chrono::milliseconds Config::NetworkTimeout(5000);
    const int Config::TaskRetryLimit = 3;
    const std::chrono::milliseconds Config::HeartbeatInterval(2000);

    bool Config::loadConfig(const std::string &filename)
    {
        // placeholder
        return true;
    }

} // namespace dtq
