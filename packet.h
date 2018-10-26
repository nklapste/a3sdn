//
// Created by nklap on 2018-10-26.
//

#ifndef A2SDN_PACKET_H
#define A2SDN_PACKET_H
/**
 * The different packet types that can be passed between the controller and switches.
 */

#include <string>
#include <vector>

using namespace std;

#define OPEN    "OPEN"
#define ACK     "ACK"
#define QUERY   "QUERY"
#define ADD     "ADD"
#define RELAY   "RELAY"

typedef tuple<string, string> MessageArg;
typedef vector<MessageArg> Message;

class Packet {
public:
    Packet(string type, Message message);

    explicit Packet(string &packetRaw);

    string getType();

    static string parseType(string &typeRaw);

    Message getMessage();

    string toString();

    static Message parseMessage(string &messageRaw);

private:
    string type;
    string rawMessage;
    Message message;
};

#endif //A2SDN_PACKET_H
