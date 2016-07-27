/* 
 * File:   Common.h
 * Author: ductn
 *
 * Created on July 20, 2016, 3:19 PM
 */

#include <string.h>
#include <sys/socket.h>
#include <stdio.h>
#include <iostream>
#include "Common.h"

void processHeader(uint8_t* buffer, PacketHeader* header) {
    memcpy(header, buffer, sizeof(PacketHeader));
}

void processData(uint8_t* buffer, uint8_t* data, uint8_t length) {
    memcpy(data, buffer + sizeof(PacketHeader), length);
}

void sendPacket(int fd, sockaddr_in* addr, PacketHeader* header, uint8_t* data,
        uint32_t dataLength) {
    uint8_t buffer[1472];
    memset(buffer, 0, 1472);
    std::cout << "Sent header: " << (int) header->type << std::endl;
    memcpy(buffer, header, sizeof (PacketHeader));
    if (data != NULL) {
        memcpy(buffer + sizeof (PacketHeader), data, dataLength);
    }
    int nBytes = sendto(fd, buffer, sizeof(buffer), 0, (sockaddr*) addr, sizeof(*addr));
    if (nBytes <= 0) {
        perror("sendto");
    }
}