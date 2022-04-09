#include "network.hpp"

// Contains code borrowed and modified from:
// https://stackoverflow.com/questions/22077802/simple-c-example-of-doing-an-http-post-and-consuming-the-response/22135885

Network *Network::instance = nullptr;

Network::Network(const char* serverAddr, int serverPort) {
    signal(SIGPIPE, SIG_IGN);
    running = true;
    consensusQuakeMagnitude = 0;
    host = serverAddr;
    port = serverPort;
    nodeId = registerNode();
    getThread = std::thread(&Network::getRequests, this);
    putThread = std::thread(&Network::putRequests, this);
    faultCheckThread = std::thread(&Network::faultCheck, this);
}

Network::~Network() {
    running = false;
    getThread.join();
    putThread.join();
    faultCheckThread.join();
    deregister();
}

Network* Network::GetInstance(const char* serverAddr, int serverPort) {
    if (instance == nullptr) {
        instance = new Network(serverAddr, serverPort);
    }
    return instance;
}

void Network::DestroyInstance() {
    delete instance;
    instance = nullptr;
}

int Network::registerNode() {
    const char *message_fmt = "GET /register HTTP/1.0\r\n\r\n";
    char message[512], response[512];
    strncpy(message, message_fmt, 512-1);
    sendRequest(message, response);
    char *token = NULL;
    token = strtok(response, "\n");
    for (int i = 0; i < 7; i++) {
        token = strtok(NULL, "\n");
    }
    return atoi(token);
}

void Network::parseResponse(char* response) {
    char *token = NULL;
    token = strtok(response, "\n");
    for (int i = 0; i < 7; i++) {
        token = strtok(NULL, "\n");
    }
    int size = strlen(token);
    severities.clear();
    int digit = 0;
    for (int i = 0; i < size; i++) {
        if (isdigit(token[i]) == 1) {
            digit = token[i] - '0';
            severities.push_back(digit);
        }
    }
    int sum = 0;
    for (auto& n : severities) {
        sum += n;
    }
    consensusQuakeMagnitude = round(sum / (double)severities.size());
}

void Network::getRequests() {
    char const *message_fmt = "GET /nodes HTTP/1.0\r\n\r\n";
    char message[512], response[512];
    strncpy(message, message_fmt, 512-1);
    
    while (running) {
        sendRequest(message, response);
        parseResponse(response);
        std::this_thread::sleep_for(std::chrono::milliseconds(3000));
    }
}

void Network::putRequests() {
    char const *message_fmt = "PUT /nodes?id=%d&severity=%d HTTP/1.0\r\n\r\n";
    char message[512], response[512];
    Node* node;

    while (running) {
        node = Node::GetInstanceIfExits();
        if (node == nullptr) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            continue;
        }
        int severity = node->getNodeQuakeMagnitude();
        sprintf(message, message_fmt, nodeId, severity);
        sendRequest(message, response);
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
    
}

void Network::faultCheck() {
    char const *message_fmt = "PUT /faultcheck HTTP/1.0\r\n\r\n";
    char message[512], response[512];
    strncpy(message, message_fmt, 512-1);
    while (running) {
        sendRequest(message, response);
        std::this_thread::sleep_for(std::chrono::milliseconds(10000));
    }
}

void Network::deregister() {
    char const *message_fmt = "DELETE /register?id=%d HTTP/1.0\r\n\r\n";
    char message[512], response[512];
    sprintf(message, message_fmt, nodeId);
    sendRequest(message, response);
}

int Network::sendRequest(char (&message)[512], char (&response)[512]) {
    struct sockaddr_in sin;
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = inet_addr(host);
    sin.sin_port = htons(port);
    int sd = socket(AF_INET, SOCK_STREAM, 0);
    connect(sd,(struct sockaddr*) &sin, sizeof(sin));
    int bytes, sent, received, total;
    total = strlen(message);
    sent = 0;
    do {
        bytes = write(sd, message + sent, total - sent);
        if (bytes < 0) {
            printf("error writing to socket: %s\n", strerror(errno));
            exit(-1);
        } else if (bytes == 0) {
            break;
        }
        sent += bytes;
    } while (sent < total);

    memset(response, 0, sizeof(response));
    total = sizeof(response) - 1;
    received = 0;
    do {
        bytes = read(sd, response + received, total - received);
        if (bytes < 0) {
            printf("error reading response from socket: %s\n", strerror(errno));
            exit(-1);
        } else if (bytes == 0) {
            break;
        }
        received += bytes;
    } while (received < total);

    close(sd);
    return received;
}

int Network::getConsensusQuakeMagnitude() {
    return consensusQuakeMagnitude;
}

int Network::getNumNodes() {
    return severities.size();
}