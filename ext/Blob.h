/*------------------------------------------------------------------------------
 * Blob.h
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
#ifndef FIRERUBY_BLOB_H
#define FIRERUBY_BLOB_H

/* Includes. */
   #ifndef FIRERUBY_FIRE_RUBY_EXCEPTION_H
      #include "FireRubyException.h"
   #endif

   #ifndef IBASE_H_INCLUDED
      #include "ibase.h"
      #define IBASE_H_INCLUDED
   #endif

   #ifndef RUBY_H_INCLUDED
      #include "ruby.h"
      #define RUBY_H_INCLUDED
   #endif
  #include "Connection.h"
  #include "Transaction.h"

/* Type definitions. */
typedef struct {
  ISC_BLOB_DESC description;
  ISC_LONG segments,
           size;
  isc_blob_handle handle;
} BlobHandle;

/* Data elements. */
extern VALUE cBlob;

/* Function prototypes. */
BlobHandle *openBlob(ISC_QUAD,
                     char *,
                     char *,
                     VALUE,
                     VALUE);
void Init_Blob(VALUE);
void blobFree(void *);
VALUE initializeBlob(VALUE);

#endif /* FIRERUBY_BLOB_H */
