/*------------------------------------------------------------------------------
 * FireRuby.h
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
#ifndef FIRERUBY_FIRE_RUBY_H
#define FIRERUBY_FIRE_RUBY_H

/* Includes. */
   #ifndef RUBY_H_INCLUDED
      #include "ruby.h"
      #define RUBY_H_INCLUDED
   #endif

   #ifndef IBASE_H_INCLUDED
      #include "ibase.h"
      #define IBASE_H_INCLUDED
   #endif

/* Definitions. */
   #define MAJOR_VERSION_NO                      0
   #define MINOR_VERSION_NO                      4
   #define BUILD_NO                              3

/* Function definitions. */
VALUE getFireRubySetting(const char *);
void getClassName(VALUE, char *);
VALUE toSymbol(const char *);
VALUE getColumnType(const XSQLVAR *);

#endif /* FIRERUBY_FIRE_RUBY_H */
