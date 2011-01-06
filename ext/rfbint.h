#ifndef RFB_INT_H
#define RFB_INT_H

#ifdef OS_UNIX
  #include <inttypes.h>
#else
  #ifndef __GNUC__
    typedef short     int16_t;
    typedef long      int32_t;
    typedef long long int64_t;
  #endif
#endif

#endif /* RFB_INT_H */
