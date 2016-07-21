/* 
 * File:   Common.h
 * Author: ductn
 *
 * Created on July 20, 2016, 3:19 PM
 */

#include <stdint.h>

#include "Common.h"

void Connection::connect() {
    assert(state_ == CLOSED);
    // @TODO: Send SYNC to server

    for (;;) {
        // @TODO: Wait for SYNC-ACK from server

        // @TODO: Send ACK back to server

        break;
    }

    state_ = ESTABLISHED;
}

void Connection::sendData(uint8_t* data) {
    
}


void Connection::close() {
    // @TODO: Send FIN to server

    state_ = FIN_WAIT_1;

    for (;;) {
        switch (state_) {
            case FIN_WAIT_1:
                // @TODO: Wait for ACK from server
                
                state_ = FIN_WAIT_2;
                break;
            case FIN_WAIT_2:
                // @TODO: Wait for FIN from server
                
                state_ = TIME_WAIT;
                break;
            case TIME_WAIT:
                // @TODO: Send ACK to server
                
                // @TODO: Wait for timeout
                
                break;
            default:
                break;
        }
        
        // Ready to close the connection
        if (state_ = CLOSED) {
            break;
        }
    }
}
