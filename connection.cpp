/**
 * a2sdn connection.cpp
 *
 * @author Nathan Klapstein (nklapste)
 * @version 0.0.0
 */

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "connection.h"

using namespace std;

/**
 * Make a FIFO for a {@code Connection}.
 *
 * @param FIFOName the name to be given to the FIFO.
 */
void Connection::makeFIFO(string &FIFOName) {
    printf("DEBUG: making FIFO at: %s\n", FIFOName.c_str());
    int status = mkfifo(FIFOName.c_str(), S_IRUSR | S_IWUSR | S_IRGRP |
                                          S_IWGRP | S_IROTH | S_IWOTH );
    if (errno || status == -1) {
        perror("ERROR: error creating FIFO connection");
        errno = 0;
    }
}

/**
 * Opens a Connection for reading a switch with id.
 *
 * @return
 */
int Connection::openReceiveFIFO() {
    printf("DEBUG: opening receiveFIFO: %s\n", receiveFIFOName.c_str());
    int i = open(receiveFIFOName.c_str(), O_RDONLY | O_NONBLOCK);
    if (!i) {
        perror("ERROR: opening receive FIFO");
    }
    return i;
}

/**
 * Opens a Connection for writing a switch with id.
 *
 * @return
 */
int Connection::openSendFIFO() {
    printf("DEBUG: opening sendFIFO: %s\n", receiveFIFOName.c_str());
    int i = open(sendFIFOName.c_str(), O_WRONLY | O_NONBLOCK);
    if (!i) {
        perror("ERROR: opening send FIFO");
    }
    return i;
}


/**
 * Getter for the Receiving FIFO filename/pipename.
 *
 * @return {@code std::string}
 */
string Connection::getReceiveFIFOName() {
    return receiveFIFOName;
}

/**
 * Getter for the Sending FIFO filename/pipename.
 *
 * @return {@code std::string}
 */
string Connection::getSendFIFOName() {
    return sendFIFOName;
}

/**
 * Generate a FIFO name based from the sender's id and receiver's id.
 *
 * @param senderId {@code uint}
 * @param receiverId {@code uint}
 * @return
 */
const string Connection::makeFIFOName(uint senderId, uint receiverId) {
    return "fifo-" + std::to_string(senderId) + "-" + std::to_string(receiverId);
}

/**
 *
 * @param srcID {@code uint}
 * @param dstID {@code uint}
 */
Connection::Connection(uint srcID, uint dstID) {
    // make the send FIFO
    sendFIFOName = makeFIFOName(srcID, dstID);
    printf("INFO: Making Connection:\n"
           "\tsrc: %u dst: %u sendFIFO: %s\n", srcID, dstID, sendFIFOName.c_str());
    makeFIFO(sendFIFOName);

    // make the receiving FIFO
    receiveFIFOName = makeFIFOName(dstID, srcID);
    printf("INFO: Making Connection:\n"
           "\tsrc: %u dst: %u receiveFIFO: %s\n", srcID, dstID, receiveFIFOName.c_str());
    makeFIFO(receiveFIFOName);
}
