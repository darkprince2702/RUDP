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
    } else {
        event_del(event_);
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
    if ((clientSocket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
        perror("socket()");
        return;
    }
    std::cout << "Client FD is: " << clientSocket << std::endl;
    // Configure address struct
    serverAddr->sin_family = AF_INET;
    serverAddr->sin_port = htons(5050);
    const char serverIP[] = "118.102.6.118";
    serverAddr->sin_addr.s_addr = inet_addr(serverIP);
    inet_pton(AF_INET, serverIP, &serverAddr->sin_addr);
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
    runLoop();
    std::cout << "Number of timeout: " << connection_->nTimeout << std::endl;
}

void Client::send(uint8_t* data, uint32_t size) {
    // Divide data into 1450-size chunk
    int nChunk = (size % 1450 == 0) ? size / 1450 : size / 1450 + 1;
    uint32_t seqNo = connection_->getSendBase();
    for (int i = 0; i < nChunk; i++) {
        Data* chunk = new Data();
        if (i != nChunk - 1) {
            chunk->length = 1450;
            chunk->sequenceNumber = seqNo;
            seqNo += chunk->length;
            chunk->data = new uint8_t[chunk->length];
            memcpy(chunk->data, data + i * 1450, chunk->length);
            chunk->isSent = false;
        } else {
            chunk->length = size % 1450;
            if (chunk->length == 0) {
                chunk->length = 1450;
            }
            chunk->sequenceNumber = seqNo;
            seqNo += chunk->length;
            chunk->data = new uint8_t[chunk->length];
            memcpy(chunk->data, data + i * 1450, chunk->length);
            chunk->isSent = false;
            chunk->isEnd = true;
        }
        connection_->addData(chunk);
    }
    connection_->setEndACK(size);
    connection_->sendFirstBatch();
    registerEvents();
    runLoop();
}

void Client::Connection::registerEvents() {
    if (eventBase_ == NULL) {
        eventBase_ = event_base_new();
    }

    if (timeoutEvent_ == NULL) {
        timeoutEvent_ = event_new(eventBase_, -1, 0, Connection::timeoutHandler,
                this);
    } else {
        event_del(timeoutEvent_);
    }

    if (event_add(timeoutEvent_, &RTO_) == -1) {
        perror("event_add()");
        return;
    }
}

void Client::listenHandler(int fd, short what, void* v) {
    std::cout << "Receive 1 packet\n";
    Client* client = (Client*) v;
    uint8_t* buffer = new uint8_t[1472];
    unsigned int addrLen = sizeof (sockaddr_in);
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
    timeoutEvent_ = NULL;
    sendBase_ = 0;
    receiveBase_ = 0;
    RTO_.tv_sec = 1;
}

void Client::Connection::transition(uint8_t* buffer) {
    if (buffer == NULL) {
        PacketHeader sendHeader;
        switch (state_) {
            case CLOSED:
                // Send SYN packet
                sendHeader.type = SYN;
                sendHeader.sequenceNumber = sendBase_;
                sendPacket(socket_, serverAddr_, &sendHeader, NULL, 0);
                markStartTime();
                state_ = SYN_SENT;
                registerEvents();
                break;
            case ESTABLISHED:
                // Send FIN packet
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
        processHeader(buffer, &receivedHeader);
        if (receivedHeader.type == SYN_ACK) {
            // Received a SYN-ACK from server, send ACK back
            markEndTime();
            calculateTime();
            windowSize_ = receivedHeader.windowSize;
            sendBase_ = receivedHeader.acknowledgmentNumber;
            receiveBase_ = receivedHeader.acknowledgmentNumber + 1;
            sendACK(receiveBase_);
            state_ = ESTABLISHED;
            breakLoop();
        }
        if (receivedHeader.type == FIN) {
            markEndTime();
            calculateTime();
            if (state_ == FIN_WAIT_1) {
                receiveBase_ = receivedHeader.sequenceNumber + 1;
                sendACK(receiveBase_);
            } else if (state_ == FIN_WAIT_2) {
                receiveBase_ = receivedHeader.sequenceNumber + 1;
                sendACK(receiveBase_);
            } else {
                // Wrong packet, do nothing
            }
            state_ = TIME_WAIT; // In both case, transit to Time-wait state
            // @TODO: 2MLS timeout and close
            breakLoop();
        }
        if (receivedHeader.type == ACK) {
            markEndTime();
            calculateTime();
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
                    for (int i = 0; i < windowSize_ && i < data_.size(); i++) {
                        if (data_[i]->isSent == false) {
                            sendData(data_[i]);
                            data_[i]->isSent = true;
                        }
                    }
                    markStartTime();
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
    memset(&sendHeader, 0, sizeof (sendHeader));
    sendHeader.type = DATA;
    sendHeader.acknowledgmentNumber = 100;
    sendHeader.sequenceNumber = data->sequenceNumber;
    sendHeader.length = data->length;
    if (data->isEnd) {
        sendHeader.isEnd = 1;
    } else {
        sendHeader.isEnd = 0;
    }
    sendPacket(socket_, serverAddr_, &sendHeader, data->data, data->length);
    std::cout << "Send data with sequence: " << sendHeader.sequenceNumber <<
            std::endl;
}

void Client::Connection::addData(Data* data) {
    data_.push_back(data);
}

void Client::Connection::breakLoop() {
    event_base_loopbreak(eventBase_);
}

void Client::Connection::setEndACK(uint32_t endACK) {
    endACK_ = sendBase_ + endACK;
}

void Client::Connection::timeoutHandler(int fd, short what, void* v) {
    Connection* conn = (Connection*) v;
    std::cout << "Timeout invoke with state: " << (int) conn->state_ << std::endl;
    switch (conn->state_) {
        case SYN_SENT:
            conn->state_ = CLOSED;
            conn->transition(NULL);
            break;
        case ESTABLISHED:
            conn->resend();
            break;
        case FIN_WAIT_1:
            conn->state_ = ESTABLISHED;
            conn->transition(NULL);
            break;
        default:
            break;
    }
    conn->nTimeout++;
}

void Client::Connection::sendFirstBatch() {
    for (int i = 0; i < windowSize_ && i < data_.size(); i++) {
        if (data_[i]->isSent == false) {
            sendData(data_[i]);
            data_[i]->isSent = true;
        }
    }
    markStartTime();
    registerEvents();
}

void Client::Connection::resend() {
    std::cout << "Resend data\n";
    for (int i = 0; i < windowSize_ && i < data_.size(); i++) {
        sendData(data_[i]);
        if (data_[i]->isSent == false) {
            data_[i]->isSent = true;
        }
    }
    markStartTime();
    registerEvents();
}

void Client::Connection::markStartTime() {
    timeval now;
    gettimeofday(&now, NULL);
    if (now.tv_sec > timeStart_.tv_sec && now.tv_usec > timeStart_.tv_usec) {
        timeStart_ = now;
    }
}

void Client::Connection::markEndTime() {
    timeval now;
    gettimeofday(&now, NULL);
    if (now.tv_sec > timeEnd_.tv_sec && now.tv_usec > timeEnd_.tv_usec) {
        timeEnd_ = now;
    }
}

void Client::Connection::calculateTime() {
    time_t sampleRTTSec = timeEnd_.tv_sec - timeStart_.tv_sec;
    time_t sampleRTTUSec = timeEnd_.tv_usec - timeStart_.tv_usec;
    // New RTT is average of current RTT and time between timeStart and timeEnd
    devRTT_.tv_sec = ((1000 - BETA) * devRTT_.tv_sec + BETA * abs(sampleRTTSec - RTT_.tv_sec)) / 1000;
    devRTT_.tv_usec = ((1000 - BETA) * devRTT_.tv_usec + BETA * abs(sampleRTTUSec - RTT_.tv_usec)) / 1000;
    RTT_.tv_sec = ((1000 - ALPHA) * RTT_.tv_sec + ALPHA * sampleRTTSec) / 1000;
    RTT_.tv_usec = ((1000 - ALPHA) * RTT_.tv_usec + ALPHA * sampleRTTUSec) / 1000;
    // Calculate new RTO
    RTO_.tv_sec = RTT_.tv_sec + 4 * devRTT_.tv_sec;
    RTO_.tv_usec = RTT_.tv_usec + 4 * devRTT_.tv_usec;
//    std::cout << "New RTT: " << RTT_.tv_sec << " sec, " << RTT_.tv_usec <<
//            " microsecond\n";
//    std::cout << "New RTO: " << RTO_.tv_sec << " sec, " << RTO_.tv_usec <<
//            " microsecond\n";
}
