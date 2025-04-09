#ifndef NETWORK_H
#define NETWORK_H

#include <string>
#include <vector>

#ifdef _WIN32
#include <winsock2.h>
#endif

namespace dtq
{

    class Network
    {
    public:
        static bool initialize();
        static void cleanup();

        // Nested class for a network connection
        class Connection
        {
        public:
            // 1) Existing constructor for creating a new socket
            Connection(const std::string &address, int port);

            // 2) New constructor for using an already-accepted socket
#ifdef _WIN32
            Connection(SOCKET acceptedSocket);
#else
            Connection(int acceptedSocket);
#endif

            ~Connection();

            bool connect();
            void disconnect();
            bool send(const std::vector<char> &data);
            bool receive(std::vector<char> &buffer);
            std::string getLastError() const;

        private:
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
