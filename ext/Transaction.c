/*------------------------------------------------------------------------------
 * Transaction.c
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
#include "Transaction.h"
#include "Common.h"
#include "Connection.h"
#include "ResultSet.h"
#include "Statement.h"

/* Function prototypes. */
static VALUE allocateTransaction(VALUE);
static VALUE commitTransaction(VALUE);
static VALUE rollbackTransaction(VALUE);
static VALUE getTransactionConnections(VALUE);
static VALUE isTransactionFor(VALUE, VALUE);
static VALUE executeOnTransaction(VALUE, VALUE);
static VALUE createTransaction(VALUE, VALUE, VALUE);
void startTransaction(TransactionHandle *, VALUE, long, char *);
void transactionFree(void *);

/* Globals. */
VALUE cTransaction;

/* Type definitions. */
typedef struct {
  isc_db_handle *database;
  long length;
  char          *tpb;
} ISC_TEB;

static char DEFAULT_TEB[]    = {isc_tpb_version3,
                                isc_tpb_write,
                                isc_tpb_wait,
                                isc_tpb_rec_version,
                                isc_tpb_read_committed};
static int DEFAULT_TEB_SIZE = 5;


/**
 * This function provides for the allocation of a new object of the Transaction
 * class.
 *
 * @param  klass  A reference to the Transaction Class object
 *
 * @return  A reference to the newly allocated object.
 *
 */
static VALUE allocateTransaction(VALUE klass) {
  VALUE transaction = Qnil;
  TransactionHandle *handle     = ALLOC(TransactionHandle);

  if(handle != NULL) {
    handle->handle = 0;
    transaction = Data_Wrap_Struct(klass, NULL, transactionFree, handle);
  } else {
    rb_raise(rb_eNoMemError,
             "Memory allocation failure allocating a transaction.");
  }

  return(transaction);
}


/**
 * This function provides the initialize method for the Transaction class.
 *
 * @param  self         A reference to the new Transaction class instance.
 * @param  connections  Either a reference to a single Connection object or to
 *                      an array of Connection objects that the transaction
 *                      will apply to.
 *
 */
static VALUE transactionInitialize(VALUE self, VALUE connections) {
  TransactionHandle *transaction = NULL;
  VALUE array        = Qnil;

  /* Determine if an array has been passed as a parameter. */
  if(TYPE(connections) == T_ARRAY) {
    array = connections;
  } else if(TYPE(connections) == T_DATA &&
            RDATA(connections)->dfree == (RUBY_DATA_FUNC)connectionFree) {
    array = rb_ary_new();
    rb_ary_push(array, connections);
  } else {
    rb_fireruby_raise(NULL,
                      "Invalid connection parameter(s) for transaction.");
  }

  /* Store the database details. */
  rb_iv_set(self, "@connections", array);

  /* Fetch the data structure and start the transaction. */
  Data_Get_Struct(self, TransactionHandle, transaction);
  startTransaction(transaction, array, 0, NULL);
  rb_tx_started(self, array);

  return(self);
}


/**
 * This function provides the commit method for the Transaction class.
 *
 * @param  self  A reference to the Transaction object being committed.
 *
 * @return  A reference to self if successful, nil otherwise.
 *
 */
static VALUE commitTransaction(VALUE self) {
  TransactionHandle *transaction = NULL;

  Data_Get_Struct(self, TransactionHandle, transaction);
  
  /* Commit the transaction. */
  if(transaction->handle != 0) {
    ISC_STATUS status[ISC_STATUS_LENGTH];

    if(isc_commit_transaction(status, &transaction->handle) != 0) {
      /* Generate an error. */
      rb_fireruby_raise(status, "Error committing transaction.");
    }
    transaction->handle = 0;
  } else {
    /* Generate an error. */
    rb_fireruby_raise(NULL, "1. Transaction is not active.");
  }

  /* Notify each connection of the transactions end. */
  rb_tx_released(rb_iv_get(self, "@connections"), self);

  /* Clear the connections list. */
  rb_iv_set(self, "@connections", rb_ary_new());

  return(self);
}


/**
 * This function provides the rollback method for the Transaction class.
 *
 * @param  self  A reference to the Transaction object being rolled back.
 *
 * @return  A reference to self if successful, nil otherwise.
 *
 */
static VALUE rollbackTransaction(VALUE self) {
  TransactionHandle *transaction = NULL;

  Data_Get_Struct(self, TransactionHandle, transaction);

  /* Roll back the transaction. */
  if(transaction->handle != 0) {
    ISC_STATUS status[ISC_STATUS_LENGTH];

    if(isc_rollback_transaction(status, &transaction->handle) != 0) {
      /* Generate an error. */
      rb_fireruby_raise(status, "Error rolling back transaction.");
    }
    transaction->handle = 0;
  } else {
    /* Generate an error. */
    rb_fireruby_raise(NULL, "2. Transaction is not active.");
  }

  /* Notify each connection of the transactions end. */
  rb_tx_released(rb_iv_get(self, "@connections"), self);

  /* Clear the connections list. */
  rb_iv_set(self, "@connections", rb_ary_new());

  return(self);
}


/**
 * This function provides the active? method for the Transaction class.
 *
 * @param  self  A reference to the Transcation object to perform the check for.
 *
 * @return  Qtrue if the transaction is active, Qfalse otherwise.
 *
 */
static VALUE transactionIsActive(VALUE self) {
  VALUE result       = Qfalse;
  TransactionHandle *transaction = NULL;

  Data_Get_Struct(self, TransactionHandle, transaction);
  if(transaction->handle != 0) {
    result = Qtrue;
  }

  return(result);
}


/**
 * This function provides the accessor for the connections Transaction class
 * attribute.
 *
 * @param  self  A reference to the Transaction object to fetch the connections
 *               for.
 *
 * @return  An Array containing the connections associated with the Transaction
 *          object.
 *
 */
static VALUE getTransactionConnections(VALUE self) {
  return(rb_iv_get(self, "@connections"));
}


/**
 * This function provides the for? method of the Transaction class.
 *
 * @param  self        A reference to the Transaction object to make the check
 *                     on
 * @param  connection  A reference to the Database object to make the check for.
 *
 * @return  Qtrue if the specified database is covered by the Transaction,
 *          Qfalse otherwise.
 *
 */
static VALUE isTransactionFor(VALUE self, VALUE connection) {
  VALUE result    = Qfalse,
        array     = rb_iv_get(self, "@connections"),
        value     = rb_funcall(array, rb_intern("size"), 0);
  int size      = 0,
      index;
  ConnectionHandle  *instance = NULL;
  TransactionHandle *transaction = NULL;

  size = (TYPE(value) == T_FIXNUM ? FIX2INT(value) : NUM2INT(value));
  Data_Get_Struct(self, TransactionHandle, transaction);
  Data_Get_Struct(connection, ConnectionHandle, instance);

  for(index = 0; index < size && result == Qfalse; index++) {
    ConnectionHandle *handle;

    value = rb_ary_entry(array, index);
    Data_Get_Struct(value, ConnectionHandle, handle);
    result = (handle == instance ? Qtrue : Qfalse);
  }

  return(result);
}


/**
 * This function provides the execute method for the Transaction class. This
 * method only works when a transaction applies to a single connection.
 *
 * @param  self  A reference to the Transaction object that the execute has
 *               been called for.
 * @param  sql   A reference to the SQL statement to be executed.
 *
 * @return  A reference to a result set of the SQL statement represents a
 *          query, nil otherwise.
 *
 */
static VALUE executeOnTransaction(VALUE self, VALUE sql) {
  VALUE results      = Qnil,
        list         = rb_iv_get(self, "@connections"),
        value        = 0,
        connection   = Qnil,
        statement    = Qnil;
  TransactionHandle *transaction = NULL;
  int size         = 0;

  /* Check that the transaction is active. */
  Data_Get_Struct(self, TransactionHandle, transaction);
  if(transaction->handle == 0) {
    rb_fireruby_raise(NULL, "Executed called on inactive transaction.");
  }

  /* Check that we have only one connection for the transaction. */
  value = rb_funcall(list, rb_intern("size"), 0);
  size  = (TYPE(value) == T_FIXNUM ? FIX2INT(value) : NUM2INT(value));
  if(size > 1) {
    rb_fireruby_raise(NULL,
                      "Execute called on a transaction that spans multiple " \
                      "connections. Unable to determine which connection to " \
                      "execute the SQL statement through.");
  }

  connection = rb_ary_entry(list, 0);
  return rb_execute_sql(connection, sql, rb_ary_new(), self);
}


/**
 * This function creates a new Transaction object for a connection. This method
 * provides the create class method for the Transaction class and allows for the
 * specification of transaction parameters.
 *
 * @param  unused       Like it says, not used.
 * @param  connections  A reference to the array of Connection objects that the
 *                      transaction will be associated with.
 * @param  parameters   A reference to an array containing the parameters to be
 *                      used in creating the transaction.
 *
 * @return  A reference to the newly created Transaction object.
 *
 */
static VALUE createTransaction(VALUE unused, VALUE connections, VALUE parameters) {
  VALUE instance     = Qnil,
        list         = Qnil;
  TransactionHandle *transaction = ALLOC(TransactionHandle);

  if(transaction == NULL) {
    rb_raise(rb_eNoMemError,
             "Memory allocation failure allocating a transaction.");
  }
  transaction->handle = 0;

  if(TYPE(connections) != T_ARRAY) {
    list = rb_ary_new();
    rb_ary_push(list, connections);
  } else {
    list = connections;
  }

  if(TYPE(parameters) != T_ARRAY) {
    rb_fireruby_raise(NULL,
                      "Invalid transaction parameter set specified.");
  }

  if(transaction != NULL) {
    VALUE value   = rb_funcall(parameters, rb_intern("size"), 0);
    long size    = 0,
         index;
    char  *buffer = NULL;

    size = TYPE(value) == T_FIXNUM ? FIX2INT(value) : NUM2INT(value);
    if((buffer = ALLOC_N(char, size)) != NULL) {
      for(index = 0; index < size; index++) {
        VALUE entry  = rb_ary_entry(parameters, index);
        int number = 0;

        number = TYPE(entry) == T_FIXNUM ? FIX2INT(entry) : NUM2INT(entry);
        buffer[index] = number;
      }

      startTransaction(transaction, list, size, buffer);
      free(buffer);

      instance = Data_Wrap_Struct(cTransaction, NULL, transactionFree,
                                  transaction);
      rb_iv_set(instance, "@connections", list);
      rb_tx_started(instance, connections);
    } else {
      rb_fireruby_raise(NULL,
                        "Memory allocation failure allocating transaction " \
                        "parameter buffer.");
    }
  } else {
    rb_raise(rb_eNoMemError,
             "Memory allocation failure allocating transaction.");
  }

  return(instance);
}


/**
 * This function begins a transaction on all of the database associated with
 * the Transaction object.
 *
 * @param  transaction  A reference to the Transaction object being started.
 * @param  connections  An array of the databases that the transaction applies
 *                      to.
 * @param  size         The length of the transaction parameter buffer passed
 *                      to the function.
 * @param  buffer       The transaction parameter buffer to be used in creating
 *                      the transaction.
 *
 */
void startTransaction(TransactionHandle *transaction,
                      VALUE connections,
                      long size,
                      char *buffer) {
  ISC_TEB          *teb   = NULL;
  VALUE value  = rb_funcall(connections, rb_intern("size"), 0),
        head   = rb_ary_entry(connections, 0);
  short length = 0;
  ConnectionHandle *first = NULL;

  /* Attempt a retrieval of the first connection. */
  if(value != Qnil) {
    /* Check that we have a connection. */
    if(TYPE(head) == T_DATA &&
       RDATA(head)->dfree == (RUBY_DATA_FUNC)connectionFree) {
      Data_Get_Struct(head, ConnectionHandle, first);
    }
  }

  /* Generate the list of connections. */
  length = (TYPE(value) == T_FIXNUM ? FIX2INT(value) : NUM2INT(value));
  if(length > 0) {
    if((teb = ALLOC_N(ISC_TEB, length)) != NULL) {
      int i;

      for(i = 0; i < length; i++) {
        VALUE entry = rb_ary_entry(connections, i);

        /* Check that we have a connection. */
        if(TYPE(entry) == T_DATA &&
           RDATA(entry)->dfree == (RUBY_DATA_FUNC)connectionFree) {
          ConnectionHandle *connection = NULL;

          Data_Get_Struct(entry, ConnectionHandle, connection);
          if(connection->handle != 0) {
            /* Store the connection details. */
            teb[i].database = &connection->handle;
            if(size > 0) {
              teb[i].length = size;
              teb[i].tpb    = buffer;
            } else {
              teb[i].length = DEFAULT_TEB_SIZE;
              teb[i].tpb    = DEFAULT_TEB;
            }
          } else {
            /* Clean up and raise an exception. */
            free(teb);
            rb_fireruby_raise(NULL,
                              "Disconnected connection specified " \
                              "starting a transaction.");
          }
        } else {
          /* Clean up and thrown an exception. */
          free(teb);
          rb_fireruby_raise(NULL,
                            "Invalid connection specified starting a " \
                            "transaction.");
        }
      }
    } else {
      rb_raise(rb_eNoMemError,
               "Memory allocation error starting transaction.");
    }
  } else {
    /* Generate an exception. */
    rb_fireruby_raise(NULL, "No connections specified for transaction.");
  }

  /* Check that theres been no errors and that we have a connection list. */
  if(teb != NULL) {
    ISC_STATUS status[ISC_STATUS_LENGTH];

    /* Attempt a transaction start. */
    if(isc_start_multiple(status, &transaction->handle, length, teb) != 0) {
      /* Generate an error. */
      rb_fireruby_raise(status, "Error starting transaction.");
    }
  }

  /* Free the database details list if need be. */
  if(teb != NULL) {
    free(teb);
  }
  
}


/**
 * This function is used to integrate with the Ruby garbage collector to insure
 * that the resources associated with a Transaction object are released when the
 * collector comes a calling.
 *
 * @param  transaction  A pointer to the TransactionHandle structure associated
 *                      with the Transaction object being collected.
 *
 */
void transactionFree(void *transaction) {
  if(transaction != NULL) {
    TransactionHandle *handle = (TransactionHandle *)transaction;

    if(handle->handle != 0) {
      ISC_STATUS status[ISC_STATUS_LENGTH];

      isc_rollback_transaction(status, &handle->handle);
    }
    free(handle);
  }
}


/**
 * This function provides a programmatic method of creating a Transaction
 * object.
 *
 * @param  connections  Either an single Connection object or an array of
 *                      Connection objects that the transaction will apply
 *                      to.
 *
 * @return  A reference to the Transaction object, or nil if an error occurs.
 *
 */
VALUE rb_transaction_new(VALUE connections) {
  VALUE transaction = allocateTransaction(cTransaction);

  transactionInitialize(transaction, connections);

  return(transaction);
}

/**
 * This function provides a convenient means of checking whether a connection
 * is covered by a transaction.
 *
 * @param  transaction  A reference to the transaction to perform the check on.
 * @param  connection   A reference to the connection to perform the check for.
 *
 * @return  Non-zero if the connection is covered by the transaction, zero if
 *          it is not.
 *
 */
int coversConnection(VALUE transaction, VALUE connection) {
  int result  = 0;
  VALUE boolean = isTransactionFor(transaction, connection);

  if(boolean == Qtrue) {
    result = 1;
  }

  return(result);
}


/**
 * This function initializes the Transaction class within the Ruby environment.
 * The class is established under the module specified to the function.
 *
 * @param  module  A reference to the module to create the class within.
 *
 */
void Init_Transaction(VALUE module) {
  cTransaction = rb_define_class_under(module, "Transaction", rb_cObject);
  rb_define_alloc_func(cTransaction, allocateTransaction);
  rb_define_method(cTransaction, "initialize", transactionInitialize, 1);
  rb_define_method(cTransaction, "initialize_copy", forbidObjectCopy, 1);
  rb_define_method(cTransaction, "active?", transactionIsActive, 0);
  rb_define_method(cTransaction, "commit", commitTransaction, 0);
  rb_define_method(cTransaction, "rollback", rollbackTransaction, 0);
  rb_define_method(cTransaction, "connections", getTransactionConnections, 0);
  rb_define_method(cTransaction, "for_connection?", isTransactionFor, 1);
  rb_define_module_function(cTransaction, "create", createTransaction, 2);
  rb_define_method(cTransaction, "execute", executeOnTransaction, 1);
  rb_define_const(cTransaction, "TPB_VERSION_1", INT2FIX(isc_tpb_version1));
  rb_define_const(cTransaction, "TPB_VERSION_3", INT2FIX(isc_tpb_version3));
  rb_define_const(cTransaction, "TPB_CONSISTENCY", INT2FIX(isc_tpb_consistency));
  rb_define_const(cTransaction, "TPB_CONCURRENCY", INT2FIX(isc_tpb_concurrency));
  rb_define_const(cTransaction, "TPB_SHARED", INT2FIX(isc_tpb_shared));
  rb_define_const(cTransaction, "TPB_PROTECTED", INT2FIX(isc_tpb_protected));
  rb_define_const(cTransaction, "TPB_EXCLUSIVE", INT2FIX(isc_tpb_exclusive));
  rb_define_const(cTransaction, "TPB_WAIT", INT2FIX(isc_tpb_wait));
  rb_define_const(cTransaction, "TPB_NO_WAIT", INT2FIX(isc_tpb_nowait));
  rb_define_const(cTransaction, "TPB_READ", INT2FIX(isc_tpb_read));
  rb_define_const(cTransaction, "TPB_WRITE", INT2FIX(isc_tpb_write));
  rb_define_const(cTransaction, "TPB_LOCK_READ", INT2FIX(isc_tpb_lock_read));
  rb_define_const(cTransaction, "TPB_LOCK_WRITE", INT2FIX(isc_tpb_lock_write));
  rb_define_const(cTransaction, "TPB_VERB_TIME", INT2FIX(isc_tpb_verb_time));
  rb_define_const(cTransaction, "TPB_COMMIT_TIME", INT2FIX(isc_tpb_commit_time));
  rb_define_const(cTransaction, "TPB_IGNORE_LIMBO", INT2FIX(isc_tpb_ignore_limbo));
  rb_define_const(cTransaction, "TPB_READ_COMMITTED", INT2FIX(isc_tpb_read_committed));
  rb_define_const(cTransaction, "TPB_AUTO_COMMIT", INT2FIX(isc_tpb_autocommit));
  rb_define_const(cTransaction, "TPB_REC_VERSION", INT2FIX(isc_tpb_rec_version));
  rb_define_const(cTransaction, "TPB_NO_REC_VERSION", INT2FIX(isc_tpb_no_rec_version));
  rb_define_const(cTransaction, "TPB_RESTART_REQUESTS", INT2FIX(isc_tpb_restart_requests));
  rb_define_const(cTransaction, "TPB_NO_AUTO_UNDO", INT2FIX(isc_tpb_no_auto_undo));
}
