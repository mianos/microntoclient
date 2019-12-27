#include <WiFiUdp.h>

class BasicNtp {
    const char *host_name;
    int ntp_port;
    WiFiUDP udp;
    IPAddress     timeServerIP;
public:
    BasicNtp(const char *host_name, int ntp_port=123) : host_name(host_name), ntp_port(ntp_port) {
        if (fcntl(sock, F_SETFL, O_NONBLOCK) == -1)
        udp.begin(ntp_port);
    }
    ~BasicNtp() {
        udp.stop();
    }
    int send(ntp_packet *packet) {
        if (!WiFi.hostByName(host_name, timeServerIP) ) { //get a random server from the pool
            printf("failed looking up host '%s'\n", host_name);
            printf("status %d\n", WiFi.status());
            return -1;
        }
        udp.beginPacket(timeServerIP, ntp_port);
        /* int sent = */
        udp.write((uint8_t *)packet, sizeof (ntp_packet));
        udp.endPacket();
        return 0;
    }

    int receive(ntp_packet *packet) {
        int cb = udp.parsePacket();
        if (cb) {
            return udp.read((char *)packet, sizeof (ntp_packet));
        }
        return 0;
    }
};
