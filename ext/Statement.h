/*------------------------------------------------------------------------------
 * Statement.h
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
#ifndef FIRERUBY_STATEMENT_H
#define FIRERUBY_STATEMENT_H

/* Includes. */
   #ifndef IBASE_H_INCLUDED
      #include "ibase.h"
      #define IBASE_H_INCLUDED
   #endif

   #ifndef RUBY_H_INCLUDED
      #include "ruby.h"
      #define RUBY_H_INCLUDED
   #endif

/* Type definitions. */
typedef struct {
  isc_stmt_handle handle;
  int type,
      inputs,
      outputs;
  short dialect;
  XSQLDA          *parameters;
  XSQLDA          *output;
} StatementHandle;

/* Function prototypes. */
/*void prepare(isc_db_handle *, isc_tr_handle *, char *, isc_stmt_handle *,
             short, int *, int *, int *);
void execute(isc_tr_handle *, isc_stmt_handle *, short, XSQLDA *,
             int, long *);*/
VALUE rb_statement_new(VALUE, VALUE);
VALUE rb_execute_sql(VALUE, VALUE, VALUE, VALUE);
void Init_Statement(VALUE);

#endif /* FIRERUBY_STATEMENT_H */
