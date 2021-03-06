/**
 * trafficfile.h
 *
 * @author Nathan Klapstein (nklapste)
 * @version 0.0.0
 */

#ifndef A3SDN_TRAFFICFILE_H
#define A3SDN_TRAFFICFILE_H

#define DELAY_FLAG "delay"

#define INVALID_LINE -1
#define DELAY_LINE 1
#define ROUTE_LINE 2

#include <tuple>
#include <chrono>

#include "switchid.h"

using namespace std;
using namespace chrono;

/**
 * Traffic file line type formatted as "swi delay interval"
 * where interval is an integer in milliseconds.
 */
typedef tuple<SwitchID, string, milliseconds> trafficFileDelayItem;

/**
 * Traffic file line type formatted as "swi srcIP dstIP"
 */
typedef tuple<SwitchID, uint, uint> trafficFileRouteItem;

int getTrafficFileLineType(string &line);

trafficFileRouteItem parseTrafficRouteItem(string &line);

trafficFileDelayItem parseTrafficDelayItem(string &line);

#endif //A3SDN_TRAFFICFILE_H

