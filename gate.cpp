/**
 * gate.cpp
 *
 * @author Nathan Klapstein (nklapste)
 * @version 0.0.0
 */

#include "gate.h"

/**
 * Getter for a {@code Gate}'s ID.
 *
 * @return {@code unit} the ID number of the Gate.
 */
uint Gate::getGateID() const {
    return gateID;
}

/**
 * Getter for a {@code Gate}'s port number.
 *
 * @return {@code unit} the port number of the Gate.
 */
uint Gate::getPortNum() const {
    return portNum;
}
