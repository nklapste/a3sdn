/**
 * a2sdn connection.cpp
 *
 * @author Nathan Klapstein (nklapste)
 * @version 0.0.0
 */

#include <fcntl.h>
#include <sys/signalfd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "connection.h"

using namespace std;

/**
 * Initialize a Connection between either a Controller and a Switch or a Switch and another Switch.
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

/**
 * Initialize a Null/dummy connection.
 *
 * Set the FIFONames to /dev/null just for safety.
 *
 * i.e. if we open this dummy connection nothing bad will happen we literally get nothing.
 */
Connection::Connection() {
    sendFIFOName = "/dev/null";
    receiveFIFOName = "/dev/null";
}

/**
 * Getter for the ReceiveFIFO filename.
 *
 * @return {@code std::string}
 */
string Connection::getReceiveFIFOName() {
    return receiveFIFOName;
}

/**
 * Getter for the SendFIFO filename.
 *
 * @return {@code std::string}
 */
string Connection::getSendFIFOName() {
    return sendFIFOName;
}

/**
 * Opens a Connection for reading a switch with id.
 *
 * @return {@code int} the file description of the ReceiveFIFO.
 */
int Connection::openReceiveFIFO() {
    printf("DEBUG: opening receiveFIFO: %s\n", getReceiveFIFOName().c_str());
    int i = open(getReceiveFIFOName().c_str(), O_RDONLY | O_NONBLOCK);
    if (errno || i == -1) {
        perror("ERROR: opening receiveFIFO");
        exit(errno);
    }
    return i;
}

/**
 * Opens a Connection for writing a switch with id.
 *
 * @return {@code int} the file description of the SendFIFO.
 */
int Connection::openSendFIFO() {
    printf("DEBUG: opening sendFIFO: %s\n", getSendFIFOName().c_str());
    int i = open(getSendFIFOName().c_str(), O_WRONLY | O_NONBLOCK);
    if (errno || i == -1) {
        perror("ERROR: opening sendFIFO");
        exit(errno);
    }
    return i;
}

/**
 * Generate a FIFO name based from the sender's id and receiver's id.
 *
 * @param senderId {@code uint}
 * @param receiverId {@code uint}
 * @return {code const std::string} name of the FIFO for the Connection.
 */
const string Connection::makeFIFOName(uint senderId, uint receiverId) {
    return "fifo-" + std::to_string(senderId) + "-" + std::to_string(receiverId);
}

/**
 * Make a FIFO for a {@code Connection}.
 *
 * If the FIFO already exists ignore the error {@code 17} as this does not affect
 * the limited scope of a2sdn.
 *
 * @param FIFOName {@code std::string} name to be given to the FIFO.
 */
void Connection::makeFIFO(string &FIFOName) {
    printf("DEBUG: making FIFO at: %s\n", FIFOName.c_str());
    int status = mkfifo(FIFOName.c_str(), S_IRUSR | S_IWUSR | S_IRGRP |
                                          S_IWGRP | S_IROTH | S_IWOTH);
    if (errno || status == -1) {
        if (errno == 17) { /* FIFO already exists - non-issue for a2sdn */
            perror("WARNING: error creating FIFO connection");
            errno = 0;
        } else { /* unexpected FIFO creation error */
            perror("ERROR: unexpected error creating FIFO connection");
            exit(errno);
        }
    }
}

