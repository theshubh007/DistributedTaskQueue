# Distributed Task Queue

## Overview

This project implements a distributed task queue system in C++ for Windows. It supports multiple clients submitting tasks and multiple workers processing them. The server manages task scheduling, distribution, and result updates.

## Features

- **Distributed Architecture:** Separate components for the server, clients, and workers.
- **Scalability:** Tuneable hyperparameters allow the system to scale based on the hardware and expected load.
- **Robust Networking:** Uses a modular networking layer for communication over sockets.
- **Logging and Monitoring:** Integrated logging for debugging and performance monitoring.
- **Thread Safety:** Concurrent access to the task queue is managed to avoid race conditions.

## Project Structure

## How to Run

### Prerequisites

1. Install [CMake](https://cmake.org/) (version 3.16 or higher).
2. Install a C++ compiler:
   - For Windows: Visual Studio 2019 or later with C++ development tools.
   - For Linux: GCC or Clang.
3. Ensure `CMake` and the compiler are added to your system's PATH.

### Build Instructions

1. Open a terminal and navigate to the project directory:
   ```bash
   cd d:\Assignments\Distributed_Task_queue
   ```
2. Create a build directory:
   ```bash
   mkdir build && cd build
   ```
3. Run CMake to configure the project:
   ```bash
   cmake .. -G "Visual Studio 16 2019"
   ```
   Replace `"Visual Studio 16 2019"` with your generator if using a different compiler.
4. Build the project:
   ```bash
   cmake --build . --config Release
   ```

### Running the Components

1. **Server**: Start the server executable:
   ```bash
   ./Release/server
   ```
2. **Client**: Start the client executable to submit tasks:
   ```bash
   ./Release/client
   ```
3. **Worker**: Start one or more worker executables to process tasks:
   ```bash
   ./Release/worker
   ```

### Running Tests

1. Build the tests:
   ```bash
   cmake --build . --config Release --target test_task_queue test_network
   ```
2. Run the tests:
   ```bash
   ctest
   ```
   Or run individual test executables:
   ```bash
   ./Release/test_task_queue
   ./Release/test_network
   ```
