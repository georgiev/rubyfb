/*------------------------------------------------------------------------------
 * Backup.c
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
#include "Backup.h"
#include "ibase.h"
#include "ServiceManager.h"
#include "Services.h"

/* Function prototypes. */
static VALUE getBackupFile(VALUE);
static VALUE setBackupFile(VALUE, VALUE);
static VALUE getBackupDatabase(VALUE);
static VALUE setBackupDatabase(VALUE, VALUE);
static VALUE getBackupBlockingFactor(VALUE);
static VALUE setBackupBlockingFactor(VALUE, VALUE);
static VALUE getBackupIgnoreChecksums(VALUE);
static VALUE setBackupIgnoreChecksums(VALUE, VALUE);
static VALUE getBackupIgnoreLimbo(VALUE);
static VALUE setBackupIgnoreLimbo(VALUE, VALUE);
static VALUE getBackupMetadataOnly(VALUE);
static VALUE setBackupMetadataOnly(VALUE, VALUE);
static VALUE getBackupGarbageCollect(VALUE);
static VALUE setBackupGarbageCollect(VALUE, VALUE);
static VALUE getBackupNonTransportable(VALUE);
static VALUE setBackupNonTransportable(VALUE, VALUE);
static VALUE getBackupConvertTables(VALUE);
static VALUE setBackupConvertTables(VALUE, VALUE);
static VALUE executeBackup(VALUE, VALUE);
static VALUE getBackupLog(VALUE);
static void createBackupBuffer(VALUE, VALUE, VALUE, char **, short *);


/* Globals. */
VALUE cBackup;

/* Definitions. */
#define IGNORE_CHECKSUMS      rb_str_new2("IGNORE_CHECKSUMS")
#define IGNORE_LIMBO          rb_str_new2("IGNORE_LIMBO")
#define METADATA_ONLY         rb_str_new2("METADATA_ONLY")
#define NO_GARBAGE_COLLECT    rb_str_new2("NO_GARBAGE_COLLECT")
#define NON_TRANSPORTABLE     rb_str_new2("NON_TRANSPORTABLE")
#define CONVERT_TABLES        rb_str_new2("CONVERT_TABLES")
#define VERBOSE_BACKUP        INT2FIX(isc_spb_verbose)
#define START_BUFFER_SIZE     1024


/**
 * This function provides the initialize method for the Backup class.
 *
 * @param  self      A reference to the Backup object to be initialized.
 * @param  database  A reference to a File or String containing the server path
 *                   and name of the primary database file.
 * @param  file      A reference to a File or String containing the server path
 *                   and name of the database backup file.
 *
 * @return  A reference to the newly initialized Backup object.
 *
 */
VALUE initializeBackup(VALUE self, VALUE database, VALUE file)
{
   VALUE path  = Qnil,
         files = rb_hash_new();

   /* Extract the database file details. */
   if(TYPE(database) == T_FILE)
   {
      path = rb_funcall(database, rb_intern("path"), 0);
   }
   else
   {
      path = rb_funcall(database, rb_intern("to_s"), 0);
   }

   /* Get the back file details. */
   if(TYPE(file) == T_FILE)
   {
      rb_hash_aset(files, rb_funcall(file, rb_intern("path"), 0), Qnil);
   }
   else
   {
      rb_hash_aset(files, rb_funcall(file, rb_intern("to_s"), 0), Qnil);
   }

   rb_iv_set(self, "@database", path);
   rb_iv_set(self, "@files", files);
   rb_iv_set(self, "@options", rb_hash_new());
   rb_iv_set(self, "@log", Qnil);

   return(self);
}


/**
 * This function provides the backup_file attribute accessor for the Backup
 * class.
 *
 * @param  self  A reference to the Backup object to make the call on.
 *
 * @return  A reference to a String containing the backup file path/name.
 *
 */
VALUE getBackupFile(VALUE self)
{
   VALUE files = rb_iv_get(self, "@files");

   return(rb_ary_entry(rb_funcall(files, rb_intern("keys"), 0), 0));
}


/**
 * This function provides the backup_file attribute mutator for the Backup
 * class.
 *
 * @param  self  A reference to the Backup object to make the call on.
 * @param  file  A reference to a File or String containing the path and name
 *               of the backup file.
 *
 */
VALUE setBackupFile(VALUE self, VALUE file)
{
   VALUE files = rb_hash_new();

   if(TYPE(file) == T_FILE)
   {
      rb_hash_aset(files, rb_funcall(file, rb_intern("path"), 0), Qnil);
   }
   else
   {
      rb_hash_aset(files, rb_funcall(file, rb_intern("to_s"), 0), Qnil);
   }
   rb_iv_set(self, "@files", files);

   return(self);
}


/**
 * This function provides the database attribute accessor for the Backup class.
 *
 * @param  self  A reference to the Backup object to make the call on.
 *
 * @return  A reference to a String containing the path and name of the main
 *          database file to be backed up.
 *
 */
VALUE getBackupDatabase(VALUE self)
{
   return(rb_iv_get(self, "@database"));
}


/**
 * This function provides the database attribute mutator for the Backup class.
 *
 * @param  self     A reference to the Backup object to make the call on.
 * @param  setting  A reference to a File or String containing the new path and
 *                  name details of the main database file to be backed up.
 *
 * @return  A reference to the Backup object that has been updated.
 *
 */
VALUE setBackupDatabase(VALUE self, VALUE setting)
{
   if(TYPE(setting) == T_FILE)
   {
      rb_iv_set(self, "@database", rb_funcall(setting, rb_intern("path"), 0));
   }
   else
   {
      rb_iv_set(self, "@database", rb_funcall(setting, rb_intern("to_s"), 0));
   }

   return(self);
}


/**
 * This function provides the blocking_factor attribute accessor for the Backup
 * class.
 *
 * @param  self  A reference to the Backup object to make the call on.
 *
 * @return  A reference to the current backup blocking factor setting for the
 *          Backup object.
 *
 */
VALUE getBackupBlockingFactor(VALUE self)
{
   VALUE options = rb_iv_get(self, "@options");

   return(rb_hash_aref(options, INT2FIX(isc_spb_bkp_factor)));
}


/**
 * This function provides the blocking_factor attribute mutator for the Backup
 * class.
 *
 * @param  self     A reference to the Backup object to make the call on.
 * @param  setting  A reference to an integer containing the new blocking factor
 *                  setting (in bytes).
 *
 * @return  A reference to the newly updated Backup object.
 *
 */
VALUE setBackupBlockingFactor(VALUE self, VALUE setting)
{
   VALUE options = rb_iv_get(self, "@options");

   if(rb_obj_is_kind_of(setting, rb_cInteger) == Qfalse)
   {
      rb_fireruby_raise(NULL, "Invalid blocking factor specified for Backup.");
   }
   rb_hash_aset(options, INT2FIX(isc_spb_bkp_factor), setting);

   return(self);
}


/**
 * This function provides the ignore_checksums attribute accessor for the Backup
 * class.
 *
 * @param  self  A reference to the Backup object to make the call on.
 *
 * @return  Either true or false.
 *
 */
VALUE getBackupIgnoreChecksums(VALUE self)
{
   VALUE result  = Qfalse,
         options = rb_iv_get(self, "@options"),
         value   = rb_hash_aref(options, IGNORE_CHECKSUMS);

   if(value != Qnil)
   {
      result = value;
   }

   return(result);
}


/**
 * This function provides the ignore_checksums attribute mutator for the Backup
 * class.
 *
 * @param  self     A reference to the Backup object to make the call on.
 * @param  setting  Either true or false. All other settings are ignored.
 *
 * @return  A reference to the newly updated Backup object.
 *
 */
VALUE setBackupIgnoreChecksums(VALUE self, VALUE setting)
{
   VALUE options = rb_iv_get(self, "@options");

   if(setting == Qtrue || setting == Qfalse)
   {
      rb_hash_aset(options, IGNORE_CHECKSUMS, setting);
   }

   return(self);
}


/**
 * This function provides the ignore_limbo attribute accessor for the Backup
 * class.
 *
 * @param  self  A reference to the Backup object to make the call on.
 *
 * @return  Either true or false.
 *
 */
VALUE getBackupIgnoreLimbo(VALUE self)
{
   VALUE result  = Qfalse,
         options = rb_iv_get(self, "@options"),
         value   = rb_hash_aref(options, IGNORE_LIMBO);

   if(value != Qnil)
   {
      result = value;
   }

   return(result);
}


/**
 * This function provides the ignore_limbo attribute mutator for the Backup
 * class.
 *
 * @param  self     A reference to the Backup object to make the call on.
 * @param  setting  Either true or false. All other settings are ignored.
 *
 * @return  A reference to the newly updated Backup object.
 *
 */
VALUE setBackupIgnoreLimbo(VALUE self, VALUE setting)
{
   VALUE options = rb_iv_get(self, "@options");

   if(setting == Qtrue || setting == Qfalse)
   {
      rb_hash_aset(options, IGNORE_LIMBO, setting);
   }

   return(self);
}


/**
 * This function provides the metadata_only attribute accessor for the Backup
 * class.
 *
 * @param  self  A reference to the Backup object to make the call on.
 *
 * @return  Either true or false.
 *
 */
VALUE getBackupMetadataOnly(VALUE self)
{
   VALUE result  = Qfalse,
         options = rb_iv_get(self, "@options"),
         value   = rb_hash_aref(options, METADATA_ONLY);

   if(value != Qnil)
   {
      result = value;
   }

   return(result);
}


/**
 * This function provides the metadata_only attribute mutator for the Backup
 * class.
 *
 * @param  self     A reference to the Backup object to make the call on.
 * @param  setting  Either true or false. All other settings are ignored.
 *
 * @return  A reference to the newly updated Backup object.
 *
 */
VALUE setBackupMetadataOnly(VALUE self, VALUE setting)
{
   VALUE options = rb_iv_get(self, "@options");

   if(setting == Qtrue || setting == Qfalse)
   {
      rb_hash_aset(options, METADATA_ONLY, setting);
   }

   return(self);
}


/**
 * This function provides the garbage_collection attribute accessor for the
 * Backup class.
 *
 * @param  self  A reference to the Backup object to make the call on.
 *
 * @return  Either true or false.
 *
 */
VALUE getBackupGarbageCollect(VALUE self)
{
   VALUE result  = Qtrue,
         options = rb_iv_get(self, "@options"),
         value   = rb_hash_aref(options, NO_GARBAGE_COLLECT);

   if(value != Qnil)
   {
      result = value;
   }

   return(result);
}


/**
 * This function provides the garbage_collection attribute mutator for the
 * Backup class.
 *
 * @param  self     A reference to the Backup object to make the call on.
 * @param  setting  Either true or false. All other settings are ignored.
 *
 * @return  A reference to the newly updated Backup object.
 *
 */
VALUE setBackupGarbageCollect(VALUE self, VALUE setting)
{
   VALUE options = rb_iv_get(self, "@options");

   if(setting == Qtrue || setting == Qfalse)
   {
      rb_hash_aset(options, NO_GARBAGE_COLLECT, setting);
   }

   return(self);
}


/**
 * This function provides the non_transportable attribute accessor for the
 * Backup class.
 *
 * @param  self  A reference to the Backup object to make the call on.
 *
 * @return  Either true or false.
 *
 */
VALUE getBackupNonTransportable(VALUE self)
{
   VALUE result  = Qfalse,
         options = rb_iv_get(self, "@options"),
         value   = rb_hash_aref(options, NON_TRANSPORTABLE);

   if(value != Qnil)
   {
      result = value;
   }

   return(result);
}


/**
 * This function provides the non_transportable attribute mutator for the Backup
 * class.
 *
 * @param  self     A reference to the Backup object to make the call on.
 * @param  setting  Either true or false. All other settings are ignored.
 *
 * @return  A reference to the newly updated Backup object.
 *
 */
VALUE setBackupNonTransportable(VALUE self, VALUE setting)
{
   VALUE options = rb_iv_get(self, "@options");

   if(setting == Qtrue || setting == Qfalse)
   {
      rb_hash_aset(options, NON_TRANSPORTABLE, setting);
   }

   return(self);
}


/**
 * This function provides the convert_tables attribute accessor for the Backup
 * class.
 *
 * @param  self  A reference to the Backup object to make the call on.
 *
 * @return  Either true or false.
 *
 */
VALUE getBackupConvertTables(VALUE self)
{
   VALUE result  = Qfalse,
         options = rb_iv_get(self, "@options"),
         value   = rb_hash_aref(options, CONVERT_TABLES);

   if(value != Qnil)
   {
      result = value;
   }

   return(result);
}


/**
 * This function provides the convert_tables attribute mutator for the Backup
 * class.
 *
 * @param  self     A reference to the Backup object to make the call on.
 * @param  setting  Either true or false. All other settings are ignored.
 *
 * @return  A reference to the newly updated Backup object.
 *
 */
VALUE setBackupConvertTables(VALUE self, VALUE setting)
{
   VALUE options = rb_iv_get(self, "@options");

   if(setting == Qtrue || setting == Qfalse)
   {
      rb_hash_aset(options, CONVERT_TABLES, setting);
   }

   return(self);
}


/**
 * This function provides the execute method for the Backup class.
 *
 * @param  self     A reference to the Backup object to be executed.
 * @param  manager  A reference to the ServiceManager object that will be used
 *                  to execute the backup.
 *
 * @return  A reference to the Backup object executed.
 *
 */
VALUE executeBackup(VALUE self, VALUE manager)
{
   ManagerHandle *handle   = NULL;
   short         length    = 0;
   char          *buffer   = NULL;
   ISC_STATUS    status[20];

   /* Check that the service manager is connected. */
   Data_Get_Struct(manager, ManagerHandle, handle);
   if(handle->handle == 0)
   {
      rb_fireruby_raise(NULL,
                        "Database backup error. Service manager not connected.");
   }

   createBackupBuffer(rb_iv_get(self, "@database"), rb_iv_get(self, "@files"),
                      rb_iv_get(self, "@options"), &buffer, &length);

   /* Start the service request. */
   if(isc_service_start(status, &handle->handle, NULL, length, buffer))
   {
      free(buffer);
      rb_fireruby_raise(status, "Error performing database backup.");
   }
   free(buffer);

   /* Query the service until it has completed. */
   rb_iv_set(self, "@log", queryService(&handle->handle));

   return(self);
}


/**
 * This function provides the log attribute accessor for the Backup class.
 *
 * @param  self  A reference to the Backup object to make the call on.
 *
 * @return  A reference to the current log attribute value.
 *
 */
VALUE getBackupLog(VALUE self)
{
   return(rb_iv_get(self, "@log"));
}


/**
 * This function creates a service parameter buffer for backup service
 * requests.
 *
 * @param  from     A reference to a String containing the path and name of the
 *                  database file to be backed up.
 * @param  to       Either a reference to a String containing the path and name
 *                  of the backup file or a hash matching file path/names to
 *                  their permitted sizes.
 * @param  options  A reference to a Hash containing the parameter options to
 *                  be used.
 * @param  buffer   A pointer that will be set to the generated parameter
 *                  buffer.
 * @param  length   A pointer to a short integer that will be assigned the
 *                  length of buffer.
 *
 */
void createBackupBuffer(VALUE from, VALUE to, VALUE options, char **buffer,
                        short *length)
{
   ID    id        = rb_intern("key?");
   VALUE names     = rb_funcall(to, rb_intern("keys"), 0),
         sizes     = rb_funcall(to, rb_intern("values"), 0),
         count     = rb_funcall(names, rb_intern("size"), 0),
         tmp_str   = Qnil;
   char  *position = NULL;
   short number,
         flags     = 0,
         extras    = 0,
         blocking  = 0,
         size      = TYPE(count) == T_FIXNUM ? FIX2INT(count) : NUM2INT(count),
         i;

   /* Check if extra options have been provided. */
   if(TYPE(options) != T_NIL)
   {
      VALUE temp = rb_funcall(options, rb_intern("size"), 0);

      extras = TYPE(temp) == T_FIXNUM ? FIX2INT(temp) : NUM2INT(temp);
   }

   /* Calculate the length needed for the buffer. */
   *length = 2;
   *length += strlen(StringValuePtr(from)) + 3;

   /* Count file name and length sizes. */
   for(i = 0; i < size; i++)
   {
      tmp_str = rb_ary_entry(names, i);
      *length += strlen(StringValuePtr(tmp_str)) + 3;
      if(i != size - 1)
      {
         VALUE num = rb_funcall(rb_ary_entry(sizes, i), rb_intern("to_s"), 0);

         *length += strlen(StringValuePtr(num)) + 3;
      }
   }

   if(extras)
   {
      if(rb_funcall(options, id, 1, INT2FIX(isc_spb_bkp_factor)) == Qtrue)
      {
         *length += 5;
         blocking = 1;
      }

      if(rb_funcall(options, id, 1, IGNORE_CHECKSUMS) == Qtrue ||
         rb_funcall(options, id, 1, IGNORE_LIMBO) == Qtrue ||
         rb_funcall(options, id, 1, METADATA_ONLY) == Qtrue ||
         rb_funcall(options, id, 1, NO_GARBAGE_COLLECT) == Qtrue ||
         rb_funcall(options, id, 1, NON_TRANSPORTABLE) == Qtrue ||
         rb_funcall(options, id, 1, CONVERT_TABLES) == Qtrue)
      {
         *length += 5;
         flags   = 1;
      }
   }

   /* Allocate the buffer. */
   *buffer = position = ALLOC_N(char, *length);
   if(*buffer == NULL)
   {
      rb_raise(rb_eNoMemError,
               "Memory allocation error preparing database back up.");
   }
   memset(*buffer, 0, *length);

   /* Populate the buffer. */
   *position++ = isc_action_svc_backup;

   *position++ = isc_spb_dbname;
   number      = strlen(StringValuePtr(from));
   ADD_SPB_LENGTH(position, number);
   memcpy(position, StringValuePtr(from), number);
   position += number;

   for(i = 0; i < size; i++)
   {
      VALUE name = rb_ary_entry(names, i);

      *position++ = isc_spb_bkp_file;
      number      = strlen(StringValuePtr(name));
      ADD_SPB_LENGTH(position, number);
      memcpy(position, StringValuePtr(name), number);
      position += number;

      if(i != size - 1)
      {
         VALUE max = rb_ary_entry(sizes, i);

         max         = rb_funcall(max, rb_intern("to_s"), 0);
         *position++ = isc_spb_bkp_length;
         number      = strlen(StringValuePtr(max));
         ADD_SPB_LENGTH(position, number);
         memcpy(position, StringValuePtr(max), number);
         position += number;
      }
   }

   if(extras && blocking)
   {
      VALUE         key  = INT2FIX(isc_spb_bkp_factor),
                    temp = rb_funcall(options, rb_intern("fetch"), 1, key);
      unsigned long size = TYPE(temp) == T_FIXNUM ? FIX2INT(temp) : NUM2INT(temp);

      *position++ = isc_spb_bkp_factor;
      memcpy(position, &size, 4);
      position += 4;
   }

   if(extras && flags)
   {
      unsigned long mask = 0;

      if(rb_funcall(options, id, 1, IGNORE_CHECKSUMS) == Qtrue)
      {
         mask |= isc_spb_bkp_ignore_checksums;
      }

      if(rb_funcall(options, id, 1, IGNORE_LIMBO) == Qtrue)
      {
         mask |= isc_spb_bkp_ignore_limbo;
      }

      if(rb_funcall(options, id, 1, METADATA_ONLY) == Qtrue)
      {
         mask |= isc_spb_bkp_metadata_only;
      }

      if(rb_funcall(options, id, 1, NO_GARBAGE_COLLECT) == Qtrue)
      {
         mask |= isc_spb_bkp_no_garbage_collect;
      }

      if(rb_funcall(options, id, 1, NON_TRANSPORTABLE) == Qtrue)
      {
         mask |= isc_spb_bkp_non_transportable;
      }

      if(rb_funcall(options, id, 1, CONVERT_TABLES) == Qtrue)
      {
         mask |= isc_spb_bkp_convert;
      }

      *position++ = isc_spb_options;
      memcpy(position, &mask, 4);
      position += 4;
   }

   *position++ = isc_spb_verbose;
}


/**
 * This function initialize the Backup class in the Ruby environment.
 *
 * @param  module  The module to create the new class definition under.
 *
 */
void Init_Backup(VALUE module)
{
   cBackup = rb_define_class_under(module, "Backup", rb_cObject);
   rb_define_method(cBackup, "initialize", initializeBackup, 2);
   rb_define_method(cBackup, "backup_file", getBackupFile, 0);
   rb_define_method(cBackup, "backup_file=", setBackupFile, 1);
   rb_define_method(cBackup, "database", getBackupDatabase, 0);
   rb_define_method(cBackup, "database=", setBackupDatabase, 1);
   rb_define_method(cBackup, "blocking_factor", getBackupBlockingFactor, 0);
   rb_define_method(cBackup, "blocking_factor=", setBackupBlockingFactor, 1);
   rb_define_method(cBackup, "ignore_checksums", getBackupIgnoreChecksums, 0);
   rb_define_method(cBackup, "ignore_checksums=", setBackupIgnoreChecksums, 1);
   rb_define_method(cBackup, "ignore_limbo", getBackupIgnoreLimbo, 0);
   rb_define_method(cBackup, "ignore_limbo=", setBackupIgnoreLimbo, 1);
   rb_define_method(cBackup, "metadata_only", getBackupMetadataOnly, 0);
   rb_define_method(cBackup, "metadata_only=", setBackupMetadataOnly, 1);
   rb_define_method(cBackup, "garbage_collect", getBackupGarbageCollect, 0);
   rb_define_method(cBackup, "garbage_collect=", setBackupGarbageCollect, 1);
   rb_define_method(cBackup, "non_transportable", getBackupNonTransportable, 0);
   rb_define_method(cBackup, "non_transportable=", setBackupNonTransportable, 1);
   rb_define_method(cBackup, "convert_tables", getBackupConvertTables, 0);
   rb_define_method(cBackup, "convert_tables=", setBackupConvertTables, 1);
   rb_define_method(cBackup, "execute", executeBackup, 1);
   rb_define_method(cBackup, "log", getBackupLog, 0);
}
