//
// Created by nklap on 2018-11-09.
//

#ifndef A3SDN_IPRANGE_H
#define A3SDN_IPRANGE_H

#include <tuple>

using namespace std;

typedef tuple<uint, uint> IPRange;

IPRange parseIPRange(const string &IPRange);

#endif //A3SDN_IPRANGE_H
