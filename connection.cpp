//
// Created by ubuntu-dev on 24/10/18.
//

#include <sys/types.h>
#include <fcntl.h>
#include "connection.h"
#include <string>
#include <fcntl.h>
#include <sys/stat.h>
#include <regex>

using namespace std;

void Connection::makeFIFO(string &FIFOName) {
    int status = mkfifo(FIFOName.c_str(), S_IRUSR | S_IWUSR | S_IRGRP |
                                             S_IWGRP | S_IROTH | S_IWOTH);
    if (errno || status==-1){
        perror("ERROR: error creating FIFO connection");
    }
}

/**
 * Opens a Connection for reading a switch with id.
 * @return
 */
int Connection::openReceiveFIFO() {
    int i = open(Connection::receiveFIFOName.c_str(), O_RDONLY);
    if (!i){
        perror("ERROR: opening receive FOFO");
    }
    return i;
}

/**
 * Opens a Connection for writing a switch with id.
 * @return
 */
int Connection::openSendFIFO() {
    int i = open(Connection::sendFIFOName.c_str(), O_WRONLY);
    if (!i){
        perror("ERROR: opening send FOFO");
    }
    return i;
}


/**
 * Getter for the Receiving FIFO filename/pipename.
 * @return {@code std::string}
 */
string Connection::getReceiveFIFOName() {
    return Connection::receiveFIFOName;
}

/**
 * Getter for the Sending FIFO filename/pipename.
 * @return {@code std::string}
 */
string Connection::getSendFIFOName() {
    return Connection::sendFIFOName;
}

/**
 *
 * @param senderId
 * @param receiverId
 * @return
 */
const string Connection::makeFIFOName(uint senderId, uint receiverId) {
    return "fifo-" + std::to_string(senderId) + "-" + std::to_string(receiverId);
}


/**
 *
 * @param srcID
 * @param dstID
 */
Connection::Connection(uint srcID, uint dstID) {
   Connection::srcID = srcID;
   Connection::dstID = dstID;
   sendFIFOName = makeFIFOName(srcID, dstID);
   receiveFIFOName = makeFIFOName(dstID, srcID);
   printf("Making Connection:\n"
          "\tsrc: %u dst: %u sendFIFO: %s receiveFIFO: %s\n", srcID, dstID, sendFIFOName.c_str(), receiveFIFOName.c_str());
}

/**
 *
 * @return
 */
int Connection::getReceiveFIFO() {
    return Connection::openReceiveFIFO();
}

/**
 *
 * @return
 */
int Connection::getSendFIFO() {
    return Connection::openSendFIFO();
}
