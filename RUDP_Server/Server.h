/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Server.h
 * Author: ductn
 *
 * Created on July 22, 2016, 10:35 AM
 */

#ifndef SERVER_H
#define SERVER_H

#include <event.h>
#include <list>
#include <vector>

class Server {
public:

    class Connection {
    public:
        void timeoutHandler(evutil_socket_t fd, short what, void *v);
        Connection();
        void registerEvents();
        void processPacket(uint8_t* data);
    private:
        sockaddr_in clientAddr;
        uint8_t state_;
        uint32_t lastestAckNo_;
        std::list<uint8_t*> buffer;
        event_base* eventBase_;
        event* timeoutEvent_;
    };

    Server(int port);
    virtual ~Server();
    void createAndListenSocket();
    void registerEvents();
    void serve();
    void stop();
    void listenHandler(evutil_socket_t fd, short what, void *v);
    event_base* getEventBase();
    
private:
    int port_;
    int socket_;
    struct event_base* eventBase_;
    event* listenEvent_;
    std::vector<Connection> connections_;
};

#endif /* SERVER_H */

