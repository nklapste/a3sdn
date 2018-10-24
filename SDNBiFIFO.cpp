//
// Created by ubuntu-dev on 24/10/18.
//

#include <sys/types.h>
#include <bits/fcntl-linux.h>
#include "SDNBiFIFO.h"
#include <string>
#include <fcntl.h>
#include <sys/stat.h>

using namespace std;

void SDNBiFIFO::makeFIFO(string &FIFOName) {
    int status = mkfifo(FIFOName.c_str(), S_IRUSR | S_IWUSR | S_IRGRP |
                                             S_IWGRP | S_IROTH | S_IWOTH);
    if (errno){
        // TODO: print error
        perror("ERROR: error creating SDNBiFIFO");
    }
}

int SDNBiFIFO::openReceiveFIFO() {
    /* Opens a SDNBiFIFO for reading a switch with id. */
    return open(SDNBiFIFO::receiveFIFOName.c_str(), O_RDONLY);
}

int SDNBiFIFO::openSendFIFO() {
    /* Opens a SDNBiFIFO for writing a switch with id. */
    return open(SDNBiFIFO::sendFIFOName.c_str(), O_WRONLY);
}

const string SDNBiFIFO::getFIFOName(uint senderId, uint receiverId) {
    return "fifo-" + std::to_string(senderId) + "-" + std::to_string(recieverId);
}

SDNBiFIFO * SDNBiFIFO::SDNBiFIFO(uint srcId, uint dstId):
    senderId(srcId),
    receiverId(dstId),
    sendFIFOName(SDNBiFIFO::getFIFOName(srcId, dstId)),
    receiveFIFOName(SDNBiFIFO::getFIFOName(dstId, srcId)) {
}
