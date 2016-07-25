/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Client.cpp
 * Author: ductn
 * 
 * Created on July 25, 2016, 5:08 PM
 */

#include "Client.h"

Client::Client() {
    socket_ = -1;
    eventBase_ = NULL;
    event_ = NULL;
    connection_ = NULL;
}

void Client::connect() {
    
}

void Client::disconnect() {

}

void Client::sendData(uint8_t* data) {

}