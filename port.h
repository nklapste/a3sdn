//
// Created by nklap on 2018-11-09.
//

#ifndef A3SDN_PORT_H
#define A3SDN_PORT_H


#include <sys/param.h>

class Port {
public:
    explicit Port(uint portNum);

    uint getPortNum();

    static uint validatePortNum(uint portNum);

private:
    uint portNum;
};


#endif //A3SDN_PORT_H
