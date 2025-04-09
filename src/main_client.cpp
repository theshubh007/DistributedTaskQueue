#include "Network.h"
#include "Task.h"
#include "Logger.h"

#include <iostream>
#include <vector>

using namespace dtq;

int main()
{
    Logger::getInstance().setLogFile("client.log");
    Logger::getInstance().log(LogLevel::INFO, "Starting Task Queue Client...");

    // Initialize network
    if (!Network::initialize())
    {
        Logger::getInstance().log(LogLevel::ERROR, "Network initialization failed.");
        return -1;
    }

    // Create a task
    Task task;
    task.taskId = 1;
    task.payload = "Process Data XYZ";
    task.status = TaskStatus::PENDING;

    // Serialize the task
    std::string serializedTask = task.serialize();

    // Connect to the server (change IP and port as necessary)
    Network::Connection connection("127.0.0.1", 5555); // Updated to match server port
    if (!connection.connect())
    {
        Logger::getInstance().log(LogLevel::ERR, "Client failed to connect to server: " + connection.getLastError());
        Network::cleanup();
        return -1;
    }

    // Send the task using the appropriate message type
    if (!connection.sendMessage(MessageType::CLIENT_ADD_TASK, serializedTask))
    {
        Logger::getInstance().log(LogLevel::ERROR, "Client failed to send task: " + connection.getLastError());
        connection.disconnect();
        Network::cleanup();
        return -1;
    }

    Logger::getInstance().log(LogLevel::INFO, "Task " + std::to_string(task.taskId) + " sent to server.");

    // Optionally, wait for a response from the server
    // (Not implemented in this demo; in a real system, you might receive an acknowledgement)

    connection.disconnect();
    Network::cleanup();
    return 0;
}