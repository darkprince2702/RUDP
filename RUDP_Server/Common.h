/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Common.h
 * Author: ductn
 *
 * Created on July 20, 2016, 3:19 PM
 */

#ifndef COMMON_H
#define COMMON_H

/* State define */
#define LISTEN 1  // Server listening
#define SYN_SENT 2  // Client sent SYN to server
#define SYN_RECEIVED 3  // Server received SYN from client
#define ESTABLISHED 4  // Connection established from both client and server
#define FIN_WAIT_1 5  // 
#define FIN_WAIT_2 6
#define CLOSING 7
#define TIME_WAIT 8
#define CLOSE_WAIT 9
#define LAST_ACK 10
#define CLOSED 11

struct PacketHeaders {
    uint8_t type;
    uint32_t sequenceNumber;
    uint8_t windowSize;
};

void sendPacket(int fd, uint8_t type, uint32_t sequenceNumber, uint8_t* data, 
        uint32_t dataLength);

void receivePacket(int fd, uint8_t* buffer, uint32_t length);

#endif /* COMMON_H */