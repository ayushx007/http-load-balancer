#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <vector>

#define PORT 8080

struct Backend {
    std::string ip;
    int port;
};

int main() {
    // 1. Define our Backend Servers
    std::vector<Backend> backends = {
        {"127.0.0.1", 8081},
        {"127.0.0.1", 8082},
        {"127.0.0.1", 8083}
    };
    
    int current_backend = 0; // Index for Round Robin

    // 2. Create Server Socket
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

    std::cout << "Load Balancer running on port " << PORT << std::endl;

    while (true) {
        int client_socket = accept(server_fd, nullptr, nullptr);
        if (client_socket < 0) continue;

        // --- ROUND ROBIN LOGIC ---
        Backend target = backends[current_backend];
        current_backend = (current_backend + 1) % backends.size(); // Rotate index
        
        std::cout << "Forwarding request to Server " << target.port << std::endl;

        // Read Client Request
        char buffer[4096] = {0};
        int bytes_read = recv(client_socket, buffer, sizeof(buffer), 0);
        if (bytes_read <= 0) {
            close(client_socket);
            continue;
        }

        // Connect to the selected Backend
        int backend_socket = socket(AF_INET, SOCK_STREAM, 0);
        sockaddr_in backend_addr;
        backend_addr.sin_family = AF_INET;
        backend_addr.sin_port = htons(target.port);
        inet_pton(AF_INET, target.ip.c_str(), &backend_addr.sin_addr);

        if (connect(backend_socket, (struct sockaddr*)&backend_addr, sizeof(backend_addr)) < 0) {
            std::cerr << "Failed to connect to backend " << target.port << std::endl;
            close(client_socket);
            close(backend_socket);
            continue;
        }

        // Forward Data
        send(backend_socket, buffer, bytes_read, 0);

        // Get Reply
        char response_buffer[4096];
        int bytes_received;
        while ((bytes_received = recv(backend_socket, response_buffer, sizeof(response_buffer), 0)) > 0) {
            send(client_socket, response_buffer, bytes_received, 0);
        }

        close(backend_socket);
        close(client_socket);
    }

    return 0;
}