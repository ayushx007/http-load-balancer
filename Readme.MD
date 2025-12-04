# High-Performance HTTP Load Balancer in C++

A lightweight, **Layer 7 HTTP Load Balancer** built from scratch using **C++** and **POSIX Sockets**. 

This project demonstrates core **Systems Programming** concepts, including low-level networking, request parsing, and traffic distribution algorithms. It acts as a reverse proxy, distributing incoming client requests across a cluster of backend servers to optimize resource utilization.

## 🚀 Key Features
* **Custom TCP Server:** Implements a raw TCP listener using `<sys/socket.h>` without external networking frameworks.
* **Reverse Proxying:** Intercepts client requests and forwards them to upstream backend servers.
* **Round-Robin Scheduling:** Distributes traffic cyclically across healthy backends (Server 1 → 2 → 3 → 1).
* **Multi-Threading:** Handles concurrent client connections using `std::thread` and `std::mutex` for non-blocking I/O.
* **Thread-Safe State:** Uses mutex locks to protect the shared round-robin counter in a multi-threaded environment.
* **Linux/WSL Compatible:** Optimized for Linux environments.

## 🏗 Architecture
The Load Balancer listens on a public port (`8080`) and maintains a registry of backend servers.

```text
    [ CLIENT 1 ]   [ CLIENT 2 ]
          |              |
          v              v
      +----------------------------+
      |  MULTI-THREADED BALANCER   |
      |   (Port 8080 | Threads)    |
      +----------------------------+
          /      |      \
         v       v       v
      [Srv 1]    [Srv 2]   [Srv 3] 
      (8081)     (8082)    (8083)
```

## 🛠️ Tech Stack
* **Language:** C++ (GCC/G++)
* **Networking:** POSIX Sockets (`socket`, `bind`, `listen`, `accept`)
* **Concurrency:** `std::thread`, `std::mutex`
* **Protocol:** HTTP/1.1
* **Platform:** Linux / WSL (Windows Subsystem for Linux)

## ⚡ Getting Started

### Prerequisites
* G++ Compiler
* Python 3 (for running dummy backend servers)

### 1. Compile the Load Balancer
```bash
g++ loadbalancer.cpp -o lb -pthread
```
> **Note:** The `-pthread` flag is required for multi-threading.

### 2. Start Backend Servers
Open 3 separate terminals to simulate a server cluster:

**Terminal 1:**
```bash
python3 -m http.server 8081
```

**Terminal 2:**
```bash
python3 -m http.server 8082
```

**Terminal 3:**
```bash
python3 -m http.server 8083
```

### 3. Run the Load Balancer
In the main terminal:
```bash
./lb
```
You should see: `Multi-Threaded Load Balancer running on port 8080`

### 4. Test It
Open your browser or use `curl` in multiple terminals to see concurrent handling:

```bash
curl http://localhost:8080
# Output: Hello from Server 1

curl http://localhost:8080
# Output: Hello from Server 2
```
