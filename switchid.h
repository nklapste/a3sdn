/**
 * switchid.h
 *
 * @author Nathan Klapstein (nklapste)
 * @version 0.0.0
 */

#ifndef A3SDN_SWITCHID_H
#define A3SDN_SWITCHID_H

#include <string>

#define NULL_SWITCH_ID_NUM 0
#define NULL_SWITCH_ID_FLAG "null"

using namespace std;

class SwitchID {
public:
    explicit SwitchID(string switchIDString);

    explicit SwitchID(uint switchIDNum);

    static uint parseSwitchID(const string &switchIDString);

    static uint validateSwitchIDNum(uint switchIDNum);

    bool isNullSwitchID();

    string getSwitchIDString();

    uint getSwitchIDNum();

private:
    string switchIDString;

    uint switchIDNum;
};

#endif //A3SDN_SWITCHID_H
