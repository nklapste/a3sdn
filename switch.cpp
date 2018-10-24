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

/*SDNBiFIFO stuff*/
#include <sys/types.h>
#include <sys/stat.h>
#include <cerrno>
#include <string>
#include <bits/fcntl-linux.h>
#include <fcntl.h>
#include "flowTable.h"
using namespace std;


void switchLoop(Switch s){


    for (;;) {
        /*
        1.  Read and process a single line from the traffic line (if the EOF has not been reached yet). The
        switch ignores empty lines, comment lines, and lines specifying other handling switches. A
        packet header is considered
        admitted
        if the line specifies the current switch

         Note:
After reading all lines in the traffic file, the program continues to monitor and process
keyboard commands, and the incoming packets from neighbouring devices.
        */

        /*
         2.  Poll the keyboard for a user command. The user can issue one of the following commands.

        list:  The program writes all entries in the flow table, and for each transmitted or re-
        ceived packet type, the program writes an aggregate count of handled packets of this
        type.

exit: The program writes the above information and exits.
         */

        /*
            3.  Poll the incoming FIFOs from the controller and the attached switches. The switch handles
            each incoming packet, as described in the Packet Types

             In addition,  upon receiving signal USER1, the switch displays the information specified by the list command
         */
        if(s.swj!=-1){
            // TODO: open FIFOs for swj
        }
        if(s.swk!=-1){
            // TODO: open FIFOs for swj
        }
    }
}

//init rule
/*
[srcIP lo= 0, srcIP hi= MAXIP, destIP lo= IPlow, destIP hi=
IPhigh, actionType= FORWARD, actionVal= 3, pri= MINPRI,
pktCount= 0]
*/

Switch::Switch(int swi, int swj, int swk, string &trafficFile, uint ipLow, uint ipHigh): swi(swi), swj(swj), swk(swk), trafficFile(trafficFile) {
    flowEntry init_rule = {
            .srcIP_lo = 0,
            .srcIP_hi = MAX_IP,
            .destIP_lo = ipLow,
            .destIP_hi = ipHigh,
            .actionType= FORWARD,  // TODO: help ambiguous
            .actionVal=3,
            .pri=MIN_PRI,
            .pktCount=0
    };
    flowTable.push_back(init_rule);
    if(swj!=-1){
        *swjBiFIFO = &SDNBiFIFO(swi, swj);
    }
    if(swk!=-1){
        *swkBiFIFO = &SDNBiFIFO(swi, swk);
    } else {
        swkBiFIFO;
    }
    printf("I am switch: %i\n", S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
}
