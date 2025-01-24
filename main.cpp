#include <winSock2.h>
#include <WS2tcpip.h>
#include <iostream>
#include <string_view>
#include <memory>
#include <stdexcept>

class WSAInitializer
{
private:
    WSADATA wsaData;
public:
    WSAInitializer()
    {
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
        {
            throw std::runtime_error("Fail to initialize WinSock DLL");
        }
    }

    const WSADATA &getWSAData() const { return wsaData; }
    ~WSAInitializer() { WSACleanup(); }
};

class SocketWrapper
{
private:
    SOCKET clientSocket;
    SocketWrapper (const SocketWrapper&) = delete; // Prevent copying
    SocketWrapper operator=(const SocketWrapper&) = delete;
public:
    explicit SocketWrapper(SOCKET s = INVALID_SOCKET) : clientSocket(s){}
    ~SocketWrapper()
    {
        if (clientSocket != INVALID_SOCKET)
        {
            shutdown(clientSocket, SD_BOTH);
            closesocket(clientSocket);
        }
    }

    SOCKET getSocket() const { return clientSocket; }
    SOCKET release()
    {
        SOCKET tempSocket = clientSocket;
        clientSocket = INVALID_SOCKET;

        return tempSocket;
    }
};

constexpr int DEFAULT_PORT = 55555;
constexpr std::string_view DEFAULT_IP = "127.0.0.1";

int main(int argc, char const *argv[])
{
    try
    {
        // Initialize WinSock
        WSAInitializer wsaInit;
        std::cout << "WSAStartup success\nStatus: " << wsaInit.getWSAData().szSystemStatus << '\n';

        // Create socket
        SocketWrapper clientSocket(socket(AF_INET, SOCK_STREAM, IPPROTO_TCP));
        if (clientSocket.getSocket() == INVALID_SOCKET)
        {
            throw std::runtime_error("Failed to create socket: " + std::to_string(WSAGetLastError()));
        }
        std::cout << "Client Socket created\n";

        // Set socket options for better performance
        constexpr int optValue = 1;
        if (setsockopt(clientSocket.getSocket(), IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char*>(&optValue), sizeof(optValue)) == SOCKET_ERROR)
        {
            throw std::runtime_error("Failed to set TCP_NODELAY" + std::to_string(WSAGetLastError()));
        }

        // Set send and receive buffer sizes
        constexpr int bufferSize = 64 * 1024; // 64KB buffers
        setsockopt(clientSocket.getSocket(), SOL_SOCKET, SO_RCVBUF, reinterpret_cast<const char*>(&bufferSize), sizeof(bufferSize));
        setsockopt(clientSocket.getSocket(), SOL_SOCKET, SO_SNDBUF, reinterpret_cast<const char*>(&bufferSize), sizeof(bufferSize));

        sockaddr_in clientService{};
        clientService.sin_family = AF_INET;
        const char* serverIP = argc > 1 ? argv[1] : DEFAULT_IP.data();
        int port = argc > 2 ? std::stoi(argv[2]) : DEFAULT_PORT;

        // Convert IPv4 address from text to binary form
        if (inet_pton(AF_INET, serverIP, &clientService.sin_addr) != 1)
        {
            throw std::runtime_error("Invalid IP address format: " + std::string(serverIP));
        }
        clientService.sin_port = htons (static_cast<unsigned short>(port)); // Convert port to network byte order

        // Connect with timeout
        u_long mode = 1;
        ioctlsocket(clientSocket.getSocket(), FIONBIO, &mode); //Set non-blocking mode
        if (connect(clientSocket.getSocket(), reinterpret_cast<sockaddr*>(&clientService), sizeof(clientService)) == SOCKET_ERROR)
        {
            if (WSAGetLastError() != WSAEWOULDBLOCK)
            {
                throw std::runtime_error("Failed to connect to server: " + std::to_string(WSAGetLastError()));
            }

            fd_set fdset;
            timeval tv{5, 0}; // 5 seconds timeout
            FD_ZERO (&fdset);
            FD_SET (clientSocket.getSocket(), &fdset);

            int result = select(0, nullptr, &fdset, nullptr, &tv);
            if (result == 0)
            {
                throw std::runtime_error("Connection timeout");
            }
            else if (result == SOCKET_ERROR)
            {
                throw std::runtime_error("Failed to connect to server: " + std::to_string(WSAGetLastError()));
            }
        }

        mode = 0;
        ioctlsocket(clientSocket.getSocket(), FIONBIO, &mode); // Set blocking mode back
        std::cout << "Connected to server successfully\n";
        std::cout << "Ready to send and recieve data\n";
    }
    catch(const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }
    return 0;
}

