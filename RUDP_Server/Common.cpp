/* 
 * File:   Common.h
 * Author: ductn
 *
 * Created on July 20, 2016, 3:19 PM
 */

#include <string.h>
#include <sys/socket.h>
#include <stdio.h>

#include "Common.h"

void processHeader(uint8_t* buffer, PacketHeader& header) {
    memcpy(&header, buffer, sizeof(header));
}

void processData(uint8_t* buffer, uint8_t* data, uint8_t length) {
    memcpy(data, buffer + sizeof(PacketHeader), length);
}

void sendPacket(int fd, sockaddr_in* addr, PacketHeader* header, uint8_t* data,
        uint32_t dataLength) {
    uint8_t buffer[1472];
    memcpy(buffer, header, sizeof (header));
    if (data != NULL) {
        memcpy(buffer + sizeof (header), data, 1450);
    }
    int nBytes = sendto(fd, buffer, sizeof(buffer), 0, (sockaddr*) addr, sizeof(*addr));
    if (nBytes <= 0) {
        perror("sendto");
    }
}