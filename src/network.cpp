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
    const char *message = "GET /register HTTP/1.0\r\n\r\n";

    struct sockaddr_in sin;
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = inet_addr(host);
    sin.sin_port = htons(port);

    int sd = socket(AF_INET, SOCK_STREAM, 0);
    connect(sd,(struct sockaddr*) &sin, sizeof(sin));

    int bytes, sent, received, total;
    char response[1024];

    total = strlen(message);
    sent = 0;
    do {
        bytes = write(sd, message + sent, total - sent);
        if (bytes < 0) {
            printf("error writing register message to server: %s\n", strerror(errno));
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
            printf("error reading registration response: %s\n", strerror(errno));
            exit(-1);
        } else if (bytes == 0) {
            break;
        }
        received += bytes;
    } while (received < total);
    close(sd);
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
    char const *message = "GET /nodes HTTP/1.0\r\n\r\n";

    struct sockaddr_in sin;
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = inet_addr(host);
    sin.sin_port = htons(port);

    int bytes, sent, received, total;
    char response[1024];

    while (running) {
        int sd = socket(AF_INET, SOCK_STREAM, 0);
        connect(sd, (struct sockaddr*) &sin, sizeof(sin));
        total = strlen(message);
        sent = 0;
        do {
            bytes = write(sd, message + sent, total - sent);
            if (bytes < 0) {
                printf("error writing get to socket: %s\n", strerror(errno));
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
                printf("error reading response to get from socket: %s\n", strerror(errno));
                exit(-1);
            } else if (bytes == 0) {
                break;
            }
            received += bytes;
        } while (received < total);
        parseResponse(response);
        close(sd);
        std::this_thread::sleep_for(std::chrono::milliseconds(3000));
    }
}

void Network::putRequests() {
    char const *message_fmt = "PUT /nodes?id=%d&severity=%d HTTP/1.0\r\n\r\n";

    struct sockaddr_in sin;
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = inet_addr(host);
    sin.sin_port = htons(port);

    int bytes, sent, received, total;

    Node* node;

    while (running) {
        node = Node::GetInstanceIfExits();
        if (node == nullptr) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            continue;
        }
        char message[256], response[128];
        int severity = node->getNodeQuakeMagnitude();
        sprintf(message, message_fmt, nodeId, severity);
        int sd = socket(AF_INET, SOCK_STREAM, 0);
        connect(sd, (struct sockaddr*) &sin, sizeof(sin));
        total = strlen(message);
        sent = 0;
        do {
            bytes = write(sd, message + sent, total - sent);
            if (bytes < 0) {
                printf("error writing put to socket: %s\n", strerror(errno));
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
                printf("error reading response to put from socket: %s\n", strerror(errno));
                exit(-1);
            } else if (bytes == 0) {
                break;
            }
            received += bytes;
        } while (received < total);

        close(sd);
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
    
}

void Network::faultCheck() {
    char const *message = "PUT /faultcheck HTTP/1.0\r\n\r\n";

    struct sockaddr_in sin;
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = inet_addr(host);
    sin.sin_port = htons(port);

    int bytes, sent, received, total;
    char response[128];

    while (running) {
        int sd = socket(AF_INET, SOCK_STREAM, 0);
        connect(sd, (struct sockaddr*) &sin, sizeof(sin));
        total = strlen(message);
        sent = 0;
        do {
            bytes = write(sd, message + sent, total - sent);
            if (bytes < 0) {
                printf("error writing faultcheck to socket: %s\n", strerror(errno));
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
                printf("error reading response to faultcheck from socket: %s\n", strerror(errno));
                exit(-1);
            } else if (bytes == 0) {
                break;
            }
            received += bytes;
        } while (received < total);
        parseResponse(response);
        close(sd);
        std::this_thread::sleep_for(std::chrono::milliseconds(10000));
    }
}

void Network::deregister() {
    char const *message_fmt = "DELETE /register?id=%d HTTP/1.0\r\n\r\n";

    struct sockaddr_in sin;
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = inet_addr(host);
    sin.sin_port = htons(port);

    int sd = socket(AF_INET, SOCK_STREAM, 0);
    connect(sd,(struct sockaddr*) &sin, sizeof(sin));

    int bytes, sent, received, total;
    char message[64], response[64];
    sprintf(message, message_fmt, nodeId);

    total = strlen(message);
    sent = 0;
    do {
        bytes = write(sd, message + sent, total - sent);
        if (bytes < 0) {
            printf("error writing delete to socket: %s\n", strerror(errno));
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
            printf("error reading response to delete from socket: %s\n", strerror(errno));
            exit(-1);
        } else if (bytes == 0) {
            break;
        }
        received += bytes;
    } while (received < total);

    close(sd);
}

int Network::getConsensusQuakeMagnitude() {
    return consensusQuakeMagnitude;
}

int Network::getNumNodes() {
    return severities.size();
}