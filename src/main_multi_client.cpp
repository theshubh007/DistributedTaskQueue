#include "Network.h"
#include "Task.h"
#include "Logger.h"
#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <string>

// We'll simulate N different users.
const int NUM_USERS = 3;

// This function simulates a single "user" that connects to the server and sends tasks.
void userSimulation(int userId, const std::string &serverIP, int serverPort)
{
  // Create a local log string to identify the user
  std::string userTag = "[User " + std::to_string(userId) + "] ";

  // Each user can send multiple tasks. Let's say 3 tasks per user for this demo.
  int tasksToSend = 3;

  for (int i = 1; i <= tasksToSend; ++i)
  {
    // 1) Initialize network if needed (only do once at the beginning).
    //    But for demonstration, each user can do it if you want them
    //    fully independent. In real code, you'd likely call this once overall.
    dtq::Network::initialize();

    // 2) Create a connection to the server
    dtq::Network::Connection conn(serverIP, serverPort);
    if (!conn.connect())
    {
      dtq::Logger::getInstance().log(dtq::LogLevel::ERR,
                                     userTag + "Failed to connect: " + conn.getLastError());
      dtq::Network::cleanup();
      return;
    }

    // 3) Create a task
    dtq::Task task;
    task.taskId = (userId * 100) + i; // e.g., user 2 => tasks 201, 202, 203
    task.payload = userTag + "Task data for job " + std::to_string(i);
    task.status = dtq::TaskStatus::PENDING;

    // 4) Serialize the task
    std::string serializedTask = task.serialize();
    std::vector<char> data(serializedTask.begin(), serializedTask.end());

    // 5) Send the task to the server
    if (!conn.send(data))
    {
      dtq::Logger::getInstance().log(dtq::LogLevel::ERR,
                                     userTag + "Failed to send task: " + conn.getLastError());
    }
    else
    {
      dtq::Logger::getInstance().log(dtq::LogLevel::INFO,
                                     userTag + "Sent task ID=" + std::to_string(task.taskId));
    }

    // 6) Close the connection
    conn.disconnect();
    dtq::Network::cleanup();

    // Simulate a short pause between tasks from the same user
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }
}

int main()
{
  // Optional: log to a file for multi-client logs
  dtq::Logger::getInstance().setLogFile("multi_client.log");
  dtq::Logger::getInstance().log(dtq::LogLevel::INFO, "Starting Multi-Client Simulation...");

  // The IP and port of your server (make sure main_server actually listens on that port)
  std::string serverIP = "127.0.0.1";
  int serverPort = 5555;

  std::vector<std::thread> userThreads;
  userThreads.reserve(NUM_USERS);

  // Spawn threads, each representing a different user
  for (int userId = 1; userId <= NUM_USERS; ++userId)
  {
    userThreads.emplace_back(std::thread(
        userSimulation, userId, serverIP, serverPort));
  }

  // Wait for all user threads to finish
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
