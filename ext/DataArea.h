/*------------------------------------------------------------------------------
 * DataArea.h
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
#ifndef FIRERUBY_DATA_AREA_H
#define FIRERUBY_DATA_AREA_H

   #ifndef IBASE_H_INCLUDED
      #include "ibase.h"
      #define IBASE_H_INCLUDED
   #endif

XSQLDA *allocateOutXSQLDA(int, isc_stmt_handle *, short);
XSQLDA *allocateInXSQLDA(int, isc_stmt_handle *, short);
void prepareDataArea(XSQLDA *);
void releaseDataArea(XSQLDA *);

#endif /* FIRERUBY_DATA_AREA_H */
