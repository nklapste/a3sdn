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

class Switch {
public:
    flowTable flowTable;

    Switch(uint ipLow, uint ipHigh);

    void makeFIFO(string &trafficFile);
};

#endif //A2SDN_SWITCH_H
