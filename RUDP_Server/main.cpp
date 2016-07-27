/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   main.cpp
 * Author: ductn
 *
 * Created on July 20, 2016, 3:11 PM
 */

#include <cstdlib>
#include "Server.h"

using namespace std;

int main(int argc, char** argv) {
    Server* server = new Server(5050);
    server->serve();
    return 0;
}

