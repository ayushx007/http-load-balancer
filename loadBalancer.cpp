#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h> // Needed for inet_pton
#include <unistd.h>
#include <cstring>

#define PORT 8080
#define BACKEND_PORT 8081
#define BACKEND_IP "127.0.0.1"

int main() {
    // 1. Create Server Socket (The "Front Door")
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        std::cerr << "Socket creation failed" << std::endl;
        return -1;
    }

    // Allow socket reuse immediately after crash (prevents "Address already in use" errors)
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

    if (listen(server_fd, 10) < 0) {
        std::cerr << "Listen failed" << std::endl;
        return -1;
    }

    std::cout << "Load Balancer listening on port " << PORT << "..." << std::endl;
    std::cout << "Forwarding to backend at " << BACKEND_IP << ":" << BACKEND_PORT << std::endl;

    while (true) {
        // 2. Accept Client Connection
        int client_socket = accept(server_fd, nullptr, nullptr);
        if (client_socket < 0) continue;

        // 3. Read Client Request
        char buffer[4096] = {0};
        int bytes_read = recv(client_socket, buffer, sizeof(buffer), 0);
        if (bytes_read <= 0) {
            close(client_socket);
            continue;
        }

        // 4. Connect to Backend Server (8081)
        int backend_socket = socket(AF_INET, SOCK_STREAM, 0);
        sockaddr_in backend_addr;
        backend_addr.sin_family = AF_INET;
        backend_addr.sin_port = htons(BACKEND_PORT);
        inet_pton(AF_INET, BACKEND_IP, &backend_addr.sin_addr);

        if (connect(backend_socket, (struct sockaddr*)&backend_addr, sizeof(backend_addr)) < 0) {
            std::cerr << "Could not connect to backend server!" << std::endl;
            close(client_socket);
            close(backend_socket);
            continue;
        }

        // 5. Forward Request to Backend
        send(backend_socket, buffer, bytes_read, 0);

        // 6. Get Response from Backend (LOOP UNTIL DONE)
        char response_buffer[4096];
        int bytes_received;
        
        // Keep reading as long as the backend is sending data
        while ((bytes_received = recv(backend_socket, response_buffer, sizeof(response_buffer), 0)) > 0) {
            send(client_socket, response_buffer, bytes_received, 0);
        }

        // 7. Close connections
        close(backend_socket);
        close(client_socket);
    }

    return 0;
}