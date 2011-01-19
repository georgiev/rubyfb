/*------------------------------------------------------------------------------
 * Row.c
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
#include "Row.h"
#include "FireRuby.h"
#include "ibase.h"
#include "ruby.h"

/* Function prototypes. */
static VALUE allocateRow(VALUE);
static VALUE initializeRow(VALUE, VALUE, VALUE, VALUE);
static VALUE columnsInRow(VALUE);
static VALUE getRowNumber(VALUE);
static VALUE getColumnName(VALUE, VALUE);
static VALUE getColumnAlias(VALUE, VALUE);
static VALUE getColumnScale(VALUE, VALUE);
static VALUE getColumnValue(VALUE, VALUE);
static VALUE eachColumn(VALUE);
static VALUE eachColumnKey(VALUE);
static VALUE eachColumnValue(VALUE);
static VALUE fetchRowValue(int, VALUE *, VALUE);
static VALUE hasColumnKey(VALUE, VALUE);
static VALUE hasColumnName(VALUE, VALUE);
static VALUE hasColumnAlias(VALUE, VALUE);
static VALUE hasColumnValue(VALUE, VALUE);
static VALUE getColumnNames(VALUE);
static VALUE getColumnAliases(VALUE);
static VALUE getColumnValues(VALUE);
static VALUE getColumnBaseType(VALUE, VALUE);
static VALUE selectRowEntries(VALUE);
static VALUE rowToArray(VALUE);
static VALUE rowToHash(VALUE);
static VALUE rowValuesAt(int, VALUE *, VALUE);

/* Globals. */
VALUE cRow;


/**
 * This function integrates with the Ruby memory allocation system to allocate
 * space for new Row objects.
 *
 * @param  klass  A reference to the Row class.
 *
 * @return  A reference to the Row class instance allocated.
 *
 */
static VALUE allocateRow(VALUE klass)
{
   VALUE     row;
   RowHandle *handle = ALLOC(RowHandle);
   
   if(handle != NULL)
   {
      /* Initialise the row fields. */
      handle->size    = 0;
      handle->number  = 0;
      handle->columns = NULL;
      row             = Data_Wrap_Struct(klass, NULL, freeRow, handle);
   }
   else
   {
      /* Generate an exception. */
      rb_raise(rb_eNoMemError, "Memory allocation failure allocating a row.");
   }
   
   return(row);
}


/**
 * This function provides the initialize method for the Row class.
 *
 * @param  self     A reference to the Row object to be initialized.
 * @param  results  A reference to the ResultSet object that generated the row
 *                  of data.
 * @param  data     A reference to an array object containing the data for the
 *                  row. This will be in the form of an array of arrays, with
 *                  each contained array containing two elements - the column
 *                  value and the column base type.
 * @param  number   A reference to the row number to be associated with the
 *                  row.
 *
 * @return  A reference to the initialize Row object.
 *
 */
static VALUE initializeRow(VALUE self, VALUE results, VALUE data, VALUE number)
{
   RowHandle *row  = NULL;
   VALUE     value = rb_funcall(data, rb_intern("size"), 0);
   
   Data_Get_Struct(self, RowHandle, row);
   rb_iv_set(self, "@number", number);
   row->size = TYPE(value) == T_FIXNUM ? FIX2INT(value) : NUM2INT(value);
   if(row->size > 0)
   {
      row->columns = ALLOC_N(ColumnHandle, row->size);
      
      if(row->columns != NULL)
      {
         int i;
         
         memset(row->columns, 0, sizeof(ColumnHandle) * row->size);
         for(i = 0; i < row->size; i++)
         {
            VALUE index,
                  name,
                  alias,
                  scale,
                  items;
                  
            index = INT2NUM(i);
            name  = rb_funcall(results, rb_intern("column_name"), 1, index);
            alias = rb_funcall(results, rb_intern("column_alias"), 1, index);
            strcpy(row->columns[i].name, StringValuePtr(name));
            strcpy(row->columns[i].alias, StringValuePtr(alias));
            items = rb_ary_entry(data, i);
            row->columns[i].value = rb_ary_entry(items, 0);
            row->columns[i].type  = rb_ary_entry(items, 1);
            row->columns[i].scale = rb_funcall(results, rb_intern("column_scale"), 1, index);
            
            if(TYPE(rb_ary_entry(items, 1)) == T_NIL)
            {
               fprintf(stderr, "Nil column type encountered.\n");
            }
         }
      }
      else
      {
         rb_raise(rb_eNoMemError, "Memory allocation failure populating row.");
      }
   }
   
   return(self);
}


/**
 * This function provides the column_count method for the Row class.
 *
 * @param  self  A reference to the Row object to fetch the column count for.
 *
 * @return  The number of columns that make up the row.
 *
 */
static VALUE columnsInRow(VALUE self)
{
   RowHandle *row = NULL;
   
   Data_Get_Struct(self, RowHandle, row);
   
   return(INT2NUM(row->size));
}


/**
 * This function provides the number method for the Row class.
 *
 * @param  self  A reference to the Row object to retrieve the number for.
 *
 * @param  A reference to the row number.
 *
 */
static VALUE getRowNumber(VALUE self)
{
   return(rb_iv_get(self, "@number"));
}


/**
 * This function provides the column_name method for the Row class.
 *
 * @param  self   A reference to the Row object to fetch the column name for.
 * @param  index  A reference to the column index to retrieve the name for.
 *
 * @return  A reference to the column name or nil if an invalid index is
 *          specified.
 *
 */
static VALUE getColumnName(VALUE self, VALUE index)
{
   VALUE     name   = Qnil;
   RowHandle *row   = NULL;
   int       number = 0;
   
   Data_Get_Struct(self, RowHandle, row);
   number = TYPE(index) == T_FIXNUM ? FIX2INT(index) : NUM2INT(index);
   if(number >= 0 && number < row->size)
   {
      name = rb_str_new2(row->columns[number].name);
   }
   
   return(name);
}


/**
 * This function provides the column_alias method for the Row class.
 *
 * @param  self   A reference to the Row object to fetch the column alias for.
 * @param  index  A reference to the column index to retrieve the alias for.
 *
 * @return  A reference to the column alias or nil if an invalid index is
 *          specified.
 *
 */
static VALUE getColumnAlias(VALUE self, VALUE index)
{
   VALUE     alias  = Qnil;
   RowHandle *row   = NULL;
   int       number = 0;
   
   Data_Get_Struct(self, RowHandle, row);
   number = TYPE(index) == T_FIXNUM ? FIX2INT(index) : NUM2INT(index);
   if(number >= 0 && number < row->size)
   {
      alias = rb_str_new2(row->columns[number].alias);
   }
   
   return(alias);
}


/**
 * This function provides the [] method for the Row class.
 *
 * @param  self   A reference to the Row object to fetch the column alias for.
 * @param  index  Either the column index or the column name of the column to
 *                retrieve the value for.
 *
 * @return  A reference to the column value or nil if an invalid column index
 *          is specified.
 *
 */
static VALUE getColumnValue(VALUE self, VALUE index)
{
   VALUE     value  = Qnil;
   RowHandle *row   = NULL;

   Data_Get_Struct(self, RowHandle, row);
   if(TYPE(index) == T_STRING)
   {
      char  name[32];
      int   i,
            done = 0;
      VALUE flag = getFireRubySetting("ALIAS_KEYS");
      
      strcpy(name, StringValuePtr(index));
      for(i = 0; i < row->size && done == 0; i++)
      {
         int match;
         
         /* Check whether its column name or column alias to compare on. */
         if(flag == Qtrue)
         {
            match = strcmp(name, row->columns[i].alias);
         }
         else
         {
            match = strcmp(name, row->columns[i].name);
         }
         
         if(match == 0)
         {
            value = row->columns[i].value;
            done  = 1;
         }
      }
   }
   else
   {
      int number = TYPE(index) == T_FIXNUM ? FIX2INT(index) : NUM2INT(index);
      
      if(number >= 0 && number < row->size)
      {
         value = row->columns[number].value;
      }
   }
   
   return(value);
}


/**
 * This function provides the each method for the Row class.
 *
 * @param  self  A reference to the Row object that the method is being called
 *               for.
 *
 * @return  A reference to the last value returned from the block or nil if no
 *          block was provided.
 *
 */
VALUE eachColumn(VALUE self)
{
   VALUE result = Qnil;
   
   if(rb_block_given_p())
   {
      RowHandle *row = NULL;
      int       i;
      VALUE     flag = getFireRubySetting("ALIAS_KEYS");
      
      Data_Get_Struct(self, RowHandle, row);
      for(i = 0; i < row->size; i++)
      {
         VALUE parameters = rb_ary_new();
         
         /* Decide whether we're keying on column name or alias. */
         if(flag == Qtrue)
         {
            rb_ary_push(parameters, rb_str_new2(row->columns[i].alias));
         }
         else
         {
            rb_ary_push(parameters, rb_str_new2(row->columns[i].name));
         }
         rb_ary_push(parameters, row->columns[i].value);
         result = rb_yield(parameters);
      }
   }
   
   return(result);
}


/**
 * This function provides iteration across the column identifiers (either name
 * or alias depending on library settings) of a Row object.
 *
 * @param  self  A reference to the Row object that the method is being called
 *               for.
 *
 * @return  A reference to the last value returned from the block or nil if no
 *          block was provided.
 *
 */
VALUE eachColumnKey(VALUE self)
{
   VALUE result = Qnil;
   
   if(rb_block_given_p())
   {
      RowHandle *row = NULL;
      int       i;
      VALUE     flag = getFireRubySetting("ALIAS_KEYS");
      
      Data_Get_Struct(self, RowHandle, row);
      for(i = 0; i < row->size; i++)
      {
         if(flag == Qtrue)
         {
            result = rb_yield(rb_str_new2(row->columns[i].alias));
         }
         else
         {
            result = rb_yield(rb_str_new2(row->columns[i].name));
         }
      }
   }
   
   return(result);
}


/**
 * This function provides the each_value method for the Row class.
 *
 * @param  self  A reference to the Row object that the method is being called
 *               for.
 *
 * @return  A reference to the last value returned from the block or nil if no
 *          block was provided.
 *
 */
VALUE eachColumnValue(VALUE self)
{
   VALUE result = Qnil;
   
   if(rb_block_given_p())
   {
      RowHandle *row = NULL;
      int       i;
      
      Data_Get_Struct(self, RowHandle, row);
      for(i = 0; i < row->size; i++)
      {
         result = rb_yield(row->columns[i].value);
      }
   }
   
   return(result);
}


/**
 * This function provides the fetch method for the Row class.
 *
 * @param  self        A reference to the Row class that the method is being 
 *                     called for.
 * @param  parameters  A reference to an array containing the parameters for
 *                     the method call. Must contain at least a key.
 *
 * @return  A reference to the column value, the alternative value of the
 *          return value for any block specified.
 *
 */
VALUE fetchRowValue(int size, VALUE *parameters, VALUE self)
{
   VALUE value = Qnil;
   
   if(size < 1)
   {
      rb_raise(rb_eArgError, "Wrong number of arguments (%d for %d)", size, 1);
   }

   /* Extract the parameters. */
   value = getColumnValue(self, parameters[0]);
   if(value == Qnil)
   {
      if(size == 1 && rb_block_given_p())
      {
         value = rb_yield(rb_ary_new());
      }
      else
      {
         if(size == 1)
         {
            rb_raise(rb_eIndexError, "Column identifier '%s' not found in row.",
                     StringValuePtr(parameters[0]));
         }
         value = parameters[1];
      }
   }
   
   return(value);
}


/**
 * This function provides the has_key? method for the Row class.
 *
 * @param  self  A reference to the Row object to call the method on.
 * @param  name  A reference to a String containing the name of the column to
 *               check for.
 *
 * @return  True if the row possesses the specified column name, false
 *          otherwise.
 *
 */
VALUE hasColumnKey(VALUE self, VALUE name)
{
   VALUE     result = Qfalse;
   RowHandle *row   = NULL;
   char      text[32];
   int       i;
   VALUE     flag = getFireRubySetting("ALIAS_KEYS");
   
   Data_Get_Struct(self, RowHandle, row);
   strcpy(text, StringValuePtr(name));
   for(i = 0; i < row->size && result == Qfalse; i++)
   {
      int match;
      
      /* Check whether key is column name or alias. */
      if(flag == Qtrue)
      {
         match = strcmp(text, row->columns[i].alias);
      }
      else
      {
         match = strcmp(text, row->columns[i].name);
      }
      
      if(match == 0)
      {
         result = Qtrue;
      }
   }
   
   return(result);
}


/**
 * This function provides the has_column? method for the Row class.
 *
 * @param  self  A reference to the Row object to call the method on.
 * @param  name  A reference to a String containing the name of the column to
 *               check for.
 *
 * @return  True if the row possesses the specified column name, false
 *          otherwise.
 *
 */
VALUE hasColumnName(VALUE self, VALUE name)
{
   VALUE     result = Qfalse;
   RowHandle *row   = NULL;
   char      text[32];
   int       i;
   
   Data_Get_Struct(self, RowHandle, row);
   strcpy(text, StringValuePtr(name));
   for(i = 0; i < row->size && result == Qfalse; i++)
   {
      if(strcmp(text, row->columns[i].name) == 0)
      {
         result = Qtrue;
      }
   }
   
   return(result);
}


/**
 * This function provides the has_alias? method for the Row class.
 *
 * @param  self  A reference to the Row object to call the method on.
 * @param  name  A reference to a String containing the alias of the column to
 *               check for.
 *
 * @return  True if the row possesses the specified column alias, false
 *          otherwise.
 *
 */
VALUE hasColumnAlias(VALUE self, VALUE name)
{
   VALUE     result = Qfalse;
   RowHandle *row   = NULL;
   char      text[32];
   int       i;
   
   Data_Get_Struct(self, RowHandle, row);
   strcpy(text, StringValuePtr(name));
   for(i = 0; i < row->size && result == Qfalse; i++)
   {
      if(strcmp(text, row->columns[i].alias) == 0)
      {
         result = Qtrue;
      }
   }
   
   return(result);
}


/**
 * This function provides the has_value? method for the Row class.
 *
 * @param  self   A reference to the Row object to call the method on.
 * @param  value  A reference to the value to be checked for.
 *
 * @return  True if the row contains a matching value, false otherwise.
 *
 */
VALUE hasColumnValue(VALUE self, VALUE value)
{
   VALUE     result = Qfalse;
   RowHandle *row   = NULL;
   int       i;
   
   Data_Get_Struct(self, RowHandle, row);
   for(i = 0; i < row->size && result == Qfalse; i++)
   {
      result = rb_funcall(row->columns[i].value, rb_intern("eql?"), 1, value);
   }
   
   return(result);
}


/**
 * This function fetches a list of column index keys for a Row object. What the
 * keys are depends on the library settings, but they will either be the column
 * names or the column aliases.
 *
 * @param  self  A reference to the Row object to make the call on.
 *
 * @return  A reference to an array containing the row keys.
 *
 */
VALUE getColumnKeys(VALUE self)
{
   VALUE flag = getFireRubySetting("ALIAS_KEYS"),
         keys = Qnil;
         
   if(flag == Qtrue)
   {
      keys = getColumnAliases(self);
   }
   else
   {
      keys = getColumnNames(self);
   }
   
   return(keys);
}


/**
 * This function provides the keys method for the Row class.
 *
 * @param  self  A reference to the Row object to call the method on.
 *
 * @return  A reference to an array containing the row column names. 
 *
 */
VALUE getColumnNames(VALUE self)
{
   VALUE     result = rb_ary_new();
   RowHandle *row   = NULL;
   int       i;
   
   Data_Get_Struct(self, RowHandle, row);
   for(i = 0; i < row->size; i++)
   {
      rb_ary_push(result, rb_str_new2(row->columns[i].name));
   }
   
   return(result);
}


/**
 * This function provides the aliases method for the Row class.
 *
 * @param  self  A reference to the Row object to call the method on.
 *
 * @return  A reference to an array containing the row column aliases. 
 *
 */
VALUE getColumnAliases(VALUE self)
{
   VALUE     result = rb_ary_new();
   RowHandle *row   = NULL;
   int       i;
   
   Data_Get_Struct(self, RowHandle, row);
   for(i = 0; i < row->size; i++)
   {
      rb_ary_push(result, rb_str_new2(row->columns[i].alias));
   }
   
   return(result);
}


/**
 * This function provides the values method for the Row class.
 *
 * @param  self  A reference to the Row object to call the method on.
 *
 * @return  A reference to an array containing the row column names. 
 *
 */
VALUE getColumnValues(VALUE self)
{
   VALUE     result = rb_ary_new();
   RowHandle *row   = NULL;
   int       i;
   
   Data_Get_Struct(self, RowHandle, row);
   for(i = 0; i < row->size; i++)
   {
      rb_ary_push(result, row->columns[i].value);
   }
   
   return(result);
}


/**
 * This function provides the get_base_type method for the Row class.
 *
 * @param  self   A reference to the Row object to call the method on.
 * @param  index  The index of the column to retrieve the base type for.
 *
 * @return  An Symbol containing the base type details.
 *
 */
VALUE getColumnBaseType(VALUE self, VALUE index)
{
   VALUE result = Qnil;

   if(TYPE(index) == T_FIXNUM)
   {
      RowHandle *row   = NULL;

      Data_Get_Struct(self, RowHandle, row);
      if(row != NULL)
      {
         int offset = FIX2INT(index);

         /* Correct negative index values. */
         if(offset < 0)
         {
            offset = row->size + offset;
         }

         if(offset >= 0 && offset < row->size)
         {
            result = row->columns[offset].type;
         }
      }
   }

   return(result);
}

/**
 * This function provides the column_scale method for the Row class.
 *
 * @param  self    A reference to the Row object to retrieve the column
 *                 alias from.
 * @param  column  An offset to the column to retrieve the scale of.
 *
 * @return  An Integer representing the scale of the column, or nil if an
 *          invalid column was specified.
 *
 */
static VALUE getColumnScale(VALUE self, VALUE index)
{
   VALUE result = Qnil;

   if(TYPE(index) == T_FIXNUM)
   {
      RowHandle *row   = NULL;

      Data_Get_Struct(self, RowHandle, row);
      if(row != NULL)
      {
         int offset = FIX2INT(index);

         /* Correct negative index values. */
         if(offset < 0)
         {
            offset = row->size + offset;
         }

         if(offset >= 0 && offset < row->size)
         {
            result = row->columns[offset].scale;
         }
      }
   }

   return(result);
}


/**
 * This function provides the select method for the Row class.
 *
 * @param  self  A reference to the Row object to make the method call on.
 *
 * @return  An array containing the entries selected by the block passed to the
 *          function.
 *
 */
VALUE selectRowEntries(VALUE self)
{
   VALUE     result = Qnil,
             flag   = getFireRubySetting("ALIAS_KEYS");
   RowHandle *row   = NULL;
   int       i;
   
   if(!rb_block_given_p())
   {
      rb_raise(rb_eStandardError, "No block specified in call to Row#select.");
   }
   
   result = rb_ary_new();
   Data_Get_Struct(self, RowHandle, row);
   for(i = 0; i < row->size; i++)
   {
      VALUE parameters = rb_ary_new();
      
      /* Check whether we're keying on column name or alias. */
      if(flag == Qtrue)
      {
         rb_ary_push(parameters, rb_str_new2(row->columns[i].alias));
      }
      else
      {
         rb_ary_push(parameters, rb_str_new2(row->columns[i].name));
      }
      rb_ary_push(parameters, row->columns[i].value);
      if(rb_yield(parameters) == Qtrue)
      {
         rb_ary_push(result, parameters);
      }
   }
   
   
   return(result);
}


/**
 * This function provides the to_a method for the Row class.
 *
 * @param  self  A reference to the Row object to make the method call on.
 *
 * @return  An array containing the entries from the Row object.
 *
 */
VALUE rowToArray(VALUE self)
{
   VALUE     result = rb_ary_new(),
             flag   = getFireRubySetting("ALIAS_KEYS");
   RowHandle *row   = NULL;
   int       i;
   
   Data_Get_Struct(self, RowHandle, row);
   for(i = 0; i < row->size; i++)
   {
      VALUE parameters = rb_ary_new();
      
      /* Check whether we're keying on column name or alias. */
      if(flag == Qtrue)
      {
         rb_ary_push(parameters, rb_str_new2(row->columns[i].alias));
      }
      else
      {
         rb_ary_push(parameters, rb_str_new2(row->columns[i].name));
      }
      rb_ary_push(parameters, row->columns[i].value);
      rb_ary_push(result, parameters);
   }
   
   return(result);
}


/**
 * This function provides the to_hash method for the Row class.
 *
 * @param  self  A reference to the Row object to make the method call on.
 *
 * @return  A hash containing the entries from the Row object.
 *
 */
VALUE rowToHash(VALUE self)
{
   VALUE     result = rb_hash_new(),
             flag   = getFireRubySetting("ALIAS_KEYS");
   RowHandle *row   = NULL;
   int       i;
   
   Data_Get_Struct(self, RowHandle, row);
   for(i = 0; i < row->size; i++)
   {
      VALUE key = Qnil;
      
      /* Check if we're keying on column name or alias. */
      if(flag == Qtrue)
      {
         key = rb_str_new2(row->columns[i].alias);
      }
      else
      {
         key = rb_str_new2(row->columns[i].name);
      }
      rb_hash_aset(result, key, row->columns[i].value);
   }
   
   return(result);
}


/**
 * This function provides the values_at method for the Row class.
 *
 * @param  self  A reference to the Row object to call the method on.
 * @param  keys  An array containing the name of the columns that should be
 *               included in the output array.
 *
 * @return  An array of the values that match the column names specified.
 *
 */
VALUE rowValuesAt(int size, VALUE *keys, VALUE self)
{
   VALUE result = rb_ary_new();
   int   i;
   
   for(i = 0; i < size; i++)
   {
      rb_ary_push(result, getColumnValue(self, keys[i]));
   }
   
   return(result);
}


/**
 * This function integrates with the Ruby garbage collector to insure that
 * all resources associated with a Row object are released whenever the Row
 * object is collected.
 *
 * @param  row  A pointer to the RowHandle object for the Row object.
 *
 */
void freeRow(void *row)
{
   if(row != NULL)
   {
      RowHandle *handle = (RowHandle *)row;
      
      if(handle->columns != NULL)
      {
         free(handle->columns);
      }
      free(handle);
   }
}


/**
 * This function provides a programmatic means of creating a Row object.
 *
 * @param  results  A reference to the ResultSet object that the row relates to.
 * @param  data     A reference to an array containing the row data.
 * @param  number   A reference to the number to be associated with the row.
 *
 * @return  A reference to the Row object created.
 *
 */
VALUE rb_row_new(VALUE results, VALUE data, VALUE number)
{
   VALUE row = allocateRow(cRow);
   
   initializeRow(row, results, data, number);
   
   return(row);
}


/**
 * This function is used to create and initialize the Row class within the
 * Ruby environment.
 *
 * @param  module  A reference to the module that the Row class will be created
 *                 under.
 *
 */
void Init_Row(VALUE module)
{
   cRow = rb_define_class_under(module, "Row", rb_cObject);
   rb_define_alloc_func(cRow, allocateRow);
   rb_include_module(cRow, rb_mEnumerable);
   rb_define_method(cRow, "initialize", initializeRow, 3);
   rb_define_method(cRow, "number", getRowNumber, 0);
   rb_define_method(cRow, "column_count", columnsInRow, 0);
   rb_define_method(cRow, "column_name", getColumnName, 1);
   rb_define_method(cRow, "column_alias", getColumnAlias, 1);
   rb_define_method(cRow, "column_scale", getColumnScale, 1);
   rb_define_method(cRow, "each", eachColumn, 0);
   rb_define_method(cRow, "each_key", eachColumnKey, 0);
   rb_define_method(cRow, "each_value", eachColumnValue, 0);
   rb_define_method(cRow, "[]", getColumnValue, 1);
   rb_define_method(cRow, "fetch", fetchRowValue, -1);
   rb_define_method(cRow, "has_key?", hasColumnKey, 1);
   rb_define_method(cRow, "has_column?", hasColumnName, 1);
   rb_define_method(cRow, "has_alias?", hasColumnAlias, 1);
   rb_define_method(cRow, "has_value?", hasColumnValue, 1);
   rb_define_method(cRow, "keys", getColumnKeys, 0);
   rb_define_method(cRow, "names", getColumnNames, 0);
   rb_define_method(cRow, "aliases", getColumnAliases, 0);
   rb_define_method(cRow, "values", getColumnValues, 0);
   rb_define_method(cRow, "get_base_type", getColumnBaseType, 1);
   rb_define_method(cRow, "select", selectRowEntries, 0);
   rb_define_method(cRow, "to_a", rowToArray, 0);
   rb_define_method(cRow, "to_hash", rowToHash, 0);
   rb_define_method(cRow, "values_at", rowValuesAt, -1);;

   rb_define_alias(cRow, "each_pair", "each");
   rb_define_alias(cRow, "include?", "has_key?");
   rb_define_alias(cRow, "key?", "has_key?");
   rb_define_alias(cRow, "member?", "has_key?");
   rb_define_alias(cRow, "value?", "has_value?");
   rb_define_alias(cRow, "length", "column_count");
   rb_define_alias(cRow, "size", "column_count");
}
