/**************************************************************************

    HTML widget for xclass. Based on tkhtml 1.28
    Copyright (C) 1997-2000 D. Richard Hipp <drh@acm.org>
    Copyright (C) 2002-2003 Hector Peraza.

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

**************************************************************************/

#ifndef __OHTMLURI_H
#define __OHTMLURI_H

#include <xclass/OBaseObject.h>


#define URI_SCHEME_MASK     (1<<0)
#define URI_AUTH_MASK       (1<<1)
#define URI_PATH_MASK       (1<<2)
#define URI_QUERY_MASK      (1<<3)
#define URI_FRAGMENT_MASK   (1<<4)

#define URI_FULL_MASK       (URI_SCHEME_MASK | URI_AUTH_MASK |  \
                             URI_PATH_MASK   | URI_QUERY_MASK | \
                             URI_FRAGMENT_MASK)

//----------------------------------------------------------------------
// A parsed URI is held in an instance of the following class.
//
// The examples are from the URI
//   http://192.168.1.1:8080/cgi-bin/printenv?name=xyzzy&addr=none#frag

class OHtmlUri : public OBaseObject {
public:
  OHtmlUri(const char *zUri = 0);
  OHtmlUri(const OHtmlUri *uri);
  virtual ~OHtmlUri();

  char *BuildUri();
  int  EqualsUri(const OHtmlUri *uri, int field_mask = URI_FULL_MASK);

public:
  int ComponentLength(const char *z, const char *zInit, const char *zTerm);
  
  char *zScheme;             // Ex: "http"
  char *zAuthority;          // Ex: "192.168.1.1:8080"
  char *zPath;               // Ex: "cgi-bin/printenv"
  char *zQuery;              // Ex: "name=xyzzy&addr=none"
  char *zFragment;           // Ex: "frag"
};


#endif  // __OHTMLURI_H
