#ifndef NETWORK_H
#define NETWORK_H

#include <string>
#include <vector>

#ifdef _WIN32
#include <winsock2.h>
#endif

namespace dtq
{
    // Distinguish message types (client add task, worker request, etc.)
    enum class MessageType : int
    {
        CLIENT_ADD_TASK = 1,
        WORKER_GET_TASK = 2,
        WORKER_SEND_RESULT = 3,
        SERVER_SEND_TASK = 4,
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
#ifdef _WIN32
            Connection(SOCKET acceptedSocket); // server side
#else
            Connection(int acceptedSocket);
#endif
            Connection(const std::string &address, int port); // client side
            ~Connection();

            bool connect(); // for client
            void disconnect();

            // Low-level send/recv
            bool send(const std::vector<char> &data);
            bool receive(std::vector<char> &buffer);

            // Framed messages
            bool sendMessage(MessageType type, const std::string &payload);
            bool receiveMessage(MessageType &outType, std::string &outPayload);

            std::string getLastError() const;

        private:
            bool sendAll(const char *data, int size);
            bool recvAll(char *data, int size);

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
