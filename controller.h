/**
 * controller.h
 *
 * @author Nathan Klapstein (nklapste)
 * @version 0.0.0
 */

#ifndef A2SDN_CONTROLLER_H
#define A2SDN_CONTROLLER_H


#include <string>
#include <tuple>
#include "connection.h"

#define MAX_NSW 7

using namespace std;

typedef tuple<uint, uint> ipRange;

class Controller {
public:
    Controller(int nSwitches);

    int getNumSwitches();

    int openReadFIFO(uint id);

    int openWriteFIFO(uint id);

    void addFIFO(uint id);

    const string getFIFOName(int x, int y);

    void makeFIFO(string &trafficFile);

private:
    string name;
    int nSwitches;
    int controllerID = 0;
    Connection conns[MAX_NSW];
};

#endif //A2SDN_CONTROLLER_H
