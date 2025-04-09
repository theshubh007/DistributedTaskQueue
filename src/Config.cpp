#include "Config.h"
#include <fstream>
#include <sstream>

namespace dtq {

// Define static members
const int Config::MaxQueueSize = 1000;
const int Config::ThreadPoolSize = 4;
const std::chrono::milliseconds Config::NetworkTimeout(5000);
const int Config::TaskRetryLimit = 3;
const std::chrono::milliseconds Config::HeartbeatInterval(2000);

// Optionally load configuration from file (this is a simple placeholder implementation)
bool Config::loadConfig(const std::string& filename) {
    std::ifstream infile(filename);
    if (!infile.is_open()) {
        return false;
    }
    std::string line;
    while (std::getline(infile, line)) {
        std::istringstream iss(line);
        // Here you would parse key=value pairs and set the corresponding configuration parameters.
        // For simplicity, this demo does not implement actual dynamic config loading.
    }
    infile.close();
    return true;
}

} // namespace dtq
