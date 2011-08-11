/*------------------------------------------------------------------------------
 * AddUser.c
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
#include "AddUser.h"
#include "ibase.h"
#include "ServiceManager.h"

/* Function prototypes. */
static VALUE initializeAddUser(int, VALUE *, VALUE);
static VALUE getUserName(VALUE);
static VALUE setUserName(VALUE, VALUE);
static VALUE getUserPassword(VALUE);
static VALUE setUserPassword(VALUE, VALUE);
static VALUE getUserFirstName(VALUE);
static VALUE setUserFirstName(VALUE, VALUE);
static VALUE getUserMiddleName(VALUE);
static VALUE setUserMiddleName(VALUE, VALUE);
static VALUE getUserLastName(VALUE);
static VALUE setUserLastName(VALUE, VALUE);
static void createAddUserBuffer(VALUE, char **, short *);


/* Globals. */
VALUE cAddUser;


/**
 * This function provides the initialize method for the AddUser class.
 *
 * @param  argc  A count of the number of parameters passed to the method
 *               call.
 * @param  argv  A pointer to the start of an array of VALUE entities that
 *               contain the method parameters.
 * @param  self  A reference to the AddUser object being initialized.
 *
 * @return  A reference to the newly initialized AddUser object.
 *
 */
VALUE initializeAddUser(int argc, VALUE *argv, VALUE self)
{
   VALUE username = Qnil,
         password = Qnil,
         first    = Qnil,
         middle   = Qnil,
         last     = Qnil,
         value    = Qnil;
   int   length   = 0;
         
   /* Check that sufficient parameters have been supplied. */
   if(argc < 2)
   {
      rb_raise(rb_eArgError, "Wrong number of arguments (%d for %d).", argc, 2);
   }
   
   username = rb_funcall(argv[0], rb_intern("to_s"), 0);
   password = rb_funcall(argv[1], rb_intern("to_s"), 0);
   if(argc > 2)
   {
      first= rb_funcall(argv[2], rb_intern("to_s"), 0);
      if(argc > 3)
      {
         middle = rb_funcall(argv[3], rb_intern("to_s"), 0);
         if(argc > 4)
         {
            last = rb_funcall(argv[4], rb_intern("to_s"), 0);
         }
      }
   }
   
   /* Check that the parameters are valid. */
   value  = rb_funcall(username, rb_intern("length"), 0);
   length = TYPE(value) == T_FIXNUM ? FIX2INT(value) : NUM2INT(value);
   if(length < 1 || length > 31)
   {
      rb_fireruby_raise(NULL,
                        "Invalid user name specified. A user name must not be "\
                        "blank and may have no more than 31 characters.");
   }

   value  = rb_funcall(password, rb_intern("length"), 0);
   length = TYPE(value) == T_FIXNUM ? FIX2INT(value) : NUM2INT(value);
   if(length < 1 || length > 31)
   {
      rb_fireruby_raise(NULL,
                        "Invalid password specified. A user password must not "\
                        "be blank and may have no more than 31 characters.");
   }
   
   /* Assign class values. */
   rb_iv_set(self, "@user_name", username);
   rb_iv_set(self, "@password", password);
   rb_iv_set(self, "@first_name", first);
   rb_iv_set(self, "@middle_name", middle);
   rb_iv_set(self, "@last_name", last);
   
   return(self);
}


/**
 * This function provides the user_name attribute accessor for the AddUser
 * class.
 *
 * @param  self  A reference to the AddUser object to make the call on.
 *
 * @return  A reference to the attribute value for the object.
 *
 */
VALUE getUserName(VALUE self)
{
   return(rb_iv_get(self, "@user_name"));
}


/**
 * This function provides the user_name attribute mutator for the AddUser class.
 *
 * @param  self     A reference to the AddUser object to make the call on.
 * @param  setting  The new value for the attribute.
 *
 * @return  A reference to the newly update AddUser object.
 *
 */
VALUE setUserName(VALUE self, VALUE setting)
{
   VALUE actual = rb_funcall(setting, rb_intern("to_s"), 0),
         value  = rb_funcall(actual, rb_intern("length"), 0);
   int   length = 0;
   
   length = TYPE(value) == T_FIXNUM ? FIX2INT(value) : NUM2INT(value);
   if(length < 1 || length > 31)
   {
      rb_fireruby_raise(NULL,
                        "Invalid user name specified. A user name must not be "\
                        "blank and may have no more than 31 characters.");
   }
   rb_iv_set(self, "@user_name", actual);
   
   return(self);
}


/**
 * This function provides the password attribute accessor for the AddUser
 * class.
 *
 * @param  self  A reference to the AddUser object to make the call on.
 *
 * @return  A reference to the attribute value for the object.
 *
 */
VALUE getUserPassword(VALUE self)
{
   return(rb_iv_get(self, "@password"));
}


/**
 * This function provides the password attribute mutator for the AddUser class.
 *
 * @param  self     A reference to the AddUser object to make the call on.
 * @param  setting  The new value for the attribute.
 *
 * @return  A reference to the newly update AddUser object.
 *
 */
VALUE setUserPassword(VALUE self, VALUE setting)
{
   VALUE actual = rb_funcall(setting, rb_intern("to_s"), 0),
         value  = rb_funcall(actual, rb_intern("length"), 0);
   int   length = 0;
   
   length = TYPE(value) == T_FIXNUM ? FIX2INT(value) : NUM2INT(value);
   if(length < 1 || length > 31)
   {
      rb_fireruby_raise(NULL,
                        "Invalid password specified. A user password must not "\
                        "be blank and may have no more than 31 characters.");
   }
   rb_iv_set(self, "@password", actual);
   
   return(self);
}


/**
 * This function provides the first_name attribute accessor for the AddUser
 * class.
 *
 * @param  self  A reference to the AddUser object to make the call on.
 *
 * @return  A reference to the attribute value for the object.
 *
 */
VALUE getUserFirstName(VALUE self)
{
   return(rb_iv_get(self, "@first_name"));
}


/**
 * This function provides the first_name attribute mutator for the AddUser
 * class.
 *
 * @param  self     A reference to the AddUser object to make the call on.
 * @param  setting  The new value for the attribute.
 *
 * @return  A reference to the newly update AddUser object.
 *
 */
VALUE setUserFirstName(VALUE self, VALUE setting)
{
   rb_iv_set(self, "@first_name", setting);
   return(self);
}


/**
 * This function provides the middle_name attribute accessor for the AddUser
 * class.
 *
 * @param  self  A reference to the AddUser object to make the call on.
 *
 * @return  A reference to the attribute value for the object.
 *
 */
VALUE getUserMiddleName(VALUE self)
{
   return(rb_iv_get(self, "@middle_name"));
}


/**
 * This function provides the middle_name attribute mutator for the AddUser
 * class.
 *
 * @param  self     A reference to the AddUser object to make the call on.
 * @param  setting  The new value for the attribute.
 *
 * @return  A reference to the newly update AddUser object.
 *
 */
VALUE setUserMiddleName(VALUE self, VALUE setting)
{
   rb_iv_set(self, "@middle_name", setting);
   return(self);
}


/**
 * This function provides the last_name attribute accessor for the AddUser
 * class.
 *
 * @param  self  A reference to the AddUser object to make the call on.
 *
 * @return  A reference to the attribute value for the object.
 *
 */
VALUE getUserLastName(VALUE self)
{
   return(rb_iv_get(self, "@last_name"));
}


/**
 * This function provides the last_name attribute mutator for the AddUser
 * class.
 *
 * @param  self     A reference to the AddUser object to make the call on.
 * @param  setting  The new value for the attribute.
 *
 * @return  A reference to the newly update AddUser object.
 *
 */
VALUE setUserLastName(VALUE self, VALUE setting)
{
   rb_iv_set(self, "@last_name", setting);
   return(self);
}


/**
 * This function provides the execute method for the AddUser class.
 *
 * @param  self     A reference to the AddUser object to be executed.
 * @param  manager  A reference to the ServiceManager that will be used to
 *                  execute the task.
 *
 * @return  A reference to the AddUser object executed.
 *
 */
VALUE executeAddUser(VALUE self, VALUE manager)
{
   ManagerHandle *handle = NULL;
   char          *buffer  = NULL;
   short         length   = 0;
   ISC_STATUS    status[20];

   /* Check that the service manager is connected. */
   Data_Get_Struct(manager, ManagerHandle, handle);
   if(handle->handle == 0)
   {
      rb_fireruby_raise(NULL,
                        "Add user error. Service manager not connected.");
   }

   createAddUserBuffer(self, &buffer, &length);

   /* Start the service request. */
   if(isc_service_start(status, &handle->handle, NULL, length, buffer))
   {
      free(buffer);
      rb_fireruby_raise(status, "Error adding user.");
   }
   free(buffer);

   return(self);
}


/**
 * This function provides the execute method for the AddUser class.
 *
 * @param  self    A reference to the AddUser object to generate the buffer for.
 * @param  buffer  A pointer to a pointer that will be set to contain the
 *                 buffer contents.
 * @param  length  A pointer to a short integer that will be assigned the length
 *                 of the buffer.
 *
 */
void createAddUserBuffer(VALUE self, char **buffer, short *length)
{
   VALUE value   = Qnil,
         first   = Qnil,
         middle  = Qnil,
         last    = Qnil;
   char  *offset = NULL;
   int   number  = 0;
   
   /* Calculate the required buffer length. */
   *length = 1;
   *length += strlen(STR2CSTR(rb_iv_get(self, "@user_name"))) + 3;
   *length += strlen(STR2CSTR(rb_iv_get(self, "@password"))) + 3;
   
   value = rb_iv_get(self, "@first_name");
   if(value != Qnil)
   {
      first   = rb_funcall(value, rb_intern("to_s"), 0);
      *length += strlen(STR2CSTR(first)) + 3;
   }
   
   value = rb_iv_get(self, "@middle_name");
   if(value != Qnil)
   {
      middle  = rb_funcall(value, rb_intern("to_s"), 0);
      *length += strlen(STR2CSTR(middle)) + 3;
   }
   
   value = rb_iv_get(self, "@last_name");
   if(value != Qnil)
   {
      last    = rb_funcall(value, rb_intern("to_s"), 0);
      *length += strlen(STR2CSTR(last)) + 3;
   }
   
   /* Create and populate the buffer. */
   offset = *buffer = ALLOC_N(char, *length);
   if(*buffer == NULL)
   {
      rb_raise(rb_eNoMemError,
               "Memory allocation error preparing to add user.");
   }
   memset(*buffer, 0, *length);

   *offset++ = isc_action_svc_add_user;
   
   *offset++ = isc_spb_sec_username;
   value     = rb_iv_get(self, "@user_name");
   number    = strlen(STR2CSTR(value));
   ADD_SPB_LENGTH(offset, number);
   memcpy(offset, STR2CSTR(value), number);
   offset    += number;
   
   *offset++ = isc_spb_sec_password;
   value     = rb_iv_get(self, "@password");
   number    = strlen(STR2CSTR(value));
   ADD_SPB_LENGTH(offset, number);
   memcpy(offset, STR2CSTR(value), number);
   offset    += number;
   
   if(first != Qnil)
   {
      *offset++ = isc_spb_sec_firstname;
      number    = strlen(STR2CSTR(first));
      ADD_SPB_LENGTH(offset, number);
      memcpy(offset, STR2CSTR(first), number);
      offset    += number;
   }
   
   if(middle != Qnil)
   {
      *offset++ = isc_spb_sec_middlename;
      number    = strlen(STR2CSTR(middle));
      ADD_SPB_LENGTH(offset, number);
      memcpy(offset, STR2CSTR(middle), number);
      offset    += number;
   }
   
   if(last != Qnil)
   {
      *offset++ = isc_spb_sec_lastname;
      number    = strlen(STR2CSTR(last));
      ADD_SPB_LENGTH(offset, number);
      memcpy(offset, STR2CSTR(last), number);
      offset    += number;
   }
}


/**
 * This function initialize the AddUser class in the Ruby environment.
 *
 * @param  module  The module to create the new class definition under.
 *
 */
void Init_AddUser(VALUE module)
{
   cAddUser = rb_define_class_under(module, "AddUser", rb_cObject);
   rb_define_method(cAddUser, "initialize", initializeAddUser, -1);
   rb_define_method(cAddUser, "user_name", getUserName, 0);
   rb_define_method(cAddUser, "user_name=", setUserName, 1);
   rb_define_method(cAddUser, "password", getUserPassword, 0);
   rb_define_method(cAddUser, "password=", setUserPassword, 1);
   rb_define_method(cAddUser, "first_name", getUserFirstName, 0);
   rb_define_method(cAddUser, "first_name=", setUserFirstName, 1);
   rb_define_method(cAddUser, "middle_name", getUserMiddleName, 0);
   rb_define_method(cAddUser, "middle_name=", setUserMiddleName, 1);
   rb_define_method(cAddUser, "last_name", getUserLastName, 0);
   rb_define_method(cAddUser, "last_name=", setUserLastName, 1);
   rb_define_method(cAddUser, "execute", executeAddUser, 1);
}
