/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Client.cpp
 * Author: ductn
 * 
 * Created on July 25, 2016, 5:08 PM
 */

#include "Client.h"
#include <cstring>

Client::Client() {
    socket_ = -1;
    eventBase_ = NULL;
    event_ = NULL;
    connection_ = NULL;
}

void Client::registerEvents() {
    if (eventBase_ == NULL) {
        eventBase_ = event_base_new();
    }

    if (event_ == NULL) {
        event_ = event_new(eventBase_, socket_, EV_READ | EV_PERSIST,
                Client::listenHandler, this);
    }
    event_add(event_, 0);
}

void Client::runLoop() {
    event_base_loop(eventBase_, 0);
}

void Client::connect() {
    int clientSocket;
    sockaddr_in* serverAddr = new sockaddr_in();
    // Create UDP socket
    if ((clientSocket = socket(PF_INET, SOCK_DGRAM, 0)) == -1) {
        perror("socket()");
        return;
    }
    // Configure address struct
    serverAddr->sin_family = AF_INET;
    serverAddr->sin_port = htons(5050);
    serverAddr->sin_addr.s_addr = inet_addr("127.0.0.1");
    memset(serverAddr->sin_zero, '\0', sizeof (serverAddr->sin_zero));
    // Done, assign to real variable
    socket_ = clientSocket;
    serverAddr_ = serverAddr;
    if (connection_ == NULL) {
        registerEvents();
        connection_ = new Connection(clientSocket, serverAddr, CLOSED, eventBase_);
        connection_->transition(NULL);
    }
    runLoop();
}

void Client::disconnect() {
    connection_->transition(NULL);
}

void Client::send(uint8_t* data, uint32_t size) {
    // Divide data into 1450-size chunk
    int nChunk = (size % 1450 == 0) ? size / 1450 : size / 1450 + 1;
    for (int i = 0; i < nChunk; i++) {
        Data* chunk = new Data;
        if (i != nChunk - 1) {
            chunk->length = 1450;
            memcpy(chunk->data, data + i * 1450, chunk->length);
            chunk->isSent = false;
        } else {
            chunk->length = size % 1450;
            if (chunk->length == 0) {
                chunk->length = 1450;
            }
            memcpy(chunk->data, data + i * 1450, chunk->length);
            chunk->isSent = false;
            chunk->isEnd = true;
        }
        connection_->addData(chunk);
    }
    connection_->setEndACK(size);
    registerEvents();
    runLoop();
}

void Client::Connection::registerEvents() {
    if (eventBase_ == NULL) {
        eventBase_ = event_base_new();
    }

    timeoutEvent_ = event_new(eventBase_, -1, 0, Connection::timeoutHandler, this);
    if (event_add(timeoutEvent_, 0) == -1) {
        perror("event_add()");
        return;
    }
}

void Client::listenHandler(int fd, short what, void* v) {
    std::cout << "Receive\n";
    Client* client = (Client*) v;
    uint8_t* buffer = new uint8_t[1472];
    unsigned int addrLen = sizeof (*client->serverAddr_);
    int nBytes = recvfrom(fd, buffer, 1472, 0, (sockaddr*) client->serverAddr_,
            &addrLen);
    client->connection_->transition(buffer);
}

Client::Connection::Connection(int socket, sockaddr_in* serverAddr,
        uint8_t state, event_base* EVB) {
    socket_ = socket;
    serverAddr_ = serverAddr;
    state_ = state;
    eventBase_ = EVB;
}

void Client::Connection::transition(uint8_t* buffer) {
    if (buffer == NULL) {
        PacketHeader sendHeader;
        switch (state_) {
            case CLOSED:
                sendHeader.type = SYN;
                sendHeader.sequenceNumber = 0;
                sendPacket(socket_, serverAddr_, &sendHeader, NULL, 0);
                state_ = SYN_SENT;
                gettimeofday(&start_, NULL);
                registerEvents();
                break;
            case ESTABLISHED:
                sendHeader.type = FIN;
                sendHeader.sequenceNumber = sendBase_;
                sendPacket(socket_, serverAddr_, &sendHeader, NULL, 0);
                state_ = FIN_WAIT_1;
                registerEvents();
                break;
            default:
                break;
        }
    } else {
        PacketHeader receivedHeader, sendHeader;
        processHeader(buffer, receivedHeader);
        if (receivedHeader.type == SYN_ACK) {
            // Received a SYN-ACK from server, send ACK back
            gettimeofday(&end_, NULL);
            timeOut_.tv_sec = end_.tv_sec - start_.tv_sec;
            timeOut_.tv_usec = end_.tv_usec - start_.tv_usec;
            sendACK(receivedHeader.sequenceNumber);
            sendPacket(socket_, serverAddr_, &sendHeader, NULL, 0);
            breakLoop();
        }
        if (receivedHeader.type == FIN) {
            if (state_ == FIN_WAIT_1) {
                sendACK(receivedHeader.sequenceNumber);
            } else if (state_ == FIN_WAIT_2) {
                sendACK(receivedHeader.sequenceNumber);
            } else {
                // Wrong packet, do nothing
            }
            state_ = TIME_WAIT; // In both case, transit to Time-wait state
            // @TODO: 2MLS timeout and close
            breakLoop();
        }
        if (receivedHeader.type == ACK) {
            if (state_ == FIN_WAIT_1) {
                // Switch to FIN-WAIT-2
                state_ = FIN_WAIT_2;
            } else if (state_ == CLOSING) {
                // Switch to TIME-WAIT
                state_ = TIME_WAIT;
                // @TODO: 2MLS timeout and close
            } else if (state_ == ESTABLISHED) {
                // Check if ackNo is compatible with send base
                if (receivedHeader.acknowledgmentNumber == endACK_) {
                    // Server receive all data, break loop and exit
                    breakLoop();
                    return;
                }
                if (receivedHeader.acknowledgmentNumber - sendBase_ <= 1450) {
                    data_.erase(data_.begin()); // Delete ACKed chunk
                    sendBase_ = receivedHeader.acknowledgmentNumber;
                    // Loop through vector and send not-sent chunk
                    for (int i = 0; i < windowsSize_ && i < data_.size(); i++) {
                        if (data_[i]->isSent == false) {
                            sendData(data_[i]);
                            data_[i]->isSent = true;
                        }
                    }
                }
                registerEvents();
            }
        }
    }
}

void Client::Connection::sendACK(uint32_t seqNo) {
    PacketHeader sendHeader;
    sendHeader.type = ACK;
    sendHeader.acknowledgmentNumber = seqNo + 1;
    sendPacket(socket_, serverAddr_, &sendHeader, NULL, 0);
}

void Client::Connection::sendData(Data* data) {
    PacketHeader sendHeader;
    sendHeader.type = DATA;
    sendHeader.sequenceNumber = data->sequenceNumber;
    sendHeader.length = data->length;
    if (data->isEnd) {
        sendHeader.isEnd = 1;
    } else {
        sendHeader.isEnd = 0;
    }
    sendPacket(socket_, serverAddr_, &sendHeader, data->data, data->length);
}

void Client::Connection::addData(Data* data) {
    data_.push_back(data);
}

void Client::Connection::breakLoop() {
    event_base_loopbreak(eventBase_);
}

void Client::Connection::setEndACK(uint32_t endACK) {
    endACK_ = endACK;
}

void Client::Connection::timeoutHandler(int fd, short what, void* v) {
    Connection* conn = (Connection*) v;
    switch (conn->state_) {
        case SYN_SENT:
            conn->state_ = CLOSED;
            conn->transition(NULL);
            break;
        case ESTABLISHED:
            conn->sendData(conn->data_.front());
            break;
        case FIN_WAIT_1:
            conn->state_ = ESTABLISHED;
            conn->transition(NULL);
            break;
        default:
            break;
    }
}
