/**
 * switch.cpp
 *
 * @author Nathan Klapstein (nklapste)
 * @version 0.0.0
 */

#include <sys/types.h>
#include <cstdio>
#include <cstdlib>
#include "switch.h"

/*FIFO stuff*/
#include <sys/types.h>
#include <sys/stat.h>
#include <cerrno>
#include <string>
#include "flowTable.h"
using namespace std;



//init rule
/*
[srcIP lo= 0, srcIP hi= MAXIP, destIP lo= IPlow, destIP hi=
IPhigh, actionType= FORWARD, actionVal= 3, pri= MINPRI,
pktCount= 0]
*/

Switch::Switch(uint ipLow, uint ipHigh) {
    flowEntry init_rule = {
            .srcIP_lo = 0,
            .srcIP_hi = MAX_IP,
            .destIP_lo = ipLow,
            .destIP_hi = ipHigh,
            .actionType= 0,  // TODO: help ambiguous
            .actionVal=3,
            .pri=MIN_PRI,
            .pktCount=0
    };
    flowTable.push_back(init_rule);
    printf("I am switch: %i\n", S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
}


void Switch::makeFIFO(string &trafficFile) {
    int status = mkfifo(trafficFile.c_str(), S_IRUSR | S_IWUSR | S_IRGRP |
                                             S_IWGRP | S_IROTH | S_IWOTH);
    if (errno){
        // TODO: print error
    }
}