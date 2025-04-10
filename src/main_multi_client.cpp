#include "Network.h"
#include "Task.h"
#include "Logger.h"
#include "Config.h"
#include <iostream>
#include <vector>
#include <thread>
#include <atomic>
#include <string>
#include <random>
#include <chrono>

const int NUM_USERS = 2;      // Reduced to just 2 users for clarity
const int TASKS_PER_USER = 5; // 5 tasks per user
bool stopClients = false;

// Get current time in milliseconds
long long nowMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
}

void clientThread(int clientId, int numTasks, int startTaskId)
{
    // Get server address and port from config
    std::string serverAddress = "127.0.0.1"; // Default value
    int serverPort = 5555; // Default value
    
    // Retry parameters
    int maxRetries = 3;
    int retryDelayMs = 500; // 500 milliseconds
    
    for (int i = 0; i < numTasks && !stopClients; i++)
    {
        int taskId = startTaskId + i;
        dtq::Logger::getInstance().log(dtq::LogLevel::INFO, "[User " + std::to_string(clientId) + "] Sending task ID=" + std::to_string(taskId));
        
        // Create a task
        dtq::Task task;
        task.taskId = taskId;
        task.payload = "User " + std::to_string(clientId) + " Task " + std::to_string(i) + " (Duration: " + std::to_string(500 + (rand() % 1000)) + "ms)";
        task.status = dtq::TaskStatus::PENDING;
        task.enqueueTimeMs = nowMs();
        
        // Serialize the task
        std::string serializedTask = task.serialize();
        
        // Create a connection to the server
        dtq::Network::Connection conn(serverAddress, serverPort);
        
        // Connect to the server with retries
        bool connected = false;
        int retries = 0;
        
        while (!connected && retries < maxRetries && !stopClients)
        {
            if (conn.connect())
            {
                connected = true;
            }
            else
            {
                dtq::Logger::getInstance().log(dtq::LogLevel::WARN, 
                    "[User " + std::to_string(clientId) + "] Connection failed: " + conn.getLastError() + 
                    ". Retrying in " + std::to_string(retryDelayMs) + "ms...");
                
                // Wait before retrying
                std::this_thread::sleep_for(std::chrono::milliseconds(retryDelayMs));
                retries++;
            }
        }
        
        if (!connected)
        {
            dtq::Logger::getInstance().log(dtq::LogLevel::ERR, 
                "[User " + std::to_string(clientId) + "] Failed to connect after " + 
                std::to_string(maxRetries) + " retries. Skipping task.");
            continue;
        }
        
        // Send the task to the server
        if (!conn.sendMessage(dtq::MessageType::CLIENT_ADD_TASK, serializedTask))
        {
            dtq::Logger::getInstance().log(dtq::LogLevel::ERR, 
                "[User " + std::to_string(clientId) + "] Failed to send task: " + conn.getLastError());
            conn.disconnect();
            continue;
        }
        
        // Receive acknowledgment from the server
        dtq::MessageType responseType;
        std::string responsePayload;
        
        if (!conn.receiveMessage(responseType, responsePayload))
        {
            dtq::Logger::getInstance().log(dtq::LogLevel::ERR, 
                "[User " + std::to_string(clientId) + "] Failed to receive acknowledgment: " + conn.getLastError());
            conn.disconnect();
            continue;
        }
        
        // Check the response type
        if (responseType == dtq::MessageType::SERVER_TASK_ACCEPTED)
        {
            dtq::Logger::getInstance().log(dtq::LogLevel::INFO, 
                "[User " + std::to_string(clientId) + "] Task accepted by server: ID=" + std::to_string(taskId));
        }
        else if (responseType == dtq::MessageType::SERVER_TASK_REJECTED)
        {
            dtq::Logger::getInstance().log(dtq::LogLevel::WARN, 
                "[User " + std::to_string(clientId) + "] Task rejected by server: ID=" + std::to_string(taskId) + 
                ", Reason: " + responsePayload);
        }
        else
        {
            dtq::Logger::getInstance().log(dtq::LogLevel::ERR, 
                "[User " + std::to_string(clientId) + "] Received unexpected response type: " + 
                std::to_string(static_cast<int>(responseType)));
        }
        
        // Close the connection
        conn.disconnect();
        
        // Add a small delay between tasks
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    dtq::Logger::getInstance().log(dtq::LogLevel::INFO, 
        "[User " + std::to_string(clientId) + "] Finished sending all tasks");
}

int main()
{
    // Initialize Windows sockets
    if (!dtq::Network::initialize())
    {
        std::cerr << "Failed to initialize network. Exiting." << std::endl;
        return 1;
    }

    dtq::Logger::getInstance().setLogFile("multi_client.log");
    dtq::Logger::getInstance().log(dtq::LogLevel::INFO, "Starting Multi-Client Simulation...");

    std::vector<std::thread> userThreads;
    userThreads.reserve(NUM_USERS);

    // Start multiple "users", each sending tasks
    for (int u = 1; u <= NUM_USERS; ++u)
    {
        userThreads.emplace_back(std::thread(clientThread, u, TASKS_PER_USER, (u * 100)));
    }

    for (auto &t : userThreads)
    {
        if (t.joinable())
        {
            t.join();
        }
    }

    dtq::Logger::getInstance().log(dtq::LogLevel::INFO, "All users have finished sending tasks.");
    
    // Cleanup Windows sockets
    dtq::Network::cleanup();
    
    return 0;
}