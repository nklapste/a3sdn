/**
 * a2sdn address.cpp
 *
 * @author Nathan Klapstein (nklapste)
 * @version 0.0.0
 */

#include <netdb.h>
#include <cstring>
#include "address.h"

#include <arpa/inet.h>

/**
 * Resolve a host name.
 *
 * Adapted from:
 * https://gist.github.com/jirihnidek/bf7a2363e480491da72301b228b35d5d
 *
 * @param host
 * @return {@code int}
 */
addrinfo * lookupHost (const string &host) {
    struct addrinfo hints, *res;
    int errcode;
    char addrstr[100];
    void *ptr;

    memset (&hints, 0, sizeof (hints));
    hints.ai_family = PF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags |= AI_CANONNAME;

    errcode = getaddrinfo (host.c_str(), NULL, &hints, &res);
    if (errcode != 0)
    {
        // TODO: better errno
        errno = 1;
        perror("getaddrinfo");
        return res;
    }

    printf ("DEBUG: Resolving hostname: %s\n", host.c_str());
    while (res)
    {
        inet_ntop (res->ai_family, res->ai_addr->sa_data, addrstr, 100);

        switch (res->ai_family)
        {
            case AF_INET:
                ptr = &((struct sockaddr_in *) res->ai_addr)->sin_addr;
                break;
            case AF_INET6:
                ptr = &((struct sockaddr_in6 *) res->ai_addr)->sin6_addr;
                break;
            default:
                printf("ERROR: unknown ai_family: %d\n", res->ai_family);
                break;
        }
        inet_ntop (res->ai_family, ptr, addrstr, 100);
        printf ("INFO: IPv%d address: %s (%s)\n", res->ai_family == PF_INET6 ? 6 : 4,
                addrstr, res->ai_canonname);
        if(res->ai_next == nullptr){
            return res;
        }
        res = res->ai_next;
    }
    return res;
}

/**
 *
 * @param ipAddr
 * @return {@code Address}
 */
Address Address::createAddressFromIPAddr(string ipAddr) {
    Address address = Address();
    addrinfo * host = lookupHost(ipAddr);
    if (errno) {
        perror("ERROR: resolving IP address");
        exit(errno);
    }
    address.ipAddr = host->ai_addr;
    address.symbolicName = host->ai_canonname;
    return address;
}

/**
 *
 * @param symbolicName
 * @return {@code Address}
 */
Address Address::createAddressFromSybolicName(string symbolicName) {
    Address address = Address();
    addrinfo * host = lookupHost(symbolicName);
    if (errno) {
        perror("ERROR: resolving domain name");
        exit(errno);
    }
    // TODO: not getting here
    address.ipAddr = host->ai_addr;
    address.symbolicName = host->ai_canonname;
    return address;
}

/**
 *
 * @return {@code std::string}
 */
string Address::getSymbolicName() {
    return symbolicName;
}

/**
 *
 * @return
 */
struct sockaddr *Address::getIPAddr() {
    return ipAddr;
}


