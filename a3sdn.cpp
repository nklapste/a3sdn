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
#include "address.h"
#include "iprange.h"

using namespace std;

/**
 * Main entry point for a2sdn.
 *
 * @param argc {@code int}
 * @param argv {@code char **}
 * @return {@code int}
 */
int main(int argc, char **argv) {

    // TODO: testing
    Address("127.0.0.1");
    Address("localhost");

    if (argc < 3 || argc > 6) {
        printf("ERROR: invalid argument format:\n"
               "\tPlease follow controller mode: 'a3sdn cont nSwitch portNumber'\n"
               "\tOr follow switch mode:         'a3sdn swi trafficFile [null|swj] [null|swk] IPLow-IPhigh portNumber'\n");
        exit(EINVAL);
    }

    string mode = argv[1];
    if (mode == CONTROLLER_MODE) {
        // parse controller mode arguments
        if (argc != 4) {
            printf("ERROR: invalid arguments for controller mode:\n"
                   "\tFor controller mode: 'a3sdn cont nSwitch portNumber'\n");
            exit(EINVAL);
        }
        // TODO: port
        Port port = Port(static_cast<uint>(stoi(argv[3])));
        uint nSwitch = static_cast<uint>(stoi(argv[2]));
        Controller controller = Controller(nSwitch, port);
        controller.start();
    } else {
        // parse switch mode arguments
        if (argc != 7) {
            printf("ERROR: invalid arguments for switch mode:\n"
                   "\tFor switch mode: 'a3sdn swi trafficFile [null|swj] [null|swk] IPLow-IPHigh portNumber'\n");
            exit(EINVAL);
        }
        SwitchID switchId = SwitchID(argv[1]);
        string trafficFile = argv[2];
        SwitchID leftSwitchId = SwitchID(argv[3]);
        SwitchID rightSwitchId = SwitchID(argv[4]);
        IPRange ipRange = parseIPRange(argv[5]);
        uint IPLow = get<0>(ipRange);
        uint IPHigh = get<1>(ipRange);

        Port port = Port(static_cast<uint>(stoi(argv[6])));
        Switch aSwitch = Switch(switchId, leftSwitchId, rightSwitchId, trafficFile, IPLow, IPHigh, port);
        aSwitch.start();
    }
    return 0;
}
