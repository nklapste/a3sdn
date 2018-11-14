//
// Created by nklap on 2018-11-07.
//

#include <iterator>
#include <vector>
#include <sstream>
#include "trafficfile.h"
#include "switch.h"

/**
 * Attempt to get the line type for a traffic file.
 *
 * @param line {@code std::string}
 * @return {@code int} integer representing the type of the traffic file line.
 */
int getTrafficFileLineType(string &line) {
    if (line.length() < 1) {
        printf("WARNING: ignoring invalid line: %s\n", line.c_str());
        return INVALID_LINE;
    }
    if (line.substr(0, 1) == "#") {
        printf("DEBUG: ignoring comment line\n");
        return INVALID_LINE;
    }

    istringstream iss(line);
    vector<string> trafficFileItems((istream_iterator<string>(iss)), istream_iterator<string>());
    if (trafficFileItems.size() != 3) {
        printf("WARNING: ignoring invalid line: %s\n", line.c_str());
        return INVALID_LINE;
    }
    if (trafficFileItems.at(1) == DELAY_FLAG) { // check if delay line
        printf("DEBUG: detected DELAY line: %s\n", line.c_str());
        return DELAY_LINE;
    } else {
        printf("DEBUG: detected ROUTE line: %s\n", line.c_str());
        return ROUTE_LINE;
    }
}

/**
 * Parse a line within the TrafficFile representing a Route item.
 *
 * @param line {@code std::string}
 * @return {@code trafficFileRouteItem}
 */
trafficFileRouteItem parseTrafficRouteItem(string &line) {
    istringstream iss(line);
    vector<string> trafficFileItems((istream_iterator<string>(iss)), istream_iterator<string>());
    uint switchID = SwitchID::parseSwitchID(trafficFileItems.at(0));
    uint srcIP = static_cast<uint>(stoi(trafficFileItems.at(1)));
    uint dstIP = static_cast<uint>(stoi(trafficFileItems.at(2)));
    printf("DEBUG: parsed trafficFileRouteItem: switchID: %u srcIP: %u dstIP: %u\n",
           switchID, srcIP, dstIP);
    return make_tuple(switchID, srcIP, dstIP);

}

/**
 * Parse a line within the TrafficFile representing a Delay item.
 *
 * @param line {@code std::string}
 * @return {@code trafficFileDelayItem}
 */
trafficFileDelayItem parseTrafficDelayItem(string &line) {
    istringstream iss(line);
    vector<string> trafficFileItems((istream_iterator<string>(iss)), istream_iterator<string>());
    uint switchID = SwitchID::parseSwitchID(trafficFileItems.at(0));
    string flag = trafficFileItems.at(1);
    clock_t interval = static_cast<clock_t>(stoi(trafficFileItems.at(2)));
    printf("DEBUG: parsed trafficFileDelayItem: switchID: %u %s interval: %lims\n",
           switchID, flag.c_str(), interval);
    return make_tuple(switchID, flag, interval);
}