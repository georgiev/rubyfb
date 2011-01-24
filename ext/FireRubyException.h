/*------------------------------------------------------------------------------
 * FireRubyException.h
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
#ifndef FIRERUBY_FIRE_RUBY_EXCEPTION_H
#define FIRERUBY_FIRE_RUBY_EXCEPTION_H

   /* Includes. */
   #ifndef RUBY_H_INCLUDED
      #include "ruby.h"
      #define RUBY_H_INCLUDED
   #endif
   
   #ifndef IBASE_H_INCLUDED
      #include "ibase.h"
      #define IBASE_H_INCLUDED
   #endif
   
   /* Function prototypes. */
   void Init_FireRubyException(VALUE);
   VALUE rb_fireruby_exception_new(const char *);
   void rb_fireruby_raise(const ISC_STATUS *, const char *);
   
#endif /* FIRERUBY_FIRE_RUBY_EXCEPTION_H */
