//
// Created by nklap on 2018-11-09.
//

#include <errno.h>
#include <cstdio>
#include <cstdlib>
#include "port.h"

/**
 * Create a {@code Port} instance. Which represents a simple port number.
 *
 * @param portNum {@code uint} A valid port number (i.e. 0-65535)
 */
Port::Port(uint portNum) {
    Port::portNum = validatePortNum(portNum);
}

/**
 * Getter to {@code Port}s number.
 *
 * @return {@code uint}
 */
uint Port::getPortNum() {
    return portNum;
}

/**
 * Validate that a given {@code uint} is a valid port number.
 *
 * @param portNum {@code uint}
 * @return {@code uint}
 */
uint Port::validatePortNum(uint portNum) {
    if (portNum>65535){
        errno = EINVAL;
        perror("ERROR: port number too large");
        exit(errno);
    }
    return portNum;
}
