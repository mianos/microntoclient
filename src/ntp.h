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
    int poll_period = 10000;
    int timeout = 2000;
public:
    unsigned long ntp_at = 0;
    unsigned long sent_at = 0;
    unsigned long received_at = 0;
    int receiving = 0;    // counter for mS loops waiting for reply
    unsigned long last_milli = 0;     // counter for poll periond
    MiniNtp(const char *host_name, void (*on_time_good)()=nullptr, int poll_period=10000, int timeout=2000) :
            state(receiving_sample),
            on_time_good(on_time_good),
            on_good_signalled(false),
            poll_period(poll_period),
            timeout(timeout) {

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

   bool receive() {
        if (!bntp->receive(&packet)) {
            return false;
        }
        received_at = millis();

        uint32_t orig_seconds =  epoch_secs_from_ntp_secs(packet.origTm_s);
        uint32_t orig_millis = mills_from_ntp_frac(packet.origTm_f);

        int32_t rx_seconds = epoch_secs_from_ntp_secs(packet.rxTm_s);
        int32_t rx_millis = mills_from_ntp_frac(packet.rxTm_f);

        int32_t tx_seconds = epoch_secs_from_ntp_secs(packet.txTm_s);
        int32_t tx_millis = mills_from_ntp_frac(packet.txTm_f);

        //uint32_t root_delay = mills_from_ntp_frac(packet.rootDelay);
        // uint32_t root_dispersion = mills_from_ntp_frac(packet.rootDispersion);

        SecMilli t4 = now();

       // SecMilli duration = tdiff(t4.secs_, orig_seconds, t4.millis_, orig_millis);
       auto orig = SecMilli(orig_seconds, orig_millis);
       auto rx = SecMilli(rx_seconds, rx_millis);
       auto tx = SecMilli(tx_seconds, tx_millis);
       auto dest = SecMilli(t4.secs_, t4.millis_);

       SecMilli src_to_dest = rx - orig;
       SecMilli tx_to_src = dest - tx;
       auto dd = src_to_dest.as_millis() + tx_to_src.as_millis();
       auto offset = ((rx - orig).as_millis() + (dest - tx).as_millis()) / 2;
       auto delay = (t4 - orig).as_millis() - (tx - rx).as_millis();

#ifdef unix
       std::cout << "offset: " << offset
                << " dd " << dd
                 << " Delay: " << delay << std::endl;
       std::cout << "orig: " << orig
            << " rx: " << rx
            << " tx: " << tx
            << " t4: " << t4
            << std::endl;
       std::cout << "src_to_dest: " << src_to_dest.as_millis()
                 << " tx_to_src: " << tx_to_src.as_millis()
//                 << " rootDelay: " << root_delay
 //                << " raw rootDelay: " << packet.rootDelay
  //               << " rootDispersion: " << root_dispersion
   //              << " raw rootDispersionay: " << packet.rootDispersion
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
            ntp_at = millis() - offset;
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
        if (sent_at + poll_period < now || sent_at == 0) {
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
                if (receiving == timeout) {
                    printf("Timeout\n");
                    receiving = 0;
                }
            }
        }
   }
};

