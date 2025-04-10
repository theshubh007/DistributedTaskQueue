# Distributed Task Queue

A high-performance distributed task queue system implemented in C++ for Windows. This system allows multiple clients to add tasks to a central queue, which are then processed by worker nodes in a distributed manner.

## System Architecture

The system consists of three main components:

1. **Server**: Central task queue manager that handles task distribution and result collection
2. **Clients**: Applications that add tasks to the queue
3. **Workers**: Nodes that process tasks and return results

```
┌─────────┐     ┌──────────────────┐     ┌─────────┐
│ Client 1 │────▶                  │     │ Worker 1 │
└─────────┘     │                  │◀────└─────────┘
                │                  │
┌─────────┐     │  Server (Queue)  │     ┌─────────┐
│ Client 2 │────▶                  │◀────│ Worker 2 │
└─────────┘     │                  │     └─────────┘
                │                  │
┌─────────┐     │                  │     ┌─────────┐
│ Client N │────▶                  │◀────│ Worker M │
└─────────┘     └──────────────────┘     └─────────┘
```

## Demo

![System Demo](Pictures/Screenshot%202025-04-09%20152659.png)

## Features

- **Scalable Architecture**: Support for multiple clients and workers
- **Reliable Task Processing**: Tasks are tracked and can be retried if processing fails
- **Performance Monitoring**: Built-in throughput reporting and latency tracking
- **Fault Tolerance**: Connection retry mechanisms and error handling
- **TCP/IP Communication**: Network layer built on Windows Socket API

## Performance Metrics

The system measures and reports:
- **Throughput**: Tasks processed per second
- **Latency**: Time from task submission to completion
- **Queue Size**: Number of pending tasks
- **Worker Utilization**: Distribution of work across workers

## Building the Project

Build the server, multi-client, and worker executables:

```bash
# Build the server
g++ -std=c++17 -Iinclude src\Config.cpp src\Logger.cpp src\Network.cpp src\Task.cpp src\TaskQueue.cpp src\main_server.cpp -o server.exe -lws2_32

# Build the multi-client
g++ -std=c++17 -Iinclude src\Config.cpp src\Logger.cpp src\Network.cpp src\Task.cpp src\TaskQueue.cpp src\main_multi_client.cpp -o multi_client.exe -lws2_32

# Build the worker
g++ -std=c++17 -Iinclude src\Config.cpp src\Logger.cpp src\Network.cpp src\Task.cpp src\TaskQueue.cpp src\main_worker.cpp -o worker.exe -lws2_32
```

## Running the System

1. Start the server:
   ```
   .\server.exe
   ```

2. Start one or more workers:
   ```
   .\worker.exe
   ```

3. Run the multi-client to add tasks to the queue:
   ```
   .\multi_client.exe
   ```

## Configuration

The system uses default settings:
- Server listens on port 5555
- Server address is 127.0.0.1 (localhost)
- Connection retry parameters are configurable

## Performance Tuning

Several parameters can be adjusted to optimize performance:
- Number of worker threads
- Connection retry parameters
- Task processing time
- Number of clients and tasks per client

## Log Files

The system generates detailed logs for monitoring and debugging:
- `server.log`: Server activity and throughput reports
- `worker.log`: Worker processing details
- `multi_client.log`: Client task submission information

## Implementation Details

- **Task Queue**: Thread-safe queue implementation with mutex protection
- **Network Layer**: Abstraction over Windows Socket API
- **Task Serialization**: Simple string-based serialization for task data
- **Logger**: Thread-safe logging with different severity levels

## Future Enhancements

- Persistent task storage for recovery
- Dynamic worker scaling
- Priority queue support
- Task dependency management
- Web-based monitoring dashboard
