/**
 * a2sdn packet.cpp
 *
 * @author Nathan Klapstein (nklapste)
 * @version 0.0.0
 */

#include <utility>
#include <iterator>
#include <sstream>
#include <string>
#include <tuple>

#include "packet.h"

/**
 * Getter for a packets type.
 *
 * @return {@code std::string}
 */
string Packet::getType() {
    return type;
}

/**
 * Constructor for a packet. Using a raw packet string representation.
 *
 * Useful for reconstructing a packets from a received packet string.
 *
 * @param packetRaw {@code std::string}
 */
Packet::Packet(string &packetRaw) {
    string delimiter = ":";
    string typeRaw = packetRaw.substr(0, packetRaw.find(delimiter));
    type = parseType(typeRaw);

    // we have no message
    if (packetRaw.length() == packetRaw.find(delimiter) + 1) {
        message = Message();
    } else {
        string messageRaw = packetRaw.substr(packetRaw.find(delimiter) + 2, packetRaw.length());
        message = parseMessage(messageRaw);
    }
}

/**
 * Parse a string to check if it is a valid packet type.
 *
 * Valid strings include:
 *  - "OPEN"
 *  - "ACK"
 *  - "QUERY"
 *  - "ADD"
 *  - "RELAY"
 *
 * @param type {@code std::string}
 * @return {@code std::string}
 */
string Packet::parseType(string &type) {
    if (type == OPEN || type == ACK || type == QUERY || type == ADD || type == RELAY) {
        // valid packet type
    } else {
        errno = EINVAL;
        perror("Invalid type for packet");
    }
    return type;
}

/**
 * Parse a string into a valid {@code Message}.
 *
 * @param messageRaw {@code std::string}
 * @return {@code Message}
 */
Message Packet::parseMessage(string &messageRaw) {
    Message message;
    istringstream iss(messageRaw);
    vector<string> messageArgsRaw((istream_iterator<string>(iss)),
                                  istream_iterator<string>());

    for (auto const &messageArgRaw: messageArgsRaw) {
        // TODO: ensure that only one colon
        std::string delimiter = ":";
        std::string key = messageArgRaw.substr(0, messageArgRaw.find(delimiter));
        std::string val = messageArgRaw.substr(messageArgRaw.find(delimiter) + 1, messageArgRaw.length());
        MessageArg messageArg = make_tuple(key, val);
        message.emplace_back(messageArg);
    }
    return message;
}

/**
 * Getter method for a {@code Packet}'s message.
 *
 * @return {@code Message}
 */
Message Packet::getMessage() {
    return message;
}

/**
 * Constructor for a packet using a packet type and a {@code Message}.
 *
 * Useful for constructing a packet from raw data.
 *
 * @param type {@code std::string}
 * @param message {@code Message}
 */
Packet::Packet(string type, Message message) {
    Packet::type = parseType(type);
    Packet::message = std::move(message);
}

/**
 * Get a serial string version of the packet ready to be written out.
 *
 * @return {@code std::string} the serial string version of a {@code Packet}.
 */
string Packet::toString() {
    string packetStr = type + ":";
    for (auto const &MessageArg: message) {
        packetStr.append(" " + get<0>(MessageArg) + ":" + get<1>(MessageArg));
    }
    return packetStr;
}

