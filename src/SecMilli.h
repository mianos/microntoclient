#pragma once

#include <time.h>
#include <sys/types.h>

struct SecMilli {
    uint32_t    secs_;
    unsigned long    millis_;
    SecMilli() : secs_(0), millis_(0) {}
    SecMilli(uint32_t secs, unsigned long millis) : secs_(secs), millis_(millis) {}
    SecMilli operator+(unsigned long millis) {
        if (!millis)
            SecMilli(secs_, millis_);
        uint32_t secs = secs_ + millis / 1000;
        unsigned long new_millis = millis_ + millis % 1000;

        if (new_millis >= 1000) {
            secs += 1;
            new_millis -= 1000;
        } else if (new_millis < 0) {
            secs -= 1;
            new_millis += 1000;
        }
        return SecMilli(secs, new_millis);
    }

    bool not_null() {
        return secs_ || millis_;
   }
    inline unsigned long as_millis() {
        return secs_ * 1000 + millis_;
    }
    void print() {
        char buffer[40];
        printf("%s\n", this->as_iso(buffer, sizeof (buffer)));
    }
   char *as_iso(char *buffer, int buffer_len) {
        struct tm timeinfo;
        time_t tt_secs = secs_;
        gmtime_r(&tt_secs, &timeinfo);
        snprintf(buffer, buffer_len, "%04d-%02d-%02dT%02d:%02d:%02d.%luZ",
                 timeinfo.tm_year + 1900,
                 timeinfo.tm_mon,
                 timeinfo.tm_mday,
                 timeinfo.tm_hour,
                 timeinfo.tm_min,
                 timeinfo.tm_sec,
                 millis_);
        return buffer;
    }
};

