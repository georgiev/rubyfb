/*------------------------------------------------------------------------------
 * ResultSet.c
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
#include "ResultSet.h"
#include "Common.h"
#include "Statement.h"
#include "Connection.h"
#include "Transaction.h"
#include "DataArea.h"
#include "ruby.h"
#include "FireRuby.h"
#include "TypeMap.h"

/* Function prototypes. */
static VALUE allocateResultSet(VALUE);
static VALUE initializeResultSet(VALUE, VALUE, VALUE);
static VALUE getResultSetRow(VALUE);
static VALUE fetchResultSetEntry(VALUE);
static VALUE closeResultSet(VALUE);
static VALUE getResultSetCount(VALUE);
static VALUE getResultSetConnection(VALUE);
static VALUE getResultSetTransaction(VALUE);
static VALUE getResultSetStatement(VALUE);
static VALUE getResultSetSQL(VALUE);
static VALUE getResultSetDialect(VALUE);
static VALUE getResultSetColumnCount(VALUE);
static VALUE getResultSetColumnName(VALUE, VALUE);
static VALUE getResultSetColumnAlias(VALUE, VALUE);
static VALUE getResultSetColumnScale(VALUE, VALUE);
static VALUE getResultSetColumnTable(VALUE, VALUE);
static VALUE getResultSetColumnType(VALUE, VALUE);
static VALUE eachResultSetRow(VALUE);
static VALUE isResultSetExhausted(VALUE);

/* Globals. */
VALUE cResultSet;

static ID NEW_ID, SIZE_ID, AT_NAME_ID, AT_ALIAS_ID, AT_KEY_ID, AT_SCALE_ID, AT_TYPE_ID,
  AT_RELATION_ID, AT_STATEMENT_ID, AT_TRANSACTION_ID, CREATE_ROW_ID;

static StatementHandle* getStatementHandle(VALUE self) {
  StatementHandle *hStatement;
  Data_Get_Struct(getResultSetStatement(self), StatementHandle, hStatement);
  
  return (hStatement);
}

short isActiveResultSet(VALUE self) {
  short result = 0;
  if ((Qtrue == rb_obj_is_kind_of(self, cResultSet))) {
    ResultsHandle *hResults = NULL;
    Data_Get_Struct(self, ResultsHandle, hResults);
    result = hResults->active;
  }
  return (result);
}

void resultSetManageTransaction(VALUE self) {
  ResultsHandle *hResults = NULL;
  Data_Get_Struct(self, ResultsHandle, hResults);
  hResults->manage_transaction = 1;
}

void resultSetManageStatement(VALUE self) {
  ResultsHandle *hResults = NULL;
  Data_Get_Struct(self, ResultsHandle, hResults);
  hResults->manage_statement = 1;
}

/**
 * This method allocates a new ResultSet object.
 *
 * @param  klass  A reference to the ResultSet Class object.
 *
 * @return  A reference to the newly allocated ResultSet object.
 *
 */
VALUE allocateResultSet(VALUE klass) {
  ResultsHandle *results = ALLOC(ResultsHandle);

  if(results == NULL) {
    rb_raise(rb_eNoMemError,
             "Memory allocation failure allocating a result set.");
  }
  results->fetched     = 0;
  results->active = 0;
  results->manage_transaction = 0;
  results->manage_statement = 0;

  return(Data_Wrap_Struct(klass, NULL, resultSetFree, results));
}


/**
 * This function provides the initialize method for the ResultSet class.
 *
 * @param  self         A reference to the ResultSet object to be initialized.
 * @param  statement    A statement to iterate over
 * @param  transaction  A reference to a privater transaction object
 *                      taht should be managed by this ResultSet.
 *
 * @return  A reference to the newly initialize ResultSet object.
 *
 */
VALUE initializeResultSet(VALUE self, VALUE statement, VALUE transaction) {
  ResultsHandle *hResults = NULL;

  Data_Get_Struct(self, ResultsHandle, hResults);
  rb_ivar_set(self, AT_STATEMENT_ID, statement);
  rb_ivar_set(self, AT_TRANSACTION_ID, transaction);
  
  hResults->active = 1;
  return(self);
}

/**
 * This function provides the row method for the ResultSet class.
 *
 * @param  self  A reference to the ResultSet object to make the call on.
 *
 * @return  Either a reference to a Row object or nil.
 *
 */
VALUE getResultSetRow(VALUE self) {
  VALUE row      = Qnil;
  ResultsHandle *hResults = NULL;

  Data_Get_Struct(self, ResultsHandle, hResults);
  if(hResults->fetched) {
    row = rb_funcall(self, CREATE_ROW_ID, 3, 
      getStatementMetadata(getResultSetStatement(self)),
      toValueArray(self),
      INT2FIX(hResults->fetched)
    );
  }
  return (row);
}


/**
 * This function provides the fetch method for the ResultSet class.
 *
 * @param  self  A reference to the ResultSet object to make the call on.
 *
 * @return  Either a reference to a Row object or nil.
 *
 */
VALUE fetchResultSetEntry(VALUE self) {
  VALUE row         = Qnil;
  StatementHandle   *hStatement   = getStatementHandle(self);
  ResultsHandle     *hResults = NULL;
  ISC_STATUS        status[ISC_STATUS_LENGTH],
                    fetch_result;

  Data_Get_Struct(self, ResultsHandle, hResults);
  if(hResults->active) {
    if (isCursorStatement(hStatement)) {
      fetch_result = isc_dsql_fetch(status, &hStatement->handle, hStatement->dialect,
                             hStatement->output);
      switch(fetch_result) {
      case 0:
        hResults->fetched += 1;
        row = getResultSetRow(self);
        break;
      case 100:
        hResults->active = 0;
        break;
      default:
        rb_fireruby_raise(status, "Error fetching query row.");
      }
    } else {
      hResults->active = 0;
      hResults->fetched = 1;
      row = getResultSetRow(self);
    }
  }
  return (row);
}

/**
 * This function provides the close method for the ResultSet class, releasing
 * resources associated with the ResultSet.
 *
 * @param  self  A reference to the ResultSet object to be closed.
 *
 * @return  A reference to the closed ResultSet object.
 *
 */
VALUE closeResultSet(VALUE self) {
  StatementHandle   *hStatement   = getStatementHandle(self);
  ResultsHandle     *hResults = NULL;
  Data_Get_Struct(self, ResultsHandle, hResults);
  ISC_STATUS status[ISC_STATUS_LENGTH];

  hResults->active = 0;
  if(isc_dsql_free_statement(status, &hStatement->handle, DSQL_close)) {
    rb_fireruby_raise(status, "Error closing cursor.");
  }
  if(hResults->manage_statement) {
    VALUE statement = getResultSetStatement(self),
          menagement_required = rb_funcall(statement, rb_intern("prepared?"), 0);
    if(Qtrue == menagement_required) {
      rb_funcall(statement, rb_intern("close"), 0);
    }  
  }
  if(hResults->manage_transaction) {
    VALUE transaction = getResultSetTransaction(self),
          menagement_required = rb_funcall(transaction, rb_intern("active?"), 0);
    if(Qtrue == menagement_required) {
      rb_funcall(transaction, rb_intern("commit"), 0);
    }  
  }

  return(self);
}


/**
 * This function provides the count method for the ResultSet class.
 *
 * @param  self  A reference to the ResultSet object to fetch the current row
 *               count for.
 *
 * @return  A reference to an integer containing a count of the number of rows
 *          fetched from the ResultSet so far.
 *
 */
VALUE getResultSetCount(VALUE self) {
  ResultsHandle *results = NULL;

  Data_Get_Struct(self, ResultsHandle, results);

  return(INT2NUM(results->fetched));
}


/**
 * This method provides the accessor method for the connection ResultSet
 * attribute.
 *
 * @param  self  A reference to the ResultSet object to fetch the attribute
 *               from.
 *
 * @return  A reference to the requested attribute object.
 *
 */
VALUE getResultSetConnection(VALUE self) {
  return(rb_funcall(getResultSetStatement(self), rb_intern("connection"), 0));
}


/**
 * This method provides the accessor method for the transaction ResultSet
 * attribute.
 *
 * @param  self  A reference to the ResultSet object to fetch the attribute
 *               from.
 *
 * @return  A reference to the requested attribute object.
 *
 */
VALUE getResultSetTransaction(VALUE self) {
  return rb_ivar_get(self, AT_TRANSACTION_ID);
}

/**
 * This method provides the accessor method for the transaction ResultSet
 * statement.
 *
 * @param  self  A reference to the ResultSet object to fetch the attribute
 *               from.
 *
 * @return  A reference to the requested attribute object.
 *
 */
VALUE getResultSetStatement(VALUE self) {
  return rb_ivar_get(self, AT_STATEMENT_ID);
}


/**
 * This method provides the accessor method for the SQL ResultSet attribute.
 *
 * @param  self  A reference to the ResultSet object to fetch the attribute
 *               from.
 *
 * @return  A reference to the requested attribute object.
 *
 */
VALUE getResultSetSQL(VALUE self) {
  return(rb_funcall(getResultSetStatement(self), rb_intern("sql"), 0));
}


/**
 * This method provides the accessor method for the dialect ResultSet attribute.
 *
 * @param  self  A reference to the ResultSet object to fetch the attribute
 *               from.
 *
 * @return  A reference to the requested attribute object.
 *
 */
VALUE getResultSetDialect(VALUE self) {
  return(rb_funcall(getResultSetStatement(self), rb_intern("dialect"), 0));
}


/**
 * This function provides the column_count attribute accessor for the ResultSet
 * class.
 *
 * @param  self  A reference to the ResultSet object to fetch the attribute for.
 *
 * @return  A reference to an integer containing the column count.
 *
 */
VALUE getResultSetColumnCount(VALUE self) {
  return(INT2NUM(getStatementHandle(self)->output->sqld));
}


/**
 * This function provides the column_name method for the ResultSet class.
 *
 * @param  self    A reference to the ResultSet object to retrieve the column
 *                 name from.
 * @param  column  An offset to the column to retrieve the name for.
 *
 * @return  A String containing the column name or nil if an invalid column
 *          was specified.
 *
 */
static VALUE getResultSetColumnName(VALUE self, VALUE column) {
  int offset = 0;
  VALUE name     = Qnil;
  StatementHandle *hStatement = getStatementHandle(self);

  offset = (TYPE(column) == T_FIXNUM ? FIX2INT(column) : NUM2INT(column));
  if(offset >= 0 && offset < hStatement->output->sqld) {
    XSQLVAR *var = hStatement->output->sqlvar;
    int index;

    for(index = 0; index < offset; index++, var++) ;
    name = rb_str_new(var->sqlname, var->sqlname_length);
  }

  return(name);
}


/**
 * This function provides the column_alias method for the ResultSet class.
 *
 * @param  self    A reference to the ResultSet object to retrieve the column
 *                 alias from.
 * @param  column  An offset to the column to retrieve the alias for.
 *
 * @return  A String containing the column alias or nil if an invalid column
 *          was specified.
 *
 */
static VALUE getResultSetColumnAlias(VALUE self, VALUE column) {
  int offset = 0;
  VALUE alias    = Qnil;
  StatementHandle *hStatement = getStatementHandle(self);

  offset = (TYPE(column) == T_FIXNUM ? FIX2INT(column) : NUM2INT(column));
  if(offset >= 0 && offset < hStatement->output->sqld) {
    XSQLVAR *var = hStatement->output->sqlvar;
    int index;

    for(index = 0; index < offset; index++, var++) ;
    alias = rb_str_new(var->aliasname, var->aliasname_length);
  }

  return(alias);
}

/**
 * This function provides the column_scale method for the ResultSet class.
 *
 * @param  self    A reference to the ResultSet object to retrieve the column
 *                 alias from.
 * @param  column  An offset to the column to retrieve the scale of.
 *
 * @return  An Integer representing the sqlscale of the column, or nil if an
 *          invalid column was specified.
 *
 */
static VALUE getResultSetColumnScale(VALUE self, VALUE column) {
  int offset = 0;
  VALUE scale    = Qnil;
  StatementHandle *hStatement = getStatementHandle(self);

  offset = (TYPE(column) == T_FIXNUM ? FIX2INT(column) : NUM2INT(column));
  if(offset >= 0 && offset < hStatement->output->sqld) {
    XSQLVAR *var = hStatement->output->sqlvar;
    int index;

    for(index = 0; index < offset; index++, var++) ;
    scale = INT2FIX(var->sqlscale);
  }

  return(scale);
}


/**
 * This function provides the column_table method for the ResultSet class.
 *
 * @param  self    A reference to the ResultSet object to retrieve the column
 *                 table name from.
 * @param  column  An offset to the column to retrieve the table name for.
 *
 * @return  A String containing the column table name or nil if an invalid
 *          column was specified.
 *
 */
static VALUE getResultSetColumnTable(VALUE self, VALUE column) {
  int offset = 0;
  VALUE name    = Qnil;
  StatementHandle *hStatement = getStatementHandle(self);

  offset = (TYPE(column) == T_FIXNUM ? FIX2INT(column) : NUM2INT(column));
  if(offset >= 0 && offset < hStatement->output->sqld) {
    XSQLVAR *var = hStatement->output->sqlvar;
    int index;

    for(index = 0; index < offset; index++, var++) ;
    name = rb_str_new(var->relname, var->relname_length);
  }

  return(name);
}


/**
 * This function attempts to extract basic type information for a column within
 * a ResultSet object.
 *
 * @param  self    A reference to the ResultSet to execute on.
 * @param  column  A integer offset for the column to extract the type of.
 *
 * @return  A Symbol representing the basic data type.
 *
 */
static VALUE getResultSetColumnType(VALUE self, VALUE column) {
  VALUE type  = toSymbol("UNKNOWN");

  if(TYPE(column) == T_FIXNUM) {
    StatementHandle *hStatement = getStatementHandle(self);
    int index = FIX2INT(column);

    /* Fix negative index values. */
    if(index < 0) {
      index = hStatement->output->sqln + index;
    }

    if(index >= 0 && index < hStatement->output->sqln) {
      type = getColumnType(&hStatement->output->sqlvar[index]);
    }
  }

  return(type);
}

VALUE yieldRows(VALUE self) {
  VALUE result = Qnil;

  /* Check if a block was provided. */
  if(rb_block_given_p()) {
    VALUE row;
    while((row = fetchResultSetEntry(self)) != Qnil) {
      result = rb_yield(row);
    }
  }

  return(result);
}

VALUE yieldResultsRows(VALUE self) {
  return rb_ensure(yieldRows, self, closeResultSet, self);
}

/**
 * This function provides the each method for the ResultSet class.
 *
 * @param  self  A reference to the ResultSet object to execute the method for.
 *
 * @return  A reference to the last value returned by the associated block or
 *          nil.
 *
 */
VALUE eachResultSetRow(VALUE self) {
  return yieldRows(self);
}

/**
 * This function provides the exhausted? method of the ResultSet class.
 *
 * @param  self  A reference to the ResultSet object to make the call for.
 *
 * @return  True if all rows have been fetched from the result set, false
 *          if they haven't.
 *
 */
VALUE isResultSetExhausted(VALUE self) {
  VALUE exhausted = Qtrue;
  ResultsHandle *hResults  = NULL;

  Data_Get_Struct(self, ResultsHandle, hResults);
  if(hResults->active) {
    exhausted = Qfalse;
  }

  return(exhausted);
}


/**
 * This function provides a programmatic means of creating a ResultSet object.
 *
 * @param  statement    A statement to iterate over
 * @param  transaction  A reference to a privater transaction object
 *                      taht should be managed by this ResultSet.
 * @param  owns_transaction  true if the result set should manage the transaction
 *
 * @return  A reference to the newly created ResultSet object.
 *
 */
VALUE rb_result_set_new(VALUE statement, VALUE transaction) {
  VALUE instance = allocateResultSet(cResultSet);
  return (initializeResultSet(instance, statement, transaction));
}

/**
 * This function integrates with the Ruby garbage collector to free all of the
 * resources for a ResultSet object that is being collected.
 *
 * @param  handle  A pointer to the ResultsHandle structure associated with the
 *                 object being collected.
 *
 */
void resultSetFree(void *handle) {
  if(handle) {
    free(handle);
  }
}

/**
 * This function initializes the ResultSet class within the Ruby environment.
 * The class is established under the module specified to the function.
 *
 * @param  module  A reference to the module to create the class within.
 *
 */
void Init_ResultSet(VALUE module) {
  NEW_ID = rb_intern("new");
  SIZE_ID = rb_intern("size");
  AT_NAME_ID = rb_intern("@name");
  AT_ALIAS_ID = rb_intern("@alias");
  AT_KEY_ID = rb_intern("@key");
  AT_SCALE_ID = rb_intern("@scale");
  AT_TYPE_ID = rb_intern("@type");
  AT_RELATION_ID = rb_intern("@relation");
  AT_STATEMENT_ID = rb_intern("@statement");
  AT_TRANSACTION_ID = rb_intern("@transaction");
  CREATE_ROW_ID = rb_intern("create_row");

  cResultSet = rb_define_class_under(module, "ResultSet", rb_cObject);
  rb_define_alloc_func(cResultSet, allocateResultSet);
  rb_include_module(cResultSet, rb_mEnumerable);
  rb_define_method(cResultSet, "initialize", initializeResultSet, 2);
  rb_define_method(cResultSet, "initialize_copy", forbidObjectCopy, 1);
  rb_define_method(cResultSet, "row_count", getResultSetCount, 0);
  rb_define_method(cResultSet, "fetch", fetchResultSetEntry, 0);
  rb_define_method(cResultSet, "close", closeResultSet, 0);
  rb_define_method(cResultSet, "connection", getResultSetConnection, 0);
  rb_define_method(cResultSet, "transaction", getResultSetTransaction, 0);
  rb_define_method(cResultSet, "statement", getResultSetStatement, 0);
  rb_define_method(cResultSet, "sql", getResultSetSQL, 0);
  rb_define_method(cResultSet, "dialect", getResultSetDialect, 0);
  rb_define_method(cResultSet, "each", eachResultSetRow, 0);
  rb_define_method(cResultSet, "column_name", getResultSetColumnName, 1);
  rb_define_method(cResultSet, "column_alias", getResultSetColumnAlias, 1);
  rb_define_method(cResultSet, "column_scale", getResultSetColumnScale, 1);
  rb_define_method(cResultSet, "column_table", getResultSetColumnTable, 1);
  rb_define_method(cResultSet, "column_count", getResultSetColumnCount, 0);
  rb_define_method(cResultSet, "exhausted?", isResultSetExhausted, 0);
  rb_define_method(cResultSet, "get_base_type", getResultSetColumnType, 1);
}
