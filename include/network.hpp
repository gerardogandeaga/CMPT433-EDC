#ifndef NETWORK_H
#define NETWORK_H

#include <thread>
#include <mutex>
#include <vector>
#include <cmath>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <signal.h>
#include "node.h"

class Network
{
public:
    Network(Network const&) = delete;
    void operator=(Network const&) = delete;

    static Network* GetInstance(const char* serverAddr, int serverPort);
    static void DestroyInstance(void);

    int getConsensusQuakeMagnitude(void);
    int getNumNodes(void);

private:
    static Network* instance;
    bool running;
    int nodeId;
    const char *host;
    int port;
    int consensusQuakeMagnitude;

    std::thread getThread;
    std::thread putThread;
    std::thread faultCheckThread;

    std::vector<int> severities;

    int registerNode(void);
    void parseResponse(char* response);
    void getRequests(void);
    void putRequests(void);
    void deregister(void);
    void faultCheck(void);
    int sendRequest(char (&message)[512], char (&response)[512]);

    Network(const char* serverAddr, int serverPort);
    ~Network();
};

#endif