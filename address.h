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
    static Address createAddressFromIPAddr(string ipAddr);

    static Address createAddressFromSybolicName(string symbolicName);

    string getIPAddr();
    string getSymbolicName();



private:
    string ipAddr;
    string symbolicName;
};


#endif //A3SDN_ADDRESS_H
