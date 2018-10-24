/**
 * switch.h
 *
 * @author Nathan Klapstein (nklapste)
 * @version 0.0.0
 */

#ifndef A2SDN_SWITCH_H
#define A2SDN_SWITCH_H


using namespace std;

#include <string>
#include "flowTable.h"
#include "SDNBiFIFO.h"

class Switch {
public:
    flowTable flowTable;

    Switch(int swi, int swj, int swk, string &trafficFile, uint ipLow, uint ipHigh);

    uint swi; // ID of the switch itself
    int swj; // ID of the first switch to connect to
    int swk; // ID of the second switch to connect to
    SDNBiFIFO swjBiFIFO = nullptr;
    SDNBiFIFO *swkBiFIFO = nullptr;
    string trafficFile;
};

#endif //A2SDN_SWITCH_H
