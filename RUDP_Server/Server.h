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
#include <map>
#include <vector>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <fstream>
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "Common.h"

class Server {
public:

    class Connection {
    public:
        static void timeoutHandler(evutil_socket_t fd, short what, void *v);
        Connection(int socket, sockaddr_in* clientAddr, uint8_t state, event_base* EVB);
        void registerEvents();
        void transition(uint8_t* buffer);
        void writeToFile();
        void deleteEvents();
    private:
        int socket_;
        sockaddr_in* clientAddr_;
        uint8_t state_;
        uint32_t receiveBase_;
        uint32_t sendBase_;
        std::map<uint32_t, Data*> chunks_;
        event_base* eventBase_;
        event* timeoutEvent_;
        timeval timeout_;
        timeval start_;
        timeval end_;
    };

    Server(int port);
    //    virtual ~Server();
    void createAndListenSocket();
    void registerEvents();
    void serve();
    void stop();
    static void listenHandler(evutil_socket_t fd, short what, void *v);
    event_base* getEventBase();
    void addConnection(Connection* conn);
    Connection* getConnection(const sockaddr_in* address);
private:
    int port_;
    int socket_;
    struct event_base* eventBase_;
    event* listenEvent_;
    std::vector<Connection*> connections_;
};

#endif /* SERVER_H */

