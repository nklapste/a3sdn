//
// Created by ubuntu-dev on 24/10/18.
//

#ifndef A2SDN_FIFO_H
#define A2SDN_FIFO_H

#include <sys/types.h>
#include <string>

using namespace std;

class SDNBiFIFO {
public:
    SDNBiFIFO * SDNBiFIFO(uint senderId, uint receiverId);
    uint senderId;
    uint receiverId;
    string receiveFIFOName;
    string sendFIFOName;
    const string getFIFOName(uint senderId, uint receiverId)
    void makeFIFO(string &FIFOName);
    int openSendFIFO();
    int openReceiveFIFO();
};

#endif //A2SDN_FIFO_H
