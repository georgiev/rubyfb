#ifndef RFB_STR_H
#define RFB_STR_H

#ifndef RUBY_H_INCLUDED
  #include "ruby.h"
  #define RUBY_H_INCLUDED
#endif
#include "rfbint.h"

VALUE rfbstr(VALUE, short, const char *, short);

#endif /* RFB_STR_H */
