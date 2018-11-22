/**
 * port.h
 *
 * @author Nathan Klapstein (nklapste)
 * @version 0.0.0
 */

#ifndef A3SDN_PORT_H
#define A3SDN_PORT_H


#include <sys/param.h>

class Port {
public:
    explicit Port(u_int16_t portNum);

    u_int16_t getPortNum();

    static u_int16_t validatePortNum(u_int16_t portNum);

private:
    u_int16_t portNum;
};


#endif //A3SDN_PORT_H
