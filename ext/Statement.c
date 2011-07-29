/*------------------------------------------------------------------------------
 * Statement.c
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
#include "Statement.h"
#include "Common.h"
#include "Connection.h"
#include "Transaction.h"
#include "DataArea.h"
#include "TypeMap.h"
#include "ResultSet.h"

/* Function prototypes. */
static VALUE allocateStatement(VALUE);
static VALUE initializeStatement(VALUE, VALUE, VALUE);
static VALUE getStatementSQL(VALUE);
static VALUE getStatementConnection(VALUE);
static VALUE getStatementDialect(VALUE);
static VALUE getStatementType(VALUE);
static VALUE getStatementParameterCount(VALUE);
static VALUE executeStatement(int, VALUE*, VALUE);
static VALUE executeStatementFor(int, VALUE*, VALUE);
static VALUE closeStatement(VALUE);
static VALUE getStatementPrepared(VALUE);
static VALUE prepareStatement(VALUE);

VALUE executeAndManageTransaction(VALUE, VALUE, VALUE);
VALUE rescueLocalTransaction(VALUE, VALUE);
VALUE rescueStatement(VALUE, VALUE);
VALUE executeInTransactionFromArray(VALUE);
VALUE executeInTransaction(VALUE, VALUE, VALUE);
void prepareInTransaction(VALUE, VALUE);
VALUE prepareFromArray(VALUE);
void statementFree(void *);
StatementHandle* getPreparedHandle(VALUE self);

/* Globals. */
VALUE cStatement;

/**
 * This function prepares a Firebird SQL statement for execution.
 *
 * @param  connection   A pointer to the database connection that will be used
 *                      to prepare the statement.
 * @param  transaction  A pointer to the database transaction that will be used
 *                      to prepare the statement.
 * @param  sql          A string containing the SQL statement to be executed.
 * @param  statement    A pointer to a Firebird statement that will be prepared.
 * @param  dialect      A short integer containing the SQL dialect to be used in
 *                      preparing the statement.
 * @param  type         A pointer to an integer that will be assigned the type
 *                      of the SQL statement prepared.
 * @param  inputs       A pointer to an integer that will be assigned a count of
 *                      the parameters for the SQL statement.
 * @param  outputs      A pointer to an integer that will be assigned a count of
 *                      the output columns for the SQL statement.
 *
 */
void fb_prepare(isc_db_handle *connection, isc_tr_handle *transaction,
             char *sql, isc_stmt_handle *statement, short dialect,
             int *type, int *inputs, int *outputs) {
  ISC_STATUS status[ISC_STATUS_LENGTH];
  XSQLDA     *da    = NULL;
  char list[] = {isc_info_sql_stmt_type},
       info[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

  /* Prepare the statement. */
  if(isc_dsql_allocate_statement(status, connection, statement)) {
    rb_fireruby_raise(status, "Error allocating a SQL statement.");
  }

  da = (XSQLDA *)ALLOC_N(char, XSQLDA_LENGTH(1));
  if(da == NULL) {
    rb_raise(rb_eNoMemError,
             "Memory allocation failure preparing a statement.");
  }
  da->version = SQLDA_VERSION1;
  da->sqln    = 1;
  if(isc_dsql_prepare(status, transaction, statement, 0, sql, dialect,
                      da)) {
    free(da);
    rb_fireruby_raise(status, "Error preparing a SQL statement.");
  }
  *outputs = da->sqld;

  /* Get the parameter count. */
  if(isc_dsql_describe_bind(status, statement, dialect, da)) {
    free(da);
    rb_fireruby_raise(status, "Error determining statement parameters.");
  }
  *inputs = da->sqld;
  free(da);

  /* Get the statement type details. */
  if(isc_dsql_sql_info(status, statement, 1, list, 20, info) ||
     info[0] != isc_info_sql_stmt_type) {
    rb_fireruby_raise(status, "Error determining SQL statement type.");
  }
  *type = isc_vax_integer(&info[3], isc_vax_integer(&info[1], 2));
}

/**
 * This function executes a previously prepare SQL statement.
 *
 * @param  transaction A pointer to the Firebird transaction handle to be used in
 *                     executing the statement.
 * @param  statement   A pointer to the Firebird statement handle to be used in
 *                     executing the statement.
 * @param  dialect     Database dialect used in the statement
 *
 * @param  parameters  A pointer to the XSQLDA block that contains the input
 *                     parameters for the SQL statement.
 * @param  type        A integer containing the type details relating to the
 *                     statement being executed.
 * @param  affected    A pointer to a long integer that will be assigned a count
 *                     of rows affected by inserts, updates or deletes.
 * @param  output      A pointer to the XSQLDA block that will hold the output
 *                     data generated by the execution.
 */
void fb_execute(isc_tr_handle *transaction, isc_stmt_handle *statement,
               short dialect, XSQLDA *parameters, int type, long *affected, XSQLDA *output) {
  ISC_STATUS status[ISC_STATUS_LENGTH];
  ISC_STATUS execute_result;

  if(output) {
    execute_result = isc_dsql_execute2(status, transaction, statement, dialect, parameters, output);
  } else {
    execute_result = isc_dsql_execute(status, transaction, statement, dialect, parameters);
  }
  if(execute_result) {
    rb_fireruby_raise(status, "Error executing SQL statement.");
  }

  /* Check if a row count is needed. */
  if(type == isc_info_sql_stmt_update || type == isc_info_sql_stmt_delete ||
     type == isc_info_sql_stmt_insert) {
    int info      = 0,
        done      = 0;
    char items[]   = {isc_info_sql_records},
         buffer[40],
    *position = buffer + 3;

    switch(type) {
    case isc_info_sql_stmt_update:
      info = isc_info_req_update_count;
      break;

    case isc_info_sql_stmt_delete:
      info = isc_info_req_delete_count;
      break;

    case isc_info_sql_stmt_insert:
      info = isc_info_req_insert_count;
      break;
    }

    if(isc_dsql_sql_info(status, statement, sizeof(items), items,
                         sizeof(buffer), buffer)) {
      rb_fireruby_raise(status, "Error retrieving affected row count.");
    }

    while(*position != isc_info_end && done == 0) {
      char current = *position++;
      long temp[]  = {0, 0};

      temp[0]  = isc_vax_integer(position, 2);
      position += 2;
      temp[1]  = isc_vax_integer(position, temp[0]);
      position += temp[0];

      if(current == info) {
        *affected = temp[1];
        done      = 1;
      }
    }
  }
}

VALUE prepareFromArray(VALUE args) {
  VALUE self = rb_ary_entry(args, 0);
  VALUE transaction = rb_ary_entry(args, 1);
  prepareInTransaction(self, transaction);
  return (Qnil);
}

void prepareInTransaction(VALUE self, VALUE transaction) {
  StatementHandle *hStatement = NULL;
  Data_Get_Struct(self, StatementHandle, hStatement);

  if(0 == hStatement->handle) {
    ConnectionHandle  *hConnection  = NULL;
    TransactionHandle *hTransaction = NULL;
    VALUE sql = rb_iv_get(self, "@sql");
    Data_Get_Struct(getStatementConnection(self), ConnectionHandle, hConnection);
    Data_Get_Struct(transaction, TransactionHandle, hTransaction);


    fb_prepare(&hConnection->handle, &hTransaction->handle,
            StringValuePtr(sql), &hStatement->handle,
            hStatement->dialect, &hStatement->type, &hStatement->inputs,
            &hStatement->outputs);

    if(hStatement->inputs > 0) {
      /* Allocate the XSQLDA */
      hStatement->parameters = allocateInXSQLDA(hStatement->inputs,
                                               &hStatement->handle,
                                               hStatement->dialect);
      prepareDataArea(hStatement->parameters);
    }
    if(hStatement->outputs > 0) {
      /* Allocate the XSQLDA */
      hStatement->output = allocateOutXSQLDA(hStatement->outputs,
                                               &hStatement->handle,
                                               hStatement->dialect);
      prepareDataArea(hStatement->output);
    }
  }
}

/**
 * This function integrates with the Ruby memory control system to provide for
 * the allocation of Statement objects.
 *
 * @param  klass  A reference to the Statement Class object.
 *
 * @return  A reference to the newly allocated Statement object.
 *
 */
VALUE allocateStatement(VALUE klass) {
  StatementHandle *statement = ALLOC(StatementHandle);

  if(statement == NULL) {
    rb_raise(rb_eNoMemError,
             "Memory allocation failure allocating a statement.");
  }

  statement->handle     = 0;
  statement->type       = -1;
  statement->inputs     = 0;
  statement->outputs    = 0;
  statement->dialect    = 0;
  statement->parameters = NULL;
  statement->output     = NULL;

  return(Data_Wrap_Struct(klass, NULL, statementFree, statement));
}


/**
 * This function provides the initialize method for the Statement class.
 *
 * @param  self         A reference to the Statement object to be initialized.
 * @param  connection   A reference to the Connection object that the statement
 *                      will execute through.
 * @param  transaction  A reference to the Transaction object that the statement
 *                      will work under.
 * @param  sql          A reference to a String object containing the text of
 *                      the SQL statement to be executed.
 *
 * @return  A reference to the newly initialized Statement object.
 *
 */
VALUE initializeStatement(VALUE self, VALUE connection, VALUE sql) {
  StatementHandle *hStatement = NULL;
  short setting    = 0;

  sql = rb_funcall(sql, rb_intern("to_s"), 0);
  
  /* Validate the inputs. */
  if(TYPE(connection) == T_DATA &&
     RDATA(connection)->dfree == (RUBY_DATA_FUNC)connectionFree) {
    if(rb_funcall(connection, rb_intern("open?"), 0) == Qfalse) {
      rb_fireruby_raise(NULL, "Closed connection specified for statement.");
    }
  } else {
    rb_fireruby_raise(NULL, "Invalid connection specified for statement.");
  }

  Data_Get_Struct(self, StatementHandle, hStatement);
  rb_iv_set(self, "@connection", connection);
  rb_iv_set(self, "@sql", sql);
  hStatement->dialect = 3; //FIXME - from connection

  return(self);
}


/**
 * This function provides the sql sttribute accessor method for the Statement
 * class.
 *
 * @param  self  A reference to the Statement object to call the method on.
 *
 * @return  A reference to a String containing the SQL statement.
 *
 */
VALUE getStatementSQL(VALUE self) {
  return(rb_iv_get(self, "@sql"));
}


/**
 * This function provides the connection sttribute accessor method for the
 * Statement class.
 *
 * @param  self  A reference to the Statement object to call the method on.
 *
 * @return  A reference to a Connection object.
 *
 */
VALUE getStatementConnection(VALUE self) {
  VALUE connection = rb_iv_get(self, "@connection");

  return(connection);
}


/**
 * This function provides the dialect sttribute accessor method for the
 * Statement class.
 *
 * @param  self  A reference to the Statement object to call the method on.
 *
 * @return  A reference to an integer containing the SQL dialect setting.
 *
 */
VALUE getStatementDialect(VALUE self) {
  StatementHandle *hStatement = NULL;
  Data_Get_Struct(self, StatementHandle, hStatement);
  return(INT2FIX(hStatement->dialect));
}

/**
 * This function provides the type attribute accessor method for the Statement
 * class.
 *
 * @param  self  A reference to the Statement object to call the method on.
 *
 * @return  A reference to an integer containing the SQL type details.
 *
 */
VALUE getStatementType(VALUE self) {
  StatementHandle *hStatement = getPreparedHandle(self);
  return(INT2FIX(hStatement->type));
}

/**
 * This function provides the parameter count sttribute accessor method for the
 * Statement class.
 *
 * @param  self  A reference to the Statement object to call the method on.
 *
 * @return  A reference to an integer containing the statement parameter count.
 *
 */
VALUE getStatementParameterCount(VALUE self) {
  StatementHandle *statement = getPreparedHandle(self);
  return(INT2NUM(statement->inputs));
}

VALUE executeAndManageTransaction(VALUE self, VALUE transaction, VALUE parameters) {
  VALUE result = Qnil;
  
  if(Qnil == transaction) {
    VALUE args = rb_ary_new();
    
    transaction = rb_transaction_new(getStatementConnection(self));
    rb_ary_push(args, self);
    rb_ary_push(args, transaction);
    rb_ary_push(args, parameters);
    
    result = rb_rescue(executeInTransactionFromArray, args, rescueLocalTransaction, transaction);
    if(isActiveResultSet(result)) {
      resultSetManageTransaction(result);
    } else {
      rb_funcall(transaction, rb_intern("commit"), 0);
    }
  } else {
    result = executeInTransaction(self, transaction, parameters);
  }
  return (result);
}

VALUE rescueLocalTransaction(VALUE transaction, VALUE error) {
  rb_funcall(transaction, rb_intern("rollback"), 0);
  rb_exc_raise(error);
  return(Qnil);
}

VALUE rescueStatement(VALUE statement, VALUE error) {
  rb_funcall(statement, rb_intern("close"), 0);
  rb_exc_raise(error);
  return(Qnil);
}

VALUE executeInTransactionFromArray(VALUE args) {
  VALUE self = rb_ary_entry(args, 0);
  VALUE transaction = rb_ary_entry(args, 1);
  VALUE parameters = rb_ary_entry(args, 2);

  return(executeInTransaction(self, transaction, parameters));
}

VALUE executeInTransaction(VALUE self, VALUE transaction, VALUE parameters) {
  VALUE result       = Qnil;
  long affected     = 0;
  StatementHandle   *hStatement   = NULL;
  TransactionHandle *hTransaction = NULL;

  prepareInTransaction(self, transaction);

  Data_Get_Struct(self, StatementHandle, hStatement);
  /* Check that sufficient parameters have been specified. */
  if(hStatement->inputs > 0) {
    VALUE value = Qnil;
    int size  = 0;

    if(parameters == Qnil) {
      rb_fireruby_raise(NULL,
                        "Empty parameter list specified for statement.");
    }

    value = rb_funcall(parameters, rb_intern("size"), 0);
    size  = TYPE(value) == T_FIXNUM ? FIX2INT(value) : NUM2INT(value);
    if(size < hStatement->inputs) {
      rb_fireruby_raise(NULL,
                        "Insufficient parameters specified for statement.");
    }
    setParameters(hStatement->parameters, parameters, transaction, getStatementConnection(self));
  }

  /* Execute the statement. */
  Data_Get_Struct(transaction, TransactionHandle, hTransaction);
  if (isc_info_sql_stmt_exec_procedure == hStatement->type) {
    fb_execute(&hTransaction->handle, &hStatement->handle, hStatement->dialect, hStatement->parameters, hStatement->type,
              &affected, hStatement->output);
  } else {
    fb_execute(&hTransaction->handle, &hStatement->handle, hStatement->dialect, hStatement->parameters, hStatement->type,
              &affected, NULL);
  }

  if (hStatement->output) {
    result = rb_result_set_new(self, transaction);
    if(rb_block_given_p()) {
      result = yieldResultsRows(result);
    }
  } else {
    result = INT2NUM(affected);
  }

  return(result);
}


/**
 * This method provides the execute method for the Statement class.
 *
 * @param  self  A reference to the Statement object to call the method on.
 *
 * @return  One of a count of the number of rows affected by the SQL statement,
 *          a ResultSet object for a query or nil.
 *
 */
VALUE executeStatement(int argc, VALUE *argv, VALUE self) {
  VALUE transaction = Qnil;
  rb_scan_args(argc, argv, "01", &transaction);
  return(executeAndManageTransaction(self, transaction, rb_ary_new()));
}


/**
 * This method provides the execute method for the Statement class.
 *
 * @param  self        A reference to the Statement object to call the method
 *                     on.
 * @param  parameters  An array containing the parameters to be used in
 *                     executing the statement.
 *
 * @return  One of a count of the number of rows affected by the SQL statement,
 *          a ResultSet object for a query or nil.
 *
 */
VALUE executeStatementFor(int argc, VALUE *argv, VALUE self) {
  VALUE transaction, parameters = Qnil;
  rb_scan_args(argc, argv, "11", &parameters, &transaction);
  return(executeAndManageTransaction(self, transaction, parameters));
}

void cleanUpStatement(StatementHandle *statement, int raise_errors) {
  if(statement->handle) {
    ISC_STATUS status[ISC_STATUS_LENGTH];
    if(isc_dsql_free_statement(status, &statement->handle, DSQL_drop)) {
      if(raise_errors) {
        rb_fireruby_raise(status, "Error closing statement.");
      }
    }
    if(statement->parameters != NULL) {
      releaseDataArea(statement->parameters);
    }
    if(statement->output != NULL) {
      releaseDataArea(statement->output);
    }
    statement->handle = 0;
  }
}

/**
 * This function provides the close method for the Statement class.
 *
 * @param  self  A reference to the Statement object to call the method on.
 *
 * @return  A reference to the newly closed Statement object.
 *
 */
VALUE closeStatement(VALUE self) {
  StatementHandle *statement = NULL;
  Data_Get_Struct(self, StatementHandle, statement);
  cleanUpStatement(statement, 1);
  return(self);
}

/**
 * This function provides the prepared? method for the Statement class.
 *
 * @param  self  A reference to the Statement object to call the method on.
 *
 * @return  Qtrue if the statement is prepared, Qfalse otherwise.
 *
 */
VALUE getStatementPrepared(VALUE self) {
  VALUE result = Qfalse;
  StatementHandle *statement = NULL;
  Data_Get_Struct(self, StatementHandle, statement);
  
  if(statement->handle) {
    result = Qtrue;
  }
  
  return(result);
}

/**
 * This function provides the prepare method for the Statement class.
 *
 * @param  self  A reference to the Statement object to call the method on.
 *
 * @return  A reference to the statement object
 *
 */
static VALUE prepareStatement(VALUE self) {
  getPreparedHandle(self);
  return (self);
}

/**
 * This function provides a programmatic means of creating a Statement object.
 *
 * @param  connection   A reference to a Connection object that will be used by
 *                      the Statement.
 * @param  sql          A reference to a String object containing the text of
 *                      of a SQL statement for the Statement.
 *
 * @return  A reference to the newly created Statement object.
 *
 */
VALUE rb_statement_new(VALUE connection, VALUE sql) {
  VALUE statement = allocateStatement(cStatement);

  initializeStatement(statement, connection, sql);

  return(statement);
}

VALUE executeSQLFromArray(VALUE args) {
  VALUE self = rb_ary_entry(args, 0);
  VALUE params = rb_ary_entry(args, 1);
  VALUE transaction = rb_ary_entry(args, 2);

  return executeAndManageTransaction(self, transaction, params);
}

/**
 * This function provides a programmatic way of executing a parametrized SQL
 * within transaction
 *
 * @param  connection  A reference to the connection object.
 * @param  sql         SQL text.
 * @param  params      Array containing parameter values for the statement.
 * @param  transaction A reference to the transaction object.
 *
 * @return  A reference to the results of executing the statement.
 *
 */
VALUE rb_execute_sql(VALUE connection, VALUE sql, VALUE params, VALUE transaction) {
  VALUE result = Qnil,
        statement = rb_statement_new(connection, sql),
        args = rb_ary_new();
  rb_ary_push(args, statement);
  rb_ary_push(args, params);
  rb_ary_push(args, transaction);

  result = rb_rescue(executeSQLFromArray, args, rescueStatement, statement);
  if(isActiveResultSet(result)) {
    resultSetManageStatement(result);
  } else {
    closeStatement(statement);
  }
  return (result);
}

/**
 * This function provides a programmatic way of preparing the statement object
 *
 * @param  self  A reference to the Statement object to call the method on.
 *
 */
StatementHandle* getPreparedHandle(VALUE self) {
  StatementHandle *hStatement;
  Data_Get_Struct(self, StatementHandle, hStatement);
  if(0 == hStatement->handle) {
    VALUE transaction = rb_transaction_new(getStatementConnection(self));
    VALUE args = rb_ary_new();
        
    rb_ary_push(args, self);
    rb_ary_push(args, transaction);
    
    rb_rescue(prepareFromArray, args, rescueLocalTransaction, transaction);
    rb_funcall(transaction, rb_intern("commit"), 0);
  }
  return (hStatement);
}

/**
 * This function integrates with the Ruby garbage collector to release the
 * resources associated with a Statement object that is being collected.
 *
 * @param  handle  A pointer to the StatementHandle structure for the Statement
 *                 object being collected.
 *
 */
void statementFree(void *handle) {
  if(handle != NULL) {
    cleanUpStatement((StatementHandle *)handle, 0);
    free(handle);
  }
}

/**
 * This function initializes the Statement class within the Ruby environment.
 * The class is established under the module specified to the function.
 *
 * @param  module  A reference to the module to create the class within.
 *
 */
void Init_Statement(VALUE module) {
  cStatement = rb_define_class_under(module, "Statement", rb_cObject);
  rb_define_alloc_func(cStatement, allocateStatement);
  rb_define_method(cStatement, "initialize", initializeStatement, 2);
  rb_define_method(cStatement, "initialize_copy", forbidObjectCopy, 1);
  rb_define_method(cStatement, "sql", getStatementSQL, 0);
  rb_define_method(cStatement, "connection", getStatementConnection, 0);
  rb_define_method(cStatement, "dialect", getStatementDialect, 0);
  rb_define_method(cStatement, "type", getStatementType, 0);
  rb_define_method(cStatement, "execute", executeStatement, -1);
  rb_define_method(cStatement, "execute_for", executeStatementFor, -1);
  rb_define_method(cStatement, "close", closeStatement, 0);
  rb_define_method(cStatement, "parameter_count", getStatementParameterCount, 0);
  rb_define_method(cStatement, "prepare", prepareStatement, 0);
  rb_define_method(cStatement, "prepared?", getStatementPrepared, 0);

  rb_define_const(cStatement, "SELECT_STATEMENT",
                  INT2FIX(isc_info_sql_stmt_select));
  rb_define_const(cStatement, "INSERT_STATEMENT",
                  INT2FIX(isc_info_sql_stmt_insert));
  rb_define_const(cStatement, "UPDATE_STATEMENT",
                  INT2FIX(isc_info_sql_stmt_update));
  rb_define_const(cStatement, "DELETE_STATEMENT",
                  INT2FIX(isc_info_sql_stmt_delete));
  rb_define_const(cStatement, "DDL_STATEMENT",
                  INT2FIX(isc_info_sql_stmt_ddl));
  rb_define_const(cStatement, "GET_SEGMENT_STATEMENT",
                  INT2FIX(isc_info_sql_stmt_get_segment));
  rb_define_const(cStatement, "PUT_SEGMENT_STATEMENT",
                  INT2FIX(isc_info_sql_stmt_put_segment));
  rb_define_const(cStatement, "EXECUTE_PROCEDURE_STATEMENT",
                  INT2FIX(isc_info_sql_stmt_exec_procedure));
  rb_define_const(cStatement, "START_TRANSACTION_STATEMENT",
                  INT2FIX(isc_info_sql_stmt_start_trans));
  rb_define_const(cStatement, "COMMIT_STATEMENT",
                  INT2FIX(isc_info_sql_stmt_commit));
  rb_define_const(cStatement, "ROLLBACK_STATEMENT",
                  INT2FIX(isc_info_sql_stmt_rollback));
  rb_define_const(cStatement, "SELECT_FOR_UPDATE_STATEMENT",
                  INT2FIX(isc_info_sql_stmt_select_for_upd));
  rb_define_const(cStatement, "SET_GENERATOR_STATEMENT",
                  INT2FIX(isc_info_sql_stmt_set_generator));
  rb_define_const(cStatement, "SAVE_POINT_STATEMENT",
                  INT2FIX(isc_info_sql_stmt_savepoint));
}
