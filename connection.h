/**
 * a2sdn connection.h
 *
 * @author Nathan Klapstein (nklapste)
 * @version 0.0.0
 */

#ifndef A2SDN_FIFO_H
#define A2SDN_FIFO_H

#include <string>
#include <sys/types.h>

using namespace std;

class Connection {
public:
    Connection(uint srcID, uint dstID);

    string getReceiveFIFOName();

    string getSendFIFOName();

    int openReceiveFIFO();

    int openSendFIFO();

private:
    string receiveFIFOName;
    string sendFIFOName;

    const string makeFIFOName(uint senderId, uint receiverId);

    void makeFIFO(string &FIFOName);
};

#endif //A2SDN_FIFO_H
