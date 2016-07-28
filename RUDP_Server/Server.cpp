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
    struct sockaddr_in serverAddr;
    // Create socket fd
    if ((fd = socket(PF_INET, SOCK_DGRAM, 0)) == -1) {
        perror("socket()");
        return;
    }
    std::cout << "Server FD is: " << fd << std::endl;
    // Initial struct
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port_);
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    memset(serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero);
    // Bind fd to addr
    if (bind(fd, (sockaddr*) & serverAddr, sizeof serverAddr) == -1) {
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
    std::cout << "Sever binded to port " << port_ << std::endl;
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
    std::cout << "Received 1 packet\n";
    Server* server = (Server*) v;
    // Receive a whole packet (1450 bytes) from client
    int bufferSize = 1472;
    uint8_t* buffer = new uint8_t[1472];
    sockaddr_in* clientAddr;
    socklen_t clientAddrLen = sizeof (sockaddr_in);
    int nBytes = recvfrom(server->socket_, buffer, bufferSize, 0,
            (sockaddr*) clientAddr, &clientAddrLen);
    // Pass buffer to connection
    Connection* conn;
    if ((conn = server->getConnection(clientAddr)) == NULL) {
        std::cout << "Created connection\n";
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
    std::cout << "Transition\n";
    PacketHeader receivedHeader;
    processHeader(buffer, &receivedHeader);
    std::cout << "Header type: " << (int) receivedHeader.type << std::endl;
    if (receivedHeader.type == SYN) {
        std::cout << "Received SYN\n";
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
            std::cout << "Sent SYNC-ACK\n";
            //            registerEvents();
        }
    }
    if (receivedHeader.type == ACK) {
        std::cout << "Received ACK\n";
        switch (state_) {
            case SYN_RECEIVED:
                state_ = ESTABLISHED;
                gettimeofday(&end_, NULL);
                timeout_.tv_sec = 10 * (end_.tv_sec - start_.tv_sec);
                timeout_.tv_usec = 10 * (end_.tv_usec - start_.tv_usec);
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
        std::cout << "Received FIN\n";
        if (state_ != ESTABLISHED) {
            // @TODO: strange case
        }
        state_ = CLOSE_WAIT;
        // Send ACK back to client
        PacketHeader sendHeader;
        sendHeader.type = ACK;
        sendHeader.acknowledgmentNumber = receivedHeader.sequenceNumber + 1;
        sendPacket(socket_, clientAddr_, &sendHeader, NULL, 0);
        std::cout << "Sent ACK (CLOSE_WAIT)\n";
        // @TODO: clean work and about to disconnect

        state_ = LAST_ACK;
        // Send FIN to client;
        sendHeader.type = FIN;
        sendHeader.sequenceNumber = sendBase_;
        sendHeader.acknowledgmentNumber = 0;
        sendPacket(socket_, clientAddr_, &sendHeader, NULL, 0);
        std::cout << "Sent FIN\n";
        registerEvents();
    }
    if (receivedHeader.type == DATA) {
        std::cout << "Received DATA\n";
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
            std::cout << "Sent ACK\n";
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
    } else {
        event_del(timeoutEvent_);
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

void Server::Connection::deleteEvents() {
    if (timeoutEvent_ != NULL) {
        event_del(timeoutEvent_);
    }
}
