/**
 * a2sdn
 *
 * @author Nathan Klapstein (nklapste)
 * @version 0.0.0
 */

#include <string>

using namespace std;
const static string SWITCH_MODE = "swi";
const static string CONTROLLER_MODE = "cont";


/**
 * Startup a2sdn in switch mode.
 */
void switchMode(){

}

/**
 * Startup a2sdn in controller mode.
 */
void controllerMode(){

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
    // TODO: cleanup argparsing code
    // TODO: ensure input met
    if (argc<=2){
        printf("ERROR: invalid argument format\n"
               "Please follow controller mode: 'a2sdn cont nSwitch'\n"
               "Or follow switch mode:         'a2sdn swi trafficFile [null|swj] [null|swk] IPlow-IPhig'");
        return 1;
    }
    string mode = argv[1];

    // TODO: maybe use switch
    if (mode==CONTROLLER_MODE){
        if (argc!=3){
            printf("ERROR: invalid arguments for controller mode\n"
                   "For controller mode: 'a2sdn cont nSwitch'");
            return 1;
        }
        int nSwitch = stoi(argv[2]);
        printf("Starting controller mode: nSwitch: %u", nSwitch);
        switchMode();
    } else if (mode==SWITCH_MODE){
        if (argc!=6){
            printf("ERROR: invalid arguments for switch mode\n"
                   "For switch mode: 'a2sdn swi trafficFile [null|swj] [null|swk] IPlow-IPhig'");
            return 1;
        }
        string trafficFile = argv[2];
        string swjFlag = argv[3];
        string swkFlag = argv[4];
        // parse ip range string IPlow-IPhigh
        string ipRangeString = argv[5];
        string delimiter = "-";
        unsigned int ipLow = stoul(ipRangeString.substr(0, ipRangeString.find(delimiter)));
        unsigned int ipHigh = stoul(ipRangeString.substr(ipRangeString.find(delimiter), ipRangeString.size()));
        printf("Starting switch mode: trafficFile: %s swjFlag: %s swkFlag: %s ipLow: %u ipHigh: %u", trafficFile.c_str(), swjFlag.c_str(), swkFlag.c_str(), ipLow, ipHigh);
        controllerMode();
    } else {
        printf("ERROR: invalid mode specified %s", argv[2]);
        return 1;
    }
    return 0;
}
