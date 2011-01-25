/*------------------------------------------------------------------------------
 * Connection.c
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
#include "Connection.h"
#include "Database.h"
#include "ResultSet.h"
#include "Statement.h"
#include "Transaction.h"
#include "Common.h"

/* Function prototypes. */
static VALUE allocateConnection(VALUE);
static VALUE initializeConnection(int, VALUE *, VALUE);
static VALUE isConnectionOpen(VALUE);
static VALUE isConnectionClosed(VALUE);
static VALUE closeConnection(VALUE);
static VALUE getConnectionDatabase(VALUE);
static VALUE startConnectionTransaction(VALUE);
static VALUE connectionToString(VALUE);
static VALUE executeOnConnection(VALUE, VALUE, VALUE);
static VALUE executeOnConnectionImmediate(VALUE, VALUE);
static VALUE getConnectionUser(VALUE);
VALUE startTransactionBlock(VALUE);
VALUE startTransactionRescue(VALUE, VALUE);
VALUE executeBlock(VALUE);
VALUE executeRescue(VALUE, VALUE);
VALUE executeImmediateBlock(VALUE);
VALUE executeImmediateRescue(VALUE, VALUE);
char *createDPB(VALUE, VALUE, VALUE, short *);

/* Globals. */
VALUE cConnection;


/**
 * This function provides the allocation functionality for the Connection
 * class.
 *
 * @param  klass  A reference to the Connection Class object.
 *
 * @return  A reference to the newly created instance.
 *
 */
static VALUE allocateConnection(VALUE klass) {
  VALUE instance    = Qnil;
  ConnectionHandle *connection = ALLOC(ConnectionHandle);

  if(connection != NULL) {
    /* Wrap the structure in a class. */
    connection->handle = 0;
    instance = Data_Wrap_Struct(klass, NULL, connectionFree, connection);
  } else {
    rb_raise(rb_eNoMemError,
             "Memory allocation failure creating a connection.");
  }

  return(instance);
}


/**
 * This function provides the initialize method for the Connection class.
 *
 * @param  argc      A count of the total number of arguments passed to the
 *                   function.
 * @param  argv      A pointer to an array of VALUEs that contain the arguments
 *                   to the function.
 * @param  self      A reference to the object being initialized.
 *
 * @return  A reference to the initialized object.
 *
 */
static VALUE initializeConnection(int argc, VALUE *argv, VALUE self) {
  ConnectionHandle *connection = NULL;
  ISC_STATUS status[ISC_STATUS_LENGTH], attach_result;
  short length   = 0;
  char             *file    = NULL,
  *dpb     = NULL;
  VALUE user     = Qnil,
        password = Qnil,
        options  = Qnil,
        tmp_str  = Qnil;

  if(argc < 1) {
    rb_raise(rb_eArgError, "Wrong number of arguments (%d for %d).", argc, 1);
  }

  if(TYPE(argv[0]) != T_DATA ||
     RDATA(argv[0])->dfree != (RUBY_DATA_FUNC)databaseFree) {
    rb_fireruby_raise(NULL, "Invalid database specified for connection.");
  }

  tmp_str = rb_iv_get(argv[0], "@file");
  file = StringValuePtr(tmp_str);

  Data_Get_Struct(self, ConnectionHandle, connection);

  /* Extract parameters. */
  if(argc > 1) {
    user = argv[1];
  }
  if(argc > 2) {
    password = argv[2];
  }
  if(argc > 3) {
    options = argv[3];
  }

  /* Open the connection connection. */
  dpb = createDPB(user, password, options, &length);
  attach_result = isc_attach_database(status, strlen(file), file, &connection->handle, length, dpb);
  free(dpb);

  if(attach_result != 0) {
    /* Generate an error. */
    rb_fireruby_raise(status, "Error opening database connection.");
  }

  /* Store connection attributes. */
  rb_iv_set(self, "@database", argv[0]);
  rb_iv_set(self, "@user", user);
  rb_iv_set(self, "@transactions", rb_ary_new());

  return(self);
}


/**
 * This function provides the open? method for the Connection class.
 *
 * @param  self  A reference to the object that the call is being made on.
 *
 * @return  Qtrue if the connection is open, Qfalse if it is closed.
 *
 */
static VALUE isConnectionOpen(VALUE self) {
  VALUE result      = Qfalse;
  ConnectionHandle *connection = NULL;

  Data_Get_Struct(self, ConnectionHandle, connection);
  if(connection->handle != 0) {
    result = Qtrue;
  }

  return(result);
}


/**
 * This function provides the closed? method for the Connection class.
 *
 * @param  self  A reference to the object that the call is being made on.
 *
 * @return  Qtrue if the connection is closed, Qfalse if it is open.
 *
 */
static VALUE isConnectionClosed(VALUE self) {
  return(isConnectionOpen(self) == Qtrue ? Qfalse : Qtrue);
}


/**
 * This method provides the close method for the Connection class.
 *
 * @param  self  A reference to the object that the call is being made on.
 *
 * @return  A reference to the closed Connection on success, nil otherwise or
 *          if the method is called on a closed Connection.
 *
 */
static VALUE closeConnection(VALUE self) {
  VALUE result      = Qnil;
  ConnectionHandle *connection = NULL;

  Data_Get_Struct(self, ConnectionHandle, connection);
  if(connection->handle != 0) {
    VALUE transactions = rb_iv_get(self, "@transactions"),
          transaction  = Qnil;
    ISC_STATUS status[ISC_STATUS_LENGTH];

    /* Roll back an outstanding transactions. */
    while((transaction = rb_ary_pop(transactions)) != Qnil) {
      VALUE active = rb_funcall(transaction, rb_intern("active?"), 0);

      if(active == Qtrue) {
        rb_funcall(transaction, rb_intern("rollback"), 0);
      }
    }

    /* Detach from the database. */
    if(isc_detach_database(status, &connection->handle) == 0) {
      connection->handle = 0;
      result             = self;
    } else {
      /* Generate an error. */
      rb_fireruby_raise(status, "Error closing connection.");
    }
  }

  return(result);
}


/**
 * This function retrieves the connection associated with a Connection object.
 *
 * @param  self  A reference to the object that the call is being made on.
 *
 * @return  A reference to the Connection connection.
 *
 */
static VALUE getConnectionDatabase(VALUE self) {
  return(rb_iv_get(self, "@database"));
}


/**
 * This function provides the start_transaction method for the Database class.
 *
 * @param  self  A reference to the Database object to start the transaction
 *               on.
 *
 * @return  A reference to a Transaction object or nil if a problem occurs.
 *
 */
static VALUE startConnectionTransaction(VALUE self) {
  VALUE result = rb_transaction_new(self);

  if(rb_block_given_p()) {
    result = rb_rescue(startTransactionBlock, result,
                       startTransactionRescue, result);
  }

  return(result);
}


/**
 * This method provides the to_s method for the Connection class.
 *
 * @param  self  A reference to the Connection object that the method will be
 *               called on.
 *
 * @return  A reference to a String object describing the connection.
 *
 */
static VALUE connectionToString(VALUE self) {
  VALUE result      = rb_str_new2("(CLOSED)");
  ConnectionHandle *connection = NULL;

  Data_Get_Struct(self, ConnectionHandle, connection);
  if(connection->handle != 0) {
    VALUE database = rb_iv_get(self, "@database"),
          user     = rb_iv_get(self, "@user"),
          file     = rb_iv_get(database, "@file");
    char text[256];

    sprintf(text, "%s@%s (OPEN)", StringValuePtr(user), StringValuePtr(file));
    result = rb_str_new2(text);
  }

  return(result);
}


/**
 * This function provides the execute method for the Connection class.
 *
 * @param  self         A reference to the connection object to perform the
 *                      execution through.
 * @param  sql          A reference to the SQL statement to be executed.
 * @param  transaction  A reference to the transction that the statement will
 *                      be executed under.
 *
 * @return  Either a ResultSet object for a query statement or nil for a
 *          non-query statement.
 *
 */
static VALUE executeOnConnection(VALUE self, VALUE sql, VALUE transaction) {
  VALUE results   = Qnil,
        statement = rb_statement_new(self, transaction, sql, INT2FIX(3));

  results = rb_execute_statement(statement);
  if(results != Qnil && rb_obj_is_kind_of(results, rb_cInteger) == Qfalse) {
    if(rb_block_given_p()) {
      VALUE row  = rb_funcall(results, rb_intern("fetch"), 0),
            last = Qnil;

      while(row != Qnil) {
        last = rb_yield(row);
        row  = rb_funcall(results, rb_intern("fetch"), 0);
      }
      rb_funcall(results, rb_intern("close"), 0);
      results = last;
    }
  }
  rb_statement_close(statement);

  return(results);
}


/**
 * This function provides the execute_immediate method for the Connection class.
 *
 * @param  self  A reference to the connection object to perform the execution
 *               through.
 * @param  sql   A reference to the SQL statement to be executed.
 *
 * @return  Always returns nil.
 *
 */
static VALUE executeOnConnectionImmediate(VALUE self, VALUE sql) {
  VALUE transaction = rb_transaction_new(self),
        set         = Qnil,
        results     = Qnil,
        array       = rb_ary_new(),
        dialect     = INT2FIX(3),
        statement   = rb_statement_new(self, transaction, sql, dialect);

  rb_ary_push(array, self);
  rb_ary_push(array, transaction);
  rb_ary_push(array, sql);
  rb_ary_push(array, statement);

  set = rb_rescue(executeBlock, array, executeRescue, array);
  if(set != Qnil) {
    if(TYPE(set) == T_DATA &&
       RDATA(set)->dfree == (RUBY_DATA_FUNC)resultSetFree) {
      rb_assign_transaction(set, transaction);
      if(rb_block_given_p()) {
        results = rb_rescue(executeImmediateBlock, set,
                            executeImmediateRescue, set);
      } else {
        results = set;
      }
    } else {
      rb_funcall(transaction, rb_intern("commit"), 0);
      results = set;
    }
  } else {
    rb_funcall(transaction, rb_intern("commit"), 0);
  }

  return(results);
}


/**
 * This function provides the user accessor method for the Connection object.
 *
 * @param  self  A reference to the Connection object to fetch theuser from.
 *
 * @return  A reference to the user name used to establish the connection.
 *
 */
static VALUE getConnectionUser(VALUE self) {
  return(rb_iv_get(self, "@user"));
}


/**
 * This function provides the block handling capabilities for the
 * start_transaction method.
 *
 * @param  transaction  The Transaction object that was created for the block.
 *
 * @return  A reference to the return value provided by the block.
 *
 */
VALUE startTransactionBlock(VALUE transaction) {
  VALUE result = rb_yield(transaction);

  rb_funcall(transaction, rb_intern("commit"), 0);

  return(result);
}


/**
 * This function provides the rescue handling capabilities for the
 * start_transaction block handling functionality.
 *
 * @param  transaction  A reference to the Transaction object that was created
 * @param  error        A reference to details relating to the exception raised.
 *                      for the block.
 *
 * @return  Would be nil but always throws an exception.
 *
 */
VALUE startTransactionRescue(VALUE transaction, VALUE error) {
  rb_funcall(transaction, rb_intern("rollback"), 0);
  rb_exc_raise(error);
  return(Qnil);
}


/**
 * This function is used to wrap the call to the executeOnConnection() function
 * made by the executeOnConnectionImmediate() function to help insure that the
 * transaction is rolled back in case of an error.
 *
 * @param  array  An array of the parameters for the function to use.
 *
 * @return  The ResultSet object generated by execution or nil if it wasn't a
 *          query.
 *
 */
VALUE executeBlock(VALUE array) {
  VALUE result      = Qnil,
        connection  = rb_ary_entry(array, 0),
        transaction = rb_ary_entry(array, 1),
        sql         = rb_ary_entry(array, 2),
        statement   = rb_ary_entry(array, 3);

  result = rb_execute_statement(statement);
  rb_statement_close(statement);

  return(result);
}


/**
 * This function provides clean up for the execution of a block associated
 * with the execute method.
 *
 * @param  array        An array of the parameters for the function to use.
 * @param  error        A reference to details relating to the exception raised.
 *
 * @return  Would always returns nil except that it always raises an exception.
 *
 */
VALUE executeRescue(VALUE array, VALUE error) {
  VALUE transaction = rb_ary_entry(array, 1),
        statement   = rb_ary_entry(array, 3);

  rb_funcall(transaction, rb_intern("rollback"), 0);
  rb_statement_close(statement);
  rb_exc_raise(error);
  return(Qnil);
}


/**
 * This function is executed to process a block passed to the execute_immedate
 * method.
 *
 * @param  set  A reference to the ResultSet to be processed by the block.
 *
 * @return  A reference to the return value generated by the block.
 *
 */
VALUE executeImmediateBlock(VALUE set) {
  VALUE result = Qnil,
        row    = rb_funcall(set, rb_intern("fetch"), 0);

  while(row != Qnil) {
    result = rb_yield(row);
    row    = rb_funcall(set, rb_intern("fetch"), 0);
  }
  rb_funcall(set, rb_intern("close"), 0);

  return(result);
}


/**
 * This function provides clean up for the execution of a block associated
 * with the execute_immediate method.
 *
 * @param  set    A reference to the ResultSet object for the block.
 * @param  error  A reference to details relating to the exception raised.
 *
 * @return  Would always returns nil except that it always raises an exception.
 *
 */
VALUE executeImmediateRescue(VALUE set, VALUE error) {
  rb_funcall(set, rb_intern("close"), 0);
  rb_exc_raise(error);
  return(Qnil);
}


/**
 * This method creates a database parameter buffer to be used in creating a
 * database connection.
 *
 * @param  user      A reference to a string containing the user name to be used
 *                   in making the connection.
 * @param  password  A reference to a string containing the password to be used
 *                   in making the connection.
 * @param  options   A hash of the options to be used in making the connection
 *                   to the database.
 * @param  length    A pointer to a short integer that will be set to the
 *                   length of the buffer.
 *
 * @return  A pointer to an array of characters containing the database
 *          parameter buffer.
 *
 */
char *createDPB(VALUE user, VALUE password, VALUE options, short *length) {
  char *dpb = NULL;
  VALUE keys;
  VALUE entry;
  int i;
  short type;

  /* Determine the dpb length and allocate it. */
  *length = 1;
  if(user != Qnil) {
    *length += strlen(StringValuePtr(user)) + 2;
  }
  if(password != Qnil) {
    *length += strlen(StringValuePtr(password)) + 2;
  }
  if(options != Qnil) {
    keys = rb_funcall(options, rb_intern("keys"), 0);

    for(i = 0; i < RARRAY_LEN(keys); i++) {
      type = FIX2INT(rb_ary_entry(keys, i));

      switch (type) {
      case isc_dpb_sql_role_name:
      case isc_dpb_lc_messages:
      case isc_dpb_lc_ctype:
      case isc_dpb_reserved:
      {
        entry = rb_hash_aref(options, INT2FIX(type));
        *length += strlen(StringValuePtr(entry)) + 2;
        break;
      }
      default:
      {
        *length += 3;
      }
      }
    }
  }
  dpb = ALLOC_N(char, *length);

  /* Populate the buffer. */
  if(dpb != NULL) {
    char *ptr = NULL;
    int size = 0;

    /* Fill out the DPB. */
    memset(dpb, 0, *length);
    dpb[0] = isc_dpb_version1;
    ptr    = &dpb[1];

    if(user != Qnil) {
      char *username = StringValuePtr(user);

      size   = strlen(username);
      *ptr++ = isc_dpb_user_name;
      *ptr++ = (char)size;
      memcpy(ptr, username, size);
      ptr    = ptr + size;
    }

    if(password != Qnil) {
      char *userpwd  = StringValuePtr(password);

      size   = strlen(userpwd);
      *ptr++ = isc_dpb_password;
      *ptr++ = (char)size;
      memcpy(ptr, userpwd, size);
      ptr    = ptr + size;
    }

    if(options != Qnil) {
      for(i = 0; i < RARRAY_LEN(keys); i++) {
        type = FIX2INT(rb_ary_entry(keys, i));
        entry = rb_hash_aref(options, INT2FIX(type));

        switch (type) {
        case isc_dpb_sql_role_name:
        case isc_dpb_lc_messages:
        case isc_dpb_lc_ctype:
        case isc_dpb_reserved:
        {
          char *text = StringValuePtr(entry);

          size   = strlen(text);
          *ptr++ = type;
          *ptr++ = (char)size;
          memcpy(ptr, text, size);
          ptr    = ptr + size;
          break;
        }
        default:
        {
          short value;
          switch (TYPE(entry)) {
          case T_FIXNUM: value = FIX2INT(entry);
          case T_NIL: value = 0;
          case T_FALSE: value = 0;
          case T_TRUE: value = 1;
          case T_UNDEF: value = 0;
          case T_FLOAT: value = NUM2INT(entry);
          case T_BIGNUM: value = NUM2INT(entry);
          default: value = 0;
          }

          *ptr++ = type;
          *ptr++ = (char)1;
          *ptr++ = value;
        }
        }
      }
    }
  } else {
    /* Generate an error. */
    rb_raise(rb_eNoMemError,
             "Memory allocation failure creating database DPB.");
  }

  return(dpb);
}


/**
 * This function allows integration with the Ruby garbage collector to insure
 * that the resources associated with a Connection object are released.
 *
 * @param  connection  A pointer to the ConnectionHandle structure associated
 *                     with a Connection object.
 *
 */
void connectionFree(void *connection) {
  if(connection != NULL) {
    ConnectionHandle *handle = (ConnectionHandle *)connection;

    if(handle->handle != 0) {
      ISC_STATUS status[ISC_STATUS_LENGTH];

      isc_detach_database(status, &handle->handle);
    }
    free(handle);
  }
}


/**
 * This function provides a programatic way of creating a new Connection
 * object.
 *
 * @param  database  A reference to the database that the connection will relate
 *                   to.
 * @param  user      A reference to the database user name to be used in making
 *                   the connection.
 * @param  password  A reference to the database password to be used in making
 *                   the connection.
 * @param  options   A hash of the options to be used in creating the connection
 *                   object.
 *
 * @return  A reference to the newly created Connection object.
 *
 */
VALUE rb_connection_new(VALUE database, VALUE user, VALUE password, VALUE options) {
  VALUE connection = allocateConnection(cConnection),
        parameters[4];

  parameters[0] = database;
  parameters[1] = user;
  parameters[2] = password;
  parameters[3] = options;

  initializeConnection(4, parameters, connection);

  return(connection);
}


/**
 * This function is called to record the beginnings of a transactions against
 * a related connection.
 *
 * @param  transaction  A reference to the newly created Transaction object.
 * @param  connection   Either a reference to a Connection object or an Array
 *                      of Connection objects that are included in the
 *                      transaction.
 *
 */
void rb_tx_started(VALUE transaction, VALUE connection) {
  VALUE array  = TYPE(connection) == T_ARRAY ? connection : rb_ary_new(),
        number = Qnil;
  long size   = 0,
       index;

  if(TYPE(connection) != T_ARRAY) {
    rb_ary_push(array, connection);
  }
  number = rb_funcall(array, rb_intern("size"), 0);
  size   = TYPE(number) == T_FIXNUM ? FIX2INT(number) : NUM2INT(number);

  for(index = 0; index < size; index++) {
    VALUE entry = rb_ary_entry(array, index),
          list  = rb_iv_get(entry, "@transactions");

    rb_ary_push(list, transaction);
  }
}


/**
 * This function is invoked by a Transaction object whenever it is committed or
 * rolled back. The connection can then discount the Transaction from its list
 * of transaction to be cleaned up and close time.
 *
 * @param  connection   A reference to the Connection object or an array of
 *                      Connection objects that is to be informed about the
 *                      transaction.
 * @param  transaction  A reference to the Transaction object that is to be
 *                      released.
 *
 */
void rb_tx_released(VALUE connection, VALUE transaction) {
  VALUE array  = TYPE(connection) == T_ARRAY ? connection : rb_ary_new(),
        number = Qnil;
  long size   = 0,
       index;

  if(TYPE(connection) != T_ARRAY) {
    rb_ary_push(array, connection);
  }
  number = rb_funcall(array, rb_intern("size"), 0);
  size   = TYPE(number) == T_FIXNUM ? FIX2INT(number) : NUM2INT(number);

  for(index = 0; index < size; index++) {
    VALUE entry = rb_ary_entry(array, index),
          list  = rb_iv_get(entry, "@transactions");

    rb_ary_delete(list, transaction);
  }
}


/**
 * This function initializes the Connection class within the Ruby environment.
 * The class is established under the module specified to the function.
 *
 * @param  module  A reference to the module to create the class within.
 *
 */
void Init_Connection(VALUE module) {
  cConnection = rb_define_class_under(module, "Connection", rb_cObject);
  rb_define_alloc_func(cConnection, allocateConnection);
  rb_define_method(cConnection, "initialize", initializeConnection, -1);
  rb_define_method(cConnection, "initialize_copy", forbidObjectCopy, 1);
  rb_define_method(cConnection, "user", getConnectionUser, 0);
  rb_define_method(cConnection, "open?", isConnectionOpen, 0);
  rb_define_method(cConnection, "closed?", isConnectionClosed, 0);
  rb_define_method(cConnection, "close", closeConnection, 0);
  rb_define_method(cConnection, "database", getConnectionDatabase, 0);
  rb_define_method(cConnection, "start_transaction", startConnectionTransaction, 0);
  rb_define_method(cConnection, "to_s", connectionToString, 0);
  rb_define_method(cConnection, "execute", executeOnConnection, 2);
  rb_define_method(cConnection, "execute_immediate", executeOnConnectionImmediate, 1);

  rb_define_const(cConnection, "MARK_DATABASE_DAMAGED", INT2FIX(isc_dpb_damaged));
  rb_define_const(cConnection, "WRITE_POLICY", INT2FIX(isc_dpb_force_write));
  rb_define_const(cConnection, "CHARACTER_SET", INT2FIX(isc_dpb_lc_ctype));
  rb_define_const(cConnection, "MESSAGE_FILE", INT2FIX(isc_dpb_lc_messages));
  rb_define_const(cConnection, "NUMBER_OF_CACHE_BUFFERS", INT2FIX(isc_dpb_num_buffers));
  rb_define_const(cConnection, "DBA_USER_NAME", INT2FIX(isc_dpb_sys_user_name));
  rb_define_const(cConnection, "SQL_ROLE_NAME", INT2FIX(isc_dpb_sql_role_name));
  rb_define_const(cConnection, "WRITE_ASYNCHRONOUS", INT2FIX(0));
  rb_define_const(cConnection, "WRITE_SYNCHRONOUS", INT2FIX(1));
}
