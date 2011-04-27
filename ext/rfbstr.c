/*------------------------------------------------------------------------------
 * rfbstr.c
 *----------------------------------------------------------------------------*/

/* Includes. */
#include "rfbstr.h"

/**
 * This function converts a sql data into ruby string
 * respecting data encoding
 *
 * @param connection  The connection object relating to the data
 * @param sqlsubtype  SQL subtype of the field (fot character types - used to hold encoding information)
 * @param data  A pointer to the sql data
 * @param length  Length of the sql data
 *
 * @return  A Ruby String object with correct encoding
 *
 */
VALUE rfbstr(VALUE connection, short sqlsubtype, const char *data, short length) {
  VALUE value = Qnil;
  if (length >= 0) {
    value = rb_str_new(data, length);
    value = rb_funcall(connection, rb_intern("force_encoding"), 2, value, INT2FIX(sqlsubtype));
  }
  return value;
}

