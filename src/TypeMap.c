/*------------------------------------------------------------------------------
 * TypeMap.c
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
#include "TypeMap.h"
#include <time.h>
#include <math.h>
#include <limits.h>
#include "Blob.h"
#include "Connection.h"
#include "Transaction.h"
#include "ResultSet.h"
#include "Statement.h"
#include "FireRuby.h"

#ifdef OS_UNIX
   #include <inttypes.h>
#else
   typedef short     int16_t;
   typedef long      int32_t;
   typedef long long int64_t;
#endif

/* Function prototypes. */
VALUE createDate(const struct tm *);
VALUE createDateTime(VALUE dt);
VALUE createTime(VALUE dt);
VALUE createSafeTime(const struct tm*);
VALUE getConstant(const char *, VALUE);
VALUE toDateTime(VALUE);
VALUE rescueConvert(VALUE, VALUE);
void storeBlob(VALUE, XSQLVAR *, ConnectionHandle *, TransactionHandle *);
void populateBlobField(VALUE, XSQLVAR *, VALUE);
void populateDoubleField(VALUE, XSQLVAR *);
void populateFloatField(VALUE, XSQLVAR *);
void populateInt64Field(VALUE, XSQLVAR *);
void populateLongField(VALUE, XSQLVAR *);
void populateShortField(VALUE, XSQLVAR *);
void populateTextField(VALUE, XSQLVAR *);
void populateDateField(VALUE, XSQLVAR *);
void populateTimeField(VALUE, XSQLVAR *);
void populateTimestampField(VALUE, XSQLVAR *);


/**
 * This function converts a single XSQLVAR entry to a Ruby VALUE type.
 *
 * @param  entry        A pointer to the SQLVAR type containing the data to be
 *                      converted.
 * @param  database     A pointer to the database handle relating to the data.
 * @param  transaction  A pointer to the transaction handle relating to the
 *                      data.
 *
 * @return  A Ruby type for the XSQLVAR entry. The actual type will depend on
 *          the field type referenced.
 *
 */
VALUE toValue(XSQLVAR *entry,
              isc_db_handle *database,
              isc_tr_handle *transaction)
{
   VALUE     value  = rb_ary_new();

   /* Check for NULL values. */
   if(!((entry->sqltype & 1) && (*entry->sqlind < 0)))
   {
      int        type    = (entry->sqltype & ~1);
      char       *array  = NULL,
                 column[256],
                 table[256];
      struct tm  datetime;
      short      length;
      double     actual;
      BlobHandle *blob   = NULL;
      VALUE      setting = getFireRubySetting("DATE_AS_DATE"),
                 working = Qnil;

      switch(type)
      {
         case SQL_ARRAY : /* Type: ARRAY */
            /* TO BE DONE! */
            break;

         case SQL_BLOB:   /* Type: BLOB */
            memset(column, 0, 256);
            memset(table, 0, 256);
            memcpy(column, entry->sqlname, entry->sqlname_length);
            memcpy(table, entry->relname, entry->relname_length);
            blob = openBlob(*(ISC_QUAD *)entry->sqldata, column, table, database,
                            transaction);
            working = Data_Wrap_Struct(cBlob, NULL, blobFree, blob);
            rb_ary_push(value, initializeBlob(working));
            rb_ary_push(value, getColumnType(entry));
            break;

         case SQL_TYPE_DATE : /* Type: DATE */
            memset(&datetime, 0, sizeof(struct tm));
            isc_decode_sql_date((ISC_DATE *)entry->sqldata, &datetime);
            datetime.tm_sec  = 0;
            datetime.tm_min  = 0;
            datetime.tm_hour = 0;
            if(setting == Qtrue)
            {
               rb_ary_push(value, createDate(&datetime));
            }
            else
            {
               rb_ary_push(value, createSafeTime(&datetime));
            }
            rb_ary_push(value, getColumnType(entry));
            break;

         case SQL_DOUBLE : /* Type: DOUBLE PRECISION, DECIMAL, NUMERIC */
            rb_ary_push(value, rb_float_new(*((double *)entry->sqldata)));
            rb_ary_push(value, getColumnType(entry));
            break;

         case SQL_FLOAT : /* Type: FLOAT */
            rb_ary_push(value, rb_float_new(*((float *)entry->sqldata)));
            rb_ary_push(value, getColumnType(entry));
            break;

         case SQL_INT64 : /* Type: DECIMAL, NUMERIC */
            if(entry->sqlscale != 0)
            {
               double divisor  = pow(10, abs(entry->sqlscale));

               actual = *((long long *)entry->sqldata);
               rb_ary_push(value, rb_float_new(actual / divisor));
            }
            else
            {
               rb_ary_push(value, LL2NUM(*((long long *)entry->sqldata)));
            }
            rb_ary_push(value, getColumnType(entry));
            break;

         case SQL_LONG : /* Type: INTEGER, DECIMAL, NUMERIC */
            if(entry->sqlscale != 0)
            {
               double divisor  = pow(10, abs(entry->sqlscale));

               actual = *((int32_t *)entry->sqldata);
               rb_ary_push(value, rb_float_new(actual / divisor));
            }
            else
            {
               rb_ary_push(value, LONG2NUM(*((int32_t *)entry->sqldata)));
            }
            rb_ary_push(value, getColumnType(entry));
            break;

         case SQL_SHORT : /* Type: SMALLINT, DECIMAL, NUMERIC */
            if(entry->sqlscale != 0)
            {
               double divisor  = pow(10, abs(entry->sqlscale));

               actual = *((short *)entry->sqldata);
               rb_ary_push(value, rb_float_new(actual / divisor));
            }
            else
            {
               rb_ary_push(value, INT2NUM(*((short *)entry->sqldata)));
            }
            rb_ary_push(value, getColumnType(entry));
            break;

         case SQL_TEXT : /* Type: CHAR */
            array = ALLOC_N(char, entry->sqllen + 1);
            if(array != NULL)
            {
               memset(array, 0, entry->sqllen + 1);
               memcpy(array, entry->sqldata, entry->sqllen);
               rb_ary_push(value, rb_str_new2(array));
               rb_ary_push(value, getColumnType(entry));
               free(array);
            }
            break;

         case SQL_TYPE_TIME : /* Type: TIME */
            isc_decode_sql_time((ISC_TIME *)entry->sqldata, &datetime);
            datetime.tm_year = 70;
            datetime.tm_mon  = 0;
            datetime.tm_mday = 1;
            rb_ary_push(value, createSafeTime(&datetime));
            rb_ary_push(value, getColumnType(entry));
            break;

         case SQL_TIMESTAMP : /* Type: TIMESTAMP */
            isc_decode_timestamp((ISC_TIMESTAMP *)entry->sqldata, &datetime);
            rb_ary_push(value, createSafeTime(&datetime));
            rb_ary_push(value, getColumnType(entry));
            break;

         case SQL_VARYING :
            memcpy(&length, entry->sqldata, 2);
            if(length >= 0)
            {
               array = ALLOC_N(char, length + 1);
               if(array != NULL)
               {
                  memset(array, 0, length + 1);
                  memcpy(array, &entry->sqldata[2], length);
                  rb_ary_push(value, rb_str_new2(array));
                  rb_ary_push(value, getColumnType(entry));
                  free(array);
               }
            }
            break;

         default :
            rb_ary_push(value, Qnil);
            rb_ary_push(value, Qnil);
      } /* End of switch. */
   }
   else
   {
      rb_ary_push(value, Qnil);
      rb_ary_push(value, getColumnType(entry));   
   }

   return(value);
}


/**
 * This function attempts to convert the data contents of a XSQLDA to a Ruby
 * array of values.
 *
 * @param  results  A reference to the ResultSet object to extract the data row
 *                  from.
 *
 * @return  A reference to the array containing the row data from the XSQLDA.
 *
 */
VALUE toArray(VALUE results)
{
   VALUE             array       = rb_ary_new(),
                     transaction = rb_iv_get(results, "@transaction"),
                     connection  = rb_iv_get(results, "@connection");
   XSQLVAR           *entry      = NULL;
   ConnectionHandle  *cHandle    = NULL;
   ResultsHandle     *rHandle    = NULL;
   TransactionHandle *tHandle    = NULL;
   int               i;

   Data_Get_Struct(connection, ConnectionHandle, cHandle);
   Data_Get_Struct(results, ResultsHandle, rHandle);
   Data_Get_Struct(transaction, TransactionHandle, tHandle);
   entry = rHandle->output->sqlvar;
   for(i = 0; i < rHandle->output->sqln; i++, entry++)
   {
      VALUE value = toValue(entry, &cHandle->handle, &tHandle->handle);

      rb_ary_push(array, value);
   }

   return(array);
}


/**
 * This function takes an array of parameters and populates the parameter set
 * for a Statement object with the details. Attempts are made to convert the
 * parameter types where appropriate but, if this isn't possible then an
 * exception is generated.
 *
 * @param  parameters  A pointer to the XSQLDA area that will be used to
 *                     hold the parameter data.
 * @param  array       A reference to an array containing the parameter data to
 *                     be used.
 * @param  source      Either a Statement or ResultSet object that can be used
 *                     to get connection and transaction details.
 *
 */
void setParameters(XSQLDA *parameters, VALUE array, VALUE source)
{
   VALUE   value;
   int     index,
           size;
   XSQLVAR *parameter = NULL;

   /* Check that sufficient parameters have been provided. */
   value     = rb_funcall(array, rb_intern("size"), 0);
   size      = (TYPE(value) == T_FIXNUM ? FIX2INT(value) : NUM2INT(value));
   parameter = parameters->sqlvar;
   if(size != parameters->sqld)
   {
      rb_raise(rb_eException,
               "Parameter set mismatch. Too many or too few parameters "\
               "specified for a SQL statement.");
   }
   parameters->sqln = parameters->sqld;
   parameters->version = 1;

   /* Populate the parameters from the array's contents. */
   for(index = 0; index < size; index++, parameter++)
   {
      int type = (parameter->sqltype & ~1);

      value = rb_ary_entry(array, index);
      /* Check for nils to indicate null values. */
      if(value != Qnil)
      {
         VALUE name = rb_funcall(value, rb_intern("class"), 0);

         parameter->sqlind = 0;
         name = rb_funcall(name, rb_intern("name"), 0);
         switch(type)
         {
            case SQL_ARRAY : /* Type: ARRAY */
               /* TO BE DONE! */
               break;

            case SQL_BLOB:   /* Type: BLOB */
               populateBlobField(value, parameter, source);
               break;

            case SQL_DOUBLE : /* Type: DOUBLE PRECISION, DECIMAL, NUMERIC */
               populateDoubleField(value, parameter);
               break;

            case SQL_FLOAT : /* Type: FLOAT */
               populateFloatField(value, parameter);
               break;

            case SQL_INT64 : /* Type: DECIMAL, NUMERIC */
               populateInt64Field(value, parameter);
               break;

            case SQL_LONG : /* Type: INTEGER, DECIMAL, NUMERIC */
               populateLongField(value, parameter);
               break;

            case SQL_SHORT : /* Type: SMALLINT, DECIMAL, NUMERIC */
               populateShortField(value, parameter);
               break;

            case SQL_TEXT : /* Type: CHAR */
               populateTextField(value, parameter);
               break;

            case SQL_TYPE_DATE : /* Type: DATE */
               populateDateField(value, parameter);
               break;

            case SQL_TYPE_TIME : /* Type: TIME */
               populateTimeField(value, parameter);
               break;

            case SQL_TIMESTAMP : /* Type: TIMESTAMP */
               populateTimestampField(value, parameter);
               break;

            case SQL_VARYING : /* Type: VARCHAR */
               populateTextField(value, parameter);
               break;

            default :
               rb_raise(rb_eException,
                        "Unknown SQL type encountered in statement parameter "\
                        "set.");
         } /* End of the switch statement. */
      }
      else
      {
         /* Mark the field as a NULL value. */
         memset(parameter->sqldata, 0, parameter->sqllen);
         *parameter->sqlind = -1;
      }
   }
}


/**
 * This function converts a struct tm to a Ruby Date instance.
 *
 * @param  date  A structure containing the date details.
 *
 * @return  A Ruby Date object.
 *
 */
VALUE createDate(const struct tm *date)
{
   VALUE result = Qnil,
         klass  = Qnil;

   klass = getClass("Date");

   /* Check if we need to require date. */
   if(klass == Qnil)
   {
      rb_require("date");
      klass = getClass("Date");
   }

   /* Check that we got the Date class. */
   if(klass != Qnil)
   {
      VALUE arguments[3];

      /* Prepare the arguments. */
      arguments[0] = INT2FIX(date->tm_year + 1900);
      arguments[1] = INT2FIX(date->tm_mon + 1);
      arguments[2] = INT2FIX(date->tm_mday);

      /* Create the class instance. */
      result = rb_funcall2(klass, rb_intern("new"), 3, arguments);
   }


   return(result);
}


/**
 * This function converts a struct tm to a Ruby DateTime instance.
 *
 * @param  datetime  A structure containing the date/time details.
 *
 * @return  A Ruby DateTime object.
 *
 */
VALUE createDateTime(VALUE dt)
{
   VALUE result = Qnil,
         klass  = Qnil;

   struct tm *datetime;
   Data_Get_Struct(dt, struct tm, datetime);

   klass = getClass("DateTime");

   /* Check if we need to require date. */
   if(klass == Qnil)
   {
      rb_require("date");
      klass = getClass("DateTime");
   }

   /* Check that we got the DateTime class. */
   if(klass != Qnil)
   {
      VALUE arguments[7];

      /* Prepare the arguments. */
      arguments[0] = INT2FIX(datetime->tm_year + 1900);
      arguments[1] = INT2FIX(datetime->tm_mon + 1);
      arguments[2] = INT2FIX(datetime->tm_mday);
      arguments[3] = INT2FIX(datetime->tm_hour);
      arguments[4] = INT2FIX(datetime->tm_min);
      arguments[5] = INT2FIX(datetime->tm_sec);
      arguments[6] = rb_funcall(rb_funcall(klass, rb_intern("now"), 0), rb_intern("offset"), 0);

      /* Create the class instance. */
      result = rb_funcall2(klass, rb_intern("new"), 7, arguments);
   }

   return(result);
}


/**
 * This function converts a struct tm to a Ruby Time instance.
 *
 * @param  datetime  A structure containing the date/time details.
 *
 * @return  A Ruby Time object.
 *
 */
VALUE createTime(VALUE dt)
{
   VALUE result = Qnil,
         klass  = Qnil;

   struct tm *datetime;
   Data_Get_Struct(dt, struct tm, datetime);

   klass = getClass("Time");

   /* Check that we got the Time class. */
   if(klass != Qnil)
   {
      VALUE arguments[6];

      /* Prepare the arguments. */
      /*fprintf(stderr, "%d-%d-%d %d:%d:%d\n", datetime->tm_year + 1900,
              datetime->tm_mon + 1, datetime->tm_mday, datetime->tm_hour,
              datetime->tm_min, datetime->tm_sec);*/
      arguments[0] = INT2FIX(datetime->tm_year + 1900);
      arguments[1] = INT2FIX(datetime->tm_mon + 1);
      arguments[2] = INT2FIX(datetime->tm_mday);
      arguments[3] = INT2FIX(datetime->tm_hour);
      arguments[4] = INT2FIX(datetime->tm_min);
      arguments[5] = INT2FIX(datetime->tm_sec);

      /* Create the class instance. */
      result = rb_funcall2(klass, rb_intern("local"), 6, arguments);
   }

   return(result);
}

/**
 * This function converts a struct tm to a Ruby Time instance. 
 * If the conversion process results in an out of range error then
 * it will convert the struct to a DateTime instance.
 *
 * @param datetime  A structure containing the date/time details.
 *
 * @return  A Ruby Time object if the arguments are in range, otherwise 
 *          a Ruby DateTime object.
 *
 */
VALUE createSafeTime(const struct tm *datetime)
{
   VALUE dt = Data_Wrap_Struct(rb_cObject, NULL, NULL, datetime);
   return rb_rescue(createTime, dt, createDateTime, dt);
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
VALUE getConstant(const char *name, VALUE module)
{
   VALUE owner = module,
         constants,
         string,
         exists,
         entry;

   /* Check that we've got somewhere to look. */
   if(owner == Qnil)
   {
      owner = rb_cModule;
   }
   constants = rb_funcall(owner, rb_intern("constants"), 0),
   string    = rb_str_new2(name),
   exists    = rb_funcall(constants, rb_intern("include?"), 1, string);

   if(exists != Qfalse)
   {
      ID    id     = rb_intern(name);
      VALUE symbol = ID2SYM(id);

      entry = rb_funcall(owner, rb_intern("const_get"), 1, symbol);
   }

   return(entry);
}


/**
 * This method fetches a Ruby module definition object based on a class name.
 * The method is assumed to have been defined at the top level.
 *
 * @return  A Ruby VALUE representing the Module requested, or nil if the
 *          module could not be located.
 *
 */
VALUE getModule(const char *name)
{
   VALUE module = getConstant(name, Qnil);

   if(module != Qnil)
   {
      VALUE type = rb_funcall(module, rb_intern("class"), 0);

      if(type != rb_cModule)
      {
         module = Qnil;
      }
   }

   return(module);
}


/**
 * This method fetches a Ruby class definition object based on a class name.
 * The class is assumed to have been defined at the top level.
 *
 * @return  A Ruby VALUE representing the requested class, or nil if the class
 *          could not be found.
 *
 */
VALUE getClass(const char *name)
{
   VALUE klass = getConstant(name, Qnil);

   if(klass != Qnil)
   {
      VALUE type = rb_funcall(klass, rb_intern("class"), 0);

      if(type != rb_cClass)
      {
         klass = Qnil;
      }
   }

   return(klass);
}


/**
 * This method fetches a module from a specified module.
 *
 * @param  name   The name of the class to fetch.
 * @param  owner  The module to search for the module in.
 *
 * @return  A Ruby VALUE representing the requested module, or nil if it could
 *          not be located.
 *
 */
VALUE getModuleInModule(const char *name, VALUE owner)
{
   VALUE module = getConstant(name, owner);

   if(module != Qnil)
   {
      VALUE type = rb_funcall(module, rb_intern("class"), 0);

      if(type != rb_cModule)
      {
         module = Qnil;
      }
   }

   return(module);
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
VALUE getClassInModule(const char *name, VALUE owner)
{
   VALUE klass = getConstant(name, owner);

   if(klass != Qnil)
   {
      VALUE type = rb_funcall(klass, rb_intern("class"), 0);

      if(type != rb_cClass)
      {
         klass = Qnil;
      }
   }

   return(klass);
}


/**
 * This function attempts to convert a VALUE to a series of date/time values.
 *
 * @param  value  A reference to the value to be converted.
 *
 * @return  A VALUE that represents an array containing the date/time details
 *          in the order year, month, day of month, hours, minutes and seconds.
 *
 */
VALUE toDateTime(VALUE value)
{
   VALUE result,
         klass = rb_funcall(value, rb_intern("class"), 0);

   if(klass == rb_cTime)
   {
      VALUE data;

      result = rb_ary_new();

      data = rb_funcall(value, rb_intern("year"), 0);
      rb_ary_push(result, INT2FIX(FIX2INT(data) - 1900));
      data = rb_funcall(value, rb_intern("month"), 0);
      rb_ary_push(result, INT2FIX(FIX2INT(data) - 1));
      rb_ary_push(result, rb_funcall(value, rb_intern("day"), 0));
      rb_ary_push(result, rb_funcall(value, rb_intern("hour"), 0));
      rb_ary_push(result, rb_funcall(value, rb_intern("min"), 0));
      rb_ary_push(result, rb_funcall(value, rb_intern("sec"), 0));
   }
   else if(klass == getClass("Date"))
   {
      VALUE data;

      result = rb_ary_new();

      data = rb_funcall(value, rb_intern("year"), 0);
      rb_ary_push(result, INT2FIX(FIX2INT(data) - 1900));
      data = rb_funcall(value, rb_intern("month"), 0);
      rb_ary_push(result, INT2FIX(FIX2INT(data) - 1));
      rb_ary_push(result, rb_funcall(value, rb_intern("mday"), 0));
      rb_ary_push(result, INT2FIX(0));
      rb_ary_push(result, INT2FIX(0));
      rb_ary_push(result, INT2FIX(0));
   }
   else
   {
      rb_raise(rb_eException, "Value conversion error.");
   }

   return(result);
}


/**
 * This function represents the rescue block for data conversions.
 *
 * @param  arguments  An array of VALUEs. The first is expected to contain the
 *                    field offset of the value being converted. The second a
 *                    string containing the name of the data type being
 *                    converted from and the third a string containing the name
 *                    of the data type being converted to.
 * @param  error      Will be populates with error details of the exception
 *                    raised.
 *
 */
VALUE rescueConvert(VALUE arguments, VALUE error)
{
   VALUE message;
   char  text[512];

   sprintf(text, "Error converting input column %d from a %s to a %s.",
           FIX2INT(rb_ary_entry(arguments, 0)),
           STR2CSTR(rb_ary_entry(arguments, 1)),
           STR2CSTR(rb_ary_entry(arguments, 2)));
   message = rb_str_new2(text);

   return(rb_funcall(rb_eException, rb_intern("exception"), 1, &message));
}


/**
 * This function creates a new blob and returns the identifier for it.
 *
 * @param  info         A pointer to a string containing the blob data.
 * @param  field        The field that the blob identifier needs to be inserted
 *                      into.
 * @param  database     A pointer to the database handle to be used in creating
 *                      the blob.
 * @param  transaction  A pointer to the transaction handle to be used in
 *                      creating the blob.
 *
 * @return  An ISC_QUAD value containing the identifier for the blob created.
 *
 */
void storeBlob(VALUE info,
               XSQLVAR *field,
               ConnectionHandle *connection,
               TransactionHandle *transaction)
{
   ISC_STATUS      status[20];
   isc_blob_handle handle  = 0;
   ISC_QUAD        *blobId = (ISC_QUAD *)field->sqldata;
   VALUE           number  = rb_funcall(info, rb_intern("length"), 0);
   long            length  = 0;
   char            *data   = STR2CSTR(info);

   length = TYPE(number) == T_FIXNUM ? FIX2INT(number) : NUM2INT(number);
   field->sqltype = SQL_BLOB;
   if(isc_create_blob(status, &connection->handle, &transaction->handle,
                      &handle, blobId) == 0)
   {
      long           offset = 0;
      unsigned short size   = 0;

      while(offset < length)
      {
         char *buffer = &data[offset];

         size = (length - offset) > USHRT_MAX ? USHRT_MAX : length - offset;
         if(isc_put_segment(status, &handle, size, buffer) != 0)
         {
            ISC_STATUS other[20];

            isc_close_blob(other, &handle);
            rb_fireruby_raise(status, "Error writing blob data.");
         }

         offset = offset + size;
      }

      if(isc_close_blob(status, &handle) != 0)
      {
         rb_fireruby_raise(status, "Error closing blob.");
      }
   }
   else
   {
      rb_fireruby_raise(status, "Error storing blob data.");
   }
}


/**
 * This function populates a blob output parameter.
 *
 * @param  value   The value to be insert into the blob.
 * @param  field   A pointer to the output field to be populated.
 * @param  source  A reference to either a Statement or ResultSet object that
 *                 contains the connection and transaction details.
 *
 */
void populateBlobField(VALUE value, XSQLVAR *field, VALUE source)
{
   VALUE             attribute;
   ConnectionHandle  *connection = NULL;
   TransactionHandle *transaction = NULL;

   if(TYPE(value) != T_STRING)
   {
      rb_fireruby_raise(NULL, "Error converting input parameter to blob.");
   }

   /* Fetch the connection and transaction details. */
   attribute = rb_iv_get(source, "@connection");
   Data_Get_Struct(attribute, ConnectionHandle, connection);
   attribute = rb_iv_get(source, "@transaction");
   Data_Get_Struct(attribute, TransactionHandle, transaction);
   storeBlob(value, field, connection, transaction);
   field->sqltype = SQL_BLOB;
}


/**
 * This method populates a date output field.
 *
 * @param  value  A reference to the value to be used to populate the field.
 * @param  field  A pointer to the output field to be populated.
 *
 */
void populateDateField(VALUE value, XSQLVAR *field)
{
   struct tm datetime;
   VALUE     arguments = rb_ary_new();

   rb_ary_push(arguments, rb_str_new2("date"));
   value = rb_rescue(toDateTime, value, rescueConvert, arguments);
   if(TYPE(value) != T_ARRAY)
   {
      VALUE message = rb_funcall(value, rb_intern("message"), 0);
      rb_fireruby_raise(NULL, STR2CSTR(message));
   }
   datetime.tm_year = FIX2INT(rb_ary_entry(value, 0));
   datetime.tm_mon  = FIX2INT(rb_ary_entry(value, 1));
   datetime.tm_mday = FIX2INT(rb_ary_entry(value, 2));
   isc_encode_sql_date(&datetime, (ISC_DATE *)field->sqldata);
   field->sqltype   = SQL_TYPE_DATE;
}


/**
 * This method populates a double output field.
 *
 * @param  value  A reference to the value to be used to populate the field.
 * @param  field  A pointer to the output field to be populated.
 *
 */
void populateDoubleField(VALUE value, XSQLVAR *field)
{
   double store;
   VALUE  actual;

   if(TYPE(value) != T_FLOAT)
   {
      if(rb_obj_is_kind_of(value, rb_cNumeric) || TYPE(value) == T_STRING)
      {
         actual = rb_funcall(value, rb_intern("to_f"), 0);
      }
      else
      {
         rb_fireruby_raise(NULL,
                           "Error converting input parameter to double.");
      }
   }

   store = NUM2DBL(value);
   memcpy(field->sqldata, &store, field->sqllen);
   field->sqltype = SQL_DOUBLE;
}


/**
 * This method populates a double output field.
 *
 * @param  value  A reference to the value to be used to populate the field.
 * @param  field  A pointer to the output field to be populated.
 *
 */
void populateFloatField(VALUE value, XSQLVAR *field)
{
   double full  = 0.0;
   float  store = 0.0;
   VALUE  actual;

   if(TYPE(value) != T_FLOAT)
   {
      if(rb_obj_is_kind_of(value, rb_cNumeric) || TYPE(value) == T_STRING)
      {
         actual = rb_funcall(value, rb_intern("to_f"), 0);
      }
      else
      {
         rb_fireruby_raise(NULL,
                           "Error converting input parameter to double.");
      }
   }

   full = NUM2DBL(value);
   store = (float)full;
   memcpy(field->sqldata, &store, field->sqllen);
   field->sqltype = SQL_FLOAT;
}


/**
 * This function populates a output parameter field with the data of a Ruby
 * value.
 *
 * @param  value  A reference to the Ruby value to be inserted into the field.
 * @param  field  A pointer to the XSQLVAR field that the value will go into.
 *
 */
void populateInt64Field(VALUE value, XSQLVAR *field)
{
   VALUE     actual = Qnil;
   long long store  = 0;

   if(rb_obj_is_kind_of(value, rb_cInteger))
   {
      actual = value;
   }
   else if(TYPE(value) == T_FLOAT)
   {
      double number = NUM2DBL(value);

      if(field->sqlscale != 0)
      {
         number = number * pow(10, abs(field->sqlscale));
         actual = INT2NUM((long)number);
      }
   }
   else if(TYPE(value) == T_STRING)
   {
      actual = rb_funcall(value, rb_intern("to_i"), 0);
   }
   else
   {
      rb_fireruby_raise(NULL,
                        "Error converting input parameter to 64 bit integer.");
   }

   store = TYPE(actual) == T_FIXNUM ? FIX2INT(actual) : NUM2INT(actual);
   memcpy(field->sqldata, &store, field->sqllen);
   field->sqltype = SQL_INT64;
}


/**
 * This function populates a output parameter field with the data of a Ruby
 * value.
 *
 * @param  value  A reference to the Ruby value to be inserted into the field.
 * @param  field  A pointer to the XSQLVAR field that the value will go into.
 *
 */
void populateLongField(VALUE value, XSQLVAR *field)
{
   VALUE     actual = Qnil;
   long long full   = 0;
   long      store  = 0;

   if(rb_obj_is_kind_of(value, rb_cInteger))
   {
      actual = value;
   }
   else if(TYPE(value) == T_FLOAT)
   {
      double number = NUM2DBL(value);

      if(field->sqlscale != 0)
      {
         number = number * pow(10, abs(field->sqlscale));
         actual = INT2NUM((long)number);
      }
   }
   else if(TYPE(value) == T_STRING)
   {
      actual = rb_funcall(value, rb_intern("to_i"), 0);
   }
   else
   {
      rb_fireruby_raise(NULL,
                        "Error converting input parameter to long integer.");
   }

   full  = TYPE(actual) == T_FIXNUM ? FIX2INT(actual) : NUM2INT(actual);
   store = (long)full;
   memcpy(field->sqldata, &store, field->sqllen);
   field->sqltype = SQL_LONG;
}


/**
 * This function populates a output parameter field with the data of a Ruby
 * value.
 *
 * @param  value  A reference to the Ruby value to be inserted into the field.
 * @param  field  A pointer to the XSQLVAR field that the value will go into.
 *
 */
void populateShortField(VALUE value, XSQLVAR *field)
{
   VALUE     actual = Qnil;
   long long full   = 0;
   short     store  = 0;

   if(rb_obj_is_kind_of(value, rb_cInteger))
   {
      actual = value;
   }
   else if(TYPE(value) == T_FLOAT)
   {
      double number = NUM2DBL(value);

      if(field->sqlscale != 0)
      {
         number = number * pow(10, abs(field->sqlscale));
         actual = INT2NUM((long)number);
      }
   }
   else if(TYPE(value) == T_STRING)
   {
      actual = rb_funcall(value, rb_intern("to_i"), 0);
   }
   else
   {
      rb_fireruby_raise(NULL,
                        "Error converting input parameter to short integer.");
   }

   full  = TYPE(actual) == T_FIXNUM ? FIX2INT(actual) : NUM2INT(actual);
   store = (short)full;
   memcpy(field->sqldata, &store, field->sqllen);
   field->sqltype = SQL_SHORT;
}


/**
 * This function populates a output parameter field with the data of a Ruby
 * value.
 *
 * @param  value  A reference to the Ruby value to be inserted into the field.
 * @param  field  A pointer to the XSQLVAR field that the value will go into.
 *
 */
void populateTextField(VALUE value, XSQLVAR *field)
{
   VALUE actual;
   char  *text  = NULL;
   short length = 0;

   if(TYPE(value) != T_STRING)
   {
      actual = value;
   }
   else
   {
      actual = rb_funcall(value, rb_intern("to_s"), 0);
   }

   text   = STR2CSTR(actual);
   length = strlen(text) > field->sqllen ? field->sqllen : strlen(text);

   if((field->sqltype & ~1) == SQL_TEXT)
   {
      memcpy(field->sqldata, text, length);
      field->sqltype = SQL_TEXT;
   }
   else
   {
      memcpy(field->sqldata, &length, sizeof(short));
      memcpy(&field->sqldata[sizeof(short)], text, length);
      field->sqltype = SQL_VARYING;
   }
	 field->sqllen = length;
}


/**
 * This method populates a date output field.
 *
 * @param  value  A reference to the value to be used to populate the field.
 * @param  field  A pointer to the output field to be populated.
 *
 */
void populateTimeField(VALUE value, XSQLVAR *field)
{
   struct tm datetime;
   VALUE     arguments = rb_ary_new();

   rb_ary_push(arguments, rb_str_new2("time"));
   value = rb_rescue(toDateTime, value, rescueConvert, arguments);
   if(TYPE(value) != T_ARRAY)
   {
      VALUE message = rb_funcall(value, rb_intern("message"), 0);
      rb_fireruby_raise(NULL, STR2CSTR(message));
   }
   datetime.tm_hour = FIX2INT(rb_ary_entry(value, 3));
   datetime.tm_min  = FIX2INT(rb_ary_entry(value, 4));
   datetime.tm_sec  = FIX2INT(rb_ary_entry(value, 5));
   isc_encode_sql_time(&datetime, (ISC_TIME *)field->sqldata);
   field->sqltype   = SQL_TYPE_TIME;
}


/**
 * This method populates a date output field.
 *
 * @param  value  A reference to the value to be used to populate the field.
 * @param  field  A pointer to the output field to be populated.
 *
 */
void populateTimestampField(VALUE value, XSQLVAR *field)
{
   struct tm datetime;
   VALUE     arguments = rb_ary_new();

   rb_ary_push(arguments, rb_str_new2("timestamp"));
   value = rb_rescue(toDateTime, value, rescueConvert, arguments);
   if(TYPE(value) != T_ARRAY)
   {
      VALUE message = rb_funcall(value, rb_intern("message"), 0);
      rb_fireruby_raise(NULL, STR2CSTR(message));
   }
   datetime.tm_year = FIX2INT(rb_ary_entry(value, 0));
   datetime.tm_mon  = FIX2INT(rb_ary_entry(value, 1));
   datetime.tm_mday = FIX2INT(rb_ary_entry(value, 2));
   datetime.tm_hour = FIX2INT(rb_ary_entry(value, 3));
   datetime.tm_min  = FIX2INT(rb_ary_entry(value, 4));
   datetime.tm_sec  = FIX2INT(rb_ary_entry(value, 5));
   isc_encode_timestamp(&datetime, (ISC_TIMESTAMP *)field->sqldata);
   field->sqltype   = SQL_TIMESTAMP;
}
