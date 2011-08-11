/*------------------------------------------------------------------------------
 * FireRubyException.c
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
#include "FireRubyException.h"
#include "time.h"

/* Function prototypes. */
static VALUE allocateFireRubyException(VALUE);
static VALUE initializeFireRubyException(VALUE, VALUE);
static VALUE getFireRubyExceptionSQLCode(VALUE);
static VALUE getFireRubyExceptionDBCode(VALUE);
static VALUE getFireRubyExceptionMessage(VALUE);
VALUE decodeError(ISC_STATUS *, const char *, VALUE);

/* Globals. */
VALUE cFireRubyException;


/**
 * This function integrates with the Ruby language to allow for the allocation
 * of new FireRubyException objects.
 *
 * @param  klass  A reference to the FireRubyException Class object.
 *
 * @return  An instance of the FireRubyException class.
 *
 */
static VALUE allocateFireRubyException(VALUE klass)
{
   VALUE           instance;
   ExceptionHandle *exception = ALLOC(ExceptionHandle);
   
   if(exception != NULL)
   {
      exception->when = (long)time(NULL);
      instance = Data_Wrap_Struct(klass, NULL, firerubyExceptionFree,
                                  exception);
   }
   else
   {
      rb_raise(rb_eNoMemError,
               "Memory allocation failure creating a FireRubyException object.");
   }
   
   return(instance);
}


/**
 * This function provides the initialize method for the FireRubyException
 * class.
 *
 * @param  self     A reference to the exception object being initialized.
 * @param  message  A string containing an error message for the exception.
 *
 * @return  A reference to the initialized exception object.
 *
 */
static VALUE initializeFireRubyException(VALUE self, VALUE message)
{
   /* Check the input type. */
   if(TYPE(message) != T_STRING)
   {
      rb_raise(rb_eException,
               "Invalid message parameter specified for exception.");
   }
   
   rb_iv_set(self, "@message", message);
   rb_iv_set(self, "@sql_code", 0);
   rb_iv_set(self, "@db_code", 0);
   
   return(self);
}


/**
 * This function provides the accessor for the SQL code attribute of the
 * FireRubyException class.
 *
 * @param  self  A reference to the exception to fetch the code from.
 *
 * @return  A reference to the requested code value.
 *
 */
static VALUE getFireRubyExceptionSQLCode(VALUE self)
{
   return(rb_iv_get(self, "@sql_code"));
}


/**
 * This function provides the accessor for the database code attribute of the
 * FireRubyException class.
 *
 * @param  self  A reference to the exception to fetch the code from.
 *
 * @return  A reference to the requested code value.
 *
 */
static VALUE getFireRubyExceptionDBCode(VALUE self)
{
   return(rb_iv_get(self, "@db_code"));
}


/**
 * This function provides the message method for the FireRubyException class.
 *
 * @param  self  A reference to the exception object to fetch the message from.
 *
 * @return  A reference to the message for the exception.
 *
 */
static VALUE getFireRubyExceptionMessage(VALUE self)
{
   return(rb_iv_get(self, "@message"));
}


/**
 * This function decodes the contents of a Firebird ISC_STATUS array into a
 * String object.
 *
 * @param  status     A pointer to the status array to be decoded.
 * @param  prefix     An initial message that may be added to the exception as
 *                    part of the decoding. Ignored if it is NULL or has zero
 *                    length.
 *
 * @return  A reference to the full decode error message string.
 *
 */
VALUE decodeException(ISC_STATUS *status, const char *prefix)
{
   VALUE      message = rb_str_new2(""),
              eol     = rb_str_new2("\n");
   char       text[512];
   int        sqlCode = isc_sqlcode(status),
              dbCode  = 0;
   ISC_STATUS **ptr   = &status;

   /* Add the prefix message if it exists. */
   if(prefix != NULL && strlen(prefix) > 0)
   {
      rb_str_concat(message, rb_str_new2(prefix));
      rb_str_concat(message, eol);
   }
   
   if(status != NULL)
   {
      /* Decode the status array. */
      dbCode = status[1];
      while(isc_interprete(text, ptr) != 0)
      {
         rb_str_concat(message, rb_str_new2(text));
         rb_str_concat(message, eol);
         memset(text, 0, 512);
      }
      
      isc_sql_interprete(sqlCode, text, 512);
      if(strlen(text) > 0)
      {
         rb_str_concat(message, rb_str_new2(text));
      }
      
      sprintf(text, "\nSQL Code = %d\n", sqlCode);
      rb_str_concat(message, rb_str_new2(text));
      sprintf(text, "Firebird Code = %d\n", dbCode);
      rb_str_concat(message, rb_str_new2(text));
   }
   
   return(message);
}


/**
 * This function provides a means to programmatically create a new instance of
 * the FireRubyException class.
 *
 * @param  message  A string containing the error message for the exception.
 *
 * @return  A reference to a newly created FireRubyException object.
 *
 */
VALUE rb_fireruby_exception_new(const char *message)
{
   VALUE exception = allocateFireRubyException(cFireRubyException);
   
   initializeFireRubyException(exception, rb_str_new2(message));
   
   return(exception);
}


/**
 * This function raises a new FireRuby exception.
 *
 * @param  status   A pointer to the Firebird status vector containing the error
 *                  details.
 * @param  message  A string containing a message to be prefixed to the error
 *                  text generated by the decoding.
 *
 */
void rb_fireruby_raise(ISC_STATUS *status, const char *message)
{
   VALUE text = decodeException(status, message);
   
   rb_raise(cFireRubyException, STR2CSTR(text));
}


/**
 * This function integrates with the Ruby garbage collector to insure that the
 * resources associated with a FireRubyException object are completely released
 * when an object of this type is collected.
 *
 * @param  exception  A pointer to the ExceptionHandle structure associated
 *                    with a FireRubyException object that is being collected.
 *
 */
void firerubyExceptionFree(void *exception)
{
   if(exception != NULL)
   {
      free((ExceptionHandle *)exception);
   }
}


/**
 * This function creates the FireRubyException class within the Ruby environment
 * under the module specified as a parameter.
 *
 * @param  module  The module to create the class under.
 *
 */
void Init_FireRubyException(VALUE module)
{
   cFireRubyException = rb_define_class_under(module, "FireRubyException",
                                              rb_eStandardError);
   rb_define_alloc_func(cFireRubyException, allocateFireRubyException);
   rb_define_method(cFireRubyException, "initialize", initializeFireRubyException, 1);
   rb_define_method(cFireRubyException, "sql_code", getFireRubyExceptionSQLCode, 0);
   rb_define_method(cFireRubyException, "db_code", getFireRubyExceptionDBCode, 0);
   rb_define_method(cFireRubyException, "message", getFireRubyExceptionMessage, 0);
}
