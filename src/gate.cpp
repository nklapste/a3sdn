/**
 * gate.cpp
 *
 * @author Nathan Klapstein (nklapste)
 * @version 0.0.0
 */

#include <sys/signalfd.h>
#include <unistd.h>
#include <sys/socket.h>
#include <cstring>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "gate.h"

#define LIST_CMD "list"
#define EXIT_CMD "exit"

#define BUFFER_SIZE 1024

using namespace std;

Gate::Gate(Port port) : port(port) {
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
 * 1. Poll the keyboard for a user command. The user can issue one of the following commands.
 *       list: The program writes all entries in the flow table, and for each transmitted or received
 *             packet type, the program writes an aggregate count of handled packets of this
 *             type.
 *       exit: The program writes the above information and exits.
 */
void Gate::checkStdin(int stdinFD) {
    char buf[BUFFER_SIZE] = "\0";
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
        printf("ERROR: invalid Gate (Controller/Switch) command: %s\n"
               "\tPlease use either 'list' or 'exit'\n", cmd.c_str());
    }
}

/**
 * In addition, upon receiving signal USER1, the Gate (Controller/Switch) displays the information
 * specified by the list command.
 */
void Gate::checkSignal(int signalFD) {
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

/**
 * Init a signal file descriptor looking for the SIGUSR1 signal.
 *
 * @return {@code int}
 */
int Gate::getSignalFD() {
    // init signal file descriptor
    sigset_t sigset;
    /* Create a sigset of all the signals that we're interested in */
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGUSR1);
    /* We must block the signals in order for signalfd to receive them */
    sigprocmask(SIG_BLOCK, &sigset, nullptr);
    if (errno) {
        perror("ERROR: setting signal mask");
        exit(errno);
    }
    return signalfd(-1, &sigset, 0);
}

/**
 * Get a null terminated {@code '\0'} message from a socket.
 *
 * TODO: simplify
 *
 * @param socketFD {@code int} a file descriptor to a socket to read from.
 * @param tmpbuf {@code char *} temporary buffer to read from
 * @return {@code std::string} the read message
 */
string Gate::getMessage(int socketFD, char *tmpbuf) {
    ssize_t num = recv(socketFD, tmpbuf, BUFFER_SIZE, 0);
    if (errno != 0 && errno != EAGAIN) {
        perror("ERROR: mes");  // TODO: remove
    } else {
        errno = 0;
    }
    tmpbuf[num] = '\0';
    if (tmpbuf[num - 1] == '\n')
        tmpbuf[num - 1] = '\0';
    string msg = string(tmpbuf);
    strcpy(tmpbuf, "");
    return msg;
}

/**
 * Getter for a {@code Gate}'s port number.
 *
 * @return {@code Port} the port of the Gate.
 */
Port Gate::getPort() const {
    return port;
}

/**
 * Getter for a {@code Gate}'s ID.
 *
 * @return {@code unit} the ID number of the Gate.
 */
uint Gate::getGateID() const {
    return gateID;
}
