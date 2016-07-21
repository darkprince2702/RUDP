/* 
 * File:   Common.h
 * Author: ductn
 *
 * Created on July 20, 2016, 3:19 PM
 */

#ifndef COMMON_H
#define COMMON_H

#include <assert.h>

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

struct PacketHeaders {
    uint8_t type;
    uint32_t sequenceNumber;
    uint8_t windowSize;
};

void sendPacket(int fd, uint8_t type, uint32_t sequenceNumber, uint8_t* data, 
        uint32_t dataLength);

void receivePacket(int fd, uint8_t* buffer, uint32_t length);

void processHeader(uint8_t* buffer, PacketHeaders& header);

void processData(uint8_t* buffer, uint32_t length);

class Connection {
private:
    uint8_t state_;
    uint32_t sequenceNumber_;
    uint32_t nexReadSeqNum_;
    uint8_t* buffer;
public:
    Connection();
    void connect();
    void sendData(uint8_t* data);
    void close();
};

class Server {
private:
    
public:
    
};

#endif /* COMMON_H */