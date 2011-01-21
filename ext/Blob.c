/*------------------------------------------------------------------------------
 * Blob.c
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
#include "Blob.h"
#include <limits.h>
#include "Common.h"

/* Function prototypes. */
static VALUE allocateBlob(VALUE);
static VALUE getBlobData(VALUE);
static VALUE closeBlob(VALUE);
static VALUE eachBlobSegment(VALUE);
char *loadBlobData(BlobHandle *);
char *loadBlobSegment(BlobHandle *, unsigned short *);

/* Globals. */
VALUE cBlob;


/**
 * This function integrates with the Ruby language to allow for the allocation
 * of new Blob objects.
 *
 * @param  klass  A reference to the Blob Class object.
 *
 */
static VALUE allocateBlob(VALUE klass)
{
   VALUE      instance;
   BlobHandle *blob = ALLOC(BlobHandle);
   
   if(blob != NULL)
   {
      memset(&blob->description, 0, sizeof(ISC_BLOB_DESC));
      blob->segments = blob->size = 0;
      blob->handle   = 0;
      instance       = Data_Wrap_Struct(klass, NULL, blobFree, blob);
   }
   else
   {
      rb_raise(rb_eNoMemError, "Memory allocation failure allocating a blob.");
   }
   
   return(instance);
}


/**
 * This function provides the initialize method for the Blob class.
 *
 * @param  self  A reference to the Blob object to be initialized.
 *
 * @return  A reference to the newly initialized Blob object.
 *
 */
VALUE initializeBlob(VALUE self)
{
   rb_iv_set(self, "@data", Qnil);
   return(self);
}


/**
 * This function fetches the data associated with a Blob object. This method
 * provides the to_s method for the Blob class.
 *
 * @param  self  A reference to the Blob object to fetch the data for.
 *
 * @return  A reference to a String containing the Blob object data.
 *
 */
static VALUE getBlobData(VALUE self)
{
   VALUE data = rb_iv_get(self, "@data");
   
   if(data == Qnil)
   {
      BlobHandle *blob   = NULL;
      
      Data_Get_Struct(self, BlobHandle, blob);
      if(blob->size > 0)
      {
         char *buffer = loadBlobData(blob);
         
         if(buffer != NULL)
         {
            data = rb_str_new(buffer, blob->size);
         }
         free(buffer);
      }
      rb_iv_set(self, "@data", data);
   }
   
   return(data);
}


/**
 * This function provides the close method for the Blob class, allowing the
 * resources for a blob to be explicitly released. It is not valid to use a
 * Blob object after it is closed.
 *
 * @param  self  A reference to the Blob object to be closed.
 *
 * @return  A reference to the closed Blob object.
 *
 */
static VALUE closeBlob(VALUE self)
{
   VALUE      data  = rb_iv_get(self, "@data");
   BlobHandle *blob = NULL;
   
   if(data != Qnil)
   {
      rb_iv_set(self, "@data", Qnil);
   }
   
   Data_Get_Struct(self, BlobHandle, blob);
   if(blob->handle != 0)
   {
      ISC_STATUS status[ISC_STATUS_LENGTH];
      
      if(isc_close_blob(status, &blob->handle) != 0)
      {
         rb_fireruby_raise(status, "Error closing blob.");
      }
      blob->handle = 0;
   }
   
   return(self);
}


/**
 * This function provides the each method for the Blob class. This function
 * feeds a segment of a blob to a block.
 *
 * @param  self  A reference to the Blob object to make the call for.
 *
 * @return  A reference to the last return value from the block called.
 *
 */
static VALUE eachBlobSegment(VALUE self)
{
   VALUE result = Qnil;
   
   if(rb_block_given_p())
   {
      BlobHandle     *blob    = NULL;
      char           *segment = NULL;
      unsigned short size     = 0;
      
      Data_Get_Struct(self, BlobHandle, blob);
      segment = loadBlobSegment(blob, &size);
      while(segment != NULL)
      {
         result = rb_yield(rb_str_new(segment, size));
         free(segment);
         segment = loadBlobSegment(blob, &size);
      }
   }
   
   return(result);
}


/**
 * This function allocates a BlobHandle structure and opens the structure for
 * use.
 *
 * @param  blobId       The unique identifier for the blob to be opened.
 * @param  table        The name of the table containing the blob being opened.
 * @param  column       The name of the column in the table that contains the
 *                      blob.
 * @param  connection   A pointer to the connection to be used in accessing the
 *                      blob.
 * @param  transaction  A pointer to the transaction to be used in accessing
 *                      the blob.
 *
 * @return  A pointer to an allocated and opened BlobHandle structure.
 *
 */
BlobHandle *openBlob(ISC_QUAD blobId,
                     char *table,
                     char *column,
                     isc_db_handle *connection,
                     isc_tr_handle *transaction)
{
   BlobHandle *blob = ALLOC(BlobHandle);
   
   if(blob != NULL)
   {
      ISC_STATUS status[ISC_STATUS_LENGTH];
      
      /* Extract the blob details and open it. */
      blob->handle = 0;
      isc_blob_default_desc(&blob->description,
                            (unsigned char *)table,
                            (unsigned char *)column);
      if(isc_open_blob2(status, connection, transaction, &blob->handle, &blobId,
                        0, NULL) == 0)
      {
         char items[] = {isc_info_blob_num_segments,
                         isc_info_blob_total_length},
              data[]  = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                         0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
              
         if(isc_blob_info(status, &blob->handle, 2, items, 20, data) == 0)
         {
            int offset = 0,
                done   = 0;
            
            while(done < 2)
            {
               int length = isc_vax_integer(&data[offset + 1], 2);
               
               if(data[offset] == isc_info_blob_num_segments)
               {
                  blob->segments = isc_vax_integer(&data[offset + 3], length);
                  done++;
               }
               else if(data[offset] == isc_info_blob_total_length)
               {
                  blob->size = isc_vax_integer(&data[offset + 3], length);
                  done++;
               }
               else
               {
                  free(blob);
                  rb_fireruby_raise(NULL, "Error reading blob details.");
               }
               offset += length + 3;
            }
         }
         else
         {
            free(blob);
            rb_fireruby_raise(status, "Error fetching blob details.");
         }
      }
      else
      {
         free(blob);
         rb_fireruby_raise(status, "Error opening blob.");
      }
   }
   
   return(blob);
}


/**
 * This function fetches the data associated with a blob.
 *
 * @param  blob  A pointer to the BlobHandle structure that will be used in
 *               loading the data.
 *
 * @return  A pointer to an array of character data containing the data of the
 *          blob.
 *
 */
char *loadBlobData(BlobHandle *blob)
{
   char *data = NULL;
   
   if(blob != NULL && blob->handle != 0)
   {
      if((data = ALLOC_N(char, blob->size)) != NULL)
      {
         ISC_STATUS status[ISC_STATUS_LENGTH],
                    result = 0;
         int        offset = 0;

         while(result == 0 || result == isc_segment)
         {
            unsigned short quantity  = 0,
                           available = 0;
            int            remains   = blob->size - offset;

            available = remains > USHRT_MAX ? USHRT_MAX : remains;
            result    = isc_get_segment(status, &blob->handle, &quantity,
                                        available, &data[offset]);
            if(result != 0 && result != isc_segment && result != isc_segstr_eof)
            {
               free(data);
               rb_fireruby_raise(status, "Error loading blob data.");
            }
            offset = offset + quantity;
         }
      }
      else
      {
         rb_raise(rb_eNoMemError,
                  "Memory allocation failure loading blob data.");
      }
   }
   else
   {
      rb_fireruby_raise(NULL, "Invalid blob specified for loading.");
   }
   
   return(data);
}


/**
 * This function fetches the data for a single segment associated with a Blob.
 *
 * @param  blob    A pointer to the BlobHandle structure that will be used in
 *                 fetching a blob segment.
 * @param  length  A pointer to an short integer that will be set to the amount
 *                 of data read in bytes.
 *
 * @return  A pointer to a block of memory containing the next blob segement
 *          data. The return value will be NULL when there are no more blob
 *          segements to be read.
 *
 */
char *loadBlobSegment(BlobHandle *blob, unsigned short *length)
{
   char *data = NULL;
   
   *length = 0;
   if(blob != NULL && blob->handle != 0)
   {
      unsigned short size = blob->description.blob_desc_segment_size;
      
      if((data = ALLOC_N(char, size)) != NULL)
      {
         ISC_STATUS     status[ISC_STATUS_LENGTH],
                        result;
                    
         result = isc_get_segment(status, &blob->handle, length, size, data);
         if(result != 0 && result != isc_segstr_eof)
         {
            free(data);
            rb_fireruby_raise(status, "Error reading blob segment.");
         }
      }
      else
      {
         rb_raise(rb_eNoMemError,
                  "Memory allocation failre loading blob segment.");
      }
   }
   else
   {
      rb_fireruby_raise(NULL, "Invalid blob specified for loading.");
   }
   
   return(data);
}


/**
 * This function integrates with the Ruby garbage collection system to insure
 * that all resources associated with a Blob object are released whenever such
 * an object is collected.
 *
 * @param  blob  A pointer to the BlobHandle structure associated with the Blob
 *               object being collected.
 *
 */
void blobFree(void *blob)
{
   if(blob != NULL)
   {
      BlobHandle *handle = (BlobHandle *)blob;

      if(handle->handle != 0)
      {
         ISC_STATUS status[ISC_STATUS_LENGTH];
         
         isc_close_blob(status, &handle->handle);
      }
      free(handle);
   }
}


/**
 * This function is used to create and initialize the Blob class within the
 * Ruby environment.
 *
 * @param  module  A reference to the module that the Blob class will be created
 *                 under.
 *
 */
void Init_Blob(VALUE module)
{
   cBlob = rb_define_class_under(module, "Blob", rb_cObject);
   rb_define_alloc_func(cBlob, allocateBlob);
   rb_define_method(cBlob, "initialize", initializeBlob, 0);
   rb_define_method(cBlob, "initialize_copy", forbidObjectCopy, 1);
   rb_define_method(cBlob, "to_s", getBlobData, 0);
   rb_define_method(cBlob, "close", closeBlob, 0);
   rb_define_method(cBlob, "each", eachBlobSegment, 0);
}
