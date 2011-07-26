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
static VALUE allocateGenerator(VALUE);
static VALUE initializeGenerator(VALUE, VALUE, VALUE);
static VALUE getGeneratorName(VALUE);
static VALUE getGeneratorConnection(VALUE);
static VALUE getLastGeneratorValue(VALUE);
static VALUE getNextGeneratorValue(VALUE, VALUE);
static VALUE dropGenerator(VALUE);
static VALUE doesGeneratorExist(VALUE, VALUE, VALUE);
static VALUE createGenerator(VALUE, VALUE, VALUE);
int checkForGenerator(const char *, isc_db_handle *);
int installGenerator(const char *, isc_db_handle *);
int deleteGenerator(const char *, isc_db_handle *);
XSQLDA *createStorage(void);
int32_t getGeneratorValue(const char *, int, isc_db_handle *);

/* Globals. */
VALUE cGenerator;


/**
 * This function provides for the allocation of new Generator objects through
 * the Ruby language.
 *
 * @param  klass  A reference to the Generator Class object.
 *
 * @return  A reference to the newly allocated Generator object.
 *
 */
static VALUE allocateGenerator(VALUE klass) {
  VALUE instance   = Qnil;
  GeneratorHandle *generator = ALLOC(GeneratorHandle);

  if(generator != NULL) {
    generator->connection = NULL;
    instance              = Data_Wrap_Struct(klass, NULL, generatorFree,
                                             generator);
  } else {
    rb_raise(rb_eNoMemError,
             "Memory allocation failure allocating a generator.");
  }

  return(instance);
}


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
  GeneratorHandle  *generator = NULL;
  ConnectionHandle *handle    = NULL;

  if(TYPE(name) != T_STRING) {
    rb_fireruby_raise(NULL, "Invalid generator name specified.");
  }

  if(TYPE(connection) != T_DATA &&
     RDATA(connection)->dfree != (RUBY_DATA_FUNC)connectionFree) {
    rb_fireruby_raise(NULL, "Invalid connection specified for generator.");
  }

  rb_iv_set(self, "@name", name);
  rb_iv_set(self, "@connection", connection);

  Data_Get_Struct(connection, ConnectionHandle, handle);
  Data_Get_Struct(self, GeneratorHandle, generator);
  generator->connection = &handle->handle;

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
  VALUE name       = rb_iv_get(self, "@name");
  GeneratorHandle  *generator = NULL;
  int32_t number     = 0;

  Data_Get_Struct(self, GeneratorHandle, generator);

  number = getGeneratorValue(StringValuePtr(name), 0, generator->connection);

  return(INT2NUM(number));
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
  VALUE name       = rb_iv_get(self, "@name");
  GeneratorHandle  *generator = NULL;
  int32_t number     = 0;

  Data_Get_Struct(self, GeneratorHandle, generator);

  /* Check the step type. */
  if(TYPE(step) != T_FIXNUM) {
    rb_fireruby_raise(NULL, "Invalid generator step value.");
  }

  number = getGeneratorValue(StringValuePtr(name), FIX2INT(step),
                             generator->connection);

  return(INT2NUM(number));
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
  GeneratorHandle *generator = NULL;
  VALUE name;

  Data_Get_Struct(self, GeneratorHandle, generator);
  name = rb_iv_get(self, "@name");

  /* Drop the generator. */
  deleteGenerator(StringValuePtr(name), generator->connection);

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
  VALUE exists     = Qfalse;
  ConnectionHandle *handle    = NULL;

  if(TYPE(connection) != T_DATA ||
     RDATA(connection)->dfree != (RUBY_DATA_FUNC)connectionFree) {
    rb_fireruby_raise(NULL, "Invalid connection specified.");
  }

  Data_Get_Struct(connection, ConnectionHandle, handle);

  if(handle->handle == 0) {
    rb_fireruby_raise(NULL, "Connection is closed.");
  }

  if(checkForGenerator(StringValuePtr(name), &handle->handle)) {
    exists = Qtrue;
  }

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
  VALUE result  = Qnil;
  ConnectionHandle *handle = NULL;

  if(TYPE(name) != T_STRING) {
    rb_fireruby_raise(NULL, "Invalid generator name specified.");
  }

  if(TYPE(connection) != T_DATA &&
     RDATA(connection)->dfree != (RUBY_DATA_FUNC)connectionFree) {
    rb_fireruby_raise(NULL,
                      "Invalid connection specified for generator creation.");
  }

  Data_Get_Struct(connection, ConnectionHandle, handle);
  if(handle->handle != 0) {
    installGenerator(StringValuePtr(name), &handle->handle);
    result = rb_generator_new(name, connection);
  } else {
    rb_fireruby_raise(NULL,
                      "Closed connection specified for generator creation.");
  }

  return(result);
}


/**
 * This function executes a check for a named generator.
 *
 * @param  name        A pointer to the string containing the generator name
 *                     to check for.
 * @param  connection  A pointer to the connection to be used to perform the
 *                     check.
 *
 * @return  Returns 0 if the generator does not exist, 1 if the generator does
 *          exist or -1 if there was an error.
 *
 */
int checkForGenerator(const char *name, isc_db_handle *connection) {
  int result    = -1;
  isc_stmt_handle statement = 0;
  ISC_STATUS status[ISC_STATUS_LENGTH];

  if(isc_dsql_allocate_statement(status, connection, &statement) == 0) {
    isc_tr_handle transaction = 0;

    if(isc_start_transaction(status, &transaction, 1, connection, 0,
                             NULL) == 0) {
      XSQLDA *da = (XSQLDA *)ALLOC_N(char, XSQLDA_LENGTH(1));

      if(da != NULL) {
        char sql[200]; // 84(statement) + 2*(31)max_generator_name = 146

        da->version = SQLDA_VERSION1;
        da->sqln    = 1;
        sprintf(sql, "SELECT COUNT(*) FROM RDB$GENERATORS WHERE RDB$GENERATOR_NAME in ('%s', UPPER('%s'))", name, name);
        if(isc_dsql_prepare(status, &transaction, &statement, strlen(sql),
                            sql, 3, da) == 0) {
          /* Prepare the XSQLDA and provide it with data room. */
          allocateOutXSQLDA(da->sqld, &statement, 3);
          prepareDataArea(da);
          if(isc_dsql_execute(status, &transaction, &statement,
                              3, da) == 0) {
            if(isc_dsql_fetch(status, &statement, 3, da) == 0) {
              int32_t count = *((long *)da->sqlvar->sqldata);

              result = (count > 0 ? 1 : 0);
            } else {
              rb_fireruby_raise(status,
                                "Error checking for generator.");
            }
          } else {
            rb_fireruby_raise(status,
                              "Error checking for generator.");
          }
        } else {
          rb_fireruby_raise(status, "Error checking for generator.");
        }

        releaseDataArea(da);
      } else {
        rb_raise(rb_eNoMemError, "Memory allocation failure checking " \
                                 "generator existence.");
      }

      if(transaction != 0) {
        isc_commit_transaction(status, &transaction);
      }
    } else {
      rb_fireruby_raise(status, "Error checking for generator.");
    }

    isc_dsql_free_statement(status, &statement, DSQL_drop);
  }

  return(result);
}


/**
 * This function creates a new generator within a database.
 *
 * @param  name        A pointer to the string containing the generator name
 *                     to be created.
 * @param  connection  A pointer to the connection to be used to create the new
 *                     generator.
 *
 * @return  Returns 0 if the generator was created or -1 if there was an error.
 *
 */
int installGenerator(const char *name, isc_db_handle *connection) {
  int result    = -1;
  isc_stmt_handle statement = 0;
  ISC_STATUS status[ISC_STATUS_LENGTH];

  if(isc_dsql_allocate_statement(status, connection, &statement) == 0) {
    isc_tr_handle transaction = 0;

    if(isc_start_transaction(status, &transaction, 1, connection, 0,
                             NULL) == 0) {
      char sql[100];

      sprintf(sql, "CREATE GENERATOR %s", name);
      if(isc_dsql_prepare(status, &transaction, &statement, strlen(sql),
                          sql, 3, NULL) == 0) {
        if(isc_dsql_execute(status, &transaction, &statement,
                            3, NULL) == 0) {
          result = 0;
        } else {
          rb_fireruby_raise(status, "Error creating generator.");
        }
      } else {
        rb_fireruby_raise(status, "Error creating generator.");
      }

      if(transaction != 0) {
        isc_commit_transaction(status, &transaction);
      }
    } else {
      rb_fireruby_raise(status, "Error creating generator.");
    }

    isc_dsql_free_statement(status, &statement, DSQL_drop);
  }

  return(result);
}


/**
 * This function drops an existing generator within a database.
 *
 * @param  name        A pointer to the string containing the generator name
 *                     to be dropped.
 * @param  connection  A pointer to the connection to be used to drop the
 *                     generator.
 *
 * @return  Returns 0 if the generator was dropped or -1 if there was an error.
 *
 */
int deleteGenerator(const char *name, isc_db_handle *connection) {
  int result    = -1;
  isc_stmt_handle statement = 0;
  ISC_STATUS status[ISC_STATUS_LENGTH];

  if(isc_dsql_allocate_statement(status, connection, &statement) == 0) {
    isc_tr_handle transaction = 0;

    if(isc_start_transaction(status, &transaction, 1, connection, 0,
                             NULL) == 0) {
      char sql[100];

      sprintf(sql, "DROP GENERATOR %s", name);
      if(isc_dsql_prepare(status, &transaction, &statement, strlen(sql),
                          sql, 3, NULL) == 0) {
        if(isc_dsql_execute(status, &transaction, &statement,
                            3, NULL) == 0) {
          result = 0;
        } else {
          rb_fireruby_raise(status, "Error dropping generator.");
        }
      } else {
        rb_fireruby_raise(status, "Error dropping generator.");
      }

      if(transaction != 0) {
        isc_commit_transaction(status, &transaction);
      }
    } else {
      rb_fireruby_raise(status, "Error dropping generator.");
    }

    isc_dsql_free_statement(status, &statement, DSQL_drop);
  }

  return(result);
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
 * This function executes a SQL statement to fetch a value from a named
 * generator in the database.
 *
 * @param  name        The name of the generator to fetch the value from.
 * @param  step        The step interval to be used in the call to the database
 *                     generator.
 * @param  connection  The connection to make the call through.
 *
 * @return  A long integer containing the generator value.
 *
 */
int32_t getGeneratorValue(const char *name, int step, isc_db_handle *connection) {
  int32_t result      = 0;
  ISC_STATUS status[ISC_STATUS_LENGTH];
  isc_tr_handle transaction = 0;
  XSQLDA        *da         = createStorage();

  if(isc_start_transaction(status, &transaction, 1, connection, 0, NULL) == 0) {
    char sql[100];

    sprintf(sql, "SELECT GEN_ID(%s, %d) FROM RDB$DATABASE", name, step);
    if(isc_dsql_exec_immed2(status, connection, &transaction, 0, sql,
                            3, NULL, da) == 0) {
      result = *((int32_t *)da->sqlvar->sqldata);
    } else {
      ISC_STATUS local[20];

      isc_rollback_transaction(local, &transaction);
      rb_fireruby_raise(status, "Error obtaining generator value.");
    }

    isc_commit_transaction(status, &transaction);
  } else {
    rb_fireruby_raise(status, "Error obtaining generator value.");
  }

  /* Clean up. */
  if(da != NULL) {
    releaseDataArea(da);
  }

  return(result);
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
  VALUE instance = allocateGenerator(cGenerator);

  initializeGenerator(instance, name, connection);

  return(instance);
}


/**
 * This function integrates with the Ruby garbage collector to insure that all
 * of the resources associated with a Generator object are release whenever a
 * Generator object is collected.
 *
 * @param  generator  A pointer to the GeneratorHandle structure associated
 *                    with the Generator object being collected.
 *
 */
void generatorFree(void *generator) {
  if(generator != NULL) {
    free((GeneratorHandle *)generator);
  }
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
  rb_define_alloc_func(cGenerator, allocateGenerator);
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
