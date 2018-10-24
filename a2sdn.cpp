/**
 * a2sdn
 *
 * @author Nathan Klapstein (nklapste)
 * @version 0.0.0
 */

#include <string>
#include <tuple>
#include <iostream>
#include <fstream>

#define MAX_NSW 7
#define FORWARD 0
#define DROP    1
#define MAXIP   1000

#define SWITCH_MODE "swi"
#define CONTROLLER_MODE "cont"

#define SWJ_FLAG "swj"
#define SWK_FLAG "swk"
#define NULL_STR "null"

#define BOOL_STR(b) (b?"true":"false")

using namespace std;

typedef tuple<uint, uint> ipRange;


/**
 * Startup a2sdn in switch mode.
 *
 * @param nSwitch
 */
void switchMode(unsigned int nSwitch) {
    printf("Starting controller mode: nSwitch: %u", nSwitch);
    uint i = 1;
    for (;;) {
        if (i == 1) exit(0);
    }
}


/**
 * Startup a2sdn in controller mode.
 *
 * @param trafficFile
 * @param swjFlag
 * @param swkFlag
 * @param ipRange1
 */
void controllerMode(const string &trafficFile, bool swjFlag, bool swkFlag, ipRange ipRange1) {
    printf("Starting switch mode: trafficFile: %s swjFlag: %s swkFlag: %s ipLow: %u ipHigh: %u",
           trafficFile.c_str(), BOOL_STR(swjFlag), BOOL_STR(swkFlag), get<0>(ipRange1), get<1>(ipRange1));
    uint i = 1;
    for (;;) {
        /*1. Read and process a single line from the traffic line (if the EOF has not been reached yet). The
        switch ignores empty lines, comment lines, and lines specifying other handling switches. A
        packet header is considered admitted if the line specifies the current switch.
        */
        string line;
        ifstream trafficFileStream (trafficFile);
        for (int lineNo = 0; getline (trafficFileStream, line) && ! trafficFileStream.eof(); lineNo++){

        }

        /*
        2. Poll the keyboard for a user command. The user can issue one of the following commands.
        • list: The program writes all entries in the flow table, and for each transmitted or received
        packet type, the program writes an aggregate count of handled packets of this
        type.
        • exit: The program writes the above information and exits.
        */
        if (i == 1) exit(0);
    }
}


/**
 * Parse the ip range argument. Which follows the format IPLow-IPHigh.
 *
 * @param ipRangeString
 * @return
 */
ipRange parseIPRange(const string &ipRangeString) {
    string delimiter = "-";
    uint ipLow = stoul(ipRangeString.substr(0, ipRangeString.find(delimiter)));
    uint ipHigh = stoul(ipRangeString.substr(ipRangeString.find(delimiter) + 1, ipRangeString.size() - 1));
    if (ipLow > ipHigh) {
        printf("ERROR: invalid ip range:\n"
               "\tIPLow: %u greater than IPHigh: %u", ipLow, ipHigh);
        exit(1);
    }
    if (ipHigh - ipLow > MAXIP) {
        printf("ERROR: invalid ip range:\n"
               "\tip range is too large: %u", ipHigh - ipLow);
        exit(1);
    }
    return make_tuple(ipLow, ipHigh);
}


/**
 * Parse the swjFlag argument. Which follows the format [null|swj].
 *
 * @param swjFlag
 * @return {@code true} if "swj" is given, {@code false} if null is given, error and exit otherwise
 */
bool parseswjFlag(const string &swjFlag) {
    if (swjFlag == SWJ_FLAG) {
        return true;
    } else if (swjFlag == NULL_STR) {
        return false;
    } else {
        printf("ERROR: invalid swj Flag: %s\n"
               "\tFor swj flag: [null|swj]", swjFlag.c_str());
        exit(1);
    }
}


/**
 * Parse the swkFlag argument. Which follows the format [null|swk].
 *
 * @param swkFlag
 * @return {@code true} if "swk" is given, {@code false} if null is given, error and exit otherwise
 */
bool parseswkFlag(const string &swkFlag) {
    if (swkFlag == SWK_FLAG) {
        return true;
    } else if (swkFlag == NULL_STR) {
        return false;
    } else {
        printf("ERROR: invalid swk Flag: %s\n"
               "\tFor swk flag: [null|swk]", swkFlag.c_str());
        exit(1);
    }
}


/**
 * Parse the command line arguements from main().
 *
 * @param argc
 * @param argv
 * @return
 */
void parseArgs(int argc, char **argv){

}


/**
 * Main entry point for a2sdn.
 *
 * @param argc
 * @param argv
 * @return
 */
int main(int argc, char **argv) {
    if (argc < 3 || argc > 6) {
        printf("ERROR: invalid argument format:\n"
               "\tPlease follow controller mode: 'a2sdn cont nSwitch'\n"
               "\tOr follow switch mode:         'a2sdn swi trafficFile [null|swj] [null|swk] IPlow-IPhig'");
        exit(1);
    }
    string mode = argv[1];

    if (mode == CONTROLLER_MODE) {
        // parse controller mode arguments
        if (argc != 3) {
            printf("ERROR: invalid arguments for controller mode:\n"
                   "\tFor controller mode: 'a2sdn cont nSwitch'");
            exit(1);
        }
        uint nSwitch = stoul(argv[2]);
        switchMode(nSwitch);
    } else if (mode == SWITCH_MODE) {
        // parse switch mode arguments
        if (argc != 6) {
            printf("ERROR: invalid arguments for switch mode:\n"
                   "\tFor switch mode: 'a2sdn swi trafficFile [null|swj] [null|swk] IPlow-IPhig'");
            exit(1);
        }
        string trafficFile = argv[2];
        bool swjFlag = parseswjFlag(argv[3]);
        bool swkFlag = parseswkFlag(argv[4]);
        ipRange ipRange1 = parseIPRange(argv[5]);

        controllerMode(trafficFile, swjFlag, swkFlag, ipRange1);
    } else {
        printf("ERROR: invalid mode specified: %s", argv[2]);
        exit(1);
    }
    return 0;
}
