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
    client->send(data, buffer.size());
    std::cout << "Send data end\n";
    client->disconnect();
    return 0;
}