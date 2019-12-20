#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <future>

#include "SecMilli.h"
#include "ntp.h"

void receive() {
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::cout << "receive" << std::endl;
}

void send_first() {
    std::cout << "send first" << std::endl;
}

void send() {
    std::cout << "send" << std::endl;
}


std::string getCurrentTimestamp()
{
	using std::chrono::system_clock;
	auto currentTime = std::chrono::system_clock::now();
	char buffer[80];

	auto transformed = currentTime.time_since_epoch().count() / 1000000;

	auto millis = transformed % 1000;

	std::time_t tt;
	tt = system_clock::to_time_t ( currentTime );
	auto timeinfo = localtime (&tt);
	strftime (buffer,80,"%F %H:%M:%S",timeinfo);
	sprintf(buffer, "%s:%03d",buffer,(int)millis);

	return std::string(buffer);
}

void get_time(MiniNtp& mntp) {
    for (int ii = 0; ii < 3; ii++) {
        mntp.send();
        mntp.receive();
        if (mntp.is_good()) {
            char buf[100];
            auto nn = mntp.now();
            std::cout << "local         : " << getCurrentTimestamp() << std::endl;
            std::cout << "RESULT        : " << nn.as_iso(buf, sizeof(buf)) << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(2));
            nn = mntp.now();
            std::cout << "RESULT after 2: " << nn.as_iso(buf, sizeof(buf)) << std::endl;
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

int main() {
    //MiniNtp mntp{"fw13.mianos.com", [](){ printf("time good\n"); }};
    MiniNtp mntp{"us.pool.ntp.org", [](){ printf("time good\n"); }};
    std::thread timeg(get_time, std::ref(mntp));

    timeg.join();
}
