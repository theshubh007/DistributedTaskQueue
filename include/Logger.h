#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <fstream>
#include <mutex>

namespace dtq
{
    enum class LogLevel
    {
        INFO,
        WARN,
        ERR
    };

    class Logger
    {
    public:
        static Logger &getInstance();
        void log(LogLevel level, const std::string &message);
        void setLogFile(const std::string &filename);

    private:
        Logger();
        ~Logger();
        Logger(const Logger &) = delete;
        Logger &operator=(const Logger &) = delete;

        std::ofstream logFile;
        std::mutex logMutex;
        std::string levelToString(LogLevel level);
    };

} // namespace dtq

#endif
