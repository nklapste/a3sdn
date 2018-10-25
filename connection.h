//
// Created by ubuntu-dev on 24/10/18.
//

#ifndef A2SDN_FIFO_H
#define A2SDN_FIFO_H

#include <sys/types.h>
#include <string>

using namespace std;

class Connection {
public:
    Connection(uint srcID, uint dstID);
    string getSendFIFOName();
    string getReceiveFIFOName();
    int getSendFIFO();
    int getReceiveFIFO();

private:
    string receiveFIFOName;
    string sendFIFOName;
    uint srcID;
    uint dstID;
    void makeFIFO(string &FIFOName);
    int openSendFIFO();
    int openReceiveFIFO();
    const string makeFIFOName(uint senderId, uint receiverId);
};

#endif //A2SDN_FIFO_H
