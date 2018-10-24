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
    Controller(int num, int max);

    int getNumSwitches();

    void openConn(char id);

    int openReadFIFO(int id);

    int openWriteFIFO(int id);

    void addFIFO(int id);

    void initConn(int id);

    const string getFIFOName(int x, int y);

    void makeFIFO(string &trafficFile);

private:
    string name;
    int nSwitches;
    Connection conns[MAX_NSW];
};

#endif //A2SDN_CONTROLLER_H
