/**************************************************************************

    This file is part of xclass, a Win95-looking GUI toolkit.
    Copyright (C) 1996, 1997 David Barth, Hector Peraza.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

**************************************************************************/

#ifndef __OSELECTEDPICTURE_H
#define __OSELECTEDPICTURE_H

#include <X11/Xlib.h>

#include <xclass/utils.h>
#include <xclass/OPicture.h>


class OXGC;

//----------------------------------------------------------------------

class OSelectedPicture : public OPicture {
protected:
  static OXGC *_selectedGC;

public:
  OSelectedPicture(const OXClient *client, const OPicture *p);
  virtual ~OSelectedPicture();

protected:
  const OXClient *_client;
};


#endif  // __OSELECTEDPICTURE_H
