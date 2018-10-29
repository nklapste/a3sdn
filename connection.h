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

    string getSendFIFOName();

    string getReceiveFIFOName();

    int openSendFIFO();

    int openReceiveFIFO();

private:
    string receiveFIFOName;
    string sendFIFOName;

    void makeFIFO(string &FIFOName);

    const string makeFIFOName(uint senderId, uint receiverId);
};

#endif //A2SDN_FIFO_H
