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

/* Function prototypes. */
static VALUE allocateStatement(VALUE);
static VALUE initializeStatement(VALUE, VALUE, VALUE);
static VALUE getStatementSQL(VALUE);
static VALUE getStatementConnection(VALUE);
static VALUE getStatementDialect(VALUE);
static VALUE getStatementType(VALUE);
static VALUE getStatementParameterCount(VALUE);
static VALUE execStatement(int, VALUE*, VALUE);
static VALUE execAndCloseStatement(int, VALUE*, VALUE);
static VALUE closeStatement(VALUE);
static VALUE getStatementPrepared(VALUE);
static VALUE prepareStatement(int, VALUE*, VALUE);
static VALUE getStatementPlan(VALUE);

static VALUE execAndManageTransaction(VALUE, VALUE, VALUE);
static VALUE execAndManageStatement(VALUE, VALUE, VALUE);
static VALUE rescueLocalTransaction(VALUE, VALUE);
static VALUE execStatementFromArray(VALUE);
static VALUE rescueStatement(VALUE, VALUE);
static VALUE execInTransactionFromArray(VALUE);
static VALUE execInTransaction(VALUE, VALUE, VALUE);
static void prepareInTransaction(VALUE, VALUE);
static VALUE prepareFromArray(VALUE);
static void statementFree(void *);
static StatementHandle* getPreparedHandle(VALUE self);

/* Globals. */
static VALUE cStatement;

static ID
  RB_INTERN_CREATE_COLUMN_METADATA,
  RB_INTERN_CREATE_RESULT_SET,
  RB_INTERN_IS_ACTIVE_RESULT_SET,
  RB_INTERN_AT_NAME,
  RB_INTERN_AT_ALIAS,
  RB_INTERN_AT_KEY,
  RB_INTERN_AT_TYPE,
  RB_INTERN_AT_SCALE,
  RB_INTERN_AT_RELATION,
  RB_INTERN_AT_METADATA,
  RB_INTERN_AT_MANAGE_STATEMENT,
  RB_INTERN_AT_MANAGE_TRANSACTION,
  RB_INTERN_ACTIVE,
  RB_INTERN_EACH,
  RB_INTERN_COMMIT,
  RB_INTERN_ROLLBACK,
  RB_INTERN_SIZE,
  RB_INTERN_CLOSE,
  RB_INTERN_TO_S,
  RB_INTERN_OPEN,
  RB_INTERN_STRIP,
  RB_INTERN_NEW;

static const ISC_STATUS FETCH_MORE = 0;
static const ISC_STATUS FETCH_COMPLETED = 100;
static const ISC_STATUS FETCH_ONE = 101;

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
 * Return affected row count for DML statements
 *
 * @param  statement   A pointer to the statement handle
 */
long fb_query_affected(StatementHandle *statement) {
  ISC_STATUS status[ISC_STATUS_LENGTH];
  ISC_STATUS execute_result;
  long result = 0;
  int info      = 0,
      done      = 0;
  char items[]   = {isc_info_sql_records},
       buffer[40],
  *position = buffer + 3;

  switch(statement->type) {
  case isc_info_sql_stmt_update:
    info = isc_info_req_update_count;
    break;
  case isc_info_sql_stmt_delete:
    info = isc_info_req_delete_count;
    break;
  case isc_info_sql_stmt_insert:
    info = isc_info_req_insert_count;
    break;
  default:
    return (result);
  }
    
  if(isc_dsql_sql_info(status, &statement->handle, sizeof(items), items,
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
      result = temp[1];
      done      = 1;
    }
  }
  return (result);
}

VALUE getStatementMetadata(VALUE self) {
  return rb_ivar_get(self, RB_INTERN_AT_METADATA);
}

/**
 * Prepare statement parsing arguments array
 *
 * @param  args   Array containing statement and transaction objects
 *
 * @return  Qnil
 */
VALUE prepareFromArray(VALUE args) {
  VALUE self = rb_ary_entry(args, 0);
  VALUE transaction = rb_ary_entry(args, 1);
  prepareInTransaction(self, transaction);
  return (Qnil);
}

/**
 * Prepare statement within transaction context
 *
 * @param  self         A reference to the statement object
 *
 * @param  transaction  A reference to the transaction object
 *
 */
void prepareInTransaction(VALUE self, VALUE transaction) {
  StatementHandle *hStatement = NULL;
  Data_Get_Struct(self, StatementHandle, hStatement);

  if(0 == hStatement->handle) {
    ConnectionHandle  *hConnection  = NULL;
    TransactionHandle *hTransaction = NULL;
    VALUE metadata, sql = rb_iv_get(self, "@sql");
    Data_Get_Struct(getStatementConnection(self), ConnectionHandle, hConnection);
    Data_Get_Struct(transaction, TransactionHandle, hTransaction);


    fb_prepare(&hConnection->handle, &hTransaction->handle,
            StringValuePtr(sql), &hStatement->handle,
            hStatement->dialect, &hStatement->type, &hStatement->inputs,
            &hStatement->outputs);

    metadata = rb_ary_new2(hStatement->outputs);
    rb_ivar_set(self, RB_INTERN_AT_METADATA, metadata);

    if(hStatement->outputs > 0) {
      int index;
      XSQLVAR *var;
      VALUE column, name, alias, key_flag = getFireRubySetting("ALIAS_KEYS");

      /* Allocate the XSQLDA */
      hStatement->output = allocateOutXSQLDA(hStatement->outputs,
                                               &hStatement->handle,
                                               hStatement->dialect);
      prepareDataArea(hStatement->output);

      var = hStatement->output->sqlvar;
      for(index = 0; index < hStatement->output->sqld; index++, var++) {
        column = rb_funcall(self, RB_INTERN_CREATE_COLUMN_METADATA, 0);
        rb_ary_store(metadata, index, column);
        name = rb_str_new(var->sqlname, var->sqlname_length);
        alias = rb_str_new(var->aliasname, var->aliasname_length);
        rb_ivar_set(column, RB_INTERN_AT_NAME, name);
        rb_ivar_set(column, RB_INTERN_AT_ALIAS, alias);
        if(key_flag == Qtrue) {
          rb_ivar_set(column, RB_INTERN_AT_KEY, alias);
        } else {
          rb_ivar_set(column, RB_INTERN_AT_KEY, name);
        }
        rb_ivar_set(column, RB_INTERN_AT_TYPE, getColumnType(var));
        rb_ivar_set(column, RB_INTERN_AT_SCALE, INT2FIX(var->sqlscale));
        rb_ivar_set(column, RB_INTERN_AT_RELATION, rb_str_new(var->relname, var->relname_length));
      }
      rb_obj_freeze(metadata);
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
  statement->output     = NULL;

  return(Data_Wrap_Struct(klass, NULL, statementFree, statement));
}


/**
 * This function provides the initialize method for the Statement class.
 *
 * @param  self         A reference to the Statement object to be initialized.
 * @param  connection   A reference to the Connection object that the statement
 *                      will execute through.
 * @param  sql          A reference to a String object containing the text of
 *                      the SQL statement to be executed.
 *
 * @return  A reference to the newly initialized Statement object.
 *
 */
VALUE initializeStatement(VALUE self, VALUE connection, VALUE sql) {
  StatementHandle *hStatement = NULL;
  short setting    = 0;

  sql = rb_funcall(sql, RB_INTERN_TO_S, 0);
  
  /* Validate the inputs. */
  if(TYPE(connection) == T_DATA &&
     RDATA(connection)->dfree == (RUBY_DATA_FUNC)connectionFree) {
    if(rb_funcall(connection, RB_INTERN_OPEN, 0) == Qfalse) {
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

/**
 * Execute statement and take care of implicit transaction management
 *
 * @param  self         A reference to the statement object
 *
 * @param  parameters   A reference to the parameter bindings object, can be Qnil
 *
 * @param  transaction  A reference to the transaction object, can be Qnil,
 *                      if so - an implicit transaction is started and resolved when appropriate
 *
 * @return  One of a count of the number of rows affected by the SQL statement,
 *          a ResultSet object for a query or nil.
 *
 */
VALUE execAndManageTransaction(VALUE self, VALUE parameters, VALUE transaction) {
  VALUE result = Qnil;
  
  if(Qnil == transaction) {
    VALUE args = rb_ary_new();
    
    transaction = rb_transaction_new(getStatementConnection(self));
    rb_ary_push(args, self);
    rb_ary_push(args, transaction);
    rb_ary_push(args, parameters);
    
    result = rb_rescue(execInTransactionFromArray, args, rescueLocalTransaction, transaction);
    if(Qtrue == rb_funcall(self, RB_INTERN_IS_ACTIVE_RESULT_SET, 1, result)) {
      rb_ivar_set(result, RB_INTERN_AT_MANAGE_TRANSACTION, Qtrue);
    } else {
      rb_funcall(transaction, RB_INTERN_COMMIT, 0);
    }
  } else {
    result = execInTransaction(self, transaction, parameters);
  }
  return (result);
}

/**
 * Execute statement and take care of closing it when appropriate
 *
 * @param  self         A reference to the statement object
 *
 * @param  parameters   A reference to the parameter bindings object, can be Qnil
 *
 * @param  transaction  A reference to the transaction object, can be Qnil,
 *                      if so - an implicit transaction is started and resolved when appropriate
 *
 * @return  One of a count of the number of rows affected by the SQL statement,
 *          a ResultSet object for a query or nil.
 *
 */
VALUE execAndManageStatement(VALUE self, VALUE parameters, VALUE transaction) {
  VALUE result = Qnil,
        args = rb_ary_new();

  rb_ary_push(args, self);
  rb_ary_push(args, parameters);
  rb_ary_push(args, transaction);
  result = rb_rescue(execStatementFromArray, args, rescueStatement, self);
  if(Qtrue == rb_funcall(self, RB_INTERN_IS_ACTIVE_RESULT_SET, 1, result)) {
    rb_ivar_set(result, RB_INTERN_AT_MANAGE_STATEMENT, Qtrue);
  } else {
    closeStatement(self);
  }
  return (result);
}

/**
 * Resolve (rollback) transaction in rescue block
 *
 * @param  transaction  A reference to the transaction object
 *
 * @param  error  A reference to the exception object
 *
 * @return  Qnil
 *
 */
VALUE rescueLocalTransaction(VALUE transaction, VALUE error) {
  rb_funcall(transaction, RB_INTERN_ROLLBACK, 0);
  rb_exc_raise(error);
  return(Qnil);
}

/**
 * Close statement in rescue block
 *
 * @param  statement  A reference to the statement object
 *
 * @param  error  A reference to the exception object
 *
 * @return  Qnil
 *
 */
VALUE rescueStatement(VALUE statement, VALUE error) {
  rb_funcall(statement, RB_INTERN_CLOSE, 0);
  rb_exc_raise(error);
  return(Qnil);
}

/**
 * Execute a statement within a transaction context parsing arguments array
 *
 * @param  args   Array containing statement, transaction and parameter bindings objects
 *
 * @return  One of a count of the number of rows affected by the SQL statement,
 *          a ResultSet object for a query or nil.
 */
VALUE execInTransactionFromArray(VALUE args) {
  VALUE self = rb_ary_entry(args, 0);
  VALUE transaction = rb_ary_entry(args, 1);
  VALUE parameters = rb_ary_entry(args, 2);

  return(execInTransaction(self, transaction, parameters));
}

/**
 * Check if the statement is a select statement
 *
 * @param  hStatement   A pointer to the statement handle
 *
 * @return  1 if the statement is a select statement, 0 otherwise
 */
short isCursorStatement(StatementHandle *hStatement) {
  switch(hStatement->type) {
    case isc_info_sql_stmt_select:
    case isc_info_sql_stmt_select_for_upd:
      return 1;
    default:
      return 0;
  }
}

static VALUE resultSetEach(VALUE resultSet) {
  return rb_funcall(resultSet, RB_INTERN_EACH, 0);
}

/**
 * Execute a statement within a transaction context
 *
 * @param  self   A reference to the statement object
 *
 * @param  transaction   A reference to the transaction object
 *
 * @param  parameters   A reference to the parameter bindings object
 *
 * @return  One of a count of the number of rows affected by the SQL statement,
 *          a ResultSet object for a query or nil.
 */
VALUE execInTransaction(VALUE self, VALUE transaction, VALUE parameters) {
  VALUE result       = Qnil;
  long affected     = 0;
  StatementHandle   *hStatement   = NULL;
  TransactionHandle *hTransaction = NULL;
  XSQLDA            *bindings = NULL;
  ISC_STATUS status[ISC_STATUS_LENGTH];
  ISC_STATUS execute_result;

  prepareInTransaction(self, transaction);
  Data_Get_Struct(self, StatementHandle, hStatement);
  if(hStatement->inputs > 0) {
    VALUE value = Qnil;
    int size  = 0;

    if(parameters == Qnil) {
      rb_fireruby_raise(NULL,
                        "Empty parameter list specified for statement.");
    }

    value = rb_funcall(parameters, RB_INTERN_SIZE, 0);
    size  = TYPE(value) == T_FIXNUM ? FIX2INT(value) : NUM2INT(value);
    if(size < hStatement->inputs) {
      rb_fireruby_raise(NULL,
                        "Insufficient parameters specified for statement.");
    }

    /* Allocate the XSQLDA */
    bindings = allocateInXSQLDA(hStatement->inputs, &hStatement->handle, hStatement->dialect);
    prepareDataArea(bindings);
    setParameters(bindings, parameters, transaction, getStatementConnection(self));
  }

  /* Execute the statement. */
  Data_Get_Struct(transaction, TransactionHandle, hTransaction);

  if (isCursorStatement(hStatement)) {
    execute_result = isc_dsql_execute(status, &hTransaction->handle, &hStatement->handle, hStatement->dialect, bindings);
  } else {
    execute_result = isc_dsql_execute2(status, &hTransaction->handle, &hStatement->handle, hStatement->dialect, bindings, hStatement->output);
  }
  if(bindings) {
    releaseDataArea(bindings);
  }
  if(execute_result) {
    rb_fireruby_raise(status, "Error executing SQL statement.");
  }
  if (hStatement->output) {
    result = rb_funcall(self, RB_INTERN_CREATE_RESULT_SET, 1, transaction);
    if(rb_block_given_p()) {
      result = rb_iterate(resultSetEach, result, rb_yield, 0);
    }
  } else {
    result = INT2NUM(fb_query_affected(hStatement));
  }

  return(result);
}

/**
 * This method provides the exec method for the Statement class.
 *
 * @param  self       A reference to the Statement object to call the method on.
 *
 * @param  argc       Parameters count
 *                    
 * @param  argv       Parameters array
 *
 * @return  One of a count of the number of rows affected by the SQL statement,
 *          a ResultSet object for a query or nil.
 *
 */
VALUE execStatement(int argc, VALUE *argv, VALUE self) {
  VALUE transaction, parameters = Qnil;

  rb_scan_args(argc, argv, "02", &parameters, &transaction);
  return execAndManageTransaction(self, parameters, transaction);
}

/**
 * This method provides the exec_and_close method for the Statement class.
 *
 * @param  self       A reference to the Statement object to call the method on.
 *
 * @param  argc       Parameters count
 *                    
 * @param  argv       Parameters array
 *
 * @return  One of a count of the number of rows affected by the SQL statement,
 *          a ResultSet object for a query or nil.
 *
 */
VALUE execAndCloseStatement(int argc, VALUE *argv, VALUE self) {
  VALUE parameters, transaction = Qnil;

  rb_scan_args(argc, argv, "02", &parameters, &transaction);
  return execAndManageStatement(self, parameters, transaction);
}

/**
 * Clean up statement handle - release allocated resources
 *
 * @param  statement  A pointer to the statement handle
 *
 * @param  raise_errors  If this parameter is 0 - no exceptions are raised from this function,
 *                      otherwise an exception is raised whenever a problem occurs while releasing resources.
 *
 */
void cleanUpStatement(StatementHandle *statement, int raise_errors) {
  if(statement->handle) {
    ISC_STATUS status[ISC_STATUS_LENGTH];
    if(isc_dsql_free_statement(status, &statement->handle, DSQL_drop)) {
      if(raise_errors) {
        rb_fireruby_raise(status, "Error closing statement.");
      }
    }
    if(statement->output != NULL) {
      releaseDataArea(statement->output);
      statement->output = NULL;
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
 * @param  argc       Parameters count
 *                    
 * @param  argv       Parameters array
 *
 * @return  A reference to the statement object
 *
 */
static VALUE prepareStatement(int argc, VALUE *argv, VALUE self) {
  VALUE transaction = Qnil;
  rb_scan_args(argc, argv, "01", &transaction);
  if(Qnil == transaction) {
    getPreparedHandle(self);
  } else {
    prepareInTransaction(self, transaction);
  }
  return (self);
}

/**
 * This function provides the plan method for the Statement class.
 *
 * @param  self  A reference to the Statement object to call the method on.
 *
 * @return  Query plan
 *
 */
VALUE getStatementPlan(VALUE self) {
  StatementHandle *statement = getPreparedHandle(self);
  ISC_STATUS status[ISC_STATUS_LENGTH];
  unsigned int dataLength = 1024;
  unsigned short retry = 1;
  VALUE result = Qnil;
  char  items[] = {isc_info_sql_get_plan};
  char *buffer = ALLOC_N(char, dataLength);
  while(retry == 1) {
    retry = 0;
    if(!isc_dsql_sql_info(status, &statement->handle, sizeof(items), items,
                         dataLength, buffer)) {
      switch(buffer[0]) {
        case isc_info_truncated:
          dataLength = dataLength + 1024;
          buffer = REALLOC_N(buffer, char, dataLength);
          retry = 1;
          break;
        case isc_info_sql_get_plan:
          dataLength = isc_vax_integer(&buffer[1], 2);
          result = rb_str_new(&buffer[3], dataLength);
          rb_funcall(result, RB_INTERN_STRIP, 0);
        default:
          retry = 0;
      }
    }
  }
  free(buffer);
  if (result == Qnil) {
    rb_fireruby_raise(status, "Error retrieving query plan.");
  }
  return (result);
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

/**
 * Execute statement parsing arguments array
 *
 * @param  args   Array containing statement, parameters and transaction objects
 *
 * @return  One of a count of the number of rows affected by the SQL statement,
 *          a ResultSet object for a query or nil.
 */
VALUE execStatementFromArray(VALUE args) {
  VALUE self = rb_ary_entry(args, 0);
  VALUE params = rb_ary_entry(args, 1);
  VALUE transaction = rb_ary_entry(args, 2);

  return execAndManageTransaction(self, params, transaction);
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
  return execAndManageStatement(rb_statement_new(connection, sql), params, transaction);
}

/**
 * This function guarantess that the returned statement handle is prepared
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
    rb_funcall(transaction, RB_INTERN_COMMIT, 0);
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

static VALUE fetch(VALUE self) {
  StatementHandle   *hStatement;
  ISC_STATUS        status[ISC_STATUS_LENGTH],
                    fetch_result;

  Data_Get_Struct(self, StatementHandle, hStatement);
  if (hStatement->outputs == 0) {
    return Qnil;
  }

  if (isCursorStatement(hStatement)) {
    fetch_result = isc_dsql_fetch(status, &hStatement->handle, hStatement->dialect,
                           hStatement->output);
    if(fetch_result != FETCH_MORE && fetch_result != FETCH_COMPLETED) {
      rb_fireruby_raise(status, "Error fetching query row.");
    }
  } else {
    fetch_result = FETCH_ONE;
  }
  
  return INT2FIX(fetch_result);
}

static VALUE closeCursor(VALUE self) {
  StatementHandle   *hStatement;
  ISC_STATUS        status[ISC_STATUS_LENGTH],
                    close_result;

  Data_Get_Struct(self, StatementHandle, hStatement);
  if (hStatement->outputs == 0) {
    rb_fireruby_raise(status, "Not a cursor statement.");
  }
  if(close_result = isc_dsql_free_statement(status, &hStatement->handle, DSQL_close)) {
    rb_fireruby_raise(status, "Error closing cursor.");
  }
  return INT2FIX(close_result);
}

static VALUE currentRow(VALUE self, VALUE transaction) {
  int i;
  XSQLVAR *entry;
  StatementHandle *hStatement;
  VALUE array, connection;

  Data_Get_Struct(self, StatementHandle, hStatement);
  if (hStatement->outputs == 0) {
    rb_fireruby_raise(NULL, "Statement has no output.");
  }

  connection  = getStatementConnection(self);
  array       = rb_ary_new2(hStatement->output->sqln);
  entry       = hStatement->output->sqlvar;
  for(i = 0; i < hStatement->output->sqln; i++, entry++) {
    rb_ary_store(array, i, toValue(entry, connection, transaction));
  }
  return(array);
}

/**
 * This function initializes the Statement class within the Ruby environment.
 * The class is established under the module specified to the function.
 *
 * @param  module  A reference to the module to create the class within.
 *
 */
void Init_Statement(VALUE module) {
  RB_INTERN_CREATE_COLUMN_METADATA = rb_intern("create_column_metadata");
  RB_INTERN_CREATE_RESULT_SET = rb_intern("create_result_set");
  RB_INTERN_IS_ACTIVE_RESULT_SET = rb_intern("is_active_result_set");
  RB_INTERN_AT_NAME = rb_intern("@name");
  RB_INTERN_AT_ALIAS = rb_intern("@alias");
  RB_INTERN_AT_KEY = rb_intern("@key");
  RB_INTERN_AT_TYPE = rb_intern("@type");
  RB_INTERN_AT_SCALE = rb_intern("@scale");
  RB_INTERN_AT_RELATION = rb_intern("@relation");
  RB_INTERN_AT_METADATA = rb_intern("@metadata");
  RB_INTERN_AT_MANAGE_STATEMENT = rb_intern("@manage_statement");
  RB_INTERN_AT_MANAGE_TRANSACTION = rb_intern("@manage_transaction");
  RB_INTERN_ACTIVE = rb_intern("active?");
  RB_INTERN_EACH = rb_intern("each");
  RB_INTERN_COMMIT = rb_intern("commit");
  RB_INTERN_ROLLBACK = rb_intern("rollback");
  RB_INTERN_SIZE = rb_intern("size");
  RB_INTERN_CLOSE = rb_intern("close");
  RB_INTERN_NEW = rb_intern("new");
  RB_INTERN_TO_S = rb_intern("to_s");
  RB_INTERN_OPEN = rb_intern("open?");
  RB_INTERN_STRIP = rb_intern("strip!");

  cStatement = rb_define_class_under(module, "Statement", rb_cObject);
  rb_define_alloc_func(cStatement, allocateStatement);
  rb_define_method(cStatement, "initialize", initializeStatement, 2);
  rb_define_method(cStatement, "initialize_copy", forbidObjectCopy, 1);
  rb_define_method(cStatement, "sql", getStatementSQL, 0);
  rb_define_method(cStatement, "connection", getStatementConnection, 0);
  rb_define_method(cStatement, "dialect", getStatementDialect, 0);
  rb_define_method(cStatement, "type", getStatementType, 0);
  rb_define_method(cStatement, "exec", execStatement, -1);
  rb_define_method(cStatement, "exec_and_close", execAndCloseStatement, -1);
  rb_define_method(cStatement, "close", closeStatement, 0);
  rb_define_method(cStatement, "parameter_count", getStatementParameterCount, 0);
  rb_define_method(cStatement, "prepare", prepareStatement, -1);
  rb_define_method(cStatement, "prepared?", getStatementPrepared, 0);
  rb_define_method(cStatement, "plan", getStatementPlan, 0);
  rb_define_method(cStatement, "fetch", fetch, 0);
  rb_define_method(cStatement, "close_cursor", closeCursor, 0);
  rb_define_method(cStatement, "current_row", currentRow, 1);

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
  rb_define_const(cStatement, "FETCH_MORE",
                  INT2FIX(FETCH_MORE));
  rb_define_const(cStatement, "FETCH_COMPLETED",
                  INT2FIX(FETCH_COMPLETED));
  rb_define_const(cStatement, "FETCH_ONE",
                  INT2FIX(FETCH_ONE));
}
