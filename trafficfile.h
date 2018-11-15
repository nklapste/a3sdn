//
// Created by nklap on 2018-11-07.
//

#ifndef A3SDN_TRAFFICFILE_H
#define A3SDN_TRAFFICFILE_H

#define DELAY_FLAG "delay"

#define INVALID_LINE -1
#define DELAY_LINE 1
#define ROUTE_LINE 2

#include <tuple>
#include "switchid.h"

using namespace std;


/**
 * Traffic file line type formatted as "swi delay interval"
 * where interval is an integer in milliseconds.
 */
typedef tuple<SwitchID, string, clock_t> trafficFileDelayItem;

/**
 * Traffic file line type formatted as "swi srcIP dstIP"
 */
typedef tuple<SwitchID, uint, uint> trafficFileRouteItem;

int getTrafficFileLineType(string &line);

trafficFileRouteItem parseTrafficRouteItem(string &line);

trafficFileDelayItem parseTrafficDelayItem(string &line);

#endif //A3SDN_TRAFFICFILE_H

