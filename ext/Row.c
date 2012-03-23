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
#include "TypeMap.h"
#include "FireRuby.h"
#include "ibase.h"
#include "ruby.h"

/* Function prototypes. */
static VALUE initializeRow(VALUE, VALUE, VALUE);
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
ID EQL_ID, FETCH_ID, COLUMN_NAME_ID, COLUMN_ALIAS_ID, COLUMN_SCALE_ID,
  AT_NAME_ID, AT_ALIAS_ID, AT_KEY_ID, AT_SCALE_ID, AT_COLUMNS_ID, AT_NUMBER_ID,
  AT_VALUE_ID, AT_TYPE_ID, NEW_ID;

VALUE getColumns(VALUE self) {
  return rb_ivar_get(self, AT_COLUMNS_ID);
}

VALUE checkRowOffset(VALUE columns, VALUE index) {
  int offset = FIX2INT(index);

  /* Correct negative index values. */
  if(offset < 0) {
    return INT2NUM(RARRAY_LEN(columns) + offset);
  }
  return index;
}

VALUE rowScan(VALUE self, VALUE key, ID slot) {
  long idx;
  VALUE columns = getColumns(self);

  for(idx=0; idx < RARRAY_LEN(columns); idx++) {
    VALUE field = rb_ary_entry(columns, idx);
    if(Qtrue == rb_funcall(rb_ivar_get(field, slot), EQL_ID, 1, key)) {
      return(field);
    }
  }
  return(Qnil);
}

VALUE rowCollect(VALUE self, ID slot) {
  VALUE columns = getColumns(self);
  long idx, size = RARRAY_LEN(columns);
  VALUE result = rb_ary_new2(size);
  for(idx=0; idx < size; idx++) {
    VALUE field = rb_ary_entry(columns, idx);
    rb_ary_store(result, idx, rb_ivar_get(field, slot));
  }
  return(result);
}

VALUE getField(VALUE columns, VALUE index) {
  return rb_funcall(columns, FETCH_ID, 1, index);
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
static VALUE initializeRow(VALUE self, VALUE results, VALUE number) {
  long idx;
  VALUE key_flag = getFireRubySetting("ALIAS_KEYS"),
        data = toFieldsArray(results);
  rb_ivar_set(self, AT_NUMBER_ID, number);
  rb_ivar_set(self, AT_COLUMNS_ID, data);

  for(idx=0; idx < RARRAY_LEN(data); idx++) {
    VALUE index = INT2NUM(idx),
          field = rb_ary_entry(data, idx),
          name = rb_funcall(results, COLUMN_NAME_ID, 1, index),
          alias = rb_funcall(results, COLUMN_ALIAS_ID, 1, index);
    rb_ivar_set(field, AT_NAME_ID, name);
    rb_ivar_set(field, AT_ALIAS_ID, alias);
    rb_ivar_set(field, AT_SCALE_ID, rb_funcall(results, COLUMN_SCALE_ID, 1, index));
    if(key_flag == Qtrue) {
      rb_ivar_set(field, AT_KEY_ID, alias);
    } else {
      rb_ivar_set(field, AT_KEY_ID, name);
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
static VALUE columnsInRow(VALUE self) {
  return LONG2NUM(RARRAY_LEN(getColumns(self)));
}


/**
 * This function provides the number method for the Row class.
 *
 * @param  self  A reference to the Row object to retrieve the number for.
 *
 * @param  A reference to the row number.
 *
 */
static VALUE getRowNumber(VALUE self) {
  return(rb_ivar_get(self, AT_NUMBER_ID));
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
static VALUE getColumnName(VALUE self, VALUE index) {
  return rb_ivar_get(getField(getColumns(self), index), AT_NAME_ID);
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
static VALUE getColumnAlias(VALUE self, VALUE index) {
  return rb_ivar_get(getField(getColumns(self), index), AT_ALIAS_ID);
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
static VALUE getColumnValue(VALUE self, VALUE index) {
  VALUE fld;

  if(TYPE(index) == T_STRING) {
    fld = rowScan(self, index, AT_KEY_ID);
  } else {
    fld = getField(getColumns(self), index);
  }
  if(Qnil == fld) {
    return(Qnil);
  }
  return rb_ivar_get(fld, AT_VALUE_ID);
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
VALUE eachColumn(VALUE self) {
  VALUE result = Qnil;

  if(rb_block_given_p()) {
    long i;
    VALUE columns = getColumns(self);
    for(i = 0; i < RARRAY_LEN(columns); i++) {
      VALUE fld = rb_ary_entry(columns, i),
                  parameters = rb_ary_new();
      rb_ary_push(parameters, rb_ivar_get(fld, AT_KEY_ID));
      rb_ary_push(parameters, rb_ivar_get(fld, AT_VALUE_ID));

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
VALUE eachColumnKey(VALUE self) {
  VALUE result = Qnil;

  if(rb_block_given_p()) {
    long i;
    VALUE columns = getColumns(self);
    for(i = 0; i < RARRAY_LEN(columns); i++) {
      result = rb_yield(rb_ivar_get(rb_ary_entry(columns, i), AT_KEY_ID));
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
VALUE eachColumnValue(VALUE self) {
  VALUE result = Qnil;

  if(rb_block_given_p()) {
    long i;
    VALUE columns = getColumns(self);
    for(i = 0; i < RARRAY_LEN(columns); i++) {
      result = rb_yield(rb_ivar_get(rb_ary_entry(columns, i), AT_VALUE_ID));
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
VALUE fetchRowValue(int size, VALUE *parameters, VALUE self) {
  VALUE value = Qnil;

  if(size < 1) {
    rb_raise(rb_eArgError, "Wrong number of arguments (%d for %d)", size, 1);
  }

  /* Extract the parameters. */
  value = getColumnValue(self, parameters[0]);
  if(value == Qnil) {
    if(size == 1 && rb_block_given_p()) {
      value = rb_yield(rb_ary_new());
    } else {
      if(size == 1) {
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
VALUE hasColumnKey(VALUE self, VALUE name) {
  if(Qnil == rowScan(self, name, AT_KEY_ID)) {
    return Qfalse;
  }
  return Qtrue;
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
VALUE hasColumnName(VALUE self, VALUE name) {
  if(Qnil == rowScan(self, name, AT_NAME_ID)) {
    return Qfalse;
  }
  return Qtrue;
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
VALUE hasColumnAlias(VALUE self, VALUE name) {
  if(Qnil == rowScan(self, name, AT_ALIAS_ID)) {
    return Qfalse;
  }
  return Qtrue;
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
VALUE hasColumnValue(VALUE self, VALUE value) {
  long i;
  VALUE columns = getColumns(self);
  for(i = 0; i < RARRAY_LEN(columns); i++) {
    if(Qtrue == rb_funcall(rb_ivar_get(rb_ary_entry(columns, i), AT_VALUE_ID), EQL_ID, 1, value)) {
      return(Qtrue);
    }
  }
  return(Qfalse);
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
VALUE getColumnKeys(VALUE self) {
  return rowCollect(self, AT_KEY_ID);
}


/**
 * This function provides the keys method for the Row class.
 *
 * @param  self  A reference to the Row object to call the method on.
 *
 * @return  A reference to an array containing the row column names.
 *
 */
VALUE getColumnNames(VALUE self) {
  return rowCollect(self, AT_NAME_ID);
}


/**
 * This function provides the aliases method for the Row class.
 *
 * @param  self  A reference to the Row object to call the method on.
 *
 * @return  A reference to an array containing the row column aliases.
 *
 */
VALUE getColumnAliases(VALUE self) {
  return rowCollect(self, AT_ALIAS_ID);
}


/**
 * This function provides the values method for the Row class.
 *
 * @param  self  A reference to the Row object to call the method on.
 *
 * @return  A reference to an array containing the row column names.
 *
 */
VALUE getColumnValues(VALUE self) {
  return rowCollect(self, AT_VALUE_ID);
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
VALUE getColumnBaseType(VALUE self, VALUE index) {
  VALUE columns = getColumns(self);
  return rb_ivar_get(getField(columns, checkRowOffset(columns, index)), AT_TYPE_ID);
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
static VALUE getColumnScale(VALUE self, VALUE index) {
  VALUE columns = getColumns(self);
  return rb_ivar_get(getField(columns, checkRowOffset(columns, index)), AT_SCALE_ID);
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
VALUE selectRowEntries(VALUE self) {
  long i;
  VALUE result = rb_ary_new(),
        columns = getColumns(self);

  if(!rb_block_given_p()) {
    rb_raise(rb_eStandardError, "No block specified in call to Row#select.");
  }

  for(i = 0; i < RARRAY_LEN(columns); i++) {
    VALUE fld = rb_ary_entry(columns, i),
                parameters = rb_ary_new();
    rb_ary_push(parameters, rb_ivar_get(fld, AT_KEY_ID));
    rb_ary_push(parameters, rb_ivar_get(fld, AT_VALUE_ID));
    if(rb_yield(parameters) == Qtrue) {
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
VALUE rowToArray(VALUE self) {
  long i;
  VALUE result = rb_ary_new(),
        columns = getColumns(self);

  for(i = 0; i < RARRAY_LEN(columns); i++) {
    VALUE fld = rb_ary_entry(columns, i),
                parameters = rb_ary_new();
    rb_ary_push(parameters, rb_ivar_get(fld, AT_KEY_ID));
    rb_ary_push(parameters, rb_ivar_get(fld, AT_VALUE_ID));
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
VALUE rowToHash(VALUE self) {
  long i;
  VALUE result = rb_hash_new(),
        columns = getColumns(self);

  for(i = 0; i < RARRAY_LEN(columns); i++) {
    VALUE fld = rb_ary_entry(columns, i);
    rb_hash_aset(result, rb_ivar_get(fld, AT_KEY_ID), rb_ivar_get(fld, AT_VALUE_ID));
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
VALUE rowValuesAt(int size, VALUE *keys, VALUE self) {
  int i;
  VALUE result = rb_ary_new();

  for(i = 0; i < size; i++) {
    rb_ary_push(result, getColumnValue(self, keys[i]));
  }

  return(result);
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
VALUE rb_row_new(VALUE results, VALUE number) {
  VALUE row = rb_funcall(cRow, NEW_ID, 2, results, number);

  RB_GC_GUARD(row);

  initializeRow(row, results, number);

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
void Init_Row(VALUE module) {
  Init_TypeMap();
  EQL_ID = rb_intern("eql?");
  FETCH_ID = rb_intern("fetch");
  COLUMN_NAME_ID = rb_intern("column_name");
  COLUMN_ALIAS_ID = rb_intern("column_alias");
  COLUMN_SCALE_ID = rb_intern("column_scale");
  AT_NAME_ID = rb_intern("@name");
  AT_ALIAS_ID = rb_intern("@alias");
  AT_KEY_ID = rb_intern("@key");
  AT_SCALE_ID = rb_intern("@scale");
  AT_COLUMNS_ID = rb_intern("@columns");
  AT_NUMBER_ID = rb_intern("@number");
  AT_VALUE_ID = rb_intern("@value");
  AT_TYPE_ID = rb_intern("@type");
  NEW_ID = rb_intern("new");
  
  cRow = rb_define_class_under(module, "Row", rb_cObject);
  rb_include_module(cRow, rb_mEnumerable);
  rb_define_method(cRow, "initialize", initializeRow, 2);
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
