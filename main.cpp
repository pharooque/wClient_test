#include <WinSock2.h>
#include <WS2tcpip.h>
#include <iostream>
#include <stdexcept>

constexpr int DEFAULT_PORT = 55555;

int main(int argc, char const *argv[])
{
    SOCKET clientSocket = INVALID_SOCKET;
    sockaddr_in clientService;

    WSADATA wsaData;
    const char* serverIP = argc > 1 ? argv[1] : "127.0.0.1";
    int port = argc > 2 ? std::stoi(argv[2]) : DEFAULT_PORT;
    
    try
    {
        // Initialize Winsock
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
        {
            throw std::runtime_error("The WINSOCK DLL cannot be initialized");
        }
        std::cout << "WSAStartup success" << std::endl;
        std::cout << "Status: " << wsaData.szSystemStatus << std::endl;

        // Create a socket
        clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (clientSocket == INVALID_SOCKET)
        {
            throw std::runtime_error("Failed to create socket: " + std::to_string(WSAGetLastError()));
        }
        std::cout << "Client Socket created" << std::endl;

        clientService.sin_family = AF_INET;
        if (inet_pton(AF_INET, serverIP, &clientService.sin_addr) != 1) // Convert IP address to binary form
        {
            throw std::runtime_error("Invalid IP address format for " + std::string(serverIP));
        }
        clientService.sin_port = htons(port); // Convert port to network byte order

        // Connect to server
        if (connect((clientSocket), reinterpret_cast<sockaddr*>(&clientService), sizeof(clientService)) == SOCKET_ERROR)
        {
            throw std::runtime_error("Failed to connect to server: " + std::to_string(WSAGetLastError()));
        }
        std::cout << "Connected to server successfully" << std::endl;

        // Signal readiness
        std::cout << "Ready to send and receive data" << std::endl;
    }
    catch(const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << '\n';
    }
    
    // Cleanup
    if (clientSocket != INVALID_SOCKET)
    {
        shutdown(clientSocket, SD_BOTH);
        closesocket(clientSocket);
    }
    WSACleanup();
    std::cout << "Program exiting" << std::endl;
    
    return 0;
}
