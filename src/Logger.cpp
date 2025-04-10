#include "Logger.h"
#include <iostream>
#include <chrono>
#include <ctime>
#include <sstream>

namespace dtq
{

    Logger &Logger::getInstance()
    {
        static Logger instance;
        return instance;
    }

    Logger::Logger()
    {
    }

    Logger::~Logger()
    {
        if (logFile.is_open())
        {
            logFile.close();
        }
    }

    void Logger::setLogFile(const std::string &filename)
    {
        std::lock_guard<std::mutex> lock(logMutex);
        if (logFile.is_open())
        {
            logFile.close();
        }
        logFile.open(filename, std::ios::out | std::ios::app);
    }

    std::string Logger::levelToString(LogLevel level)
    {
        switch (level)
        {
        case LogLevel::INFO:
            return "INFO";
        case LogLevel::WARN:
            return "WARN";
        case LogLevel::ERR:
            return "ERROR";
        default:
            return "UNKNOWN";
        }
    }

    void Logger::log(LogLevel level, const std::string &message)
    {
        std::lock_guard<std::mutex> lock(logMutex);
        auto now = std::chrono::system_clock::now();
        std::time_t now_c = std::chrono::system_clock::to_time_t(now);
        std::string timeStr(std::ctime(&now_c));
        if (!timeStr.empty() && timeStr.back() == '\n')
        {
            timeStr.pop_back();
        }

        std::ostringstream oss;
        oss << timeStr << " [" << levelToString(level) << "] " << message;
        std::string logMsg = oss.str();

        std::cout << logMsg << std::endl;
        if (logFile.is_open())
        {
            logFile << logMsg << std::endl;
        }
    }

} // namespace dtq
