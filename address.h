/**
 * a2sdn address.h
 *
 * @author Nathan Klapstein (nklapste)
 * @version 0.0.0
 */

#ifndef A3SDN_ADDRESS_H
#define A3SDN_ADDRESS_H

#include <string>

using namespace std;

class Address {
public:
    explicit Address(string domain);

    string getSymbolicName();

    string getIPAddr();

private:
    string ipAddr;

    string symbolicName;
};

#endif //A3SDN_ADDRESS_H
