/**************************************************************************

    This is an example xclass OpenGL application.
    Copyright (C) 2000-2006, Hector Peraza.

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

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <limits.h>

#include <xclass/XCconfig.h>

#ifdef HAVE_GL_GL_H

#include <xclass/utils.h>
#include <xclass/OXMainFrame.h>
#include <xclass/OXGLView.h>


//----------------------------------------------------------------------

class GLView : public OXGLView {
public:
  GLView(const OXWindow *p);
  virtual ~GLView();
  
  virtual void SetupTextures();
  virtual void RenderScene();

protected:
  GLuint _texName;
};

class MainFrame : public OXMainFrame {
public:
  MainFrame(const OXWindow *p, int w, int h);
  virtual ~MainFrame();

protected:
  GLView *_glView;
};


//----------------------------------------------------------------------

int main(int argc, char **argv) {
  OXClient *clientX = new OXClient(argc, argv);

  MainFrame *mainw = new MainFrame(clientX->GetRoot(), 10, 10);
  mainw->Resize(600, 400);

  mainw->MapWindow();

  clientX->Run();

  exit(0);
}

MainFrame::MainFrame(const OXWindow *p, int w, int h) :
  OXMainFrame(p, w, h) {

  //------ GL frame

  OXCompositeFrame *glf = new OXCompositeFrame(this, 10, 10,
                                               SUNKEN_FRAME | DOUBLE_BORDER);
  AddFrame(glf, new OLayoutHints(LHINTS_EXPAND_X | LHINTS_EXPAND_Y));

  _glView = new GLView(glf);
  glf->AddFrame(_glView, new OLayoutHints(LHINTS_EXPAND_X | LHINTS_EXPAND_Y));

  SetWindowName("OpenGL test application");
  SetClassHints("XCLASS", "XCLASS");

  MapSubwindows();
}

MainFrame::~MainFrame() {
}

/*
 * Teapot code (c) Copyright 1993, Silicon Graphics, Inc.
 *
 * ALL RIGHTS RESERVED
 *
 * Permission to use, copy, modify, and distribute this software
 * for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies and that
 * both the copyright notice and this permission notice appear in
 * supporting documentation, and that the name of Silicon
 * Graphics, Inc. not be used in advertising or publicity
 * pertaining to distribution of the software without specific,
 * written prior permission.
 *
 * THE MATERIAL EMBODIED ON THIS SOFTWARE IS PROVIDED TO YOU
 * "AS-IS" AND WITHOUT WARRANTY OF ANY KIND, EXPRESS, IMPLIED OR
 * OTHERWISE, INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF
 * MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.  IN NO
 * EVENT SHALL SILICON GRAPHICS, INC.  BE LIABLE TO YOU OR ANYONE
 * ELSE FOR ANY DIRECT, SPECIAL, INCIDENTAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OF ANY KIND, OR ANY DAMAGES WHATSOEVER,
 * INCLUDING WITHOUT LIMITATION, LOSS OF PROFIT, LOSS OF USE,
 * SAVINGS OR REVENUE, OR THE CLAIMS OF THIRD PARTIES, WHETHER OR
 * NOT SILICON GRAPHICS, INC.  HAS BEEN ADVISED OF THE POSSIBILITY
 * OF SUCH LOSS, HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * ARISING OUT OF OR IN CONNECTION WITH THE POSSESSION, USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 * US Government Users Restricted Rights
 *
 * Use, duplication, or disclosure by the Government is subject to
 * restrictions set forth in FAR 52.227.19(c)(2) or subparagraph
 * (c)(1)(ii) of the Rights in Technical Data and Computer
 * Software clause at DFARS 252.227-7013 and/or in similar or
 * successor clauses in the FAR or the DOD or NASA FAR
 * Supplement.  Unpublished-- rights reserved under the copyright
 * laws of the United States.  Contractor/manufacturer is Silicon
 * Graphics, Inc., 2011 N.  Shoreline Blvd., Mountain View, CA
 * 94039-7311.
 *
 * OpenGL(TM) is a trademark of Silicon Graphics, Inc.
 */

/* -- PRIVATE FUNCTIONS ---------------------------------------------------- */

/*
 * Rim, body, lid, and bottom data must be reflected in x and y;
 * handle and spout data across the y axis only.
 */
static int patchdata[][16] = {
    { 102, 103, 104, 105,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,  15 }, /* rim    */
    {  12,  13,  14,  15,  16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27 }, /* body   */
    {  24,  25,  26,  27,  29,  30,  31,  32,  33,  34,  35,  36,  37,  38,  39,  40 },
    {  96,  96,  96,  96,  97,  98,  99, 100, 101, 101, 101, 101,   0,   1,   2,   3 }, /* lid    */
    {   0,   1,   2,   3, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117 },
    { 118, 118, 118, 118, 124, 122, 119, 121, 123, 126, 125, 120,  40,  39,  38,  37 }, /* bottom */
    {  41,  42,  43,  44,  45,  46,  47,  48,  49,  50,  51,  52,  53,  54,  55,  56 }, /* handle */
    {  53,  54,  55,  56,  57,  58,  59,  60,  61,  62,  63,  64,  28,  65,  66,  67 },
    {  68,  69,  70,  71,  72,  73,  74,  75,  76,  77,  78,  79,  80,  81,  82,  83 }, /* spout  */
    {  80,  81,  82,  83,  84,  85,  86,  87,  88,  89,  90,  91,  92,  93,  94,  95 }
};

static double cpdata[][3] = {
    {0.2, 0, 2.7}, {0.2, -0.112, 2.7}, {0.112, -0.2, 2.7}, {0,
    -0.2, 2.7}, {1.3375, 0, 2.53125}, {1.3375, -0.749, 2.53125},
    {0.749, -1.3375, 2.53125}, {0, -1.3375, 2.53125}, {1.4375,
    0, 2.53125}, {1.4375, -0.805, 2.53125}, {0.805, -1.4375,
    2.53125}, {0, -1.4375, 2.53125}, {1.5, 0, 2.4}, {1.5, -0.84,
    2.4}, {0.84, -1.5, 2.4}, {0, -1.5, 2.4}, {1.75, 0, 1.875},
    {1.75, -0.98, 1.875}, {0.98, -1.75, 1.875}, {0, -1.75,
    1.875}, {2, 0, 1.35}, {2, -1.12, 1.35}, {1.12, -2, 1.35},
    {0, -2, 1.35}, {2, 0, 0.9}, {2, -1.12, 0.9}, {1.12, -2,
    0.9}, {0, -2, 0.9}, {-2, 0, 0.9}, {2, 0, 0.45}, {2, -1.12,
    0.45}, {1.12, -2, 0.45}, {0, -2, 0.45}, {1.5, 0, 0.225},
    {1.5, -0.84, 0.225}, {0.84, -1.5, 0.225}, {0, -1.5, 0.225},
    {1.5, 0, 0.15}, {1.5, -0.84, 0.15}, {0.84, -1.5, 0.15}, {0,
    -1.5, 0.15}, {-1.6, 0, 2.025}, {-1.6, -0.3, 2.025}, {-1.5,
    -0.3, 2.25}, {-1.5, 0, 2.25}, {-2.3, 0, 2.025}, {-2.3, -0.3,
    2.025}, {-2.5, -0.3, 2.25}, {-2.5, 0, 2.25}, {-2.7, 0,
    2.025}, {-2.7, -0.3, 2.025}, {-3, -0.3, 2.25}, {-3, 0,
    2.25}, {-2.7, 0, 1.8}, {-2.7, -0.3, 1.8}, {-3, -0.3, 1.8},
    {-3, 0, 1.8}, {-2.7, 0, 1.575}, {-2.7, -0.3, 1.575}, {-3,
    -0.3, 1.35}, {-3, 0, 1.35}, {-2.5, 0, 1.125}, {-2.5, -0.3,
    1.125}, {-2.65, -0.3, 0.9375}, {-2.65, 0, 0.9375}, {-2,
    -0.3, 0.9}, {-1.9, -0.3, 0.6}, {-1.9, 0, 0.6}, {1.7, 0,
    1.425}, {1.7, -0.66, 1.425}, {1.7, -0.66, 0.6}, {1.7, 0,
    0.6}, {2.6, 0, 1.425}, {2.6, -0.66, 1.425}, {3.1, -0.66,
    0.825}, {3.1, 0, 0.825}, {2.3, 0, 2.1}, {2.3, -0.25, 2.1},
    {2.4, -0.25, 2.025}, {2.4, 0, 2.025}, {2.7, 0, 2.4}, {2.7,
    -0.25, 2.4}, {3.3, -0.25, 2.4}, {3.3, 0, 2.4}, {2.8, 0,
    2.475}, {2.8, -0.25, 2.475}, {3.525, -0.25, 2.49375},
    {3.525, 0, 2.49375}, {2.9, 0, 2.475}, {2.9, -0.15, 2.475},
    {3.45, -0.15, 2.5125}, {3.45, 0, 2.5125}, {2.8, 0, 2.4},
    {2.8, -0.15, 2.4}, {3.2, -0.15, 2.4}, {3.2, 0, 2.4}, {0, 0,
    3.15}, {0.8, 0, 3.15}, {0.8, -0.45, 3.15}, {0.45, -0.8,
    3.15}, {0, -0.8, 3.15}, {0, 0, 2.85}, {1.4, 0, 2.4}, {1.4,
    -0.784, 2.4}, {0.784, -1.4, 2.4}, {0, -1.4, 2.4}, {0.4, 0,
    2.55}, {0.4, -0.224, 2.55}, {0.224, -0.4, 2.55}, {0, -0.4,
    2.55}, {1.3, 0, 2.55}, {1.3, -0.728, 2.55}, {0.728, -1.3,
    2.55}, {0, -1.3, 2.55}, {1.3, 0, 2.4}, {1.3, -0.728, 2.4},
    {0.728, -1.3, 2.4}, {0, -1.3, 2.4}, {0, 0, 0}, {1.425,
    -0.798, 0}, {1.5, 0, 0.075}, {1.425, 0, 0}, {0.798, -1.425,
    0}, {0, -1.5, 0.075}, {0, -1.425, 0}, {1.5, -0.84, 0.075},
    {0.84, -1.5, 0.075}
};

static double tex[2][2][2] =
{
    { {0.0, 0.0}, {1.0, 0.0} },
    { {0.0, 1.0}, {1.0, 1.0} }
};

static void teapot(GLint grid, GLdouble scale, GLenum type)
{
    double p[4][4][3], q[4][4][3], r[4][4][3], s[4][4][3];
    long i, j, k, l;

    glPushAttrib(GL_ENABLE_BIT | GL_EVAL_BIT);
    glEnable(GL_AUTO_NORMAL);
    glEnable(GL_NORMALIZE);
    glEnable(GL_MAP2_VERTEX_3);
    glEnable(GL_MAP2_TEXTURE_COORD_2);

    glPushMatrix();
    glRotated(270.0, 1.0, 0.0, 0.0);
    glScaled(0.5 * scale, 0.5 * scale, 0.5 * scale);
    glTranslated(0.0, 0.0, -1.5);

    for (i = 0; i < 10; i++) {
      for (j = 0; j < 4; j++) {
        for (k = 0; k < 4; k++) {
          for (l = 0; l < 3; l++) {
            p[j][k][l] = cpdata[patchdata[i][j * 4 + k]][l];
            q[j][k][l] = cpdata[patchdata[i][j * 4 + (3 - k)]][l];
            if (l == 1)
              q[j][k][l] *= -1.0;
            if (i < 6) {
              r[j][k][l] =
                cpdata[patchdata[i][j * 4 + (3 - k)]][l];
              if (l == 0)
                r[j][k][l] *= -1.0;
              s[j][k][l] = cpdata[patchdata[i][j * 4 + k]][l];
              if (l == 0)
                s[j][k][l] *= -1.0;
              if (l == 1)
                s[j][k][l] *= -1.0;
            }
          }
        }
      }

      glMap2d(GL_MAP2_TEXTURE_COORD_2, 0.0, 1.0, 2, 2, 0.0, 1.0, 4, 2,
        &tex[0][0][0]);
      glMap2d(GL_MAP2_VERTEX_3, 0.0, 1.0, 3, 4, 0.0, 1.0, 12, 4,
        &p[0][0][0]);
      glMapGrid2d(grid, 0.0, 1.0, grid, 0.0, 1.0);
      glEvalMesh2(type, 0, grid, 0, grid);
      glMap2d(GL_MAP2_VERTEX_3, 0.0, 1.0, 3, 4, 0.0, 1.0, 12, 4,
        &q[0][0][0]);
      glEvalMesh2(type, 0, grid, 0, grid);
      if (i < 6) {
        glMap2d(GL_MAP2_VERTEX_3, 0.0, 1.0, 3, 4, 0.0, 1.0, 12, 4,
          &r[0][0][0]);
        glEvalMesh2(type, 0, grid, 0, grid);
        glMap2d(GL_MAP2_VERTEX_3, 0.0, 1.0, 3, 4, 0.0, 1.0, 12, 4,
          &s[0][0][0]);
        glEvalMesh2(type, 0, grid, 0, grid);
      }
    }

    glPopMatrix();
    glPopAttrib();
}

/* -- INTERFACE FUNCTIONS -------------------------------------------------- */

/*
 * Renders a beautiful wired teapot...
 */
void renderWireTeapot(GLdouble size)
{
    /*
     * We will use the general teapot rendering code
     */
    teapot(10, size, GL_LINE);
}

/*
 * Renders a beautiful filled teapot...
 */
void renderSolidTeapot(GLdouble size)
{
    /*
     * We will use the general teapot rendering code
     */
    teapot(7, size, GL_FILL);
}

//----------------------------------------------------------------------

GLView::GLView(const OXWindow *p) : OXGLView(p, 10, 10) {
}

GLView::~GLView() {
}

void GLView::SetupTextures() {
  // load a sample texture
  FILE *f = fopen("env.ppm", "r");
  if (f) {
    char tmp[PATH_MAX];
    int  xres, yres, zres;

    fgets(tmp, 20, f);
    if (strcmp(tmp, "P6\n") != 0) {
      // invalid file type
      fclose(f);
      return;
    }

    xres = yres = zres = -1;

    for (;;) {
      fgets(tmp, PATH_MAX, f);
      if (tmp[0] == '#') continue;
      sscanf(tmp, "%d %d", &xres, &yres);
      break;
    }

    for (;;) {
      fgets(tmp, PATH_MAX, f);
      if (tmp[0] == '#') continue;
      sscanf(tmp, "%d", &zres);
      break;
    }

    // TODO: check image size, must be a power of two, and smaller
    // than the largest value allowed

    if (xres < 4 || xres > 5000 || yres < 4 || yres > 5000) {
      // too small or too large
      fclose(f);
      return;
    }

    if (zres <= 0)  {
      // invalid depth
      fclose(f);
      return;
    }

    GLubyte *image = new GLubyte[xres * yres * 4];

    for (int y = yres - 1; y >= 0; --y) {
      for (int x = 0; x < xres; ++x) {
        int ix = 4 * (y * xres + x);
        image[ix + 0] = getc(f);
        image[ix + 1] = getc(f);
        image[ix + 2] = getc(f);
        image[ix + 3] = 255;
      }
    }

    fclose(f);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glGenTextures(1, &_texName);
    glBindTexture(GL_TEXTURE_2D, _texName);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, xres, yres, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, image);

    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);

    delete[] image;
  } else {
    fprintf(stderr, "Texture image not found (env.ppm)\n");
  }
}

void GLView::RenderScene() {

  // setup environment mapping
  glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
  glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
  glEnable(GL_TEXTURE_GEN_S);
  glEnable(GL_TEXTURE_GEN_T);

  glEnable(GL_TEXTURE_2D);
  glColor4f(0.8f, 0.75f, 0.4f, 0.0f);
  glPushMatrix();
  glRotatef(-90.0f, 0.0f, 1.0f, 0.0f);
  renderSolidTeapot(50.0);
  glPopMatrix();

  glDisable(GL_TEXTURE_GEN_S);
  glDisable(GL_TEXTURE_GEN_T);

  glDisable(GL_TEXTURE_2D);
}

#else   // HAVE_GL_GL_H

int main() {
  fprintf(stderr, "\n");
  fprintf(stderr, "Sorry, no OpenGL support was compiled in xclass.\n");
  fprintf(stderr, "Try rebuilding the library.\n");
  fprintf(stderr, "\n");
  return 1;
}

#endif  // HAVE_GL_GL_H
