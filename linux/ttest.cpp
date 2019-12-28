#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <random>

#include "SecMilli.h"
#include "ntp.h"

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
    for (;;) {
        mntp.run();
    }
}

void print_proc(MiniNtp& mntp) {
    for (;;) {
        if (mntp.is_good()) {
            char buf[100];
            auto nn = mntp.now();
            auto currentTime = std::chrono::system_clock::now();
            auto transformed = currentTime.time_since_epoch().count() / 1000000;
            int millis = transformed % 1000;
            auto ct = getCurrentTimestamp();
            std::cout << "local: " << ct
                    << "   RESULT: " << nn.as_iso(buf, sizeof(buf))
                    << " diff: " << millis - (int)nn.millis_
                    << std::endl;

        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}


int main() {
    //MiniNtp mntp{"fw13.mianos.com", [](){ printf("time good\n"); }};
    MiniNtp mntp{"131.84.1.10", [](){ printf("time good\n"); }};
 //   MiniNtp mntp{"10.8.0.1", [](){ printf("time good\n"); }};

        
    std::thread timeg(get_time, std::ref(mntp));
    std::thread print_time(print_proc, std::ref(mntp));

    timeg.join();
    print_time.join();
}
