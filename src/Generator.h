/*------------------------------------------------------------------------------
 * Generator.h
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
#ifndef FIRERUBY_GENERATOR_H
#define FIRERUBY_GENERATOR_H

   /* Includes. */
   #ifndef FIRERUBY_FIRE_RUBY_EXCEPTION_H
      #include "FireRubyException.h"
   #endif

   #ifndef FIRERUBY_CONNECTION_H
      #include "Connection.h"
   #endif
   
   #ifndef RUBY_H_INCLUDED
      #include "ruby.h"
      #define RUBY_H_INCLUDED
   #endif
   
   /* Type definitions. */
   typedef struct
   {
      isc_db_handle *connection;
   } GeneratorHandle;

   /* Function prototypes. */
   void Init_Generator(VALUE);
   VALUE rb_generator_new(VALUE, VALUE);
   void generatorFree(void *);

#endif /* FIRERUBY_GENERATOR_H */
