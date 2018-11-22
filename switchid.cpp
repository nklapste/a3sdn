/**
 * switchid.cpp
 *
 * @author Nathan Klapstein (nklapste)
 * @version 0.0.0
 */

#include <regex>
#include "switchid.h"
#include "controller.h"

/**
 * Parse the switch id. Should match the format {@code 'swi'} where {@code 'i'} is a numeric character.
 *
 * @param switchID {@code std::string}
 * @return {@code uint}
 */
uint SwitchID::parseSwitchID(const string &switchIDString) {
    regex rgx("(sw)([1-9]+[0-9]*)");
    match_results<string::const_iterator> matches;

    std::regex_match(switchIDString, matches, rgx);
    for (std::size_t index = 1; index < matches.size(); ++index) {
    }

    if (std::regex_search(switchIDString, matches, rgx)) {
        uint switchIDNum = static_cast<uint>(std::stoi(matches[2], nullptr, 10));
        return validateSwitchIDNum(switchIDNum);
    } else {
        printf("ERROR: invalid switchID argument: %s\n", switchIDString.c_str());
        exit(EINVAL);
    }
}

/**
 * Validate that a given SwitchID number is valid.
 *
 * It must be between {@code MIN_SWITCHES} and {@code MAX_SWITCHES}.
 *
 * @param switchIDNum
 * @return {@code uint}
 */
uint SwitchID::validateSwitchIDNum(uint switchIDNum) {
    if (switchIDNum < MIN_SWITCHES) {
        printf("ERROR: switchID is to low: %u\n"
               "\tMIN_SWITCHES=%u\n", switchIDNum, MIN_SWITCHES);
        exit(EINVAL);
    } else if (switchIDNum > MAX_SWITCHES) {
        printf("ERROR: switchID is to high: %u\n"
               "\tMAX_SWITCHES=%u\n", switchIDNum, MAX_SWITCHES);
        exit(EINVAL);
    } else {
        return switchIDNum;
    }
}

/**
 * Constructor for a {@code SwitchID} from a {@code std::string}.
 *
 * @param switchIDString {@code std::string}
 */
SwitchID::SwitchID(string switchIDString) {
    if (switchIDString == NULL_SWITCH_ID_FLAG) {
        SwitchID::switchIDNum = NULL_SWITCH_ID_NUM;
        SwitchID::switchIDString = NULL_SWITCH_ID_FLAG;
    } else {
        SwitchID::switchIDNum = parseSwitchID(switchIDString);
        SwitchID::switchIDString = "sw" + to_string(SwitchID::switchIDNum);
    }
}

/**
 * Getter for a {@code SwitchID}'s full string representation.
 *
 * i.e. "sw<switchIDNum>"
 * e.g. "sw1"
 *
 * @return {@code std::string}
 */
string SwitchID::getSwitchIDString() {
    return switchIDString;
}

/**
 * Getter for a {@code SwitchID}'s numerical ID representation.
 *
 * @return {@code uint}
 */
uint SwitchID::getSwitchIDNum() const {
    return switchIDNum;
}

/**
 * Check if a {@code SwitchID} is noting a null switch.
 *
 * @return {@code bool}
 */
bool SwitchID::isNullSwitchID() {
    return switchIDNum == NULL_SWITCH_ID_NUM;
}
