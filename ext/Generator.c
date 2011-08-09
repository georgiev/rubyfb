/*------------------------------------------------------------------------------
 * Generator.c
 *------------------------------------------------------------------------------
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
#include "Generator.h"
#include "Common.h"
#include "ResultSet.h"
#include "Statement.h"
#include "Transaction.h"
#include "DataArea.h"
#include "rfbint.h"

/* Function prototypes. */
static VALUE initializeGenerator(VALUE, VALUE, VALUE);
static VALUE getGeneratorName(VALUE);
static VALUE getGeneratorConnection(VALUE);
static VALUE getLastGeneratorValue(VALUE);
static VALUE getNextGeneratorValue(VALUE, VALUE);
static VALUE dropGenerator(VALUE);
static VALUE doesGeneratorExist(VALUE, VALUE, VALUE);
static VALUE createGenerator(VALUE, VALUE, VALUE);
XSQLDA *createStorage(void);

/* Globals. */
VALUE cGenerator;

/**
 * This function provides the initialize method for the Generator class.
 *
 * @param  self        A reference to the object instance being initialized.
 * @param  name        A reference to a string containing the name of the
 *                     generator.
 * @param  connection  A reference to the Connection object that will be used
 *                     by the Generator.
 *
 * @return  A reference to the newly initialized Generator object.
 *
 */
static VALUE initializeGenerator(VALUE self,
                                 VALUE name,
                                 VALUE connection) {
  if(TYPE(name) != T_STRING) {
    rb_fireruby_raise(NULL, "Invalid generator name specified.");
  }

  if(TYPE(connection) != T_DATA &&
     RDATA(connection)->dfree != (RUBY_DATA_FUNC)connectionFree) {
    rb_fireruby_raise(NULL, "Invalid connection specified for generator.");
  }

  rb_iv_set(self, "@name", name);
  rb_iv_set(self, "@connection", connection);

  return(self);
}


/**
 * This function provides the accessor for the name attribute of the Generator
 * class.
 *
 * @param  self  A reference to the Generator object to fetch the name for.
 *
 * @return  A reference to a String containing the Generator name.
 *
 */
static VALUE getGeneratorName(VALUE self) {
  return(rb_iv_get(self, "@name"));
}


/**
 * This function provides the accessor for the connection attribute for the
 * Generator class.
 *
 * @param  self  A reference to the Generator object to fetch the connection
 *               for.
 *
 * @return  A reference to the Connection object for the generator.
 *
 */
static VALUE getGeneratorConnection(VALUE self) {
  return(rb_iv_get(self, "@connection"));
}


/**
 * This function fetches the last value retrieved from a Generator.
 *
 * @param  self  A reference to the Generator to fetch the value for.
 *
 * @return  A reference to the last value retrieved from the generator.
 *
 */
static VALUE getLastGeneratorValue(VALUE self) {
  return getNextGeneratorValue(self, INT2NUM(0));
}


/**
 * This function provides the next method for the Generator class.
 *
 * @param  self  A reference to the Generator object to fetch the next value
 *               for.
 * @param  step  The amount of increment to be applied to the Generator.
 *
 * @return  A reference to an integer containing the next generator value.
 *
 */
static VALUE getNextGeneratorValue(VALUE self, VALUE step) {
  VALUE  result_set = Qnil,
         row        = Qnil,
         connection = rb_iv_get(self, "@connection"),
         name       = rb_iv_get(self, "@name");
  char sql[100];

  /* Check the step type. */
  if(TYPE(step) != T_FIXNUM) {
    rb_fireruby_raise(NULL, "Invalid generator step value.");
  }

  sprintf(sql, "SELECT GEN_ID(%s, %d) FROM RDB$DATABASE", StringValuePtr(name), FIX2INT(step));
  result_set = rb_execute_sql(connection, rb_str_new2(sql), Qnil, Qnil);
  row = rb_funcall(result_set, rb_intern("fetch"), 0);
  rb_funcall(result_set, rb_intern("close"), 0);
  
  row = rb_funcall(row, rb_intern("values"), 0);
  return rb_funcall(row, rb_intern("first"), 0);
}


/**
 * This function provides the drop method for the Generator class.
 *
 * @param  self  A reference to the Generator object to be dropped.
 *
 * @return  A reference to the Generator object dropped.
 *
 */
static VALUE dropGenerator(VALUE self) {
  VALUE connection, name;
  char sql[100];

  connection = rb_iv_get(self, "@connection");
  name = rb_iv_get(self, "@name");

  sprintf(sql, "DROP GENERATOR %s", StringValuePtr(name));
  rb_execute_sql(connection, rb_str_new2(sql), Qnil, Qnil);

  return(self);
}


/**
 * This method provides the exists? class method for the Generator class.
 *
 * @param  klass       This parameter is ignored.
 * @param  name        A reference to a String containing the generator name to
 *                     check for.
 * @param  connection  A reference to the Connection object that the check will
 *                     be made through.
 *
 * @return  True if the generator already exists in the database, false
 *          otherwise.
 *
 */
static VALUE doesGeneratorExist(VALUE klass, VALUE name, VALUE connection) {
  VALUE exists = Qfalse,
        result_set = Qnil;
  char sql[200]; // 93(statement) + 2*(31)max_generator_name = 155

  if(TYPE(connection) != T_DATA ||
     RDATA(connection)->dfree != (RUBY_DATA_FUNC)connectionFree) {
    rb_fireruby_raise(NULL, "Invalid connection specified.");
  }

  sprintf(sql, "SELECT RDB$GENERATOR_NAME FROM RDB$GENERATORS WHERE RDB$GENERATOR_NAME in ('%s', UPPER('%s'))", StringValuePtr(name), StringValuePtr(name));
  result_set = rb_execute_sql(connection, rb_str_new2(sql), Qnil, Qnil);
  if(Qnil != rb_funcall(result_set, rb_intern("fetch"), 0)) {
    exists = Qtrue;
  }
  rb_funcall(result_set, rb_intern("close"), 0);

  return(exists);
}


/**
 * This function attempts to create a new Generator given a name and database
 * connection. The function provides the create class method for the Generator
 * class.
 *
 * @param  klass       This parameter is ignored.
 * @param  name        A reference to a String containing the name of the new
 *                     generator.
 * @param  connection  A reference to the Connection object to create the new
 *                     generator through.
 *
 * @return  A reference to a Generator object.
 *
 */
static VALUE createGenerator(VALUE klass, VALUE name, VALUE connection) {
  char sql[100];

  if(TYPE(name) != T_STRING) {
    rb_fireruby_raise(NULL, "Invalid generator name specified.");
  }

  if(TYPE(connection) != T_DATA &&
     RDATA(connection)->dfree != (RUBY_DATA_FUNC)connectionFree) {
    rb_fireruby_raise(NULL,
                      "Invalid connection specified for generator creation.");
  }

  sprintf(sql, "CREATE GENERATOR %s", StringValuePtr(name));
  rb_execute_sql(connection, rb_str_new2(sql), Qnil, Qnil);

  return rb_generator_new(name, connection);
}

/**
 * This function prepares storage space for statements that will fetch values
 * from a generator.
 *
 * @return  A pointer to an allocated XSQLDA that is ready to receive values
 *          from a generator.
 *
 */
XSQLDA *createStorage(void) {
  XSQLDA *da = (XSQLDA *)ALLOC_N(char, XSQLDA_LENGTH(1));

  if(da != NULL) {
    XSQLVAR *var = da->sqlvar;

    da->version   = SQLDA_VERSION1;
    da->sqln      = 1;
    da->sqld      = 1;
    var->sqltype  = SQL_LONG;
    var->sqlscale = 0;
    var->sqllen   = sizeof(long);
    prepareDataArea(da);
  } else {
    rb_raise(rb_eNoMemError,
             "Memory allocation failure allocating generator storage space.");
  }

  return(da);
}


/**
 * This function provides a means of programmatically creating a Generator
 * object.
 *
 * @param  name        A reference to the new generator name.
 * @param  step        A reference to the generator interval step value.
 * @param  connection  A reference to a Connection object.
 *
 * @return  A reference to the new Generator object.
 *
 */
VALUE rb_generator_new(VALUE name, VALUE connection) {
  VALUE args[2];
  args[0] = name;
  args[1] = connection;

  return rb_class_new_instance(2, args, cGenerator);
}

/**
 * This function initializes the Generator class within the Ruby environment.
 * The class is established under the module specified to the function.
 *
 * @param  module  A reference to the module to create the class within.
 *
 */
void Init_Generator(VALUE module) {
  cGenerator = rb_define_class_under(module, "Generator", rb_cObject);
  rb_define_method(cGenerator, "initialize", initializeGenerator, 2);
  rb_define_method(cGenerator, "initialize_copy", forbidObjectCopy, 1);
  rb_define_method(cGenerator, "last", getLastGeneratorValue, 0);
  rb_define_method(cGenerator, "next", getNextGeneratorValue, 1);
  rb_define_method(cGenerator, "connection", getGeneratorConnection, 0);
  rb_define_method(cGenerator, "name", getGeneratorName, 0);
  rb_define_method(cGenerator, "drop", dropGenerator, 0);
  rb_define_module_function(cGenerator, "exists?", doesGeneratorExist, 2);
  rb_define_module_function(cGenerator, "create", createGenerator, 2);
}
