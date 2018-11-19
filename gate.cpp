/**
 * gate.cpp
 *
 * @author Nathan Klapstein (nklapste)
 * @version 0.0.0
 */

#include <sys/signalfd.h>
#include <unistd.h>
#include "gate.h"


#define LIST_CMD "list"
#define EXIT_CMD "exit"

#define BUFFER_SIZE 1024

/**
 * Getter for a {@code Gate}'s ID.
 *
 * @return {@code unit} the ID number of the Gate.
 */
uint Gate::getGateID() const {
    return gateID;
}


/**
 * Print statistics on the packets sent and received by the {@code Gate}.
 */
void Gate::listPacketStats() {
    printf("Packet Stats:\n");
    printf("\tReceived:    OPEN:%u, ACK:%u, QUERY:%u, ADDRULE:%u, RELAYIN: %u, ADMIT:%u\n",
           rOpenCount, rAckCount, rQueryCount, rAddCount, rRelayCount, admitCount);
    printf("\tTransmitted: OPEN:%u, ACK:%u, QUERY:%u, ADDRULE:%u, RELAYOUT:%u\n",
           tOpenCount, tAckCount, tQueryCount, tAddCount, tRelayCount);
}

/**
 * Getter for a {@code Gate}'s port number.
 *
 * @return {@code Port} the port of the Gate.
 */
Port Gate::getPort() const {
    return port;
}

Gate::Gate(Port port) : port(port) {
}


/**
 * 1. Poll the keyboard for a user command. The user can issue one of the following commands.
 *       list: The program writes all entries in the flow table, and for each transmitted or received
 *             packet type, the program writes an aggregate count of handled packets of this
 *             type.
 *       exit: The program writes the above information and exits.
 */
void Gate::check_stdin(int stdinFD) {
    char buf[BUFFER_SIZE];
    ssize_t r = read(stdinFD, buf, BUFFER_SIZE);
    if (!r) {
        printf("WARNING: stdin closed\n");
    }
    string cmd = string(buf);
    // trim off all whitespace
    while (!cmd.empty() && !isalpha(cmd.back())) cmd.pop_back();

    if (cmd == LIST_CMD) {
        list();
    } else if (cmd == EXIT_CMD) {
        list();
        printf("INFO: exit command received: terminating\n");
        exit(0);
    } else {
        printf("ERROR: invalid Controller command: %s\n"
               "\tPlease use either 'list' or 'exit'\n", cmd.c_str());
    }
}

/**
 * In addition, upon receiving signal USER1, the switch displays the information specified by the list command.
 */
void Gate::check_signal(int signalFD) {
    struct signalfd_siginfo info{};
    /* We have a valid signal, read the info from the fd */
    ssize_t r = read(signalFD, &info, sizeof(info));
    if (!r) {
        printf("WARNING: signal reading error\n");
    }
    unsigned sig = info.ssi_signo;
    if (sig == SIGUSR1) {
        printf("DEBUG: received SIGUSR1 signal\n");
        list();
    }
}
