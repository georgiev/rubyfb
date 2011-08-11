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
#include "Row.h"
#include "TypeMap.h"
#include "ruby.h"

/* Function prototypes. */
static VALUE allocateResultSet(VALUE);
static VALUE initializeResultSet(VALUE, VALUE, VALUE, VALUE, VALUE, VALUE);
static VALUE fetchResultSetEntry(VALUE);
static VALUE closeResultSet(VALUE);
static VALUE getResultSetCount(VALUE);
static VALUE getResultSetConnection(VALUE);
static VALUE getResultSetTransaction(VALUE);
static VALUE getResultSetSQL(VALUE);
static VALUE getResultSetDialect(VALUE);
static VALUE getResultSetColumnCount(VALUE);
static VALUE getResultSetColumnName(VALUE, VALUE);
static VALUE getResultSetColumnAlias(VALUE, VALUE);
static VALUE getResultSetColumnTable(VALUE, VALUE);
static VALUE getResultSetColumnType(VALUE, VALUE);
static VALUE eachResultSetRow(VALUE);
static VALUE isResultSetExhausted(VALUE);
static void resultSetMark(void *);
static void cleanupHandle(isc_stmt_handle *handle);
static void resolveResultsTransaction(ResultsHandle *results, char *resolveMethod);


/* Globals. */
VALUE cResultSet;

/**
 * This method allocates a new ResultSet object.
 *
 * @param  klass  A reference to the ResultSet Class object.
 *
 * @return  A reference to the newly allocated ResultSet object.
 *
 */
VALUE allocateResultSet(VALUE klass)
{
   ResultsHandle *results = ALLOC(ResultsHandle);
   
   if(results == NULL)
   {
      rb_raise(rb_eNoMemError,
               "Memory allocation failure allocating a result set.");
   }
   results->handle      = 0;
   results->output      = NULL;
   results->exhausted   = 0;
   results->fetched     = 0;
   results->dialect     = 0;
   results->procedure_output_fetch_state = -1;
   results->transaction = Qnil;
   
   return(Data_Wrap_Struct(klass, resultSetMark, resultSetFree, results));
}


/**
 * This function provides the initialize method for the ResultSet class.
 *
 * @param  self         A reference to the ResultSet object to be initialized.
 * @param  connection   A reference to a Connection object that will be used
 *                      in executing the statement.
 * @param  transaction  A reference to a Transaction object that will be used
 *                      in executing the statement.
 * @param  sql          A reference to a String containing the SQL statement to
 *                      be executed. Must be a query.
 * @param  dialect      A reference to an integer containing the SQL dialect to
 *                      be used in executing the query.
 * @param  parameters   A reference to an array containing the parameters to be
 *                      used in executing the query.
 *
 * @return  A reference to the newly initialize ResultSet object.
 *
 */
VALUE initializeResultSet(VALUE self, VALUE connection, VALUE transaction,
                          VALUE sql, VALUE dialect, VALUE parameters)
{
   short             setting     = 0;
   int               type        = 0,
                     inputs      = 0,
                     outputs     = 0;
   long              affected    = 0;
   ResultsHandle     *results    = NULL;
   VALUE             value       = Qnil;
   ConnectionHandle  *cHandle    = NULL;
   TransactionHandle *tHandle    = NULL;
   XSQLDA            *params     = NULL;
   
   /* Validate the inputs. */
   if(TYPE(connection) == T_DATA &&
      RDATA(connection)->dfree == (RUBY_DATA_FUNC)connectionFree)
   {
      if(rb_funcall(connection, rb_intern("open?"), 0) == Qfalse)
      {
         rb_fireruby_raise(NULL, "Closed connection specified for result set.");
      }
   }
   else
   {
      rb_fireruby_raise(NULL, "Invalid connection specified for result set.");
   }

   if(TYPE(transaction) == T_DATA &&
      RDATA(transaction)->dfree == (RUBY_DATA_FUNC)transactionFree)
   {
      if(rb_funcall(transaction, rb_intern("active?"), 0) == Qfalse)
      {
         rb_fireruby_raise(NULL, "Inactive transaction specified for result set.");
      }
   }
   else
   {
      rb_fireruby_raise(NULL, "Invalid transaction specified for result set.");
   }
   
   value = rb_funcall(dialect, rb_intern("to_i"), 0);
   if(TYPE(value) == T_FIXNUM)
   {
      setting = FIX2INT(value);
      if(setting < 1 || setting > 3)
      {
         rb_fireruby_raise(NULL,
                           "Invalid dialect value specified for result set. "\
                           "The dialect value must be between 1 and 3.");
      }
   }
   else
   {
      rb_fireruby_raise(NULL,
                        "Invalid dialect value specified for result set. The "\
                        "dialect value must be between 1 and 3.");
   }
   
   /* Prepare the result set. */
   Data_Get_Struct(connection, ConnectionHandle, cHandle);
   Data_Get_Struct(transaction, TransactionHandle, tHandle);
   Data_Get_Struct(self, ResultsHandle, results);
   prepare(&cHandle->handle, &tHandle->handle, STR2CSTR(sql), &results->handle,
           setting, &type, &inputs, &outputs);
           
   if(type != isc_info_sql_stmt_select &&
      type != isc_info_sql_stmt_select_for_upd &&
      type != isc_info_sql_stmt_exec_procedure)
   {
      cleanupHandle(&results->handle);
      rb_fireruby_raise(NULL,
                        "Non-query SQL statement specified for result set.");
   }
   
   rb_iv_set(self, "@connection", connection);
   rb_iv_set(self, "@transaction", transaction);
   rb_iv_set(self, "@sql", rb_funcall(sql, rb_intern("to_s"), 0));
   rb_iv_set(self, "@dialect", value);
   results->dialect = setting;
   
   /* UNCOMMENT FOR DEBUGGING PURPOSES! */
   /*strcpy(results->sql, STR2CSTR(sql));*/
   
   /* Check if input parameters are needed. */
   if(inputs > 0)
   {
      VALUE value = Qnil;
      int   size  = 0;
      
      if(parameters == Qnil)
      {
         cleanupHandle(&results->handle);
         rb_fireruby_raise(NULL,
                           "Empty parameter list specified for result set.");
      }
      
      value = rb_funcall(parameters, rb_intern("size"), 0);
      size  = TYPE(value) == T_FIXNUM ? FIX2INT(value) : NUM2INT(value);
      if(size < inputs)
      {
         cleanupHandle(&results->handle);
         rb_fireruby_raise(NULL,
                           "Insufficient parameters specified for result set.");
      }
      
      /* Allocate the XSQLDA and populate it. */
      params = allocateInXSQLDA(inputs, &results->handle, setting);
      prepareDataArea(params);
      setParameters(params, parameters, self);
   }
   
   /* Allocate output storage. */
   results->output = allocateOutXSQLDA(outputs, &results->handle, setting);
   prepareDataArea(results->output);
   
   /* Execute the statement and clean up. */
	if(type == isc_info_sql_stmt_exec_procedure) {
		/* Execute with output params */
		execute_2(&tHandle->handle, &results->handle, setting, params, type,
					&affected, results->output);
		/* Initialize procedure output fetch */
		results->procedure_output_fetch_state = 0;
		/* Early resolve transaction */
		resolveResultsTransaction (results, "commit");
	} else {
		execute(&tHandle->handle, &results->handle, setting, params, type,
					&affected);
	}
   if(params != NULL)
   {
      releaseDataArea(params);
   }
   
   return(self);
}


/**
 * This function provides the fetch method for the ResultSet class.
 *
 * @param  self  A reference to the ResultSet object to make the call on.
 *
 * @return  Either a reference to a Row object or nil.
 *
 */
VALUE fetchResultSetEntry(VALUE self)
{
   VALUE         row      = Qnil;
   ResultsHandle *results = NULL;
   ISC_STATUS    status[20],
                 value;
   
   Data_Get_Struct(self, ResultsHandle, results);
   if(results->handle != 0)
   {
		value = results->procedure_output_fetch_state;
		if(value < 0) {
			value = isc_dsql_fetch(status, &results->handle, results->dialect,
											results->output);
		} else {
			/* move procedure_output_fetch_state ahead - fetch only one row */
			results->procedure_output_fetch_state = 100;
		}
		VALUE array  = Qnil,
				number = Qnil;
		switch(value)
		{
			case 0 :
				array  = toArray(self);
				number = INT2NUM(++(results->fetched));
				row    = rb_row_new(self, array, number);
				break;
			case 100 :
				results->exhausted = 1;
				resolveResultsTransaction(results, "commit");
				break;
			default :
				rb_fireruby_raise(status, "Error fetching query row.");
		}
   }
   
   return(row);
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
VALUE closeResultSet(VALUE self)
{
   ResultsHandle *results = NULL;
   
   Data_Get_Struct(self, ResultsHandle, results);
   if(results->handle != 0)
   {
      ISC_STATUS status[20];
      
      if(isc_dsql_free_statement(status, &results->handle, DSQL_drop))
      {
         rb_fireruby_raise(status, "Error closing result set.");
      }
      results->handle = 0;
   }
   
   if(results->output != NULL)
   {
      releaseDataArea(results->output);
      results->output = NULL;
   }
   
	resolveResultsTransaction(results, "commit");
   
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
VALUE getResultSetCount(VALUE self)
{
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
VALUE getResultSetConnection(VALUE self)
{
   return(rb_iv_get(self, "@connection"));
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
VALUE getResultSetTransaction(VALUE self)
{
   return(rb_iv_get(self, "@transaction"));
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
VALUE getResultSetSQL(VALUE self)
{
   return(rb_iv_get(self, "@sql"));
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
VALUE getResultSetDialect(VALUE self)
{
   return(rb_iv_get(self, "@dialect"));
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
VALUE getResultSetColumnCount(VALUE self)
{
   VALUE         count    = Qnil;
   ResultsHandle *results = NULL;
   
   Data_Get_Struct(self, ResultsHandle, results);
   if(results != NULL)
   {
      count = INT2NUM(results->output->sqld);
   }
   
   return(count);
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
static VALUE getResultSetColumnName(VALUE self, VALUE column)
{
   VALUE         name     = Qnil;
   ResultsHandle *results = NULL;
   
   Data_Get_Struct(self, ResultsHandle, results);
   if(results != NULL)
   {
      int offset = 0;
      
      offset = (TYPE(column) == T_FIXNUM ? FIX2INT(column) : NUM2INT(column));
      if(offset >= 0 && offset < results->output->sqld)
      {
         XSQLVAR *var = results->output->sqlvar;
         int     index;
         
         for(index = 0; index < offset; index++, var++);
         name = rb_str_new(var->sqlname, var->sqlname_length);
      }
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
static VALUE getResultSetColumnAlias(VALUE self, VALUE column)
{
   VALUE         alias    = Qnil;
   ResultsHandle *results = NULL;
   
   Data_Get_Struct(self, ResultsHandle, results);
   if(results != NULL)
   {
      int offset = 0;
      
      offset = (TYPE(column) == T_FIXNUM ? FIX2INT(column) : NUM2INT(column));
      if(offset >= 0 && offset < results->output->sqld)
      {
         XSQLVAR *var = results->output->sqlvar;
         int     index;
         
         for(index = 0; index < offset; index++, var++);
         alias = rb_str_new(var->aliasname, var->aliasname_length);
      }
   }
   
   return(alias);
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
static VALUE getResultSetColumnTable(VALUE self, VALUE column)
{
   VALUE         name    = Qnil;
   ResultsHandle *results = NULL;
   
   Data_Get_Struct(self, ResultsHandle, results);
   if(results != NULL)
   {
      int offset = 0;
      
      offset = (TYPE(column) == T_FIXNUM ? FIX2INT(column) : NUM2INT(column));
      if(offset >= 0 && offset < results->output->sqld)
      {
         XSQLVAR *var = results->output->sqlvar;
         int     index;
         
         for(index = 0; index < offset; index++, var++);
         name = rb_str_new(var->relname, var->relname_length);
      }
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
static VALUE getResultSetColumnType(VALUE self, VALUE column)
{
   VALUE type  = toSymbol("UNKNOWN");
   
   if(TYPE(column) == T_FIXNUM)
   {
      ResultsHandle *handle = NULL;
      
      Data_Get_Struct(self, ResultsHandle, handle);
      if(handle != NULL)
      {
         int index = FIX2INT(column);
         
         /* Fix negative index values. */
         if(index < 0)
         {
            index = handle->output->sqln + index;
         }
         
         if(index >= 0 && index < handle->output->sqln)
         {
            type = getColumnType(&handle->output->sqlvar[index]);
         }
      }
   }
   
   
   return(type);
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
VALUE eachResultSetRow(VALUE self)
{
   VALUE result = Qnil;
   
   /* Check if a block was provided. */
   if(rb_block_given_p())
   {
      VALUE row;
      
      while((row = fetchResultSetEntry(self)) != Qnil)
      {
         result = rb_yield(row);
      }
   }
   
   return(result);
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
VALUE isResultSetExhausted(VALUE self)
{
   VALUE         exhausted = Qfalse;
   ResultsHandle *results  = NULL;
   
   Data_Get_Struct(self, ResultsHandle, results);
   if(results->exhausted)
   {
      exhausted = Qtrue;
   }
   
   return(exhausted);
}


/**
 * This method integrates with the Ruby garbage collector to insure that, if
 * a ResultSet contains a reference to a Transaction object, it doesn't get
 * collected by the garbage collector unnecessarily.
 *
 * @param  handle  A void pointer to the structure associated with the object
 *                 being scanned by the garbage collector.
 *
 */
void resultSetMark(void *handle)
{
   ResultsHandle *results = (ResultsHandle *)handle;
   
   if(results != NULL)
   {
      if(results->transaction != Qnil)
      {
         rb_gc_mark(results->transaction);
      }
   }
}


/**
 * This function provides a programmatic means of creating a ResultSet object.
 *
 * @param  connection   A reference to a Connection object that will be used
 *                      in executing the statement.
 * @param  transaction  A reference to a Transaction object that will be used
 *                      in executing the statement.
 * @param  sql          A reference to a String containing the SQL statement to
 *                      be executed. Must be a query.
 * @param  dialect      A reference to an integer containing the SQL dialect to
 *                      be used in executing the query.
 * @param  parameters   A reference to an array containing the parameters to be
 *                      used in executing the query.
 *
 * @return  A reference to the newly created ResultSet object.
 *
 */
VALUE rb_result_set_new(VALUE connection, VALUE transaction, VALUE sql,
                        VALUE dialect, VALUE parameters)
{
   VALUE instance = allocateResultSet(cResultSet);
   
   initializeResultSet(instance, connection, transaction, sql, dialect,
                       parameters);
   
   return(instance);
}


/**
 * This function assigns an anonymous transaction to a ResultSet, giving the
 * object responsibility for closing it.
 *
 * @param  set          A reference to the ResultSet object that will be taking
 *                      ownership of the Transaction.
 * @param  transaction  A reference to the Transaction object that the ResultSet
 *                      is assuming responsibility for.
 *
 */
void rb_assign_transaction(VALUE set, VALUE transaction)
{
   ResultsHandle *results = NULL;
   
   Data_Get_Struct(set, ResultsHandle, results);
   results->transaction = transaction;
}


/**
 * This function integrates with the Ruby garbage collector to free all of the
 * resources for a ResultSet object that is being collected.
 *
 * @param  handle  A pointer to the ResultsHandle structure associated with the
 *                 object being collected.
 *
 */
void resultSetFree(void *handle)
{
   if(handle != NULL)
   {
      ResultsHandle *results = (ResultsHandle *)handle;
      
      if(results->handle != 0)
      {
         ISC_STATUS status[20];
         
         /* UNCOMMENT FOR DEBUG PURPOSES! */
         /*fprintf(stderr, "Releasing statement handle for...\n%s\n", results->sql);*/
         isc_dsql_free_statement(status, &results->handle, DSQL_drop);
      }
      
      if(results->output != NULL)
      {
         releaseDataArea(results->output);
      }
      
		resolveResultsTransaction(results, "rollback");
      free(results);
   }
}


/**
 * This is a simple function to aid in the clean up of statement handles.
 *
 * @param  handle  A pointer to the statement handle to be cleaned up.
 *
 */
void cleanupHandle(isc_stmt_handle *handle)
{
   if(*handle != 0)
   {
      ISC_STATUS status[20];
         
      /* UNCOMMENT FOR DEBUG PURPOSES! */
      /*fprintf(stderr, "Cleaning up a statement handle.\n");*/
      isc_dsql_free_statement(status, handle, DSQL_drop);
   }
}


/**
 * Simple helper function to resolve ResultsHandle associated transaction
 *
 * @param results - the ResultsHandle handle
 *
 * @param resolveMethod - transaction resolving method - eg. commit, rollback
 *
 */
void resolveResultsTransaction(ResultsHandle *results, char *resolveMethod)
{
	if(results->transaction != Qnil)
	{
		rb_funcall(results->transaction, rb_intern(resolveMethod), 0);
		results->transaction = Qnil;
	}
}


/**
 * This function initializes the ResultSet class within the Ruby environment.
 * The class is established under the module specified to the function.
 *
 * @param  module  A reference to the module to create the class within.
 *
 */
void Init_ResultSet(VALUE module)
{
   cResultSet = rb_define_class_under(module, "ResultSet", rb_cObject);
   rb_define_alloc_func(cResultSet, allocateResultSet);
   rb_include_module(cResultSet, rb_mEnumerable);
   rb_define_method(cResultSet, "initialize", initializeResultSet, 5);
   rb_define_method(cResultSet, "initialize_copy", forbidObjectCopy, 1);
   rb_define_method(cResultSet, "row_count", getResultSetCount, 0);
   rb_define_method(cResultSet, "fetch", fetchResultSetEntry, 0);
   rb_define_method(cResultSet, "close", closeResultSet, 0);
   rb_define_method(cResultSet, "connection", getResultSetConnection, 0);
   rb_define_method(cResultSet, "transaction", getResultSetTransaction, 0);
   rb_define_method(cResultSet, "sql", getResultSetSQL, 0);
   rb_define_method(cResultSet, "dialect", getResultSetDialect, 0);
   rb_define_method(cResultSet, "each", eachResultSetRow, 0);
   rb_define_method(cResultSet, "column_name", getResultSetColumnName, 1);
   rb_define_method(cResultSet, "column_alias", getResultSetColumnAlias, 1);
   rb_define_method(cResultSet, "column_table", getResultSetColumnTable, 1);
   rb_define_method(cResultSet, "column_count", getResultSetColumnCount, 0);
   rb_define_method(cResultSet, "exhausted?", isResultSetExhausted, 0);
   rb_define_method(cResultSet, "get_base_type", getResultSetColumnType, 1);
}
