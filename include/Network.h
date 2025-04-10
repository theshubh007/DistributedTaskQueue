#ifndef NETWORK_H
#define NETWORK_H

#include <string>
#include <vector>

#ifdef _WIN32
#include <winsock2.h>
#endif

namespace dtq
{
    enum class MessageType : int
    {
        CLIENT_ADD_TASK = 1,
        WORKER_REQUEST_TASK = 2,
        WORKER_SUBMIT_RESULT = 3,
        SERVER_ASSIGN_TASK = 4,
        SERVER_TASK_ACCEPTED = 5,
        SERVER_TASK_REJECTED = 6,
        WORKER_TASK_RECEIVED = 7,
        SERVER_RESULT_CONFIRMED = 8,
        INVALID = 99
    };

    class Network
    {
    public:
        static bool initialize();
        static void cleanup();

        class Connection
        {
        public:
            // Client-side constructor
            Connection(const std::string &serverAddr, int port)
                : serverAddress(serverAddr), serverPort(port), socketDescriptor(INVALID_SOCKET) {}
            
            // Server-side constructor for accepted connections
#ifdef _WIN32
            explicit Connection(SOCKET acceptedSocket) 
                : serverAddress(""), serverPort(0), socketDescriptor(acceptedSocket) {}
#else
            explicit Connection(int acceptedSocket)
                : serverAddress(""), serverPort(0), socketDescriptor(acceptedSocket) {}
#endif
            
            ~Connection() { disconnect(); }
            
            bool connect();
            void disconnect();
            bool sendMessage(MessageType type, const std::string &payload);
            bool receiveMessage(MessageType &type, std::string &payload);
            const std::string &getLastError() const { return lastError; }

        private:
            bool send(const char* data, int size);
            bool recvAll(char* buffer, int size);
            
            std::string serverAddress;
            int serverPort;
#ifdef _WIN32
            SOCKET socketDescriptor;
#else
            int socketDescriptor;
#endif
            std::string lastError;
        };
    };

} // namespace dtq

#endif // NETWORK_H
