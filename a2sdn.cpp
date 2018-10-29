/**
 * a2sdn
 *
 * Mainly contains argparsing functionality and the initialization of
 * either the {@code Controller} or {@code Switch}.
 *
 * @author Nathan Klapstein (nklapste)
 * @version 0.0.0
 */

#include <string>

#include "controller.h"
#include "switch.h"

using namespace std;

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
 * @param argc {@code int}
 * @param argv {@code char **}
 * @return {@code int}
 */
int main(int argc, char **argv) {
    if (argc < 3 || argc > 6) {
        printf("ERROR: invalid argument format:\n"
               "\tPlease follow controller mode: 'a2sdn cont nSwitch'\n"
               "\tOr follow switch mode:         'a2sdn swi trafficFile [null|swj] [null|swk] IPLow-IPhigh'\n");
        exit(EINVAL);
    }
    string mode = argv[1];

    if (mode == CONTROLLER_MODE) {
        // parse controller mode arguments
        if (argc != 3) {
            printf("ERROR: invalid arguments for controller mode:\n"
                   "\tFor controller mode: 'a2sdn cont nSwitch'\n");
            exit(EINVAL);
        }
        Controller controller = Controller((uint) stoi(argv[2]));
        controller.start();
    } else {
        // parse switch mode arguments
        if (argc != 6) {
            printf("ERROR: invalid arguments for switch mode:\n"
                   "\tFor switch mode: 'a2sdn swi trafficFile [null|swj] [null|swk] IPLow-IPHigh'\n");
            exit(EINVAL);
        }
        string switchId = argv[1];
        string trafficFile = argv[2];
        string leftSwitchId = argv[3];
        string rightSwitchId = argv[4];
        string IPRangeStr = argv[5];
        Switch aSwitch = Switch(switchId, leftSwitchId, rightSwitchId, trafficFile, IPRangeStr);
        aSwitch.start();
    }
    return 0;
}
