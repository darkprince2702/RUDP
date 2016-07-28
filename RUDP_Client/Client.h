/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Client.h
 * Author: ductn
 *
 * Created on July 25, 2016, 5:08 PM
 */

#ifndef CLIENT_H
#define CLIENT_H

#include "../RUDP_Server/Common.h"
#include <event.h>
#include <queue>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iostream>
#include <string>
#include <stdlib.h>

class Client {
public:

    class Connection {
    public:
        static void timeoutHandler(evutil_socket_t fd, short what, void *v);
        Connection(int socket, sockaddr_in* serverAddr, uint8_t state,
                event_base* EVB);
        void registerEvents();
        void sendACK(uint32_t seqNo);
        void sendFirstBatch();
        void sendData(Data* data);
        void resend();
        void transition(uint8_t* buffer);
        void breakLoop();
        void addData(Data* data);
        void setEndACK(uint32_t endACK);
        uint32_t getSendBase(){return sendBase_;}
        void markStartTime();
        void markEndTime();
        void calculateTime();
        int nTimeout;
    private:
        int socket_;
        sockaddr_in* serverAddr_;
        uint8_t state_;
        uint32_t receiveBase_;
        uint32_t sendBase_;
        uint32_t endACK_;
        std::vector<Data*> data_;
        event_base* eventBase_;
        event* timeoutEvent_;
        uint16_t windowSize_;
        timeval RTT_;
        timeval devRTT_;
        timeval RTO_;
        timeval timeStart_;
        timeval timeEnd_;
    };

    Client();
//    virtual ~Client();
    void connect();
    void disconnect();
    void send(uint8_t* data, uint32_t size);
    void registerEvents();
    static void listenHandler(evutil_socket_t fd, short what, void *v);
    void runLoop();
private:
    int socket_;
    sockaddr_in* serverAddr_;
    event_base* eventBase_;
    event* event_;
    Connection* connection_;
};

#endif /* CLIENT_H */

