/**
 * a2sdn address.cpp
 *
 * @author Nathan Klapstein (nklapste)
 * @version 0.0.0
 */

#include <netdb.h>
#include <cstring>
#include <arpa/inet.h>
#include <tuple>

#include "address.h"

typedef tuple<string, string> HostInfo;

/**
 * Attempt to resolve a IP address, hostname, or Fully Qualified Domain Name (FQDN)
 * into a valid {@code struct addrinfo} using {@code getaddrinfo}.
 *
 * TODO: only give IPv4
 *
 * Adapted from:
 * https://gist.github.com/jirihnidek/bf7a2363e480491da72301b228b35d5d
 *
 * @param domain
 * @return {@code HostInfo} return the ip address and common-name/domain/ip of a the inputted domain.
 */
HostInfo lookupHost(const string &domain) {
    struct addrinfo hints{}, *res;
    int errcode;
    char addrstr[100];
    void *ptr = nullptr;
    string canonname = domain;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = PF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags |= AI_CANONNAME;

    errcode = getaddrinfo(domain.c_str(), nullptr, &hints, &res);
    if (errcode != 0) {
        perror("ERROR: resolving domain");
        exit(errcode);
    }

    printf("DEBUG: Resolving hostname: %s\n", domain.c_str());
    while (res) {
        inet_ntop(res->ai_family, res->ai_addr->sa_data, addrstr, 100);

        switch (res->ai_family) {
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
        inet_ntop(res->ai_family, ptr, addrstr, 100);
        // for some reason if they give us an ipv6 the next ipv4
        // will have no canonname even though it should be the same
        if (res->ai_canonname == nullptr) {
            // set resolved addrinfo ai_canonname to the last resolved cannonname
            // or worst case our given domain
            res->ai_canonname = const_cast<char *>(canonname.c_str());
        } else {
            // update cannon name with the cannon name found by getaddrinfo
            canonname = res->ai_canonname;
        }
        printf("INFO: Resolved domain: %s IPv%d address: %s canonname: (%s)\n",
               domain.c_str(), res->ai_family == PF_INET6 ? 6 : 4, addrstr, res->ai_canonname);
        if (res->ai_next == nullptr) {
            return make_tuple(addrstr, canonname);
        }
        res = res->ai_next;
    }
    return make_tuple(addrstr, canonname);
}

/**
 * Construct an {@code Address} from a string representation of a IP address, hostname,
 * or Fully Qualified Domain Name (FQDN).
 *
 * @param domain {@code std::string}
 */
Address::Address(string domain) {
    HostInfo hostInfo = lookupHost(domain);
    Address::ipAddr = get<0>(hostInfo);
    Address::symbolicName = get<1>(hostInfo);
}

/**
 * Getter for the symbolic (non-ip address) name of a given {@code Address}.
 *
 * @return {@code std::string}
 */
string Address::getSymbolicName() {
    return symbolicName;
}

/**
 * Getter for the {@code struct sockaddr} of a given {@code Address}.
 *
 * @return {@code std::string}
 */
string Address::getIPAddr() {
    return ipAddr;
}
