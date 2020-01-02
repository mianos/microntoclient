#include <fstream>
#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <random>
#include <memory>

#include "SecMilli.h"
#include "ntp.h"

std::string getCurrentTimestamp()
{
	auto currentTime = std::chrono::system_clock::now();
	char buffer[80];

    auto seconds = std::chrono::time_point_cast<std::chrono::seconds>(currentTime);
    auto fraction = currentTime - seconds;
	std::time_t tt;
	tt = std::chrono::system_clock::to_time_t(currentTime);
	auto timeinfo = localtime (&tt);
	strftime(buffer, 50, "%F %H:%M:%S", timeinfo);
    auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(fraction);
	sprintf(buffer, "%s:%03ld", buffer, milliseconds.count());

	return std::string(buffer);
}

std::unique_ptr<std::ofstream> of;

void print(MiniNtp& mntp) {
    char buf[100];
    auto nn = mntp.now();
    auto ct = getCurrentTimestamp();

    auto currentTime = std::chrono::system_clock::now();
    auto seconds = std::chrono::time_point_cast<std::chrono::seconds>(currentTime);
    auto fraction = currentTime - seconds;
    auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(fraction);
    auto ms = milliseconds.count();
    std::cout << "local: " << ct
            << "   RESULT: " << nn.local(buf, sizeof(buf), 10)
            << " diff: " << (ms + 1000) - (int)nn.millis_ - 1000
            << std::endl;
}

void get_time(MiniNtp& mntp) {
    for (;;) {
        if (mntp.run()) {
            print(mntp);
        }
    }
}

void print_proc(MiniNtp& mntp) {
    for (;;) {
        if (mntp.is_good()) {
            print(mntp);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}


int main() {
    //MiniNtp mntp{"fw13.mianos.com", [](){ printf("time good\n"); }};
//    MiniNtp mntp{"131.84.1.10", [](){ printf("time good\n"); }};
    MiniNtp mntp{"10.8.0.1", [](){ printf("time good\n"); }, 5000};

    of = std::make_unique<std::ofstream>("data.csv");
        
    std::thread timeg(get_time, std::ref(mntp));
//    std::thread print_time(print_proc, std::ref(mntp));

    timeg.join();
//    print_time.join();
}
