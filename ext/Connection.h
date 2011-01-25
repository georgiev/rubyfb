/*------------------------------------------------------------------------------
 * Connection.h
 *----------------------------------------------------------------------------*/
/**
 * Copyright © Peter Wood, 2005
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with the
 * License. You may obtain a copy of the License at
 *
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License for
 * the specificlanguage governing rights and  limitations under the License.
 *
 * The Original Code is the FireRuby extension for the Ruby language.
 *
 * The Initial Developer of the Original Code is Peter Wood. All Rights
 * Reserved.
 *
 * @author  Peter Wood
 * @version 1.0
 */
#ifndef FIRERUBY_CONNECTION_H
#define FIRERUBY_CONNECTION_H

/* Includes. */
   #ifndef FIRERUBY_FIRE_RUBY_H
      #include "FireRuby.h"
   #endif

   #ifndef FIRERUBY_FIRE_RUBY_EXCEPTION_H
      #include "FireRubyException.h"
   #endif

/* Structure definitions. */
typedef struct {
  isc_db_handle handle;
} ConnectionHandle;

/* Function prototypes. */
void Init_Connection(VALUE);
VALUE rb_connection_new(VALUE, VALUE, VALUE, VALUE);
void rb_tx_started(VALUE, VALUE);
void rb_tx_released(VALUE, VALUE);
void connectionFree(void *);

#endif /* FIRERUBY_CONNECTION_H */
