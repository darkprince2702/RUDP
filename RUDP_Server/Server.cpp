/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Server.cpp
 * Author: ductn
 * 
 * Created on July 22, 2016, 10:36 AM
 */

#include "Server.h"

Server::Server(int port) {
    port_ = port;
    socket_ = 0;
    eventBase_ = NULL;
    listenEvent_ = NULL;
}

void Server::createAndListenSocket() {
    int fd, error;
    char port[sizeof ("65536") + 1];
    struct addrinfo hints, *res, *res0;
    struct sockaddr_in serverAddr;
    // Initialize hints
    memset(&hints, 0, sizeof (hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;
    sprintf(port, "%d", port_);
    if ((error = getaddrinfo(NULL, port, &hints, &res0))) {
        perror("getaddrinfo()");
        return;
    }
    for (res = res0; res != NULL; res = res->ai_next) {
        if (res->ai_family == AF_INET6 || res->ai_next == NULL) {
            break;
        }
    }
    // Create socket fd
    if ((fd = socket(PF_INET, SOCK_DGRAM, 0)) == -1) {
        freeaddrinfo(res0);
        perror("socket()");
        return;
    }
    // Initial struct
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(5050);
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    memset(serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero);
    // Bind fd to addr
    if (bind(fd, (sockaddr*) &serverAddr, sizeof(serverAddr)) == -1) {
        freeaddrinfo(res0);
        perror("bind()");
        return;
    }
    // Set socket fd to nonblock
    int flags;
    if ((flags = fcntl(fd, F_GETFL)) < 0 ||
            ((fcntl(fd, F_SETFD, flags | O_NONBLOCK) < 0))) {
        perror("fcntl() NONBLOCK");
        return;
    }

    socket_ = fd;
    std::cout << "Sever binded to port " << port << std::endl;
}

void Server::addConnection(Connection* conn) {
    connections_.push_back(conn);
}

Server::Connection* Server::getConnection(const sockaddr_in* address) {
    if (connections_.size()) {
        return connections_.back();
    } else {
        return NULL;
    }
}

event_base* Server::getEventBase() {
    return eventBase_;
}

void Server::registerEvents() {
    if (eventBase_ == NULL) {
        eventBase_ = event_base_new();
    }

    if (socket_ >= 0) {
        listenEvent_ = event_new(eventBase_, socket_, EV_READ | EV_PERSIST, Server::listenHandler,
                this);
        if (event_add(listenEvent_, 0) == -1) {
            perror("registerEvents() - eventadd()");
            return;
        }
    }
    std::cout << "Listen event registered\n";
}

void Server::serve() {
    if (!socket_) {
        createAndListenSocket();
        registerEvents();
    }
    // Enter event base loop
    event_base_loop(eventBase_, 0);
}

void Server::stop() {
    event_base_loopbreak(eventBase_);
    perror("Exit event base loop");
    return;
}

void Server::listenHandler(int fd, short what, void* v) {
    std::cout << "Receive a packet from client\n";
    Server* server = (Server*) v;
    // Receive a whole packet (1450 bytes) from client
    uint8_t* buffer = new uint8_t[1472];
    sockaddr_in* clientAddr;
    socklen_t clientAddrLen = sizeof (*clientAddr);
    int nBytes = recvfrom(server->socket_, buffer, sizeof (buffer), 0,
            (sockaddr*) clientAddr, &clientAddrLen);
    // Pass buffer to connection
    Connection* conn;
    if ((conn = server->getConnection(clientAddr)) == NULL) {
        std::cout << "Create a connection\n";
        conn = new Connection(server->socket_, clientAddr, CLOSED, server->eventBase_);
        server->addConnection(conn);
    }
    conn->transition(buffer);
}

Server::Connection::Connection(int socket, sockaddr_in* clientAddr, uint8_t state, event_base* EVB) {
    socket_ = socket;
    clientAddr_ = clientAddr;
    state_ = state;
    eventBase_ = EVB;
    sendBase_ = 0;
    receiveBase_ = 0;
}

void Server::Connection::transition(uint8_t* buffer) {
    std::cout << "Start connection's transition\n";
    PacketHeader receivedHeader;
    processHeader(buffer, receivedHeader);
    if (receivedHeader.type == SYN) {
        if (state_ == CLOSED) {
            state_ = SYN_RECEIVED;
            PacketHeader sendHeader;
            sendHeader.type = SYN_ACK;
            sendHeader.sequenceNumber = sendBase_;
            receiveBase_ = receivedHeader.sequenceNumber + 1;
            sendHeader.acknowledgmentNumber = receiveBase_;
            sendHeader.windowSize = 64;
            gettimeofday(&start_, NULL);
            sendPacket(socket_, clientAddr_, &sendHeader, NULL, 0);
            std::cout << "Sent SYNC-ACK to client\n";
            registerEvents();
        }
    }
    if (receivedHeader.type == ACK) {
        switch (state_) {
            case SYN_RECEIVED:
                state_ = ESTABLISHED;
                gettimeofday(&end_, NULL);
                timeout_.tv_sec = end_.tv_sec - start_.tv_sec;
                timeout_.tv_usec = end_.tv_usec - start_.tv_usec;
                sendBase_ = receivedHeader.acknowledgmentNumber;
                break;
            case LAST_ACK:
                // @TODO: close connection
                break;
            default:
                // @TODO: received wrong package
                break;
        }
    }
    if (receivedHeader.type == FIN) {
        if (state_ != ESTABLISHED) {
            // @TODO: strange case
        }
        state_ = CLOSE_WAIT;
        // Send ACK back to client
        PacketHeader sendHeader;
        sendHeader.type = ACK;
        sendHeader.acknowledgmentNumber = receivedHeader.sequenceNumber + 1;
        sendPacket(socket_, clientAddr_, &sendHeader, NULL, 0);
        // @TODO: clean work and about to disconnect

        state_ = LAST_ACK;
        // Send FIN to client;
        sendHeader.type = FIN;
        sendHeader.sequenceNumber = sendBase_;
        sendHeader.acknowledgmentNumber = 0;
        sendPacket(socket_, clientAddr_, &sendHeader, NULL, 0);
        registerEvents();
    }
    if (receivedHeader.type == DATA) {
        // Check if receive correct packet
        if (receivedHeader.sequenceNumber == receiveBase_) {
            uint8_t* data = new uint8_t[receivedHeader.length];
            processData(buffer, data, receivedHeader.length);
            Data* strData;
            strData->data = data;
            strData->length = receivedHeader.length;
            // Store in memory
            data_.push_back(strData);
            receiveBase_ += receivedHeader.length;
            // Send ACK to client
            PacketHeader sendHeader;
            sendHeader.type = ACK;
            sendHeader.acknowledgmentNumber = receiveBase_;
            sendHeader.windowSize = WINDOWS_SIZE;
            sendPacket(socket_, clientAddr_, &sendHeader, NULL, 0);
        }
    }
}

void Server::Connection::timeoutHandler(int fd, short what, void* v) {
    Connection* conn = (Connection*) v;
    PacketHeader sendHeader;
    if (conn->state_ == SYN_RECEIVED) {
        sendHeader.type = SYN_ACK;
        sendHeader.sequenceNumber = conn->sendBase_;
        sendHeader.acknowledgmentNumber = conn->receiveBase_;
        sendHeader.windowSize = 64;
        sendPacket(conn->socket_, conn->clientAddr_, &sendHeader, NULL, 0);
    } else if (conn->state_ == LAST_ACK) {
        sendHeader.type = FIN;
        sendHeader.sequenceNumber = conn->sendBase_;
        sendHeader.acknowledgmentNumber = 0;
        sendPacket(conn->socket_, conn->clientAddr_, &sendHeader, NULL, 0);
    }
}

void Server::Connection::registerEvents() {
    if (eventBase_ == NULL) {
        eventBase_ = event_base_new();
    }

    if (timeoutEvent_ == NULL) {
        timeoutEvent_ = event_new(eventBase_, -1, 0, Connection::timeoutHandler,
                this);
    }

    if (event_add(timeoutEvent_, &timeout_) == -1) {
        perror("event_add()");
        return;
    }
}

void Server::Connection::writeToFile() {
    std::ofstream fStream;
    fStream.open("output", std::ios::out | std::ios::binary);
    if (!fStream) {
        perror("open()");
    }
    std::list<Data*>::iterator pList;
    for (pList = data_.begin(); pList != data_.end(); pList++) {
        fStream.write(reinterpret_cast<char*> ((*pList)->data), (*pList)->length);
    }
    fStream.close();
}
