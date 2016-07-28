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
#include <sys/stat.h>

using namespace std;

int main(int argc, char** argv) {
    Client* client = new Client();
    client->connect();
    // Read file
    const char filename[] = "input";
    struct stat st;
    if (stat(filename, &st) == -1) {
        perror("stat()");
    }
    int filesize = st.st_size;
    std::vector<uint8_t> buffer;
    uint8_t* data;
    char temp;
    std::ifstream fStream;
    fStream.open("input", std::ios::in | std::ios::binary);
    //    while(!fStream.eof()) {
    //        fStream.get(temp);
    //        buffer.push_back(static_cast<uint8_t>(temp));
    //    }
    //    data = &buffer[0];
    data = new uint8_t[filesize];
    fStream.read((char*) data, filesize);
    std::cout << "Size of file is: " << filesize << std::endl;
    client->send(data, filesize);
    client->disconnect();
    return 0;
}