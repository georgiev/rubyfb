/*------------------------------------------------------------------------------
 * RemoveUser.c
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
#include "RemoveUser.h"
#include "ibase.h"
#include "ServiceManager.h"

/* Function prototypes. */
static VALUE initializeRemoveUser(VALUE, VALUE);
static VALUE getUserName(VALUE);
static VALUE setUserName(VALUE, VALUE);
static void createRemoveUserBuffer(VALUE, char **, short *);


/* Globals. */
VALUE cRemoveUser;


/**
 * This function provides the initialize method for the RemoveUser class.
 *
 * @param  self      A reference to the RemoveUser object being initialized.
 * @param  username  A reference to a String containing the user name of the
 *                   user to be removed.
 *
 * @return  A reference to the newly initialized RemoveUser object.
 *
 */
VALUE initializeRemoveUser(VALUE self, VALUE username) {
  VALUE actual = rb_funcall(username, rb_intern("to_s"), 0),
        value  = Qnil;
  int length = 0;

  /* Check that the parameters are valid. */
  value  = rb_funcall(actual, rb_intern("length"), 0);
  length = TYPE(value) == T_FIXNUM ? FIX2INT(value) : NUM2INT(value);
  if(length < 1 || length > 31) {
    rb_fireruby_raise(NULL,
                      "Invalid user name specified. A user name must not be " \
                      "blank and may have no more than 31 characters.");
  }

  /* Assign class values. */
  rb_iv_set(self, "@user_name", actual);

  return(self);
}


/**
 * This function provides the user_name attribute accessor for the RemoveUser
 * class.
 *
 * @param  self  A reference to the RemoveUser object to make the call on.
 *
 * @return  A reference to the attribute value for the object.
 *
 */
VALUE getUserName(VALUE self) {
  return(rb_iv_get(self, "@user_name"));
}


/**
 * This function provides the user_name attribute mutator for the RemoveUser
 * class.
 *
 * @param  self     A reference to the RemoveUser object to make the call on.
 * @param  setting  The new value for the attribute.
 *
 * @return  A reference to the newly update RemoveUser object.
 *
 */
VALUE setUserName(VALUE self, VALUE setting) {
  VALUE actual = rb_funcall(setting, rb_intern("to_s"), 0),
        value  = rb_funcall(actual, rb_intern("length"), 0);
  int length = 0;

  length = TYPE(value) == T_FIXNUM ? FIX2INT(value) : NUM2INT(value);
  if(length < 1 || length > 31) {
    rb_fireruby_raise(NULL,
                      "Invalid user name specified. A user name must not be " \
                      "blank and may have no more than 31 characters.");
  }
  rb_iv_set(self, "@user_name", actual);

  return(self);
}



/**
 * This function provides the execute method for the RemoveUser class.
 *
 * @param  self     A reference to the RemoveUser object to be executed.
 * @param  manager  A reference to the ServiceManager that will be used to
 *                  execute the task.
 *
 * @return  A reference to the RemoveUser object executed.
 *
 */
VALUE executeRemoveUser(VALUE self, VALUE manager) {
  ManagerHandle *handle = NULL;
  char          *buffer  = NULL;
  short length   = 0;
  ISC_STATUS status[ISC_STATUS_LENGTH];

  /* Check that the service manager is connected. */
  Data_Get_Struct(manager, ManagerHandle, handle);
  if(handle->handle == 0) {
    rb_fireruby_raise(NULL,
                      "Remove user error. Service manager not connected.");
  }

  createRemoveUserBuffer(self, &buffer, &length);

  /* Start the service request. */
  if(isc_service_start(status, &handle->handle, NULL, length, buffer)) {
    free(buffer);
    rb_fireruby_raise(status, "Error removing user.");
  }
  free(buffer);

  return(self);
}


/**
 * This function provides the execute method for the RemoveUser class.
 *
 * @param  self    A reference to the RemoveUser object to generate the buffer for.
 * @param  buffer  A pointer to a pointer that will be set to contain the
 *                 buffer contents.
 * @param  length  A pointer to a short integer that will be assigned the length
 *                 of the buffer.
 *
 */
void createRemoveUserBuffer(VALUE self, char **buffer, short *length) {
  VALUE value   = Qnil,
        tmp_str = Qnil;
  char  *offset = NULL;
  int number  = 0;

  /* Calculate the required buffer length. */
  *length = 1;
  tmp_str = rb_iv_get(self, "@user_name");
  *length += strlen(StringValuePtr(tmp_str)) + 3;

  /* Create and populate the buffer. */
  offset = *buffer = ALLOC_N(char, *length);
  if(*buffer == NULL) {
    rb_raise(rb_eNoMemError,
             "Memory allocation error preparing to remove user.");
  }
  memset(*buffer, 0, *length);

  *offset++ = isc_action_svc_delete_user;

  *offset++ = isc_spb_sec_username;
  value     = rb_iv_get(self, "@user_name");
  number    = strlen(StringValuePtr(value));
  ADD_SPB_LENGTH(offset, number);
  memcpy(offset, StringValuePtr(value), number);
  offset    += number;
}


/**
 * This function initialize the RemoveUser class in the Ruby environment.
 *
 * @param  module  The module to create the new class definition under.
 *
 */
void Init_RemoveUser(VALUE module) {
  cRemoveUser = rb_define_class_under(module, "RemoveUser", rb_cObject);
  rb_define_method(cRemoveUser, "initialize", initializeRemoveUser, 1);
  rb_define_method(cRemoveUser, "user_name", getUserName, 0);
  rb_define_method(cRemoveUser, "user_name=", setUserName, 1);
  rb_define_method(cRemoveUser, "execute", executeRemoveUser, 1);
}
