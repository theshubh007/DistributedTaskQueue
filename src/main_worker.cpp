#include "Network.h"
#include "Task.h"
#include "TaskQueue.h"
#include "Logger.h"
#include "Config.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <atomic>

using namespace dtq;

static std::atomic<bool> stopWorkers{false};

void workerThread(int workerId)
{
    dtq::Logger::getInstance().log(dtq::LogLevel::INFO, "[Worker " + std::to_string(workerId) + "] Worker starting");
    
    // Get server address and port from config
    std::string serverAddress = "127.0.0.1"; // Default value
    int serverPort = 5555; // Default value
    
    // Retry parameters
    int maxRetries = 3;
    int retryDelayMs = 500; // 500 milliseconds
    
    while (!stopWorkers.load())
    {
        // Create a connection to the server
        dtq::Network::Connection conn(serverAddress, serverPort);
        
        // Connect to the server with retries
        bool connected = false;
        int retries = 0;
        
        while (!connected && retries < maxRetries && !stopWorkers.load())
        {
            if (conn.connect())
            {
                connected = true;
            }
            else
            {
                dtq::Logger::getInstance().log(dtq::LogLevel::WARN, 
                    "[Worker " + std::to_string(workerId) + "] Connection failed: " + conn.getLastError() + 
                    ". Retrying in " + std::to_string(retryDelayMs) + "ms...");
                
                // Wait before retrying
                std::this_thread::sleep_for(std::chrono::milliseconds(retryDelayMs));
                retries++;
            }
        }
        
        if (!connected)
        {
            dtq::Logger::getInstance().log(dtq::LogLevel::ERR, 
                "[Worker " + std::to_string(workerId) + "] Failed to connect after " + 
                std::to_string(maxRetries) + " retries. Retrying later...");
            std::this_thread::sleep_for(std::chrono::seconds(3));
            continue;
        }
        
        // Request a task from the server
        if (!conn.sendMessage(dtq::MessageType::WORKER_REQUEST_TASK, ""))
        {
            dtq::Logger::getInstance().log(dtq::LogLevel::ERR, 
                "[Worker " + std::to_string(workerId) + "] Failed to request task: " + conn.getLastError());
            conn.disconnect();
            std::this_thread::sleep_for(std::chrono::seconds(1));
            continue;
        }
        
        // Receive the task from the server
        dtq::MessageType responseType;
        std::string payload;
        
        if (!conn.receiveMessage(responseType, payload))
        {
            dtq::Logger::getInstance().log(dtq::LogLevel::ERR, 
                "[Worker " + std::to_string(workerId) + "] Failed to receive task: " + conn.getLastError());
            conn.disconnect();
            std::this_thread::sleep_for(std::chrono::seconds(1));
            continue;
        }
        
        if (responseType != dtq::MessageType::SERVER_ASSIGN_TASK)
        {
            dtq::Logger::getInstance().log(dtq::LogLevel::ERR, 
                "[Worker " + std::to_string(workerId) + "] Received unexpected message type: " + 
                std::to_string(static_cast<int>(responseType)));
            conn.disconnect();
            std::this_thread::sleep_for(std::chrono::seconds(1));
            continue;
        }
        
        // Check if we received an empty payload (no tasks available)
        if (payload.empty())
        {
            dtq::Logger::getInstance().log(dtq::LogLevel::INFO, 
                "[Worker " + std::to_string(workerId) + "] No tasks available");
            conn.disconnect();
            std::this_thread::sleep_for(std::chrono::seconds(1));
            continue;
        }
        
        // Deserialize the task
        dtq::Task task = dtq::Task::deserialize(payload);
        
        // Send acknowledgment to the server
        if (!conn.sendMessage(dtq::MessageType::WORKER_TASK_RECEIVED, ""))
        {
            dtq::Logger::getInstance().log(dtq::LogLevel::ERR, 
                "[Worker " + std::to_string(workerId) + "] Failed to send acknowledgment: " + conn.getLastError());
            conn.disconnect();
            std::this_thread::sleep_for(std::chrono::seconds(1));
            continue;
        }
        
        // Close the connection while processing
        conn.disconnect();
        
        // Process the task
        dtq::Logger::getInstance().log(dtq::LogLevel::INFO, 
            "[Worker " + std::to_string(workerId) + "] Processing task ID=" + std::to_string(task.taskId));
        
        // Extract duration from payload if available
        int processingTime = 1000; // Default 1 second
        size_t durationPos = task.payload.find("Duration: ");
        if (durationPos != std::string::npos) {
            size_t msPos = task.payload.find("ms", durationPos);
            if (msPos != std::string::npos) {
                std::string durationStr = task.payload.substr(durationPos + 10, msPos - (durationPos + 10));
                try {
                    processingTime = std::stoi(durationStr);
                } catch (const std::exception& e) {
                    dtq::Logger::getInstance().log(dtq::LogLevel::WARN, 
                        "[Worker " + std::to_string(workerId) + "] Failed to parse duration: " + e.what());
                }
            }
        }
        
        // Simulate task processing
        std::this_thread::sleep_for(std::chrono::milliseconds(processingTime));
        
        // Update task status and result
        task.status = dtq::TaskStatus::COMPLETED;
        task.result = "Processed by Worker " + std::to_string(workerId) + " in " + std::to_string(processingTime) + "ms";
        
        // Create a new connection to submit the result
        dtq::Network::Connection resultConn(serverAddress, serverPort);
        
        // Connect to the server with retries
        connected = false;
        retries = 0;
        
        while (!connected && retries < maxRetries && !stopWorkers.load())
        {
            if (resultConn.connect())
            {
                connected = true;
            }
            else
            {
                dtq::Logger::getInstance().log(dtq::LogLevel::WARN, 
                    "[Worker " + std::to_string(workerId) + "] Result connection failed: " + resultConn.getLastError() + 
                    ". Retrying in " + std::to_string(retryDelayMs) + "ms...");
                
                // Wait before retrying
                std::this_thread::sleep_for(std::chrono::milliseconds(retryDelayMs));
                retries++;
            }
        }
        
        if (!connected)
        {
            dtq::Logger::getInstance().log(dtq::LogLevel::ERR, 
                "[Worker " + std::to_string(workerId) + "] Failed to connect for result submission after " + 
                std::to_string(maxRetries) + " retries. Retrying task later...");
            std::this_thread::sleep_for(std::chrono::seconds(1));
            continue;
        }
        
        // Submit the result to the server
        std::string serializedResult = task.serialize();
        if (!resultConn.sendMessage(dtq::MessageType::WORKER_SUBMIT_RESULT, serializedResult))
        {
            dtq::Logger::getInstance().log(dtq::LogLevel::ERR, 
                "[Worker " + std::to_string(workerId) + "] Failed to submit result: " + resultConn.getLastError());
            resultConn.disconnect();
            std::this_thread::sleep_for(std::chrono::seconds(1));
            continue;
        }
        
        // Receive confirmation from the server
        dtq::MessageType confirmationType;
        std::string confirmationPayload;
        
        if (!resultConn.receiveMessage(confirmationType, confirmationPayload))
        {
            dtq::Logger::getInstance().log(dtq::LogLevel::ERR, 
                "[Worker " + std::to_string(workerId) + "] Failed to receive result confirmation: " + resultConn.getLastError());
            resultConn.disconnect();
            std::this_thread::sleep_for(std::chrono::seconds(1));
            continue;
        }
        
        if (confirmationType != dtq::MessageType::SERVER_RESULT_CONFIRMED)
        {
            dtq::Logger::getInstance().log(dtq::LogLevel::ERR, 
                "[Worker " + std::to_string(workerId) + "] Received unexpected confirmation type: " + 
                std::to_string(static_cast<int>(confirmationType)));
            resultConn.disconnect();
            std::this_thread::sleep_for(std::chrono::seconds(1));
            continue;
        }
        
        dtq::Logger::getInstance().log(dtq::LogLevel::INFO, 
            "[Worker " + std::to_string(workerId) + "] Result for task ID=" + std::to_string(task.taskId) + " confirmed by server");
        
        // Close the result connection
        resultConn.disconnect();
    }
}

int main()
{
    // Initialize Windows sockets
    if (!dtq::Network::initialize())
    {
        std::cerr << "Failed to initialize network. Exiting." << std::endl;
        return 1;
    }

    dtq::Logger::getInstance().setLogFile("worker.log");
    dtq::Logger::getInstance().log(dtq::LogLevel::INFO, "Starting Distributed Task Queue Worker");
    dtq::Logger::getInstance().log(dtq::LogLevel::INFO, "Starting 2 worker threads");

    std::vector<std::thread> workers;
    for (int i = 1; i <= 2; i++)
    {
        workers.emplace_back(std::thread(workerThread, i));
    }
    dtq::Logger::getInstance().log(dtq::LogLevel::INFO, "Worker pool running. Press Enter to stop...");

    // Wait for user input
    std::cin.get();
    stopWorkers.store(true);

    for (auto &th : workers)
    {
        if (th.joinable())
            th.join();
    }
    dtq::Logger::getInstance().log(dtq::LogLevel::INFO, "All workers stopped.");
    
    // Cleanup Windows sockets
    dtq::Network::cleanup();
    
    return 0;
}
