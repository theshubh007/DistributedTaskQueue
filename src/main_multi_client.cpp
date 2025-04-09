#include "Network.h"
#include "Task.h"
#include "Logger.h"
#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <string>

const int NUM_USERS = 3;

void userSimulation(int userId, const std::string &serverIP, int serverPort)
{
  std::string userTag = "[User " + std::to_string(userId) + "] ";
  int tasksToSend = 3; // Increase to 100 or 1000 for stress testing

  for (int i = 1; i <= tasksToSend; ++i)
  {
    dtq::Network::initialize();

    dtq::Network::Connection conn(serverIP, serverPort);
    if (!conn.connect())
    {
      dtq::Logger::getInstance().log(dtq::LogLevel::ERR,
                                     userTag + "Failed to connect: " + conn.getLastError());
      dtq::Network::cleanup();
      return;
    }

    dtq::Task task;
    task.taskId = (userId * 100) + i;
    task.payload = userTag + "Payload for task " + std::to_string(i);

    // Send the task to server
    if (!conn.sendMessage(dtq::MessageType::CLIENT_ADD_TASK, task.serialize()))
    {
      dtq::Logger::getInstance().log(dtq::LogLevel::ERR,
                                     userTag + "Failed to send task: " + conn.getLastError());
    }
    else
    {
      dtq::Logger::getInstance().log(dtq::LogLevel::INFO,
                                     userTag + "Sent task ID=" + std::to_string(task.taskId));
    }

    conn.disconnect();
    dtq::Network::cleanup();

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }
}

int main()
{
  dtq::Logger::getInstance().setLogFile("multi_client.log");
  dtq::Logger::getInstance().log(dtq::LogLevel::INFO, "Starting Multi-Client Simulation...");

  std::string serverIP = "127.0.0.1";
  int serverPort = 5555; // Changed from 5255 to 5555

  std::vector<std::thread> userThreads;
  userThreads.reserve(NUM_USERS);

  for (int u = 1; u <= NUM_USERS; ++u)
  {
    userThreads.emplace_back(std::thread(userSimulation, u, serverIP, serverPort));
  }

  for (auto &t : userThreads)
  {
    if (t.joinable())
    {
      t.join();
    }
  }

  dtq::Logger::getInstance().log(dtq::LogLevel::INFO, "All users have finished sending tasks.");
  return 0;
}