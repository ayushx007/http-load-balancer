#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <vector>
#include <thread>
#include <mutex>
#include <chrono> // For sleep

#define PORT 8080

struct Backend {
    std::string ip;
    int port;
    bool is_online; // New status flag
};

// Global state
std::vector<Backend> backends = {
    {"127.0.0.1", 8081, true},
    {"127.0.0.1", 8082, true},
    {"127.0.0.1", 8083, true}
};

int current_backend = 0;
std::mutex lb_mutex;

// --- HEALTH CHECK THREAD ---
// This runs in the background forever
void health_check() {
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(5)); // Check every 5 seconds
        
        // No mutex needed for reading, but good practice if we were adding/removing backends
        // We will lock just for printing to keep logs clean
        
        for (auto& backend : backends) {
            int sock = socket(AF_INET, SOCK_STREAM, 0);
            sockaddr_in addr;
            addr.sin_family = AF_INET;
            addr.sin_port = htons(backend.port);
            inet_pton(AF_INET, backend.ip.c_str(), &addr.sin_addr);

            // Set timeout so we don't hang if server is dead
            struct timeval timeout;
            timeout.tv_sec = 1;
            timeout.tv_usec = 0;
            setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
            setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));

            if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
                // If it was online, mark it offline
                if (backend.is_online) {
                    std::lock_guard<std::mutex> lock(lb_mutex);
                    std::cout << "\n[Health Check] ❌ Backend " << backend.port << " is DOWN." << std::endl;
                    backend.is_online = false;
                }
            } else {
                // If it was offline, mark it online
                if (!backend.is_online) {
                    std::lock_guard<std::mutex> lock(lb_mutex);
                    std::cout << "\n[Health Check] ✅ Backend " << backend.port << " is back UP." << std::endl;
                    backend.is_online = true;
                }
                close(sock);
            }
        }
    }
}

void handle_client(int client_socket) {
    Backend target;
    bool found = false;

    // CRITICAL SECTION: Select a HEALTHY backend
    {
        std::lock_guard<std::mutex> lock(lb_mutex);
        
        // Try to find a healthy server (try max attempts = number of servers)
        int start_index = current_backend;
        for (size_t i = 0; i < backends.size(); i++) {
            target = backends[current_backend];
            current_backend = (current_backend + 1) % backends.size();
            
            if (target.is_online) {
                found = true;
                break;
            }
        }
    }

    if (!found) {
        std::cerr << "All backends are down!" << std::endl;
        const char* error_msg = "HTTP/1.1 503 Service Unavailable\r\nContent-Length: 21\r\n\r\nNo servers available.";
        send(client_socket, error_msg, strlen(error_msg), 0);
        close(client_socket);
        return;
    }

    std::cout << "[Thread " << std::this_thread::get_id() << "] Forwarding to " << target.port << std::endl;

    // 1. Read Client Request
    char buffer[4096] = {0};
    int bytes_read = recv(client_socket, buffer, sizeof(buffer), 0);
    if (bytes_read <= 0) {
        close(client_socket);
        return;
    }

    // 2. Connect to Backend
    int backend_socket = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in backend_addr;
    backend_addr.sin_family = AF_INET;
    backend_addr.sin_port = htons(target.port);
    inet_pton(AF_INET, target.ip.c_str(), &backend_addr.sin_addr);

    if (connect(backend_socket, (struct sockaddr*)&backend_addr, sizeof(backend_addr)) < 0) {
        close(client_socket);
        close(backend_socket);
        return;
    }

    // 3. Forward Request
    send(backend_socket, buffer, bytes_read, 0);

    // 4. Get Response
    char response_buffer[4096];
    int bytes_received;
    while ((bytes_received = recv(backend_socket, response_buffer, sizeof(response_buffer), 0)) > 0) {
        send(client_socket, response_buffer, bytes_received, 0);
    }

    close(backend_socket);
    close(client_socket);
}

int main() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) return -1;

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        std::cerr << "Bind failed" << std::endl;
        return -1;
    }

    if (listen(server_fd, 10) < 0) return -1;

    std::cout << "Health-Checked Load Balancer running on port " << PORT << std::endl;

    // Launch Health Check Thread (Detached)
    std::thread(health_check).detach();

    while (true) {
        int client_socket = accept(server_fd, nullptr, nullptr);
        if (client_socket < 0) continue;
        std::thread(handle_client, client_socket).detach();
    }

    return 0;
}