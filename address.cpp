/**
 * a2sdn address.cpp
 *
 * @author Nathan Klapstein (nklapste)
 * @version 0.0.0
 */

#include <netdb.h>
#include <cstring>
#include <arpa/inet.h>

#include "address.h"

/**
 * Attempt to resolve a IP address, hostname, or FQDN.
 *
 * TODO: only give IPv4
 *
 * Adapted from:
 * https://gist.github.com/jirihnidek/bf7a2363e480491da72301b228b35d5d
 *
 * @param domain
 * @return {@code int}
 */
addrinfo * lookupHost (const string &domain) {
    struct addrinfo hints{}, *res;
    int errcode;
    char addrstr[100];
    void *ptr = nullptr;

    memset (&hints, 0, sizeof (hints));
    hints.ai_family = PF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags |= AI_CANONNAME;

    errcode = getaddrinfo (domain.c_str(), nullptr, &hints, &res);
    if (errcode != 0)
    {
        perror("ERROR: resolving domain");
        exit(errcode);
    }

    printf ("DEBUG: Resolving hostname: %s\n", domain.c_str());
    while (res)
    {
        inet_ntop(res->ai_family, res->ai_addr->sa_data, addrstr, 100);

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
                exit(EINVAL);
        }
        inet_ntop (res->ai_family, ptr, addrstr, 100);
        printf ("INFO: Resolved domain: %s IPv%d address: %s canonname: (%s)\n",
                domain.c_str(), res->ai_family == PF_INET6 ? 6 : 4, addrstr, res->ai_canonname);
        if(res->ai_next == nullptr) {
            return res;
        }
        res = res->ai_next;
    }
    return res;
}

/**
 *
 * @param domain
 * @return {@code Address}
 */
Address::Address(string domain) {
    addrinfo * host = lookupHost(domain);
    Address::ipAddr = host->ai_addr;
    Address::symbolicName = host->ai_canonname;
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


