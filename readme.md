g++ -std=c++17 -Iinclude src\Config.cpp src\Logger.cpp src\Network.cpp src\Task.cpp src\TaskQueue.cpp src\main_server.cpp -o server.exe -lws2_32


g++ -std=c++17 -Iinclude src\Config.cpp src\Logger.cpp src\Network.cpp src\Task.cpp src\TaskQueue.cpp src\main_server.cpp -o server.exe -lws2_32

g++ -std=c++17 -Iinclude src\Config.cpp src\Logger.cpp src\Network.cpp src\Task.cpp src\TaskQueue.cpp src\main_multi_client.cpp -o multi_client.exe -lws2_32

//Build a Distributed Task Queue //WHEN task gets added in the queue, Multiple clients adding tasks/job in the queue and retrieving them to perform the task and update the results of the task Demonstrate the efficiency of the task queue and the maximum throughput it can handle and what hyper parameters are required to define the throughput i have to build using c++ windows os,