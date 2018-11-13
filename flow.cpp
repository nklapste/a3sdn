/**
 * flow.h
 *
 * @author Nathan Klapstein (nklapste)
 * @version 0.0.0
 */

#include "flow.h"

/**
* Convert a {@code actionType} number into its human readable string name.
*
* @param actionType {@code uint}
* @return {@code std::string} the human readable string name of the actionType. Or the actionType number cast to a
*         {@code std::string} via the {@code std::to_string} method.
*/
string toActionName(uint actionType) {
    if (actionType == DELIVER) {
        return "DELIVER";
    } else if (actionType == FORWARD) {
        return "FORWARD";
    } else if (actionType == DROP) {
        return "DROP";
    } else {
        printf("WARNING: unknown actionType: %u unable to match to a human readable string name\n", actionType);
        return to_string(actionType);
    }
}