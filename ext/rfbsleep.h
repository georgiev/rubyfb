#ifndef RFB_SLEEP_H
#define RFB_SLEEP_H

#define rfb_sleep(v) sleep((v))

#ifndef OS_UNIX
#ifdef __GNUC__

#undef rfb_sleep
#define rfb_sleep(v) Sleep((v))

#endif
#endif

#endif /* RFB_SLEEP_H */
