#ifndef NETWORK_H
#define NETWORK_H

#include <thread>
#include <vector>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <signal.h>

class Network
{
public:
    Network(Network const&) = delete;
    void operator=(Network const&) = delete;

    static Network* GetInstance(const char* serverAddr, int serverPort);
    static void DestroyInstance(void);

private:
    static Network* instance;
    bool running;
    int nodeId;
    const char *host;
    int port;

    std::thread getThread;
    std::thread putThread;

    std::vector<int> severities;

    int registerNode(void);
    void parseResponse(char* response);
    void getRequests(void);
    void putRequests(void);
    void deregister(void);

    Network(const char* serverAddr, int serverPort);
    ~Network();
};

#endif