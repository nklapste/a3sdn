//
// Created by nklap on 2018-10-26.
//

#include <iterator>
#include <sstream>
#include <string>
#include <tuple>
#include "packet.h"

string Packet::getType() {
    return type;
}

Packet::Packet(string packetRaw) {
    string delimiter = ": ";
    string typeRaw = packetRaw.substr(0, packetRaw.find(delimiter));
    type = parseType(typeRaw);
    string messageRaw = packetRaw.substr(packetRaw.find(delimiter)+1, packetRaw.length());
    message = parseMessage(messageRaw);
}

string Packet::parseType(string type) {
    if (type==OPEN){

    }
    else if (type==ACK){

    } else if (type==QUERY){

    } else if (type==ADD){

    } else if (type==RELAY){

    } else {
        errno=EINVAL;
        // TODO: maybe use printf
        perror("Invalid type for packet");
    }
    return type;
}


/**
 *
 * @param messageRaw
 * @return
 */
Message Packet::parseMessage(string messageRaw) {
    Message message;
    istringstream iss(messageRaw);
    vector<string> messageArgsRaw((istream_iterator<string>(iss)),
                                     istream_iterator<string>());

    for(auto const& messageArgRaw: messageArgsRaw) {
        // TODO: ensure that only one colon
        std::string delimiter = ":";
        std::string key = messageArgRaw.substr(0, messageArgRaw.find(delimiter));
        std::string val = messageArgRaw.substr(messageArgRaw.find(delimiter)+1, messageArgRaw.length());
        MessageArg messageArg = make_tuple(key, val);
        message.emplace_back(messageArg);
    }
    return message;
}

Message Packet::getMessage() {
    return message;
}

Packet::Packet(string type, Message message) {
    type = parseType(type);
    Packet::message = message;

}

/**
 * Get a serial string version of the packet ready to be written out.
 * @return {@code std::string}
 */
string Packet::toString() {
    string str = type + ":";
    for(auto const& MessageArg: message){
        str.append(" "+get<0>(MessageArg)+":"+get<1>(MessageArg));
    }
    return str;
}

