#pragma once

#include <time.h>

#include <sys/types.h>

struct SecMilli {
    long    secs_;
    unsigned long    millis_;
    SecMilli() : secs_(0), millis_(0) {}
    SecMilli(long secs, unsigned long millis) : secs_(secs), millis_(millis) {}
    SecMilli operator+(unsigned long millis) {
        if (!millis)
            SecMilli(secs_, millis_);
        long secs = secs_ + millis / 1000;
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


    SecMilli operator-(const SecMilli& const_rhs) {
        SecMilli rhs(const_rhs);
        int32_t sec_diff = secs_ - rhs.secs_;
        int32_t smill_diff = millis_ - rhs.millis_;


        sec_diff += smill_diff / 1000;
        smill_diff = smill_diff % 1000;
        return SecMilli(sec_diff, smill_diff);
    }

    SecMilli operator+(const SecMilli& const_rhs) {
        SecMilli rhs(const_rhs);
        int32_t sec_add = secs_ - rhs.secs_;
        int32_t smill_add = rhs.millis_ - millis_;

        sec_add += smill_add / 1000;
        smill_add = smill_add % 1000;
        return SecMilli(sec_add, smill_add);
    }

    bool not_null() {
        return secs_ || millis_;
   }
    inline long as_millis() {
        return secs_ * 1000 + millis_;
    }
    void print() {
        char buffer[40];
        printf("%s\n", this->as_iso(buffer, sizeof (buffer)));
    }
   char *as_iso(char *buffer, int buffer_len) const {
        struct tm timeinfo;
        time_t tt_secs = secs_;
        gmtime_r(&tt_secs, &timeinfo);
        snprintf(buffer, buffer_len, "%04d-%02d-%02dT%02d:%02d:%02d.%lu",
                 timeinfo.tm_year + 1900,
                 timeinfo.tm_mon,
                 timeinfo.tm_mday,
                 timeinfo.tm_hour,
                 timeinfo.tm_min,
                 timeinfo.tm_sec,
                 millis_);
        return buffer;
    }
    char *local(char *buffer, int buffer_len) const {
        struct tm timeinfo;
        time_t tt_secs = secs_;
        localtime_r(&tt_secs, &timeinfo);
        snprintf(buffer, buffer_len, "%02d:%02d:%02d.%lu",
                 timeinfo.tm_hour % 12,
                 timeinfo.tm_min,
                 timeinfo.tm_sec,
                 millis_);
        return buffer;
    }
    char *local(char *buffer, int buffer_len, int hours_offset, bool dst) const {
        struct tm timeinfo;
        time_t tt_secs = secs_ + 3600 * hours_offset + (dst ? 3600 : 0);
        gmtime_r(&tt_secs, &timeinfo);
        snprintf(buffer, buffer_len, "%02d:%02d:%02d.%lu",
                 timeinfo.tm_hour,
                 timeinfo.tm_min,
                 timeinfo.tm_sec,
                 millis_);
        return buffer;
    }
#if defined(__linux__) || defined(__APPLE__)
   friend std::ostream& operator<<(std::ostream& os, const SecMilli& sm) {
        char buffer[40];
        sm.as_iso(buffer, sizeof (buffer));
        os << buffer;
       return os;
   }
#endif
};

