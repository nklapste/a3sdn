//
// Created by ubuntu-dev on 24/10/18.
//
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "connection.h"

using namespace std;

void Connection::makeFIFO(string &FIFOName) {
    int status = mkfifo(FIFOName.c_str(), S_IRUSR | S_IWUSR | S_IRGRP |
                                          S_IWGRP | S_IROTH | S_IWOTH | O_NONBLOCK);
    if (errno || status==-1){
        perror("ERROR: error creating FIFO connection");
        errno=0;
    }
}

/**
 * Opens a Connection for reading a switch with id.
 * @return
 */
int Connection::openReceiveFIFO() {
    printf("opening receiveFIFO: %s", receiveFIFOName.c_str());
    int i = open(receiveFIFOName.c_str(), O_RDONLY | O_NONBLOCK);
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
    printf("opening sendFIFO: %s", receiveFIFOName.c_str());
    int i = open(sendFIFOName.c_str(), O_WRONLY | O_NONBLOCK);
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
    return receiveFIFOName;
}

/**
 * Getter for the Sending FIFO filename/pipename.
 * @return {@code std::string}
 */
string Connection::getSendFIFOName() {
    return sendFIFOName;
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
   sendFIFOName = makeFIFOName(srcID, dstID);
   receiveFIFOName = makeFIFOName(dstID, srcID);
   printf("Making Connection:\n"
          "\tsrc: %u dst: %u sendFIFO: %s receiveFIFO: %s\n", srcID, dstID, sendFIFOName.c_str(), receiveFIFOName.c_str());
   makeFIFO(sendFIFOName);
   makeFIFO(receiveFIFOName);
}
