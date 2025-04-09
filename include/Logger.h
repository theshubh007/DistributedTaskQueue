#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <fstream>
#include <mutex>

namespace dtq
{

    // Log levels. Use ERR instead of ERROR to avoid Windows macro conflicts.
    enum class LogLevel
    {
        INFO,
        WARN,
        ERR
    };

    class Logger
    {
    public:
        // Retrieve the singleton instance.
        static Logger &getInstance();

        // Log a message with the given log level.
        void log(LogLevel level, const std::string &message);

        // Optionally set the log file.
        void setLogFile(const std::string &filename);

    private:
        Logger(); // Private constructor for singleton pattern.
        ~Logger();
        Logger(const Logger &) = delete; // Disable copy constructor.
        Logger &operator=(const Logger &) = delete;

        std::ofstream logFile;
        std::mutex logMutex;

        // Helper to convert log level to string.
        std::string levelToString(LogLevel level);
    };

} // namespace dtq

#endif // LOGGER_H
