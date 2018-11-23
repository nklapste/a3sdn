/**
 * port.cpp
 *
 * @author Nathan Klapstein (nklapste)
 * @version 0.0.0
 */

#include <errno.h>
#include <cstdio>
#include <cstdlib>
#include "port.h"

/**
 * Create a {@code Port} instance. Which represents a simple port number.
 *
 * @param portNum {@code u_int16_t} A valid port number (i.e. 0-65535)
 */
Port::Port(u_int16_t portNum) {
    Port::portNum = validatePortNum(portNum);
}

/**
 * Getter to {@code Port}s number.
 *
 * @return {@code u_int16_t}
 */
u_int16_t Port::getPortNum() {
    return portNum;
}

/**
 * Validate that a given {@code u_int16_t} is a valid port number.
 *
 * @param portNum {@code u_int16_t}
 * @return {@code u_int16_t}
 */
u_int16_t Port::validatePortNum(u_int16_t portNum) {
    if (portNum > 65535) {
        errno = EINVAL;
        perror("ERROR: port number too large");
        exit(errno);
    }
    return portNum;
}
