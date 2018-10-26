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
#include <vector>
#include "connection.h"

#define MAX_SWITCHES 7
#define LIST_CMD "list"
#define EXIT_CMD "exit"
#define CONTROLLER_ID 0

using namespace std;

typedef tuple<uint, uint> ipRange;

class Controller {
public:
    Controller(uint nSwitches);
    uint getNumSwitches();
    Connection getConnection(uint switchId);
    void start();
private:
    uint nSwitches;
    vector<Connection> connections;
};

#endif //A2SDN_CONTROLLER_H