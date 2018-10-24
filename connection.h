/**
 * connection.h
 *
 * @author Nathan Klapstein (nklapste)
 * @version 0.0.0
 */

#ifndef A2SDN_CONNECTION_H
#define A2SDN_CONNECTION_H

#include <string>

using namespace std;

struct Connection {
    int swID;
    string rFIFO;
    string wFIFO;
    int rfd;
    int wfd;
};

#endif //A2SDN_CONNECTION_H
