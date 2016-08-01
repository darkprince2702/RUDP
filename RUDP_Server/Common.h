/* 
 * File:   Common.h
 * Author: ductn
 *
 * Created on July 20, 2016, 3:19 PM
 */

#ifndef COMMON_H
#define COMMON_H

#include <assert.h>
#include <stdint.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>
#include <iostream>
#include <boost/chrono.hpp>

/* State define */
#define LISTEN 1  // Server listening
#define SYN_SENT 2  // Client sent SYN to server
#define SYN_RECEIVED 3  // Server received SYN from client
#define ESTABLISHED 4  // Connection established from both client and server
#define FIN_WAIT_1 5  // Sent FIN to server and wait for ACK
#define FIN_WAIT_2 6  // Received ACK from server that confirmed to close
#define CLOSING 7  // Received FIN from FIN_WAIT_1, wait for a ACK from server
#define TIME_WAIT 8  // Recied FIN from FIN_WAIT_2, send ACK to server and timout
#define CLOSE_WAIT 9  // Received FIN from client, send ACK back
#define LAST_ACK 10  // Sent FIN to client, and wait ACK from client
#define CLOSED 11  // Timouted from TIME_WAIT (client) or received ACK in
                   // LAST_ACK state (server)

/* Packet type define */
#define DATA 0
#define SYN 1
#define SYN_ACK 2
#define ACK 3
#define FIN 4
#define FIN_ACK 5
#define RST 6

/* Other define */
#define WINDOW_SIZE 45
#define ALPHA 125
#define BETA 250

struct PacketHeader {
    uint8_t type;
    uint16_t length;
    uint32_t sequenceNumber;
    uint32_t acknowledgmentNumber;
    uint16_t windowSize;
    uint8_t isEnd;
};

struct Data {
    uint8_t* data;
    uint32_t length;
    uint32_t sequenceNumber;
    bool isEnd;
    timeval sentTime;
};

void sendPacket(int fd, sockaddr_in* addr, PacketHeader* header, uint8_t* data, 
        uint32_t dataLength);

void receivePacket(int fd, uint8_t* buffer, uint32_t length);

void processHeader(uint8_t* buffer, PacketHeader* header);

void processData(uint8_t* buffer, uint8_t* data, uint32_t length);

#endif /* COMMON_H */