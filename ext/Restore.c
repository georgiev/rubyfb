/*------------------------------------------------------------------------------
 * Restore.c
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
#include "Restore.h"
#include "ibase.h"
#include "ServiceManager.h"
#include "Services.h"

/* Function prototypes. */
static VALUE initializeRestore(VALUE, VALUE, VALUE);
static VALUE getRestoreFile(VALUE);
static VALUE setRestoreFile(VALUE, VALUE);
static VALUE getRestoreDatabase(VALUE);
static VALUE setRestoreDatabase(VALUE, VALUE);
static VALUE getRestoreCacheBuffers(VALUE);
static VALUE setRestoreCacheBuffers(VALUE, VALUE);
static VALUE getRestorePageSize(VALUE);
static VALUE setRestorePageSize(VALUE, VALUE);
static VALUE getRestoreAccessMode(VALUE);
static VALUE setRestoreAccessMode(VALUE, VALUE);
static VALUE getRestoreBuildIndices(VALUE);
static VALUE setRestoreBuildIndices(VALUE, VALUE);
static VALUE getRestoreCreateShadows(VALUE);
static VALUE setRestoreCreateShadows(VALUE, VALUE);
static VALUE getRestoreCheckValidity(VALUE);
static VALUE setRestoreCheckValidity(VALUE, VALUE);
static VALUE getRestoreCommitTables(VALUE);
static VALUE setRestoreCommitTables(VALUE, VALUE);
static VALUE getRestoreMode(VALUE);
static VALUE setRestoreMode(VALUE, VALUE);
static VALUE getRestoreUseAllSpace(VALUE);
static VALUE setRestoreUseAllSpace(VALUE, VALUE);
static VALUE executeRestore(VALUE, VALUE);
static VALUE getRestoreLog(VALUE);
static void createRestoreBuffer(VALUE, VALUE, VALUE, char **, short *);


/* Globals. */
VALUE cRestore;

/* Definitions. */
#define CACHE_BUFFERS    INT2FIX(isc_spb_res_buffers)
#define PAGE_SIZE        INT2FIX(isc_spb_res_page_size)
#define ACCESS_MODE      INT2FIX(isc_spb_res_access_mode)
#define READ_ONLY        INT2FIX(isc_spb_prp_am_readonly)
#define READ_WRITE       INT2FIX(isc_spb_prp_am_readwrite)
#define BUILD_INDICES    rb_str_new2("BUILD_INDICES")
#define NO_SHADOWS       rb_str_new2("NO_SHADOWS")
#define VALIDITY_CHECKS  rb_str_new2("VALIDITY_CHECKS")
#define COMMIT_TABLES    rb_str_new2("COMMIT_TABLES")
#define RESTORE_MODE     rb_str_new2("RESTORE_MODE")
#define REPLACE_DATABASE INT2FIX(isc_spb_res_replace)
#define CREATE_DATABASE  INT2FIX(isc_spb_res_create)
#define USE_ALL_SPACE    rb_str_new2("USE_ALL_SPACE")

/**
 * This function provides the initialize method for the Restore class.
 *
 * @param  self      A reference to the Restore object to be initialized.
 * @param  file      A reference to a String or File containing the path/name
 *                   of the database backup file to restore from.
 * @param  database  A reference to a String or File containing the path/name
 *                   of the database file to be restored to.
 *
 * @return  A reference to the newly initialized Restore object.
 *
 */
VALUE initializeRestore(VALUE self, VALUE file, VALUE database)
{
   VALUE from    = Qnil,
         to      = rb_hash_new(),
         options = rb_hash_new();

   /* Extract the parameters. */
   if(TYPE(file) == T_FILE)
   {
      from = rb_funcall(file, rb_intern("path"), 0);
   }
   else
   {
      from = rb_funcall(file, rb_intern("to_s"), 0);
   }

   if(TYPE(database) == T_FILE)
   {
      to = rb_funcall(database, rb_intern("path"), 0);
   }
   else
   {
      to = rb_funcall(database, rb_intern("to_s"), 0);
   }
   rb_hash_aset(options, RESTORE_MODE, CREATE_DATABASE);

   /* Store the object attributes. */
   rb_iv_set(self, "@backup_file", from);
   rb_iv_set(self, "@database", to);
   rb_iv_set(self, "@options", options);
   rb_iv_set(self, "@log", Qnil);

   return(self);
}


/**
 * This function provides the backup_file attribute accessor for the Restore
 * class.
 *
 * @param  self  A reference to the Restore object to fetch the attribute from.
 *
 * @return  A reference to the attribute value.
 *
 */
VALUE getRestoreFile(VALUE self)
{
   return(rb_iv_get(self, "@backup_file"));
}


/**
 * This function provides the backup_file attribute accessor for the Restore
 * class.
 *
 * @param  self     A reference to the Restore object to fetch the attribute
 *                  from.
 * @param  setting  A reference to the new attribute value.
 *
 * @return  A reference to the newly update Restore object.
 *
 */
VALUE setRestoreFile(VALUE self, VALUE setting)
{
   VALUE actual = Qnil;

   if(TYPE(setting) == T_FILE)
   {
      actual = rb_funcall(setting, rb_intern("path"), 0);
   }
   else
   {
      actual = rb_funcall(setting, rb_intern("to_s"), 0);
   }
   rb_iv_set(self, "@backup_file", actual);

   return(self);
}


/**
 * This function provides the database attribute accessor for the Restore
 * class.
 *
 * @param  self  A reference to the Restore object to fetch the attribute from.
 *
 * @return  A reference to the attribute value.
 *
 */
VALUE getRestoreDatabase(VALUE self)
{
   return(rb_iv_get(self, "@database"));
}


/**
 * This function provides the database attribute accessor for the Restore
 * class.
 *
 * @param  self     A reference to the Restore object to fetch the attribute
 *                  from.
 * @param  setting  A reference to the new attribute value.
 *
 * @return  A reference to the newly update Restore object.
 *
 */
VALUE setRestoreDatabase(VALUE self, VALUE setting)
{
   VALUE actual = Qnil;

   if(TYPE(setting) == T_FILE)
   {
      actual = rb_funcall(setting, rb_intern("path"), 0);
   }
   else
   {
      actual = rb_funcall(setting, rb_intern("to_s"), 0);
   }
   rb_iv_set(self, "@database", actual);

   return(self);
}


/**
 * This function provides the cache_buffers attribute accessor for the Restore
 * class.
 *
 * @param  self  A reference to the Restore object to access the attribute on.
 *
 * @return  A reference to the current cache buffers setting.
 *
 */
VALUE getRestoreCacheBuffers(VALUE self)
{
   return(rb_hash_aref(rb_iv_get(self, "@options"), CACHE_BUFFERS));
}


/**
 * This function provides the cache_buffers attribute mutator for the Restore
 * class.
 *
 * @param  self     A reference to the Restore object to set the attribute on.
 * @param  setting  A reference to the new setting for the attribute.
 *
 * @return  A reference to the newly updated Restore object.
 *
 */
VALUE setRestoreCacheBuffers(VALUE self, VALUE setting)
{
   if(rb_obj_is_kind_of(setting, rb_cInteger) == Qfalse)
   {
      rb_fireruby_raise(NULL,
                        "Invalid cache buffers setting specified for database "\
                        "restore.");
   }
   rb_hash_aset(rb_iv_get(self, "@options"), CACHE_BUFFERS, setting);

   return(self);
}


/**
 * This function provides the page_size attribute accessor for the Restore
 * class.
 *
 * @param  self  A reference to the Restore object to access the attribute on.
 *
 * @return  A reference to the current cache buffers setting.
 *
 */
VALUE getRestorePageSize(VALUE self)
{
   return(rb_hash_aref(rb_iv_get(self, "@options"), PAGE_SIZE));
}


/**
 * This function provides the page_size attribute mutator for the Restore
 * class.
 *
 * @param  self     A reference to the Restore object to set the attribute on.
 * @param  setting  A reference to the new setting for the attribute.
 *
 * @return  A reference to the newly updated Restore object.
 *
 */
VALUE setRestorePageSize(VALUE self, VALUE setting)
{
   if(rb_obj_is_kind_of(setting, rb_cInteger) == Qfalse)
   {
      rb_fireruby_raise(NULL,
                        "Invalid page size setting specified for database "\
                        "restore.");
   }
   rb_hash_aset(rb_iv_get(self, "@options"), PAGE_SIZE, setting);

   return(self);
}


/**
 * This function provides the access_mode attribute accessor for the Restore
 * class.
 *
 * @param  self  A reference to the Restore object to access the attribute on.
 *
 * @return  A reference to the current cache buffers setting.
 *
 */
VALUE getRestoreAccessMode(VALUE self)
{
   return(rb_hash_aref(rb_iv_get(self, "@options"), ACCESS_MODE));
}


/**
 * This function provides the access_mode attribute mutator for the Restore
 * class.
 *
 * @param  self     A reference to the Restore object to set the attribute on.
 * @param  setting  A reference to the new setting for the attribute.
 *
 * @return  A reference to the newly updated Restore object.
 *
 */
VALUE setRestoreAccessMode(VALUE self, VALUE setting)
{
   int value = 0;

   if(rb_obj_is_kind_of(setting, rb_cInteger) == Qfalse)
   {
      rb_fireruby_raise(NULL,
                        "Invalid access mode setting specified for database "\
                        "restore.");
   }

   value = TYPE(setting) == T_FIXNUM ? FIX2INT(setting) : NUM2INT(setting);
   if(value != isc_spb_prp_am_readonly && value != isc_spb_prp_am_readwrite)
   {
      rb_fireruby_raise(NULL,
                        "Invalid access mode value specified for database "\
                        "restore.");
   }

   rb_hash_aset(rb_iv_get(self, "@options"), ACCESS_MODE, setting);

   return(self);
}


/**
 * This function provides the build_indices attribute accessor for the Restore
 * class.
 *
 * @param  self  A reference to the Restore object to access the attribute on.
 *
 * @return  A reference to the current cache buffers setting.
 *
 */
VALUE getRestoreBuildIndices(VALUE self)
{
   VALUE result  = Qtrue,
         setting = rb_hash_aref(rb_iv_get(self, "@options"), BUILD_INDICES);

   if(setting != Qnil)
   {
      result = setting;
   }

   return(result);
}


/**
 * This function provides the build_indices attribute mutator for the Restore
 * class.
 *
 * @param  self     A reference to the Restore object to set the attribute on.
 * @param  setting  A reference to the new setting for the attribute.
 *
 * @return  A reference to the newly updated Restore object.
 *
 */
VALUE setRestoreBuildIndices(VALUE self, VALUE setting)
{
   if(setting != Qfalse && setting != Qtrue)
   {
      rb_fireruby_raise(NULL,
                        "Invalid build indices setting specified for database "\
                        "restore.");
   }
   rb_hash_aset(rb_iv_get(self, "@options"), BUILD_INDICES, setting);

   return(self);
}


/**
 * This function provides the create_shadows attribute accessor for the Restore
 * class.
 *
 * @param  self  A reference to the Restore object to access the attribute on.
 *
 * @return  A reference to the current cache buffers setting.
 *
 */
VALUE getRestoreCreateShadows(VALUE self)
{
   VALUE result  = Qfalse,
         setting = rb_hash_aref(rb_iv_get(self, "@options"), NO_SHADOWS);

   if(setting != Qnil)
   {
      result = setting;
   }

   return(result);
}


/**
 * This function provides the create_shadows attribute mutator for the Restore
 * class.
 *
 * @param  self     A reference to the Restore object to set the attribute on.
 * @param  setting  A reference to the new setting for the attribute.
 *
 * @return  A reference to the newly updated Restore object.
 *
 */
VALUE setRestoreCreateShadows(VALUE self, VALUE setting)
{
   if(setting != Qfalse && setting != Qtrue)
   {
      rb_fireruby_raise(NULL,
                        "Invalid create shadows setting specified for "\
                        "database restore.");
   }
   rb_hash_aset(rb_iv_get(self, "@options"), NO_SHADOWS, setting);

   return(self);
}


/**
 * This function provides the validity_checks attribute accessor for the Restore
 * class.
 *
 * @param  self  A reference to the Restore object to access the attribute on.
 *
 * @return  A reference to the current cache buffers setting.
 *
 */
VALUE getRestoreCheckValidity(VALUE self)
{
   VALUE result  = Qtrue,
         setting = rb_hash_aref(rb_iv_get(self, "@options"), VALIDITY_CHECKS);

   if(setting != Qnil)
   {
      result = setting;
   }

   return(result);
}


/**
 * This function provides the validity_checks attribute mutator for the Restore
 * class.
 *
 * @param  self     A reference to the Restore object to set the attribute on.
 * @param  setting  A reference to the new setting for the attribute.
 *
 * @return  A reference to the newly updated Restore object.
 *
 */
VALUE setRestoreCheckValidity(VALUE self, VALUE setting)
{
   if(setting != Qfalse && setting != Qtrue)
   {
      rb_fireruby_raise(NULL,
                        "Invalid validity checks setting specified for "\
                        "database restore.");
   }
   rb_hash_aset(rb_iv_get(self, "@options"), VALIDITY_CHECKS, setting);

   return(self);
}


/**
 * This function provides the commit_tables attribute accessor for the Restore
 * class.
 *
 * @param  self  A reference to the Restore object to access the attribute on.
 *
 * @return  A reference to the current cache buffers setting.
 *
 */
VALUE getRestoreCommitTables(VALUE self)
{
   VALUE result  = Qfalse,
         setting = rb_hash_aref(rb_iv_get(self, "@options"), COMMIT_TABLES);

   if(setting != Qnil)
   {
      result = setting;
   }

   return(result);
}


/**
 * This function provides the commit_tables attribute mutator for the Restore
 * class.
 *
 * @param  self     A reference to the Restore object to set the attribute on.
 * @param  setting  A reference to the new setting for the attribute.
 *
 * @return  A reference to the newly updated Restore object.
 *
 */
VALUE setRestoreCommitTables(VALUE self, VALUE setting)
{
   if(setting != Qfalse && setting != Qtrue)
   {
      rb_fireruby_raise(NULL,
                        "Invalid commit tables setting specified for "\
                        "database restore.");
   }
   rb_hash_aset(rb_iv_get(self, "@options"), COMMIT_TABLES, setting);

   return(self);
}


/**
 * This function provides the mode attribute accessor for the Restore class.
 *
 * @param  self  A reference to the Restore object to access the attribute on.
 *
 * @return  A reference to the current cache buffers setting.
 *
 */
VALUE getRestoreMode(VALUE self)
{
   VALUE result  = Qfalse,
         setting = rb_hash_aref(rb_iv_get(self, "@options"), RESTORE_MODE);

   if(setting != Qnil)
   {
      result = setting;
   }

   return(result);
}


/**
 * This function provides the mode attribute mutator for the Restore class.
 *
 * @param  self     A reference to the Restore object to set the attribute on.
 * @param  setting  A reference to the new setting for the attribute.
 *
 * @return  A reference to the newly updated Restore object.
 *
 */
VALUE setRestoreMode(VALUE self, VALUE setting)
{
   int value;

   if(rb_obj_is_kind_of(setting, rb_cInteger) == Qfalse)
   {
      rb_fireruby_raise(NULL,
                        "Invalid mode setting specified for database restore.");
   }

   value = TYPE(setting) == T_FIXNUM ? FIX2INT(setting) : NUM2INT(setting);
   if(value != isc_spb_res_create && value != isc_spb_res_replace)
   {
      rb_fireruby_raise(NULL,
                        "Unrecognised mode setting specified for database "\
                        "restore.");
   }
   rb_hash_aset(rb_iv_get(self, "@options"), RESTORE_MODE, setting);

   return(self);
}


/**
 * This function provides the use_all_space attribute accessor for the Restore
 * class.
 *
 * @param  self  A reference to the Restore object to access the attribute on.
 *
 * @return  A reference to the current cache buffers setting.
 *
 */
VALUE getRestoreUseAllSpace(VALUE self)
{
   VALUE result  = Qfalse,
         setting = rb_hash_aref(rb_iv_get(self, "@options"), USE_ALL_SPACE);

   if(setting != Qnil)
   {
      result = setting;
   }

   return(result);
}


/**
 * This function provides the use_all+_space attribute mutator for the Restore
 * class.
 *
 * @param  self     A reference to the Restore object to set the attribute on.
 * @param  setting  A reference to the new setting for the attribute.
 *
 * @return  A reference to the newly updated Restore object.
 *
 */
VALUE setRestoreUseAllSpace(VALUE self, VALUE setting)
{
   if(setting != Qfalse && setting != Qtrue)
   {
      rb_fireruby_raise(NULL,
                        "Invalid space usage setting specified for database "\
                        "restore.");
   }
   rb_hash_aset(rb_iv_get(self, "@options"), USE_ALL_SPACE, setting);

   return(self);
}


/**
 * This function provides the execute method for the Restore class.
 *
 * @param  self     A reference to the Restore object to call the method on.
 * @param  manager  A reference to the ServiceManager that will be used to
 *                  execute the task.
 *
 * @return  A reference to the Restore object executed.
 *
 */
VALUE executeRestore(VALUE self, VALUE manager)
{
   ManagerHandle *handle = NULL;
   char          *buffer = NULL;
   short         length  = 0;
   ISC_STATUS    status[20];

   /* Check that the service manager is connected. */
   Data_Get_Struct(manager, ManagerHandle, handle);
   if(handle->handle == 0)
   {
      rb_fireruby_raise(NULL,
                        "Database restore error. Service manager not connected.");
   }

   createRestoreBuffer(rb_iv_get(self, "@backup_file"),
                       rb_iv_get(self, "@database"),
                       rb_iv_get(self, "@options"), &buffer, &length);

   /* Start the service request. */
   if(isc_service_start(status, &handle->handle, NULL, length, buffer))
   {
      free(buffer);
      rb_fireruby_raise(status, "Error performing database restore.");
   }
   free(buffer);

   /* Query the service until it is complete. */
   rb_iv_set(self, "@log", queryService(&handle->handle));

   return(self);
}


/**
 * This function provides the log attribute accessor for the Restore class.
 *
 * @param  self  A reference to the Restore object to execute the method on.
 *
 * @return  A reference to the log attribute value.
 *
 */
VALUE getRestoreLog(VALUE self)
{
   return(rb_iv_get(self, "@log"));
}


/**
 * This function create the service parameter buffer used in executing a Restore
 * object.
 *
 * @param  file      A reference to a String containing the path and name of
 *                   the backup file to restore the database from.
 * @param  database  A reference to a String containing the path and name for
 *                   the restored database.
 * @param  options   A reference to a Hash containing the options to be used
 *                   in restoring the database.
 * @param  buffer    A pointer to a pointer to a character array that will be
 *                   set to the created service parameter buffer (allocated off
 *                   the heap).
 * @param  length    A pointer to a short integer that will be assigned the
 *                   length of the service parameter buffer created.
 *
 */
void createRestoreBuffer(VALUE file, VALUE database, VALUE options,
                         char **buffer, short *length)
{
   char  *offset = NULL;
   int   number  = 0;
   long  mask    = 0;
   VALUE cache   = rb_hash_aref(options, CACHE_BUFFERS),
         page    = rb_hash_aref(options, PAGE_SIZE),
         mode    = rb_hash_aref(options, ACCESS_MODE),
         policy  = rb_hash_aref(options, RESTORE_MODE);

   /* Determine the length of the buffer. */
   *length = 7;
   *length += strlen(STR2CSTR(file)) + 3;
   *length += strlen(STR2CSTR(database)) + 3;
   if(cache != Qnil)
   {
      *length += 5;
   }
   if(page != Qnil)
   {
      *length += 5;
   }
   if(mode != Qnil)
   {
      *length += 2;
   }

   /* Create and populate the buffer. */
   offset = *buffer = ALLOC_N(char, *length);
   if(buffer == NULL)
   {
      rb_raise(rb_eNoMemError,
               "Memory allocation error preparing database restore.");
   }
   memset(*buffer, 8, *length);

   *offset++ = isc_action_svc_restore;

   number    = strlen(STR2CSTR(file));
   *offset++ = isc_spb_bkp_file;
   ADD_SPB_LENGTH(offset, number);
   memcpy(offset, STR2CSTR(file), number);
   offset    += number;

   number    = strlen(STR2CSTR(database));
   *offset++ = isc_spb_dbname;
   ADD_SPB_LENGTH(offset, number);
   memcpy(offset, STR2CSTR(database), number);
   offset    += number;

   if(cache != Qnil)
   {
      long value;

      value = TYPE(cache) == T_FIXNUM ? FIX2INT(cache) : NUM2INT(cache);
      *offset++ = isc_spb_res_buffers;
      ADD_SPB_NUMERIC(offset, value);
   }

   if(page != Qnil)
   {
      long value;

      value = TYPE(page) == T_FIXNUM ? FIX2INT(page) : NUM2INT(page);
      *offset++ = isc_spb_res_page_size;
      ADD_SPB_NUMERIC(offset, value);
   }

   if(mode != Qnil)
   {
      *offset++ = isc_spb_res_access_mode;
      *offset++ = (char)FIX2INT(mode);
   }

   mask = FIX2INT(policy);

   if(rb_hash_aref(options, BUILD_INDICES) == Qfalse)
   {
      mask |= isc_spb_res_deactivate_idx;
   }

   if(rb_hash_aref(options, NO_SHADOWS) == Qtrue)
   {
      mask |= isc_spb_res_no_shadow;
   }

   if(rb_hash_aref(options, VALIDITY_CHECKS) == Qfalse)
   {
      mask |= isc_spb_res_no_validity;
   }

   if(rb_hash_aref(options, COMMIT_TABLES) == Qtrue)
   {
      mask |= isc_spb_res_one_at_a_time;
   }

   if(rb_hash_aref(options, USE_ALL_SPACE) == Qtrue)
   {
      mask |= isc_spb_res_use_all_space;
   }

   *offset++ = isc_spb_options;
   ADD_SPB_NUMERIC(offset, mask);

   *offset++ = isc_spb_verbose;
}


/**
 * This function initialize the Restore class in the Ruby environment.
 *
 * @param  module  The module to create the new class definition under.
 *
 */
void Init_Restore(VALUE module)
{
   cRestore = rb_define_class_under(module, "Restore", rb_cObject);
   rb_define_method(cRestore, "initialize", initializeRestore, 2);
   rb_define_method(cRestore, "backup_file", getRestoreFile, 0);
   rb_define_method(cRestore, "backup_file=", setRestoreFile, 1);
   rb_define_method(cRestore, "database", getRestoreDatabase, 0);
   rb_define_method(cRestore, "database=", setRestoreDatabase, 1);
   rb_define_method(cRestore, "cache_buffers", getRestoreCacheBuffers, 0);
   rb_define_method(cRestore, "cache_buffers=", setRestoreCacheBuffers, 1);
   rb_define_method(cRestore, "page_size", getRestorePageSize, 0);
   rb_define_method(cRestore, "page_size=", setRestorePageSize, 1);
   rb_define_method(cRestore, "access_mode", getRestoreAccessMode, 0);
   rb_define_method(cRestore, "access_mode=", setRestoreAccessMode, 1);
   rb_define_method(cRestore, "build_indices", getRestoreBuildIndices, 0);
   rb_define_method(cRestore, "build_indices=", setRestoreBuildIndices, 1);
   rb_define_method(cRestore, "no_shadows", getRestoreCreateShadows, 0);
   rb_define_method(cRestore, "no_shadows=", setRestoreCreateShadows, 1);
   rb_define_method(cRestore, "check_validity", getRestoreCheckValidity, 0);
   rb_define_method(cRestore, "check_validity=", setRestoreCheckValidity, 1);
   rb_define_method(cRestore, "commit_tables", getRestoreCommitTables, 0);
   rb_define_method(cRestore, "commit_tables=", setRestoreCommitTables, 1);
   rb_define_method(cRestore, "restore_mode", getRestoreMode, 0);
   rb_define_method(cRestore, "restore_mode=", setRestoreMode, 1);
   rb_define_method(cRestore, "use_all_space", getRestoreUseAllSpace, 0);
   rb_define_method(cRestore, "use_all_space=", setRestoreUseAllSpace, 1);
   rb_define_method(cRestore, "execute", executeRestore, 1);
   rb_define_method(cRestore, "log", getRestoreLog, 0);

   rb_define_const(cRestore, "ACCESS_READ_ONLY", INT2FIX(isc_spb_prp_am_readonly));
   rb_define_const(cRestore, "ACCESS_READ_WRITE", INT2FIX(isc_spb_prp_am_readwrite));
   rb_define_const(cRestore, "MODE_CREATE", INT2FIX(isc_spb_res_replace));
   rb_define_const(cRestore, "MODE_REPLACE", INT2FIX(isc_spb_res_create));
}
