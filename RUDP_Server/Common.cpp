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
    int bufferSize = 1472;
    uint8_t buffer[bufferSize];
    memset(buffer, 0, bufferSize);
    std::cout << "Sent header: " << (int) header->type << std::endl;
    memcpy(buffer, header, sizeof (PacketHeader));
    if (data != NULL) {
        memcpy(buffer + sizeof (PacketHeader), data, dataLength);
    }
    int nBytes = sendto(fd, buffer, bufferSize, 0, (sockaddr*) addr, sizeof(sockaddr_in));
    if (nBytes <= 0) {
        perror("sendto");
    }
}