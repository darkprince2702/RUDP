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

class Client {
public:
    
    class Connection {
    public:
        static void timeoutHandler(evutil_socket_t fd, short what, void *v);
        Connection(int socket, sockaddr_in* clientAddr, uint8_t state, event_base* EVB);
        void registerEvents();
        void processPacket(uint8_t* data);
        void transition(uint8_t* buffer);
    private:
        int socket_;
        sockaddr_in* clientAddr_;
        uint8_t state_;
        uint32_t receiveBase_;
        uint32_t sendBase_;
        std::queue<Data*> data_;
        event_base* eventBase_;
        event* timeoutEvent_;
    };
    
    Client();
    virtual ~Client();
    void connect();
    void disconnect();
    void sendData(uint8_t* data);
private:
    int socket_;
    event_base* eventBase_;
    event* event_;
    Connection* connection_;
};

#endif /* CLIENT_H */

