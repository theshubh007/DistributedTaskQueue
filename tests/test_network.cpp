#include "Network.h"
#include "Logger.h"
#include <iostream>
#include <cassert>

int main() {
    // Test network initialization.
    bool initSuccess = dtq::Network::initialize();
    assert(initSuccess);

    // Create a connection to a dummy server.
    // Assuming nothing is listening on port 9999, the connection should fail.
    dtq::Network::Connection conn("127.0.0.1", 9999);
    bool connectSuccess = conn.connect();
    // Since no server is running on this port, we expect connect() to return false.
    assert(!connectSuccess);
    
    std::cout << "Network tests passed. Error (as expected): " << conn.getLastError() << std::endl;
    
    dtq::Network::cleanup();
    return 0;
}
