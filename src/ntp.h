#include <time.h>

#include "sysntp.h"

#if defined(__linux__) || defined(__APPLE__)
#define unix
#endif

#ifdef unix
#include <mutex>
#include "linux_bntp.h"
extern std::string getCurrentTimestamp();
extern unsigned long millis();
#else
#include "arduino_bntp.h"
#endif


class MiniNtp {
    const unsigned NTP_TIMESTAMP_DELTA = 2208988800;
//    const double millis_to_ntp_frac = 1000.0 / 4294967296.0;
    ntp_packet packet;
    BasicNtp *bntp;
    SecMilli last_ntp;
    unsigned long ntp_at;
    
    enum {ready=0, receiving_sample=1, receiving_with_sample=2, good=3} state;
    void    (*on_time_good)();
    bool on_good_signalled;
#ifdef unix
    std::mutex at_last_mtx;
#endif
public:
    MiniNtp(const char *host_name, void (*on_time_good)()=nullptr) :
            state(receiving_sample),
            on_time_good(on_time_good), on_good_signalled(false) {

        bntp = new BasicNtp(host_name, 123);
        packet = {};
    }
    ~MiniNtp() {
        delete bntp;
    }

    bool is_good() {
        return state == good;
    }

    inline unsigned int ntp_secs_from_epoch_secs(uint32_t epoch_secs) {
        return htonl(epoch_secs + NTP_TIMESTAMP_DELTA);
    }
    inline unsigned int epoch_secs_from_ntp_secs(uint32_t ntp_secs) {
        uint32_t ntp_secs_in_h =  ntohl(ntp_secs);
        return ntp_secs_in_h ? ntp_secs_in_h - NTP_TIMESTAMP_DELTA : 0;
    }

    inline unsigned int mills_from_ntp_frac(uint32_t frac) {
        return ntohl(frac) / 4294967;
    }
    inline unsigned int ntp_frac_from_mills(unsigned long smillis) {
        return htonl(smillis * 4294967);
    }
    SecMilli now() {
        if (state == good || state == receiving_with_sample) {
            std::cout << "now with good or sample" << std::endl;
#ifdef unix
           std::lock_guard<std::mutex> guard(at_last_mtx);
#endif
           return last_ntp + (millis() - ntp_at);
        } else {
            std::cout << "now none" << std::endl;
            return SecMilli();
        }
   }

   void send() {
        packet = {};
        packet.li_vn_mode =  0x1b;
        SecMilli current = now();
        packet.txTm_s = ntp_secs_from_epoch_secs(current.secs_);
        std::cout << "Sending secs: " << current.secs_ << std::endl;
        packet.txTm_f = ntp_frac_from_mills(current.millis_);
        std::cout << "Sending msecs: " << current.millis_ << std::endl;
        current.print();
        std::cout << "current: " << getCurrentTimestamp() << std::endl;
        //ntp_at = millis();
        bntp->send(&packet);
   }

    SecMilli    tdiff(uint32_t sec2, uint32_t sec1, uint32_t mill2, uint32_t mill1) {
        int32_t sec_diff = sec2 - sec1;
        int32_t smill_diff = mill2 - mill1;
        sec_diff += smill_diff / 1000;
        smill_diff = smill_diff % 1000;
        if (smill_diff < 0) {
            smill_diff += 1000;
            sec_diff -= 1;
        }
#ifdef unix
        std::cout << "in sec2: " << sec2
                  << " in sec1: " << sec1
                  << " in milli2: " << mill2
                  << " in milli1: " << mill1
                  << " sec diff: " << sec_diff
                  << " milli diff: " << smill_diff
                  << std::endl;
#endif
        return SecMilli(sec_diff, smill_diff);
    }

   bool receive() {
        if (!bntp->receive(&packet)) {
            printf("No packet\n");
            return false;
        }
        auto now_millis = millis();

        int32_t tx_seconds = epoch_secs_from_ntp_secs(packet.txTm_s);
        int32_t tx_millis = mills_from_ntp_frac(packet.txTm_f);

        uint32_t orig_seconds =  epoch_secs_from_ntp_secs(packet.origTm_s);
        uint32_t orig_millis = mills_from_ntp_frac(packet.origTm_f);

#ifdef unix
        std::lock_guard<std::mutex> guard(at_last_mtx);
#endif

        if (!orig_seconds) {
            // set ntp at
            std::cout << "or s: " << epoch_secs_from_ntp_secs(packet.origTm_s) << " or sec f: " << mills_from_ntp_frac(packet.origTm_f) << std::endl;
            ntp_at = now_millis;
            last_ntp = SecMilli(tx_seconds, tx_millis);
        } else {
            std::cout << "receive: tx seconds: " << tx_seconds << " from wire: " << packet.txTm_s << std::endl;
            std::cout << "stratum: " << (unsigned)packet.stratum
                << " poll " << (unsigned)packet.poll
                << " precision " << (unsigned)packet.precision
                << std::endl;
            // get difference between origin and tx
            SecMilli diff = tdiff(tx_seconds, orig_seconds, tx_millis, orig_millis);
            auto diff_millis = diff.as_millis();
            std::cout << "Difference millis: " << diff_millis << std::endl;
            if (diff_millis < 0) {
                ntp_at -= diff_millis;
                std::cout << "Negative diff ======" << std::endl;
            } else {
                auto total_time_from_send_to_receive =  now_millis - ntp_at;
                auto  time_from_ntp_tx_to_now = total_time_from_send_to_receive - diff_millis;
                // time from send to now, take this off the total time to mark the local millisecond point the server tx time was.
                ntp_at = now_millis - time_from_ntp_tx_to_now;
                last_ntp = SecMilli(tx_seconds, tx_millis);
                if (!on_good_signalled) {
                    if (on_time_good != nullptr) {
                        on_time_good();
                    }
                    on_good_signalled = true;
                }
            }
        }
        if (state == receiving_sample) {
            state = receiving_with_sample;
        } else {
            state = good;
        }
        return true;
   }
};

