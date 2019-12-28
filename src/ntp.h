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
    
    enum {ready=0, receiving_sample=1, receiving_with_sample=2, good=3} state;
    void    (*on_time_good)();
    bool on_good_signalled;
#ifdef unix
    std::mutex at_last_mtx;
#endif
public:
    unsigned long ntp_at = 0;
    unsigned long sent_at = 0;
    unsigned long received_at = 0;
    int receiving = 0;    // counter for mS loops waiting for reply
    unsigned long last_milli = 0;     // counter for poll periond
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
#ifdef unix
           std::lock_guard<std::mutex> guard(at_last_mtx);
#endif
           return last_ntp + (millis() - ntp_at);
        } else {
            return SecMilli();
        }
   }

   void send() {
        packet = {};
        packet.li_vn_mode =  0x1b;
        SecMilli current = now();
        if (current.not_null()) {
            packet.txTm_s = ntp_secs_from_epoch_secs(current.secs_);
            packet.txTm_f = ntp_frac_from_mills(current.millis_);
        }
        sent_at = millis();
        bntp->send(&packet);
   }

#if 0
    SecMilli    tdiff(uint32_t sec2, uint32_t sec1, uint32_t mill2, uint32_t mill1) {
#if 0
        printf("in sec2: %d", sec2);
        printf(" in sec1: %d", sec1);
        printf(" in milli2: %d", mill2);
        printf(" in milli1: %d", mill1);
#endif
        if (mill2 < mill1) {
            mill2 += 1000;
            sec1 += 1;
        }

        int32_t sec_diff = sec2 - sec1;
        int32_t smill_diff = mill2 - mill1;


        sec_diff += smill_diff / 1000;
        smill_diff = smill_diff % 1000;
#if 0
        printf(" sec diff:%d ", sec_diff);
        printf(" milli diff: %d\n", smill_diff);
#endif
        return SecMilli(sec_diff, smill_diff);
    }
#endif

   bool receive() {
        if (!bntp->receive(&packet)) {
            return false;
        }
        received_at = millis();

        int32_t rx_seconds = epoch_secs_from_ntp_secs(packet.rxTm_s);
        int32_t rx_millis = mills_from_ntp_frac(packet.rxTm_f);

        int32_t tx_seconds = epoch_secs_from_ntp_secs(packet.txTm_s);
        int32_t tx_millis = mills_from_ntp_frac(packet.txTm_f);

        uint32_t orig_seconds =  epoch_secs_from_ntp_secs(packet.origTm_s);
        uint32_t orig_millis = mills_from_ntp_frac(packet.origTm_f);

       SecMilli t4 = now();
       // SecMilli duration = tdiff(t4.secs_, orig_seconds, t4.millis_, orig_millis);
       auto orig = SecMilli(orig_seconds, orig_millis);
       auto rx = SecMilli(rx_seconds, rx_millis);
       auto tx = SecMilli(tx_seconds, tx_millis);
       auto dest = SecMilli(t4.secs_, t4.millis_);

       SecMilli src_to_dest = rx - orig;
       SecMilli tx_to_src = dest - tx;
       auto dd = src_to_dest.as_millis() + tx_to_src.as_millis();
#ifdef unix
       std::cout << "orig: " << orig
            << " rx: " << rx
            << " tx: " << tx
            << " t4: " << t4
            << " dd: " << dd
            << std::endl;
       std::cout << "src_to_dest: " << src_to_dest.as_millis()
                 << " tx_to_src: " << tx_to_src.as_millis()
                 << std::endl;
#endif
#ifdef unix
        std::lock_guard<std::mutex> guard(at_last_mtx);
#endif
        if (!orig_seconds) {
            // set ntp at
            ntp_at = received_at;
            last_ntp = SecMilli(tx_seconds, tx_millis);
        } else {
            // get difference between origin and tx
            if (dest.as_millis() > 0) {
                state = good;
            }
            //ntp_at = sent_at + dd / 2;
            // ntp_at = sent_at + dd;
            ntp_at = sent_at + dd / 2;
            last_ntp = tx;
#ifdef unix
            std::cout << "last_ntp: " << last_ntp
                << " tx: " << tx
                << " ntp_at: " << ntp_at
                << std::endl;
#endif
            if (state == good && !on_good_signalled) {
                if (on_time_good != nullptr) {
                    on_time_good();
                }
                on_good_signalled = true;
            }
        }
        if (state == receiving_sample) {
            state = receiving_with_sample;
        } 
        return true;
   } 
   void run() {
        auto now = millis();
        if (sent_at + 10000 < now || sent_at == 0) {
            printf("send\n");
            send();
            receiving = 1;
        }
        if (receiving) {
            if (sent_at + receiving <= now) {
                if (receive()) {
                    receiving = 0;
                    return;
                }
                receiving++;
                if (receiving == 2000) {
                    printf("Timeout\n");
                    receiving = 0;
                }
            }
        }
   }
};

