/*------------------------------------------------------------------------------
 * Common.c
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

/* Includes. */
#include "Common.h"
#include "FireRubyException.h"


/**
 * This function is used to preclude the use of any of the Ruby copy methods
 * (clone or dup) with an object.
 *
 * @param  copy      A reference to the object copy.
 * @param  original  A reference to the original object that is being copied.
 *
 * @return  Never returns as an exception is always generated.
 *
 */
VALUE forbidObjectCopy(VALUE copy, VALUE original) {
  VALUE message = rb_str_new2("Copying of CLASS_NAME objects is forbidden."),
        value   = rb_funcall(original, rb_intern("class"), 0),
        array[2];

  array[0] = rb_str_new2("CLASS_NAME");
  array[1] = rb_funcall(value, rb_intern("name"), 0);
  message = rb_funcall(message, rb_intern("gsub"), 2, array);

  rb_fireruby_raise(NULL, StringValuePtr(message));

  return(Qnil);
}
