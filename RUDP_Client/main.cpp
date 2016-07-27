/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   main.cpp
 * Author: ductn
 *
 * Created on July 20, 2016, 3:13 PM
 */

#include <cstdlib>
#include <fstream>
#include <vector>
#include "Client.h"

using namespace std;
int main(int argc, char** argv) {
    Client* client = new Client();
    client->connect();
    // Read file
    std::cout << "Go here 1\n";
    std::vector<uint8_t> buffer;
    uint8_t* data;
    char temp;
    std::ifstream fStream;
    fStream.open("input", std::ios::in | std::ios::binary);
    while(!fStream.eof()) {
        fStream.get(temp);
        buffer.push_back(static_cast<uint8_t>(temp));
    }
    data = &buffer[0];
    std::cout << "Go here 2\n";
    client->send(data, buffer.size());
    client->disconnect();
    return 0;
}

