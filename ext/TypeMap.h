/*------------------------------------------------------------------------------
 * TypeMap.h
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
#ifndef FIRERUBY_TYPE_MAP_H
#define FIRERUBY_TYPE_MAP_H

   #ifndef IBASE_H_INCLUDED
      #include "ibase.h"
      #define IBASE_H_INCLUDED
   #endif

   #ifndef RUBY_H_INCLUDED
      #include "ruby.h"
      #define RUBY_H_INCLUDED
   #endif

   #ifndef FIRERUBY_DATABASE_H
      #include "ResultSet.h"
   #endif

/* Function prototypes. */
VALUE toArray(VALUE);
void setParameters(XSQLDA *, VALUE, VALUE, VALUE);
VALUE getModule(const char *);
VALUE getClass(const char *);
VALUE getClassInModule(const char *, VALUE);
VALUE getModuleInModule(const char *, VALUE);

#endif /* FIRERUBY_TYPE_MAP_H */
