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

std::random_device r;
std::default_random_engine e1(r());



void get_time(MiniNtp& mntp) {
    std::uniform_int_distribution<int> uniform_dist(1000, 6000);
    for (;;) {
        mntp.send();
        int count = 0;
        while (!mntp.receive()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            count++;
        }
        std::cout << "Count: " << count << std::endl;
        std::cout << mntp.sent_at << " : " << mntp.received_at << " diff: " << mntp.received_at - mntp.sent_at << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(uniform_dist(e1)));
    }
}

void print_proc(MiniNtp& mntp) {
    for (;;) {
        if (mntp.is_good()) {
            char buf[100];
            auto nn = mntp.now();
            std::cout << "local: " << getCurrentTimestamp()
                    << "   RESULT: " << nn.as_iso(buf, sizeof(buf)) << std::endl;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}


int main() {
    //MiniNtp mntp{"fw13.mianos.com", [](){ printf("time good\n"); }};
    //MiniNtp mntp{"131.84.1.10", [](){ printf("time good\n"); }};
    MiniNtp mntp{"10.8.0.1", [](){ printf("time good\n"); }};

        
    std::thread timeg(get_time, std::ref(mntp));
    std::thread print_time(print_proc, std::ref(mntp));

    timeg.join();
    print_time.join();
}
