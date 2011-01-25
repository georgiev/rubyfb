/*------------------------------------------------------------------------------
 * Database.c
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
#include "Database.h"
#include "FireRubyException.h"
#include "Connection.h"
#include "ruby.h"

/* Function prototypes. */
static VALUE allocateDatabase(VALUE);
static VALUE initializeDatabase(int, VALUE *, VALUE);
static VALUE getDatabaseFile(VALUE);
static VALUE connectToDatabase(int, VALUE *, VALUE);
static VALUE createDatabase(int, VALUE *, VALUE);
static VALUE dropDatabase(VALUE, VALUE, VALUE);
static VALUE getDatabaseCharacterSet(VALUE);
static VALUE setDatabaseCharacterSet(VALUE, VALUE);
VALUE connectBlock(VALUE);
VALUE connectEnsure(VALUE);

/* Globals. */
VALUE cDatabase;


/**
 * This function provides the functionality to allow for the allocation of
 * new Database objects.
 *
 * @param  klass  A reference to the Database Class object.
 *
 * @return  A reference to the new Database class instance.
 *
 */
static VALUE allocateDatabase(VALUE klass) {
  VALUE instance  = Qnil;
  DatabaseHandle *database = ALLOC(DatabaseHandle);

  if(database != NULL) {
    /* Wrap the structure in a class. */
    instance = Data_Wrap_Struct(klass, NULL, databaseFree, database);
  } else {
    rb_raise(rb_eNoMemError,
             "Memory allocation failure creating a Database object.");
  }

  return(instance);
}


/**
 * This function provides the initialize method for the Database class.
 *
 * @param  argc  A count of the total number of parameters passed to the
 *               function.
 * @param  argv  A pointer to an array containing the parameters passed to the
 *               function.
 * @param  self  A reference to the object that the method call is being made
 *               to.
 *
 * @return  A reference to the new initialized object instance.
 *
 */
VALUE initializeDatabase(int argc, VALUE *argv, VALUE self) {
  VALUE options = rb_hash_new();

  /* Check the number of parameters. */
  if(argc < 1) {
    rb_raise(rb_eArgError, "Wrong number of arguments (%d for %d).", argc, 1);
  }

  /* Perform type checks on the input parameters. */
  Check_Type(argv[0], T_STRING);
  if(argc > 1) {
    Check_Type(argv[1], T_STRING);
    rb_hash_aset(options, INT2FIX(isc_dpb_lc_ctype), argv[1]);
  }

  /* Store the database details. */
  rb_iv_set(self, "@file", argv[0]);
  rb_iv_set(self, "@options", options);

  return(self);
}


/**
 * This function provides the file attribute accessor for the Database class.
 *
 * @param  self  A reference to the object that the method is being called on.
 *
 * @return  A reference to the database file name associated with the Database
 *          object
 *
 */
static VALUE getDatabaseFile(VALUE self) {
  return(rb_iv_get(self, "@file"));
}


/**
 * This method attempts to open a connection to a database using the details
 * in a Database object.
 *
 * @param  argc  A count of the number of parameters passed to the function.
 * @param  argv  A pointer to the start of an array containing the function
 *               parameter values.
 * @param  self  A reference to the object that the call is being made on.
 *
 * @return  A reference to a Connection object or nil if an error occurs.
 *
 */
static VALUE connectToDatabase(int argc, VALUE *argv, VALUE self) {
  VALUE result     = Qnil,
        user       = Qnil,
        password   = Qnil,
        options    = rb_iv_get(self, "@options"),
        connection = Qnil;

  if(argc > 0) {
    user = argv[0];
  }
  if(argc > 1) {
    password = argv[1];
  }
  if(argc > 2) {
    rb_funcall(options, rb_intern("update"), 1, argv[2]);
  }

  connection = rb_connection_new(self, user, password, options);
  if(rb_block_given_p()) {
    result = rb_ensure(connectBlock, connection, connectEnsure, connection);
  } else {
    result = connection;
  }

  return(result);
}


/**
 * This function provides the create class method for Database class.
 *
 * @param  usused    Like it says, not used.
 * @param  file      A String containing the  name of the primary file for the
 *                   database to be created.
 * @param  user      The database user that will be used in creating the new
 *                   database.
 * @param  password  A String containing the password for the user to be used
 *                   in creating the database.
 * @param  size      The size of pages that will be used by the database.
 * @param  set       A String containing the name of the character set to be
 *                   used in creating the database.
 *
 * @return  A reference to a Database object on success, nil on failure.
 *
 */
static VALUE createDatabase(int argc, VALUE *argv, VALUE unused) {
  VALUE database    = Qnil,
        set         = Qnil,
        tmp_str     = Qnil;
  char sql[512]    = "",
  *text       = NULL;
  int value       = 1024;
  isc_db_handle connection  = 0;
  isc_tr_handle transaction = 0;
  ISC_STATUS status[ISC_STATUS_LENGTH];

  /* Check that sufficient parameters have been provided. */
  if(argc < 3) {
    rb_raise(rb_eArgError, "Wrong number of parameters (%d for %d).", argc,
             3);
  }

  /* Check values that are permitted restricted values. */
  if(argc > 3) {
    value = FIX2INT(argv[3]);
    if(value != 1024 && value != 2048 && value != 4096 && value != 8192) {
      rb_raise(rb_eException,
               "Invalid database page size value . Valid values are 1024, " \
               "2048 4096 or 8192.");
    }
  }

  /* Prepare the SQL statement. */
  tmp_str = rb_funcall(argv[0], rb_intern("to_s"), 0);
  sprintf(sql, "CREATE DATABASE '%s'", StringValuePtr(tmp_str));

  tmp_str = rb_funcall(argv[1], rb_intern("to_s"), 0);
  text = StringValuePtr(tmp_str);
  if(strlen(text) > 0) {
    strcat(sql, " USER '");
    strcat(sql, text);
    strcat(sql, "'");
  }

  tmp_str = rb_funcall(argv[2], rb_intern("to_s"), 0);
  text = StringValuePtr(tmp_str);
  if(strlen(text) > 0) {
    strcat(sql, " PASSWORD '");
    strcat(sql, text);
    strcat(sql, "'");
  }

  if(argc > 3) {
    char text[50];

    sprintf(text, " PAGE_SIZE = %d", value);
    strcat(sql, text);
  }

  if(argc > 4 && argv[4] != Qnil) {
    char *text = NULL;

    set  = rb_funcall(argv[4], rb_intern("to_s"), 0);
    text = StringValuePtr(set);
    if(strlen(text) > 0) {
      strcat(sql, " DEFAULT CHARACTER SET ");
      strcat(sql, text);
    }
  }
  strcat(sql, ";");

  if(isc_dsql_execute_immediate(status, &connection, &transaction, 0, sql,
                                3, NULL) != 0) {
    rb_fireruby_raise(status, "Database creation error.");
  }

  if(connection != 0) {
    isc_detach_database(status, &connection);
  }

  database = rb_database_new(argv[0]);
  if(set != Qnil) {
    setDatabaseCharacterSet(database, set);
  }

  return(database);
}


/**
 * This function attempts to attach to and drop a specified database. This
 * function provides the drop method for the Database class.
 *
 * @param  self      A reference to the Database object representing the
 *                   database to be dropped.
 * @param  user      A reference to the database user name that will be used
 *                   in dropping the database
 * @param  password  A reference to the user password that will be used in
 *                   dropping the database.
 *
 * @return  Always returns nil.
 *
 */
static VALUE dropDatabase(VALUE self, VALUE user, VALUE password) {
  VALUE connection = rb_connection_new(self, user, password, Qnil);
  ConnectionHandle *cHandle   = NULL;
  ISC_STATUS status[ISC_STATUS_LENGTH];

  Data_Get_Struct(connection, ConnectionHandle, cHandle);
  if(isc_drop_database(status, &cHandle->handle) != 0) {
    rb_fireruby_raise(status, "Error dropping database.");
  }

  return(Qnil);
}


/**
 * This function retrieves the current character set specification for a
 * Database object.
 *
 * @param  self  A reference to the Database object to make the call on.
 *
 * @return  A reference to the database character set. May be nil.
 *
 */
static VALUE getDatabaseCharacterSet(VALUE self) {
  VALUE options = rb_iv_get(self, "@options");

  return(rb_hash_aref(options, INT2FIX(isc_dpb_lc_ctype)));
}


/**
 * This function assigns a character set for use with a Database object. To
 * revert to an unspecified character set assign nil to this value.
 *
 * @param  self  A reference to the Database object to be updated.
 * @param  set   A string containing the name of the character set.
 *
 * @return  A reference to the updated Database object.
 *
 */
static VALUE setDatabaseCharacterSet(VALUE self, VALUE set) {
  VALUE options = rb_iv_get(self, "@options");

  if(set != Qnil) {
    rb_hash_aset(options, INT2FIX(isc_dpb_lc_ctype), set);
  } else {
    rb_hash_delete(options, INT2FIX(isc_dpb_lc_ctype));
  }

  return(self);
}


/**
 * This function is used to integrate with the Ruby garbage collection system
 * to guarantee the release of the resources associated with a Database object.
 *
 * @param  database  A pointer to the DatabaseHandle structure associated with
 *                   a Database object being destroyed.
 *
 */
void databaseFree(void *database) {
  if(database != NULL) {
    free((DatabaseHandle *)database);
  }
}


/**
 * This function implements the handling of blocks passed to a call to the
 * connect method of the Database class. The block passed to the connect
 * function is expected to take a single parameter, which will be the connection
 * created.
 *
 * @param  connection  A reference to the connection established for the block.
 *
 * @return  The return value provided by execution of the provided block.
 *
 */
VALUE connectBlock(VALUE connection) {
  return(rb_yield(connection));
}


/**
 * This function provides the ensure aspect for the block handling associated
 * with the connect method. This method ensures that the connection opened for
 * the block is closed when the block completes.
 *
 * @param  connection  A reference to the connection to be closed.
 *
 * @return  Always Qnil.
 *
 */
VALUE connectEnsure(VALUE connection) {
  rb_funcall(connection, rb_intern("close"), 0);
  return(Qnil);
}


/**
 * This function provides a means to programmatically create a Database object.
 *
 * @param  file A reference to a String containing the database file details.
 *
 * @return  A reference to the Database object created.
 *
 */
VALUE rb_database_new(VALUE file) {
  VALUE database     = allocateDatabase(cDatabase),
        parameters[] = {file};

  initializeDatabase(1, parameters, database);

  return(database);
}


/**
 * This function initializes the Database class within the Ruby environment.
 * The class is established under the module specified to the function.
 *
 * @param  module  A reference to the module to create the class within.
 *
 */
void Init_Database(VALUE module) {
  cDatabase = rb_define_class_under(module, "Database", rb_cObject);
  rb_define_alloc_func(cDatabase, allocateDatabase);
  rb_define_method(cDatabase, "initialize", initializeDatabase, -1);
  rb_define_method(cDatabase, "file", getDatabaseFile, 0);
  rb_define_method(cDatabase, "connect", connectToDatabase, -1);
  rb_define_method(cDatabase, "drop", dropDatabase, 2);
  rb_define_method(cDatabase, "character_set", getDatabaseCharacterSet, 0);
  rb_define_method(cDatabase, "character_set=", setDatabaseCharacterSet, 1);
  rb_define_module_function(cDatabase, "create", createDatabase, -1);
}
