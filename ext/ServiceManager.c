/*------------------------------------------------------------------------------
 * ServiceManager.c
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
#include "ServiceManager.h"
#include "Common.h"

/* Function prototypes. */
static VALUE allocateServiceManager(VALUE);
static VALUE initializeServiceManager(VALUE, VALUE);
static VALUE connectServiceManager(VALUE, VALUE, VALUE);
static VALUE disconnectServiceManager(VALUE);
static VALUE isServiceManagerConnected(VALUE);
static VALUE executeServiceTasks(int, VALUE *, VALUE);

/* Globals. */
VALUE cServiceManager;


/**
 * This function integrates with the Ruby memory allocation functionality to
 * allow for the creation of new ServiceManager objects.
 *
 * @param  klass  A reference to the ServiceManager Class object.
 *
 * @return  A reference to the newly allocated ServiceManager object.
 *
 */
VALUE allocateServiceManager(VALUE klass) {
  VALUE instance = Qnil;
  ManagerHandle *manager = ALLOC(ManagerHandle);

  if(manager == NULL) {
    rb_raise(rb_eNoMemError,
             "Memory allocation failure creating a service manager.");
  }
  manager->handle = NULL;
  instance        = Data_Wrap_Struct(klass, NULL, serviceManagerFree, manager);

  return(instance);
}


/**
 * This method provides the initialize method for the ServiceManager class.
 *
 * @param  self  A reference to the ServiceManager object to be initialized.
 * @param  host  The name of the database host for the service manager.
 *
 */
VALUE initializeServiceManager(VALUE self, VALUE host) {
  VALUE text = rb_funcall(host, rb_intern("to_s"), 0);

  rb_iv_set(self, "@host", text);

  return(self);
}


/**
 * This function provides the connect method for the ServiceManager class.
 *
 * @param  self      A reference to the ServiceManager object to be connected.
 * @param  user      A string containing the user that will be used for service
 *                   execution.
 * @param  password  A string containing the user password that will be used
 *                   for service execution.
 *
 * @return  A reference to the newly connected ServiceManager object.
 *
 */
VALUE connectServiceManager(VALUE self, VALUE user, VALUE password) {
  ManagerHandle *manager  = NULL;
  char          *buffer   = NULL,
  *position = NULL,
  *text     = NULL,
  *service  = NULL;
  short length    = 2,
        size      = 0;
  VALUE host      = rb_iv_get(self, "@host");
  ISC_STATUS status[ISC_STATUS_LENGTH];

  Data_Get_Struct(self, ManagerHandle, manager);
  if(manager->handle != 0) {
    rb_fireruby_raise(NULL, "Service manager already connected.");
  }

  /* Calculate the size of the service parameter buffer. */
  length += strlen(StringValuePtr(user)) + 2;
  length += strlen(StringValuePtr(password)) + 2;

  /* Create the service parameter buffer. */
  position = buffer = ALLOC_N(char, length);
  if(position == NULL) {
    rb_raise(rb_eNoMemError,
             "Memory allocation failure creating service parameter buffer.");
  }
  memset(buffer, 0, length);

  /* Populate the service parameter buffer. */
  *position++ = isc_spb_version;
  *position++ = isc_spb_current_version;

  text        = StringValuePtr(user);
  size        = strlen(text);
  *position++ = isc_spb_user_name;
  *position++ = size;
  strncpy(position, text, size);
  position    += size;

  text        = StringValuePtr(password);
  size        = strlen(text);
  *position++ = isc_spb_password;
  *position++ = size;
  strncpy(position, text, size);

  /* Create the service name. */
  size    = strlen(StringValuePtr(host)) + 13;
  service = ALLOC_N(char, size);
  if(service == NULL) {
    free(buffer);
    rb_raise(rb_eNoMemError,
             "Memory allocation failure service manager service name.");
  }
  memset(service, 0, size);
  sprintf(service, "%s:service_mgr", StringValuePtr(host));

  /* Make the attachment call. */
  if(isc_service_attach(status, 0, service, &manager->handle, length,
                        buffer)) {
    free(buffer);
    free(service);
    rb_fireruby_raise(status, "Error connecting service manager.");
  }

  /* Clean up. */
  free(buffer);
  free(service);

  return(self);
}


/**
 * This function provides the disconnect method for the ServiceManager class.
 *
 * @param  self  A reference to the ServiceManager object to be disconnected.
 *
 * @return  A reference to the disconnected ServiceManager object.
 *
 */
VALUE disconnectServiceManager(VALUE self) {
  ManagerHandle *manager = NULL;

  Data_Get_Struct(self, ManagerHandle, manager);
  if(manager->handle != 0) {
    ISC_STATUS status[ISC_STATUS_LENGTH];

    if(isc_service_detach(status, &manager->handle)) {
      rb_fireruby_raise(status, "Error disconnecting service manager.");
    }
    manager->handle = 0;
  }

  return(self);
}


/**
 * This function provides the connected? method of the ServiceManager class.
 *
 * @param  self  A reference to the ServiceManager object to be checked.
 *
 * @return  True if the manager is connected, false otherwise.
 *
 */
VALUE isServiceManagerConnected(VALUE self) {
  VALUE result   = Qfalse;
  ManagerHandle *manager = NULL;

  Data_Get_Struct(self, ManagerHandle, manager);
  if(manager->handle != 0) {
    result = Qtrue;
  }

  return(result);
}


/**
 * This function provides the execute method for the ServiceManager class.
 *
 * @param  argc  A count of the number of arguments passed to the function
 *               call.
 * @param  argv  A pointer to the start of an array of VALUE entities that are
 *               the function parameters.
 * @param  self  A reference to the ServiceManager object that the call is
 *               being made on.
 *
 * @return  A reference to the ServiceManager class.
 *
 */
VALUE executeServiceTasks(int argc, VALUE *argv, VALUE self) {
  if(argc > 0) {
    int i;

    for(i = 0; i < argc; i++) {
      rb_funcall(argv[i], rb_intern("execute"), 1, self);
    }
  }

  return(self);
}


/**
 * This function integrates with the Ruby garbage collection functionality to
 * insure that the resources associated with a ServiceManager object a fully
 * released whenever the object gets collected.
 *
 * @param  manager  A reference to the ManagerHandle associated with the
 *                  ServiceManager being collected.
 *
 */
void serviceManagerFree(void *manager) {
  if(manager != NULL) {
    ManagerHandle *handle = (ManagerHandle *)manager;

    if(handle->handle != 0) {
      ISC_STATUS status[ISC_STATUS_LENGTH];

      isc_service_detach(status, &handle->handle);
    }
    free(handle);
  }
}


/**
 * This function provides a programmatic means of creating a ServiceManager
 * object.
 *
 * @param  host  A reference to the host that the service manager will connect
 *               to.
 *
 * @return  A reference to the newly created ServiceManager object.
 *
 */
VALUE rb_service_manager_new(VALUE host) {
  VALUE manager = allocateServiceManager(cServiceManager);

  initializeServiceManager(manager, host);

  return(manager);
}


/**
 * This function initialize the ServiceManager class in the Ruby environment.
 *
 * @param  module  The module to create the new class definition under.
 *
 */
void Init_ServiceManager(VALUE module) {
  cServiceManager = rb_define_class_under(module, "ServiceManager", rb_cObject);
  rb_define_alloc_func(cServiceManager, allocateServiceManager);
  rb_define_method(cServiceManager, "initialize", initializeServiceManager, 1);
  rb_define_method(cServiceManager, "initialize_copy", forbidObjectCopy, 1);
  rb_define_method(cServiceManager, "connect", connectServiceManager, 2);
  rb_define_method(cServiceManager, "disconnect", disconnectServiceManager, 0);
  rb_define_method(cServiceManager, "connected?", isServiceManagerConnected, 0);
  rb_define_method(cServiceManager, "execute", executeServiceTasks, -1);
}
