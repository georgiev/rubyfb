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

/**
 * This method fetches a Ruby constant definition. If the module specified to
 * the function is nil then the top level is assume
 *
 * @param  name    The name of the constant to be retrieved.
 * @param  module  A reference to the Ruby module that should contain the
 *                 constant.
 *
 * @return  A Ruby VALUE representing the constant.
 *
 */
static VALUE getConstant(const char *name, VALUE module) {
  VALUE owner = module,
        constants,
        exists,
        entry = Qnil,
        symbol = ID2SYM(rb_intern(name));

  /* Check that we've got somewhere to look. */
  if(owner == Qnil) {
    owner = rb_cModule;
  }

  constants = rb_funcall(owner, rb_intern("constants"), 0),
  exists = rb_funcall(constants, rb_intern("include?"), 1, symbol);
  if(exists == Qfalse) {
    /* 1.8 style lookup */
    exists = rb_funcall(constants, rb_intern("include?"), 1, rb_str_new2(name));
  }
  if(exists != Qfalse) {
    entry = rb_funcall(owner, rb_intern("const_get"), 1, symbol);
  }
  return(entry);
}


/**
 * This function fetches a class from a specified module.
 *
 * @param  name   The name of the class to be retrieved.
 * @param  owner  The module to search for the class in.
 *
 * @return  A Ruby VALUE representing the requested module, or nil if it could
 *          not be located.
 *
 */
static VALUE getClassInModule(const char *name, VALUE owner) {
  VALUE klass = getConstant(name, owner);

  if(klass != Qnil) {
    VALUE type = rb_funcall(klass, rb_intern("class"), 0);

    if(type != rb_cClass) {
      klass = Qnil;
    }
  }

  return(klass);
}

/**
 * This method fetches a Ruby class definition object based on a class name.
 * The class is assumed to have been defined at the top level.
 *
 * @return  A Ruby VALUE representing the requested class, or nil if the class
 *          could not be found.
 *
 */
VALUE getClass(const char *name) {
  return getClassInModule(name, Qnil);
}

