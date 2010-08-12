/*------------------------------------------------------------------------------
 * DataArea.c
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
#include "DataArea.h"
#include "FireRubyException.h"

#ifdef OS_UNIX
   #include <inttypes.h>
#else
   typedef short     int16_t;
   typedef long      int32_t;
   typedef long long int64_t;
#endif


/**
 * This function allocates the memory for a output XSQLDA based on a specified
 * XSQLDA providing base details.
 *
 * @param  size       A count of the number of entries needed in the XSQLDA.
 * @param  statement  A pointer to the statement handle that will be used in
 *                    preparing the data area.
 * @param  dialect    The SQL dialect to be used in preparing the data area.
 *
 * @return  A pointer to the newly allocated XSQLDA.
 *
 */
XSQLDA *allocateOutXSQLDA(int size, isc_stmt_handle *statement, short dialect)
{
   XSQLDA *area = (XSQLDA *)ALLOC_N(char, XSQLDA_LENGTH(size));

   if(area != NULL)
   {
      ISC_STATUS status[20];

      area->sqln    = size;
      area->version = SQLDA_VERSION1;
      if(isc_dsql_describe(status, statement, dialect, area) != 0)
      {
         /* Release the memory and generate an error. */
         free(area);
         rb_fireruby_raise(status, "Error allocating output storage space.");
      }
   }
   else
   {
      /* Generate an error. */
      rb_raise(rb_eNoMemError,
               "Memory allocation failure preparing output SQL data "\
               "definition area.");
   }

   return(area);
}


/**
 * This function allocates the memory for a input XSQLDA based on a specified
 * XSQLDA providing base details.
 *
 * @param  size       A count of the number of entries needed in the XSQLDA.
 * @param  statement  A pointer to the statement handle that will be used in
 *                    preparing the data area.
 * @param  dialect    The SQL dialect to be used in preparing the data area.
 *
 * @return  A pointer to the newly allocated XSQLDA.
 *
 */
XSQLDA *allocateInXSQLDA(int size, isc_stmt_handle *statement, short dialect)
{
   XSQLDA *area  = NULL;

   area = (XSQLDA *)ALLOC_N(char, XSQLDA_LENGTH(size));

   if(area != NULL)
   {
      ISC_STATUS status[20];

      area->sqln    = size;
      area->version = SQLDA_VERSION1;
      if(isc_dsql_describe_bind(status, statement, dialect, area) != 0)
      {
         /* Release the memory and generate an error. */
         free(area);
         rb_fireruby_raise(status, "Error allocating input storage space.");
      }
   }
   else
   {
      /* Generate an error. */
      rb_raise(rb_eNoMemError,
               "Memory allocation failure preparing input SQL data "\
               "definition area.");
   }

   return(area);
}


/**
 * This function initializes a previously allocated XSQLDA with space for the
 * data it will contain.
 *
 * @param  da  A pointer to the XSQLDA to have data space allocated for.
 *
 */
void prepareDataArea(XSQLDA *da)
{
   XSQLVAR *field = da->sqlvar;
   int     index;

   for(index = 0; index < da->sqld; index++, field++)
   {
      int type  = (field->sqltype & ~1),
          total = (field->sqllen / 2) + 2;

      field->sqldata = NULL;
      field->sqlind  = NULL;
      switch(type)
      {
         case SQL_ARRAY :
            fprintf(stderr, "Allocating for an array (NOT).\n");
            break;

         case SQL_BLOB :
            field->sqldata = (char *)ALLOC(ISC_QUAD);
            break;

         case SQL_DOUBLE :
            field->sqldata = (char *)ALLOC(double);
            break;

         case SQL_FLOAT :
            field->sqldata = (char *)ALLOC(float);
            break;

         case SQL_INT64 :
            field->sqldata = (char *)ALLOC(int64_t);
            break;

         case SQL_LONG :
            field->sqldata = (char *)ALLOC(int32_t);
            break;

         case SQL_SHORT :
            field->sqldata = (char *)ALLOC(int16_t);
            break;

         case SQL_TEXT :
            field->sqldata = ALLOC_N(char, field->sqllen + 1);
            break;

         case SQL_TIMESTAMP :
            field->sqldata = (char *)ALLOC(ISC_TIMESTAMP);
            break;

         case SQL_TYPE_DATE :
            field->sqldata = (char *)ALLOC(ISC_DATE);
            break;

         case SQL_TYPE_TIME:
            field->sqldata = (char *)ALLOC(ISC_TIME);
            break;

         case SQL_VARYING :
            field->sqldata = (char *)ALLOC_N(short, total);
            break;

         default :
            rb_fireruby_raise(NULL, "Unknown SQL data type encountered.");
      }
      field->sqlind  = ALLOC(short);
      *field->sqlind = 0;
   }
}


/**
 * This method cleans up all the internal memory associated with a XSQLDA.
 *
 * @param  da  A reference to the XSQLDA to be cleared up.
 *
 */
void releaseDataArea(XSQLDA *da)
{
   XSQLVAR *field = da->sqlvar;
   int     index;

   for(index = 0; index < da->sqld; index++, field++)
   {
      int type  = (field->sqltype & ~1);

      switch(type)
      {
         case SQL_ARRAY :
            fprintf(stderr, "Releasing an array (NOT).\n");
            break;

         case SQL_BLOB :
            free((ISC_QUAD *)field->sqldata);
            break;

         case SQL_DOUBLE :
            free((double *)field->sqldata);
            break;

         case SQL_FLOAT :
            free((float *)field->sqldata);
            break;

         case SQL_INT64 :
            free((int64_t *)field->sqldata);
            break;

         case SQL_LONG :
            free((int32_t *)field->sqldata);
            break;

         case SQL_SHORT :
            free((int16_t *)field->sqldata);
            break;

         case SQL_TEXT :
            free(field->sqldata);
            break;

         case SQL_TIMESTAMP :
            free((ISC_TIMESTAMP *)field->sqldata);
            break;

         case SQL_TYPE_DATE :
            free((ISC_DATE *)field->sqldata);
            break;

         case SQL_TYPE_TIME:
            free((ISC_TIME *)field->sqldata);
            break;

         case SQL_VARYING :
            free((short *)field->sqldata);
            break;
      }
      if(field->sqlind != NULL)
      {
         free(field->sqlind);
      }

      field->sqldata = NULL;
      field->sqlind  = NULL;
   }
}
