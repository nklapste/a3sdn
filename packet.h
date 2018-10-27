/**
 * a2sdn packet.h
 *
 * @author Nathan Klapstein (nklapste)
 * @version 0.0.0
 */

#ifndef A2SDN_PACKET_H
#define A2SDN_PACKET_H

#include <string>
#include <vector>

using namespace std;

/**
 * The different packet types that can be passed between the controller and switches.
 */
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
    Message message;
};

#endif //A2SDN_PACKET_H
