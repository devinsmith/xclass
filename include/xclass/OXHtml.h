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

#ifndef __OXHTML_H
#define __OXHTML_H

#include <xclass/utils.h>
#include <xclass/OXClient.h>
#include <xclass/OXView.h>
#include <xclass/OXFont.h>
#include <xclass/OIdleHandler.h>
#include <xclass/OTimer.h>
#include <xclass/OHashTable.h>
#include <xclass/OMessage.h>
#include <xclass/OImage.h>
#include <xclass/OHtmlTokens.h>


//----------------------------------------------------------------------

#define HTML_RELIEF_FLAT    0
#define HTML_RELIEF_SUNKEN  1
#define HTML_RELIEF_RAISED  2

//#define TABLE_TRIM_BLANK 1


// Debug must be turned on for testing to work.

#define DEBUG

#define CANT_HAPPEN  \
  fprintf(stderr, \
          "Unplanned behavior in the HTML Widget in file %s line %d\n", \
          __FILE__, __LINE__)

#define UNTESTED  \
  fprintf(stderr, \
          "Untested code executed in the HTML Widget in file %s line %d\n", \
          __FILE__, __LINE__)


// Sanity checking macros.

#ifdef DEBUG
#define HtmlAssert(X) \
  if(!(X)){ \
    fprintf(stderr,"Assertion failed on line %d of %s\n",__LINE__,__FILE__); \
  }
#define HtmlCantHappen \
  fprintf(stderr,"Can't happen on line %d of %s\n",__LINE__,__FILE__);
#else
#define HtmlAssert(X)
#define HtmlCantHappen
#endif

// Bitmasks for the HtmlTraceMask global variable

#define HtmlTrace_Table1       0x00000001
#define HtmlTrace_Table2       0x00000002
#define HtmlTrace_Table3       0x00000004
#define HtmlTrace_Table4       0x00000008
#define HtmlTrace_Table5       0x00000010
#define HtmlTrace_Table6       0x00000020
#define HtmlTrace_GetLine      0x00000100
#define HtmlTrace_GetLine2     0x00000200
#define HtmlTrace_FixLine      0x00000400
#define HtmlTrace_BreakMarkup  0x00001000
#define HtmlTrace_Style        0x00002000
#define HtmlTrace_Input1       0x00004000

// The TRACE macro is used to print internal information about the
// HTML layout engine during testing and debugging. The amount of
// information printed is governed by a global variable named
// HtmlTraceMask. If bits in the first argument to the TRACE macro
// match any bits in HtmlTraceMask variable, then the trace message
// is printed.
//
// All of this is completely disabled, of course, if the DEBUG macro
// is not defined.

#ifdef DEBUG
extern int HtmlTraceMask;
extern int HtmlDepth;
# define TRACE_INDENT  printf("%*s",HtmlDepth-3,"")
# define TRACE(Flag, Args) \
    if( (Flag)&HtmlTraceMask ){ \
       TRACE_INDENT; printf Args; fflush(stdout); \
    }
# define TRACE_PUSH(Flag)  if( (Flag)&HtmlTraceMask ){ HtmlDepth+=3; }
# define TRACE_POP(Flag)   if( (Flag)&HtmlTraceMask ){ HtmlDepth-=3; }
#else
# define TRACE_INDENT
# define TRACE(Flag, Args)
# define TRACE_PUSH(Flag)
# define TRACE_POP(Flag)
#endif


//----------------------------------------------------------------------

// Various data types. This code is designed to run on a modern cached
// architecture where the CPU runs a lot faster than the memory bus. Hence
// we try to pack as much data into as small a space as possible so that it
// is more likely to fit in cache. The extra CPU instruction or two needed
// to unpack the data is not normally an issue since we expect the speed of
// the memory bus to be the limiting factor.

typedef unsigned char  Html_u8;      // 8-bit unsigned integer
typedef short          Html_16;      // 16-bit signed integer
typedef unsigned short Html_u16;     // 16-bit unsigned integer
typedef int            Html_32;      // 32-bit signed integer

// An instance of the following structure is used to record style
// information on each Html element.

struct SHtmlStyle {
  unsigned int font    : 6;      // Font to use for display
  unsigned int color   : 6;      // Foreground color
  signed int subscript : 4;      // Positive for <sup>, negative for <sub>
  unsigned int align   : 2;      // Horizontal alignment
  unsigned int bgcolor : 6;      // Background color
  unsigned int expbg   : 1;      // Set to 1 if bgcolor explicitely set
  unsigned int flags   : 7;      // the STY_ flags below
};


// We allow 8 different font families: Normal, Bold, Italic and Bold-Italic
// in either variable or constant width. Within each family there can be up
// to 7 font sizes from 1 (the smallest) up to 7 (the largest). Hence, the
// widget can use a maximum of 56 fonts. The ".font" field of the style is
// an integer between 0 and 55 which indicates which font to use.

// HP: we further subdivide the .font field into two 3-bit subfields (size
// and family). That makes easier to manipulate the family field.

#define N_FONT_FAMILY     8
#define N_FONT_SIZE       7
#define N_FONT            ((N_FONT_FAMILY << 3) | N_FONT_SIZE)
#define NormalFont(X)     (X)
#define BoldFont(X)       ((X) | (1 << 3))
#define ItalicFont(X)     ((X) | (2 << 3))
#define CWFont(X)         ((X) | (4 << 3))
#define FontSize(X)       ((X) & 007)
#define FontFamily(X)     ((X) & 070)
#define FONT_Any          -1
#define FONT_Default      3
#define FontSwitch(Size, Bold, Italic, Cw) \
                          ((Size) | ((Bold+(Italic)*2+(Cw)*4) << 3))

// Macros for manipulating the fontValid bitmap of an OXHtml object.

#define FontIsValid(I)     ((fontValid[(I)>>3] & (1<<((I)&3)))!=0)
#define FontSetValid(I)    (fontValid[(I)>>3] |= (1<<((I)&3)))
#define FontClearValid(I)  (fontValid[(I)>>3] &= ~(1<<((I)&3)))


// Information about available colors.
//
// The widget will use at most N_COLOR colors. 4 of these colors are
// predefined. The rest are user selectable by options to various markups.
// (Ex: <font color=red>)
//
// All colors are stored in the apColor[] array of the main widget object.
// The ".color" field of the SHtmlStyle is an integer between 0 and
// N_COLOR-1 which indicates which of these colors to use.

#define N_COLOR             32      // Total number of colors

#define COLOR_Normal         0      // Index for normal color (black)
#define COLOR_Unvisited      1      // Index for unvisited hyperlinks
#define COLOR_Visited        2      // Color for visited hyperlinks
#define COLOR_Selection      3      // Background color for the selection
#define COLOR_Background     4      // Default background color
#define N_PREDEFINED_COLOR   5      // Number of predefined colors


// The "align" field of the style determines how text is justified
// horizontally. ALIGN_None means that the alignment is not specified.
// (It should probably default to ALIGN_Left in this case.)

#define ALIGN_Left   1
#define ALIGN_Right  2
#define ALIGN_Center 3
#define ALIGN_None   0


// Possible value of the "flags" field of SHtmlStyle are shown below.
//
//  STY_Preformatted       If set, the current text occurred within
//                         <pre>..</pre>
//
//  STY_StrikeThru         Draw a solid line thru the middle of this text.
//
//  STY_Underline          This text should drawn with an underline.
//
//  STY_NoBreak            This text occurs within <nobr>..</nobr>
//
//  STY_Anchor             This text occurs within <a href=X>..</a>.
//
//  STY_DT                 This text occurs within <dt>..</dt>.                
//
//  STY_Invisible          This text should not appear in the main HTML
//                         window. (For example, it might be within 
//                         <title>..</title> or <marquee>..</marquee>.)

#define STY_Preformatted    0x001
#define STY_StrikeThru      0x002
#define STY_Underline       0x004
#define STY_NoBreak         0x008
#define STY_Anchor          0x010
#define STY_DT              0x020
#define STY_Invisible       0x040
#define STY_FontMask        (STY_StrikeThru|STY_Underline)


//----------------------------------------------------------------------
// The first thing done with input HTML text is to parse it into
// OHtmlElements. All sizing and layout is done using these elements.

// Every element contains at least this much information:

class OHtmlElement : public OBaseObject {
public:
  OHtmlElement(int etype = 0);

  virtual int  IsMarkup() const { return (type > Html_Block); }
  virtual const char *MarkupArg(const char *tag, const char *zDefault) { return 0; }
  virtual int  GetAlignment(int dflt) { return dflt; }
  virtual int  GetOrderedListType(int dflt) { return dflt; }
  virtual int  GetUnorderedListType(int dflt) { return dflt; }
  virtual int  GetVerticalAlignment(int dflt) { return dflt; }

public:
  OHtmlElement *pNext;        // Next input token in a list of them all
  OHtmlElement *pPrev;        // Previous token in a list of them all
  SHtmlStyle style;           // The rendering style for this token
  Html_u8 type;               // The token type.
  Html_u8 flags;              // The HTML_ flags below
  Html_16 count;              // Various uses, depending on "type"
  int id;                     // Unique identifier
  int offs;                   // Offset within zText
};


// Bitmasks for the "flags" field of the OHtmlElement

#define HTML_Visible   0x01   // This element produces "ink"
#define HTML_NewLine   0x02   // type == Html_Space and ends with newline
#define HTML_Selected  0x04   // Some or all of this Html_Block is selected
                              // Used by Html_Block elements only.


// Each text element holds additional information as shown here. Notice that
// extra space is allocated so that zText[] will be large enough to hold the
// complete text of the element. X and y coordinates are relative to the
// virtual canvas. The y coordinate refers to the baseline.

class OHtmlTextElement : public OHtmlElement {
public:
  OHtmlTextElement(int size);
  virtual ~OHtmlTextElement();

  Html_32 y;                  // y coordinate where text should be rendered
  Html_16 x;                  // x coordinate where text should be rendered
  Html_16 w;                  // width of this token in pixels
  Html_u8 ascent;             // height above the baseline
  Html_u8 descent;            // depth below the baseline
  Html_u8 spaceWidth;         // Width of one space in the current font
  char *zText;                // Text for this element. Null terminated
};


// Each space element is represented like this:

class OHtmlSpaceElement : public OHtmlElement {
public:
  OHtmlSpaceElement() : OHtmlElement(Html_Space) {}

  Html_16 w;                  // Width of a single space in current font
  Html_u8 ascent;             // height above the baseline
  Html_u8 descent;            // depth below the baseline
};


// Most markup uses this class. Some markup extends this class with
// additional information, but most use it as is, at the very least.
//
// If the markup doesn't have arguments (the "count" field of
// OHtmlElement is 0) then the extra "argv" field of this class
// is not allocated and should not be used.

class OHtmlMarkupElement : public OHtmlElement {
public:
  OHtmlMarkupElement(int type, int argc, int arglen[], char *argv[]);
  virtual ~OHtmlMarkupElement();

  virtual const char *MarkupArg(const char *tag, const char *zDefault);
  virtual int  GetAlignment(int dflt);
  virtual int  GetOrderedListType(int dflt);
  virtual int  GetUnorderedListType(int dflt);
  virtual int  GetVerticalAlignment(int dflt);

public://protected:
  char **argv;
};


// The maximum number of columns allowed in a table. Any columns beyond
// this number are ignored.

#define HTML_MAX_COLUMNS 40


// This class is used for each <table> element.
//
// In the minW[] and maxW[] arrays, the [0] element is the overall
// minimum and maximum width, including cell padding, spacing and 
// the "hspace". All other elements are the minimum and maximum 
// width for the contents of individual cells without any spacing or
// padding.

class OHtmlTable : public OHtmlMarkupElement {
public:
  OHtmlTable(int type, int argc, int arglen[], char *argv[]);
  ~OHtmlTable();

public:
  Html_u8 borderWidth;           // Width of the border
  Html_u8 nCol;                  // Number of columns
  Html_u16 nRow;                 // Number of rows
  Html_32 y;                     // top edge of table border
  Html_32 h;                     // height of the table border
  Html_16 x;                     // left edge of table border
  Html_16 w;                     // width of the table border
  int minW[HTML_MAX_COLUMNS+1];  // minimum width of each column
  int maxW[HTML_MAX_COLUMNS+1];  // maximum width of each column
  OHtmlElement *pEnd;            // Pointer to the end tag element
  OImage *bgImage;               // A background for the entire table
  int hasbg;                     // 1 if a table above has bgImage
};


// Each <td> or <th> markup is represented by an instance of the 
// following class.
//
// Drawing for a cell is a sunken 3D border with the border width given
// by the borderWidth field in the associated <table> object.

class OHtmlCell : public OHtmlMarkupElement {
public:
  OHtmlCell(int type, int argc, int arglen[], char *argv[]);
  ~OHtmlCell();

public:
  Html_16 rowspan;          // Number of rows spanned by this cell
  Html_16 colspan;          // Number of columns spanned by this cell
  Html_16 x;                // X coordinate of left edge of border
  Html_16 w;                // Width of the border
  Html_32 y;                // Y coordinate of top of border indentation
  Html_32 h;                // Height of the border
  OHtmlTable *pTable;       // Pointer back to the <table>
  OHtmlElement *pRow;       // Pointer back to the <tr>
  OHtmlElement *pEnd;       // Element that ends this cell
  OImage *bgImage;          // Background for the cell
};


// This class is used for </table>, </td>, <tr>, </tr> and </th> elements.
// It points back to the <table> element that began the table. It is also
// used by </a> to point back to the original <a>. I'll probably think of
// other uses before all is said and done...

class OHtmlRef : public OHtmlMarkupElement {
public:
  OHtmlRef(int type, int argc, int arglen[], char *argv[]);
  ~OHtmlRef();

public:
  OHtmlElement *pOther;         // Pointer to some other Html element
  OImage *bgImage;              // A background for the entire row
};


// An instance of the following class is used to represent
// each <LI> markup.

class OHtmlLi : public OHtmlMarkupElement {
public:
  OHtmlLi(int type, int argc, int arglen[], char *argv[]);

public:
  Html_u8 ltype;    // What type of list is this?
  Html_u8 ascent;   // height above the baseline
  Html_u8 descent;  // depth below the baseline
  Html_16 cnt;      // Value for this element (if inside <OL>)
  Html_16 x;        // X coordinate of the bullet
  Html_32 y;        // Y coordinate of the bullet
};


// The ltype field of an OHtmlLi or OHtmlListStart object can take on 
// any of the following values to indicate what type of bullet to draw.
// The value in OHtmlLi will take precedence over the value in
// OHtmlListStart if the two values differ.

#define LI_TYPE_Undefined 0     // If in OHtmlLi, use the OHtmlListStart value
#define LI_TYPE_Bullet1   1     // A solid circle
#define LI_TYPE_Bullet2   2     // A hollow circle
#define LI_TYPE_Bullet3   3     // A hollow square
#define LI_TYPE_Enum_1    4     // Arabic numbers
#define LI_TYPE_Enum_A    5     // A, B, C, ...
#define LI_TYPE_Enum_a    6     // a, b, c, ...
#define LI_TYPE_Enum_I    7     // Capitalized roman numerals
#define LI_TYPE_Enum_i    8     // Lower-case roman numerals


// An instance of this class is used for <UL> or <OL> markup.

class OHtmlListStart : public OHtmlMarkupElement {
public:
  OHtmlListStart(int type, int argc, int arglen[], char *argv[]);

public:
  Html_u8 ltype;           // One of the LI_TYPE_ defines above
  Html_u8 compact;         // True if the COMPACT flag is present
  Html_u16 cnt;            // Next value for <OL>
  Html_u16 width;          // How much space to allow for indentation
  OHtmlListStart *lPrev;   // Next higher level list, or NULL
};


#define HTML_MAP_RECT    1
#define HTML_MAP_CIRCLE  2
#define HTML_MAP_POLY    3

class OHtmlMapArea : public OHtmlMarkupElement {
public:
  OHtmlMapArea(int type, int argc, int arglen[], char *argv[]);
  
public:
  int mType;
  int *coords;
  int num;
};  


//----------------------------------------------------------------------

// Structure to chain extension data onto.

struct SHtmlExtensions {
  void *exts;
  int typ;
  int flags;
  SHtmlExtensions *next;
};


//----------------------------------------------------------------------

// Information about each image on the HTML widget is held in an instance
// of the following class. All images are held on a list attached to the
// main widget object.
//
// This class is NOT an element. The <IMG> element is represented by an
// OHtmlImageMarkup object below. There is one OHtmlImageMarkup for each
// <IMG> in the source HTML. There is one of these objects for each unique
// image loaded. (If two <IMG> specify the same image, there are still two
// OHtmlImageMarkup objects but only one OHtmlImage object that is shared
// between them.)

class OXHtml;
class OHtmlImageMarkup;

class OHtmlImage : public OBaseObject {
public:
  OHtmlImage(OXHtml *h, const char *url, const char *width,
             const char *height);
  virtual ~OHtmlImage();

public:
  OXHtml *_html;           // The owner of this image
  OImage *image;           // The image token
  Html_32 w;               // Requested width of this image (0 if none)
  Html_32 h;               // Requested height of this image (0 if none)
  char *zUrl;              // The URL for this image.
  char *zWidth, *zHeight;  // Width and height in the <img> markup.
  OHtmlImage *pNext;       // Next image on the list
  OHtmlImageMarkup *pList; // List of all <IMG> markups that use this 
                           // same image
  OTimer *timer;           // for animations
};

// Each <img> markup is represented by an instance of the following
// class.
//
// If pImage == 0, then we use the alternative text in zAlt.

class OHtmlImageMarkup : public OHtmlMarkupElement {
public:
  OHtmlImageMarkup(int type, int argc, int arglen[], char *argv[]);

public:
  Html_u8 align;           // Alignment. See IMAGE_ALIGN_ defines below
  Html_u8 textAscent;      // Ascent of text font in force at the <IMG>
  Html_u8 textDescent;     // Descent of text font in force at the <IMG>
  Html_u8 redrawNeeded;    // Need to redraw this image because the image
                           // content changed.
  Html_16 h;               // Actual height of the image
  Html_16 w;               // Actual width of the image
  Html_16 ascent;          // How far image extends above "y"
  Html_16 descent;         // How far image extends below "y"
  Html_16 x;               // X coordinate of left edge of the image
  Html_32 y;               // Y coordinate of image baseline
  const char *zAlt;        // Alternative text
  OHtmlImage *pImage;      // Corresponding OHtmlImage object
  OHtmlElement *pMap;      // usemap
  OHtmlImageMarkup *iNext; // Next markup using the same OHtmlImage object
};


// Allowed alignments for images. These represent the allowed arguments
// to the "align=" field of the <IMG> markup.

#define IMAGE_ALIGN_Bottom        0
#define IMAGE_ALIGN_Middle        1
#define IMAGE_ALIGN_Top           2
#define IMAGE_ALIGN_TextTop       3
#define IMAGE_ALIGN_AbsMiddle     4
#define IMAGE_ALIGN_AbsBottom     5
#define IMAGE_ALIGN_Left          6
#define IMAGE_ALIGN_Right         7


// All kinds of form markup, including <INPUT>, <TEXTAREA> and <SELECT>
// are represented by instances of the following class.
//
// (later...)  We also use this for the <APPLET> markup. That way,
// the window we create for an <APPLET> responds to the OXHtml::MapControls()
// and OXHtml::UnmapControls() function calls. For an <APPLET>, the
// pForm field is NULL. (Later still...) <EMBED> works just like
// <APPLET> so it uses this class too.

class OHtmlForm;

class OHtmlInput : public OHtmlMarkupElement {
public:
  OHtmlInput(int type, int argc, int arglen[], char *argv[]);

  void Empty();

public:
  OHtmlForm *pForm;        // The <FORM> to which this belongs
  OHtmlInput *iNext;       // Next element in a list of all input elements
  OXFrame *frame;          // The xclass window that implements this control
  OXHtml *html;            // The HTML widget this control is attached to
  OHtmlElement *pEnd;      // End tag for <TEXTAREA>, etc.
  Html_u16 inpId;          // Unique id for this element
  Html_u16 subId;          // For radio - an id, for select - option count
  Html_32  y;              // Baseline for this input element
  Html_u16 x;              // Left edge
  Html_u16 w, h;           // Width and height of this control
  Html_u8 padLeft;         // Extra padding on left side of the control
  Html_u8 align;           // One of the IMAGE_ALIGN_xxx types
  Html_u8 textAscent;      // Ascent for the current font
  Html_u8 textDescent;     // descent for the current font
  Html_u8 itype;           // What type of input is this?
  Html_u8 sized;           // True if this input has been sized already
  Html_u16 cnt;            // Used to derive widget name. 0 if no widget
};


// An input control can be one of the following types. See the
// comment about <APPLET> on the OHtmlInput class insight into
// INPUT_TYPE_Applet.

#define INPUT_TYPE_Unknown      0
#define INPUT_TYPE_Checkbox     1
#define INPUT_TYPE_File         2
#define INPUT_TYPE_Hidden       3
#define INPUT_TYPE_Image        4
#define INPUT_TYPE_Password     5
#define INPUT_TYPE_Radio        6
#define INPUT_TYPE_Reset        7
#define INPUT_TYPE_Select       8
#define INPUT_TYPE_Submit       9
#define INPUT_TYPE_Text        10
#define INPUT_TYPE_TextArea    11
#define INPUT_TYPE_Applet      12
#define INPUT_TYPE_Button      13


// There can be multiple <FORM> entries on a single HTML page.
// Each one must be given a unique number for identification purposes,
// and so we can generate unique state variable names for radiobuttons,
// checkbuttons, and entry boxes.

class OHtmlForm : public OHtmlMarkupElement {
public:
  OHtmlForm(int type, int argc, int arglen[], char *argv[]);

public:
  Html_u16 formId;         // Unique number assigned to this form
  unsigned int elements;   // Number of elements
  unsigned int hasctl;     // Has controls
  OHtmlElement *pFirst;    // First form element
  OHtmlElement *pEnd;      // Pointer to end tag element
};


// Information used by a <HR> markup

class OHtmlHr : public OHtmlMarkupElement {
public:
  OHtmlHr(int type, int argc, int arglen[], char *argv[]);

public:
  Html_32  y;              // Baseline for this input element
  Html_u16 x;              // Left edge
  Html_u16 w, h;           // Width and height of this control
  Html_u8 is3D;            // Is it drawn 3D?
};


// Information used by a <A> markup

class OHtmlAnchor : public OHtmlMarkupElement {
public:
  OHtmlAnchor(int type, int argc, int arglen[], char *argv[]);

public:
  Html_32  y;              // Top edge for this element
};


// Information about the <SCRIPT> markup. The parser treats <SCRIPT>
// specially. All text between <SCRIPT> and </SCRIPT> is captured and
// is indexed to by the nStart field of this class.
//
// The nStart field indexs to a spot in the zText field of the OXHtml object.
// The nScript field determines how long the script is.

class OHtmlScript : public OHtmlMarkupElement {
public:
  OHtmlScript(int type, int argc, int arglen[], char *argv[]);

public:
  int nStart;              // Start of the script (index into OXHtml::zText)
  int nScript;             // Number of characters of text in zText holding
                           // the complete text of this script
};


// A block is a single unit of display information. This can be one or more
// text elements, or the border of table, or an image, etc.
//
// Blocks are used to improve display speed and to improve the speed of
// linear searchs through the token list. A single block will typically
// contain enough information to display a dozen or more Text and Space
// elements all with a single call to OXFont::DrawChars(). The blocks are
// linked together on their own list, so we can search them much faster than
// elements (since there are fewer of them.)
//
// Of course, you can construct pathological HTML that has as many Blocks as
// it has normal tokens. But you haven't lost anything. Using blocks just
// speeds things up in the common case.
//
// Much of the information needed for display is held in the original
// OHtmlElement objects. "pNext" points to the first object in the list
// which can be used to find the "style" "x" and "y".
//
// If n is zero, then "pNext" might point to a special OHtmlElement
// that defines some other kind of drawing, like <LI> or <IMG> or <INPUT>.

class OHtmlBlock : public OHtmlElement {
public:
  OHtmlBlock();
  virtual ~OHtmlBlock();

public:
  char *z;                    // Space to hold text when n > 0
  int top, bottom;            // Extremes of y coordinates
  Html_u16 left, right;       // Left and right boundry of this object
  Html_u16 n;                 // Number of characters in z[]
  OHtmlBlock *bPrev, *bNext;  // Linked list of all Blocks
};


// A stack of these structures is used to keep track of nested font and
// style changes. This allows us to easily revert to the previous style
// when we encounter and end-tag like </em> or </h3>.
//
// This stack is used to keep track of the current style while walking
// the list of elements. After all elements have been assigned a style,
// the information in this stack is no longer used.

struct SHtmlStyleStack {
  SHtmlStyleStack *pNext;  // Next style on the stack
  int type;                // A markup that ends this style. Ex: Html_EndEM
  SHtmlStyle style;        // The currently active style.
};


// A stack of the following structures is used to remember the
// left and right margins within a layout context.

struct SHtmlMargin {
  int indent;              // Size of the current margin
  int bottom;              // Y value at which this margin expires
  int tag;                 // Markup that will cancel this margin
  SHtmlMargin *pNext;      // Previous margin
};


// How much space (in pixels) used for a single level of indentation due
// to a <UL> or <DL> or <BLOCKQUOTE>, etc.

#define HTML_INDENT 36


//----------------------------------------------------------------------

// A layout context holds all state information used by the layout engine.

class OHtmlLayoutContext : public OBaseObject {
public:
  OHtmlLayoutContext();

  void LayoutBlock();
  void Reset();
  
  void PopIndent();
  void PushIndent();
  
protected:
  void PushMargin(SHtmlMargin **ppMargin, int indent, int bottom, int tag);
  void PopOneMargin(SHtmlMargin **ppMargin);
  void PopMargin(SHtmlMargin **ppMargin, int tag);
  void PopExpiredMargins(SHtmlMargin **ppMarginStack, int y);
  void ClearMarginStack(SHtmlMargin **ppMargin);
  
  OHtmlElement *GetLine(OHtmlElement *pStart, OHtmlElement *pEnd,
                        int width, int minX, int *actualWidth);

  void FixAnchors(OHtmlElement *p, OHtmlElement *pEnd, int y);
  int  FixLine(OHtmlElement *pStart, OHtmlElement *pEnd,
               int bottom, int width, int actualWidth, int leftMargin,
               int *maxX);
  void Paragraph(OHtmlElement *p);
  void ComputeMargins(int *pX, int *pY, int *pW);
  void ClearObstacle(int mode);
  OHtmlElement *DoBreakMarkup(OHtmlElement *p);
  int  InWrapAround();
  void WidenLine(int reqWidth, int *pX, int *pY, int *pW);
  
  OHtmlElement *TableLayout(OHtmlTable *p);

public:
  OXHtml *html;                // The html widget undergoing layout
  OHtmlElement *pStart;        // Start of elements to layout
  OHtmlElement *pEnd;          // Stop when reaching this element
  int headRoom;                // Extra space wanted above this line
  int top;                     // Absolute top of drawing area
  int bottom;                  // Bottom of previous line
  int left, right;             // Left and right extremes of drawing area
  int pageWidth;               // Width of the layout field, including
                               // the margins
  int maxX, maxY;              // Maximum X and Y values of paint
  SHtmlMargin *leftMargin;     // Stack of left margins
  SHtmlMargin *rightMargin;    // Stack of right margins
};


// With 28 different fonts and 16 colors, we could in principle have
// as many as 448 different GCs. But in practice, a single page of
// HTML will typically have much less than this. So we won't try to
// keep all GCs on hand. Instead, we'll keep around the most recently
// used GCs and allocate new ones as necessary.
//
// The following structure is used to build a cache of GCs in the
// main widget object.

#define N_CACHE_GC 32
struct GcCache {
  GC gc;                // The graphics context
  Html_u8 font;         // Font used for this context
  Html_u8 color;        // Color used for this context
  Html_u8 index;        // Index used for LRU replacement
};


// An SHtmlIndex is a reference to a particular character within a
// particular Text or Space token. 

struct SHtmlIndex {
  OHtmlElement *p;      // The token containing the character
  int i;                // Index of the character
};


// Used by the tokenizer

struct SHtmlTokenMap {
  const char *zName;          // Name of a markup
  Html_16 type;               // Markup type code
  Html_16 objType;            // Which kind of OHtml... object to alocate
  SHtmlTokenMap *pCollide;    // Hash table collision chain
};


// Markup element types to be allocated by the tokenizer.
// Do not confuse with .type field in OHtmlElement

#define O_HtmlMarkupElement   0
#define O_HtmlCell            1
#define O_HtmlTable           2
#define O_HtmlRef             3
#define O_HtmlLi              4
#define O_HtmlListStart       5
#define O_HtmlImageMarkup     6
#define O_HtmlInput           7
#define O_HtmlForm            8
#define O_HtmlHr              9
#define O_HtmlAnchor          10
#define O_HtmlScript          11
#define O_HtmlMapArea         12


//----------------------------------------------------------------------

// The HTML widget. A derivate of OXView.

class OXListBox;

class OXHtml : public OXView {
public:
  OXHtml(const OXWindow *p, int w, int h, int id = -1);
  virtual ~OXHtml();
  
  virtual int HandleFocusChange(XFocusChangeEvent *event);
  virtual int HandleButton(XButtonEvent *event);
  virtual int HandleMotion(XMotionEvent *event);

  virtual int HandleIdleEvent(OIdleHandler *i);
  virtual int HandleTimer(OTimer *timer);
  
  virtual int ProcessMessage(OMessage *msg);

  virtual int DrawRegion(OPosition coord, ODimension size, int clear = True);
  virtual bool ItemLayout();

public:   // user commands

  int  ParseText(char *text, const char *index = 0);
  
  void SetTableRelief(int relief);
  int  GetTableRelief() const { return tableRelief; }

  void SetRuleRelief(int relief);
  int  GetRuleRelief() const { return ruleRelief; }
  int  GetRulePadding() const { return rulePadding; }
  
  void UnderlineLinks(int onoff);
  
  void SetBaseUri(const char *uri);
  const char *GetBaseUri() const { return zBase; }
  
  int GotoAnchor(const char *name);

public:   // reloadable methods

  // called when the widget is cleared
  virtual void Clear();

  // User function to resolve URIs
  virtual char *ResolveUri(const char *uri);

  // User function to get an image from a URL
  virtual OImage *LoadImage(const char *uri, int w = 0, int h = 0) ;//
//    { return 0; }

  // User function to tell if a hyperlink has already been visited
  virtual int IsVisited(const char *url)
    { return False; }

  // User function to to process tokens of the given type
  virtual int ProcessToken(OHtmlElement *pElem, const char *name, int type)
    { return False; }

  virtual OXFont *GetFont(int iFont);

  // The HTML parser will invoke the following methods from time
  // to time to find out information it needs to complete formatting of
  // the document.

  // Method for handling <frameset> markup
  virtual int ProcessFrame()
    { return False; }

  // Method to process applets
  virtual OXFrame *ProcessApplet(OHtmlInput *input)
    { return False; }

  // Called when parsing forms
  virtual int FormCreate(OHtmlForm *form, const char *zUrl, const char *args)
    { /* printf("%s\n", args); */ return False; }

  // Called when user presses Submit
  virtual int FormAction(OHtmlForm *form, int id)
    { /* printf("Form %d action!\n", form->formId); */ return False; }

  // Invoked to find font names
  virtual char *GetFontName()
    { return NULL; }

  // Invoked for each <SCRIPT> markup
  virtual char *ProcessScript(OHtmlScript *script)
    { return NULL; }

public:
  const char *GetText() const { return zText; }

  int GetMarginWidth() { return margins.l + margins.r; }
  int GetMarginHeight() { return margins.t + margins.b; }

  const char *GetHref(int x, int y, const char **target = NULL);
  
  OHtmlImage *GetImage(OHtmlImageMarkup *p);
  
  int  InArea(OHtmlMapArea *p, int left, int top, int x, int y);
  OHtmlElement *GetMap(const char *name);

  void ResetBlocks() { firstBlock = lastBlock = 0; }
  int  ElementCoords(OHtmlElement *p, int i, int pct, int *coords);

  OHtmlElement *TableDimensions(OHtmlTable *pStart, int lineWidth);
  int  CellSpacing(OHtmlElement *pTable);
  void MoveVertically(OHtmlElement *p, OHtmlElement *pLast, int dy);

  void PrintList(OHtmlElement *first, OHtmlElement *last);

  char *GetTokenName(OHtmlElement *p);
  char *DumpToken(OHtmlElement *p);

  void EncodeText(OString *str, const char *z);

protected:
  void _Clear();
  void ClearGcCache();
  void ResetLayoutContext();
  void Redraw();
  void ComputeVirtualSize();
  
  void ScheduleRedraw();
  
  void RedrawArea(int left, int top, int right, int bottom);
  void RedrawBlock(OHtmlBlock *p);
  void RedrawEverything();
  void RedrawText(int y);
  
  float colorDistance(XColor *pA, XColor *pB);
  int isDarkColor(XColor *p);
  int isLightColor(XColor *p);
  int GetColorByName(const char *zColor);
  int GetDarkShadowColor(int iBgColor);
  int GetLightShadowColor(int iBgColor);
  int GetColorByValue(XColor *pRef);
  
  void FlashCursor();
  
  GC GetGC(int color, int font);
  GC GetAnyGC();
  
  void AnimateImage(OHtmlImage *image);
  void ImageChanged(OHtmlImage *image, int newWidth, int newHeight);
  int  GetImageAlignment(OHtmlElement *p);
  int  GetImageAt(int x, int y);
  const char *GetPctWidth(OHtmlElement *p, char *opt, char *ret);
  void TableBgndImage(OHtmlElement *p);
  
  OHtmlElement *FillOutBlock(OHtmlBlock *p);
  void UnlinkAndFreeBlock(OHtmlBlock *pBlock);
  void AppendBlock(OHtmlElement *pToken, OHtmlBlock *pBlock);

  void StringHW(const char *str, int *h, int *w);
  OHtmlElement *MinMax(OHtmlElement *p, int *pMin, int *pMax,
                       int lineWidth, int hasbg);

  void DrawSelectionBackground(OHtmlBlock *pBlock, Drawable drawable,  
                               int x, int y);
  void DrawRect(Drawable drawable, OHtmlElement *src,
                int x, int y, int w, int h, int depth, int relief);
  void BlockDraw(OHtmlBlock *pBlock, Drawable drawable,
                 int drawableLeft, int drawableTop,
                 int drawableWidth, int drawableHeight, Pixmap pixmap);
  void DrawImage(OHtmlImageMarkup *image, Drawable drawable,
                 int drawableLeft, int drawableTop,
                 int drawableRight, int drawableBottom);
  void DrawTableBgnd(int x, int y, int w, int h, Drawable d, OImage *image);
                          
  OHtmlElement *FindStartOfNextBlock(OHtmlElement *p, int *pCnt);
  void FormBlocks();
  
  void AppendElement(OHtmlElement *pElem);
  int  Tokenize();
  void AppToken(OHtmlElement *pNew, OHtmlElement *p, int offs);
  OHtmlMarkupElement *MakeMarkupEntry(int objType, int type, int argc,
                                      int arglen[], char *argv[]);
  void TokenizerAppend(const char *text);
  OHtmlElement *InsertToken(OHtmlElement *pToken,
                            char *zType, char *zArgs, int offs);
  SHtmlTokenMap *NameToPmap(char *zType);
  int  NameToType(char *zType);
  const char *TypeToName(int type);
  int  TextInsertCmd(int argc, char **argv);
  SHtmlTokenMap* GetMarkupMap(int n);
  
  OHtmlElement *TokenByIndex(int N, int flag);
  int  TokenNumber(OHtmlElement *p);

  void maxIndex(OHtmlElement *p, int *pIndex, int isLast);
  int  IndexMod(OHtmlElement **pp, int *ip, char *cp);
  void FindIndexInBlock(OHtmlBlock *pBlock, int x,
                        OHtmlElement **ppToken, int *pIndex);
  void IndexToBlockIndex(SHtmlIndex sIndex,
                         OHtmlBlock **ppBlock, int *piIndex);
  int  DecodeBaseIndex(const char *zBase,
                       OHtmlElement **ppToken, int *pIndex);
  int  GetIndex(const char *zIndex, OHtmlElement **ppToken, int *pIndex);
  
  void LayoutDoc();
  
  int  MapControls();
  void UnmapControls();
  void DeleteControls();
  int  ControlSize(OHtmlInput *p);
  void SizeAndLink(OXFrame *frame, OHtmlInput *pElem);
  int  FormCount(OHtmlInput *p, int radio);
  void AddFormInfo(OHtmlElement *p);
  void AddSelectOptions(OXListBox *lb, OHtmlElement *p, OHtmlElement *pEnd);
  void AppendText(OString *str, OHtmlElement *pFirst, OHtmlElement *pEnd);
                                                       
  void UpdateSelection(int forceUpdate);
  void UpdateSelectionDisplay();
  void LostSelection();
  int  SelectionSet(const char *startIx, const char *endIx);
  void UpdateInsert();
  int  SetInsert(const char *insIx);
  
  const char *GetUid(const char *string);
  XColor *AllocColor(const char *name);
  XColor *AllocColorByValue(XColor *color);
  void FreeColor(XColor *color);
  
  SHtmlStyle GetCurrentStyle();
  void PushStyleStack(int tag, SHtmlStyle style);
  SHtmlStyle PopStyleStack(int tag);
  
  void MakeInvisible(OHtmlElement *p_first, OHtmlElement *p_last);
  int  GetLinkColor(const char *zURL);
  void AddStyle(OHtmlElement *p);
  void Sizer();
  
  int  NextMarkupType(OHtmlElement *p);

  OHtmlElement *AttrElem(const char *name, char *value);
  
public:
  void AppendArglist(OString *str, OHtmlMarkupElement *pElem);
  OHtmlElement *FindEndNest(OHtmlElement *sp, int en, OHtmlElement *lp);
  OString *ListTokens(OHtmlElement *p, OHtmlElement *pEnd);
  OString *TableText(OHtmlTable *pTable, int flags);

protected:
  OHtmlElement *pFirst;         // First HTML token on a list of them all
  OHtmlElement *pLast;          // Last HTML token on the list
  int nToken;                   // Number of HTML tokens on the list.
                                // Html_Block tokens don't count.
  OHtmlElement *lastSized;      // Last HTML element that has been sized
  OHtmlElement *nextPlaced;     // Next HTML element that needs to be 
                                // positioned on canvas.
  OHtmlBlock *firstBlock;       // List of all OHtmlBlock tokens
  OHtmlBlock *lastBlock;        // Last OHtmlBlock in the list
  OHtmlInput *firstInput;       // First <INPUT> element
  OHtmlInput *lastInput;        // Last <INPUT> element
  int nInput;                   // The number of <INPUT> elements
  int nForm;                    // The number of <FORM> elements
  int varId;                    // Used to construct a unique name for a
                                // global array used by <INPUT> elements
  int inputIdx;                 // Unique input index
  int radioIdx;                 // Unique radio index

  // Information about the selected region of text

  SHtmlIndex selBegin;          // Start of the selection
  SHtmlIndex selEnd;            // End of the selection
  OHtmlBlock *pSelStartBlock;   // Block in which selection starts
  Html_16 selStartIndex;        // Index in pSelStartBlock of first selected
                                // character
  Html_16 selEndIndex;          // Index of last selecte char in pSelEndBlock
  OHtmlBlock *pSelEndBlock;     // Block in which selection ends

  // Information about the insertion cursor 

  int insOnTime;                // How long the cursor states one (millisec)
  int insOffTime;               // How long it is off (milliseconds)
  int insStatus;                // Is it visible?
  OTimer *insTimer;             // Timer used to flash the insertion cursor
  SHtmlIndex ins;               // The insertion cursor position
  OHtmlBlock *pInsBlock;        // The OHtmlBlock containing the cursor
  int insIndex;                 // Index in pInsBlock of the cursor

  // The following fields hold state information used by the tokenizer.

  char *zText;                  // Complete text of the unparsed HTML
  int nText;                    // Number of characters in zText
  int nAlloc;                   // Space allocated for zText
  int nComplete;                // How much of zText has actually been
                                // converted into tokens
  int iCol;                     // The column in which zText[nComplete]
                                // occurs. Used to resolve tabs in input
  int iPlaintext;               // If not zero, this is the token type that
                                // caused us to go into plaintext mode. One
                                // of Html_PLAINTEXT, Html_LISTING or
                                // Html_XMP
  OHtmlScript *pScript;            // <SCRIPT> currently being parsed

  OIdleHandler *_idle;

  // These fields hold state information used by the HtmlAddStyle routine.
  // We have to store this state information here since HtmlAddStyle
  // operates incrementally. This information must be carried from
  // one incremental execution to the next.

  SHtmlStyleStack *styleStack;  // The style stack
  int paraAlignment;            // Justification associated with <p>
  int rowAlignment;             // Justification associated with <tr>
  int anchorFlags;              // Style flags associated with <A>...</A>
  int inDt;                     // Style flags associated with <DT>...</DT>
  int inTr;                     // True if within <tr>..</tr>
  int inTd;                     // True if within <td>..</td> or <th>..</th>
  OHtmlAnchor *anchorStart;     // Most recent <a href=...>
  OHtmlForm *formStart;         // Most recent <form>
  OHtmlInput *formElemStart;    // Most recent <textarea> or <select>
  OHtmlInput *formElemLast;     // Most recent <input>, <textarea> or <select>
  OHtmlListStart *innerList;    // The inner most <OL> or <UL>
  OHtmlElement *loEndPtr;       // How far AddStyle has gone to
  OHtmlForm *loFormStart;       // For AddStyle

  // These fields are used to hold the state of the layout engine.
  // Because the layout is incremental, this state must be held for
  // the life of the widget.

  OHtmlLayoutContext layoutContext;

  // Information used when displaying the widget:

  int highlightWidth;		// Width in pixels of highlight to draw
				// around widget when it has the focus.
				// <= 0 means don't draw a highlight.
  OInsets margins;              // document margins (separation between the
                                // edge of the clip window and rendered HTML).
  XColor *highlightBgColorPtr;  // Color for drawing traversal highlight
				// area when highlight is off.
  XColor *highlightColorPtr;	// Color for drawing traversal highlight.
  OXFont *aFont[N_FONT];        // Information about all screen fonts
  char fontValid[(N_FONT+7)/8]; // If bit N%8 of work N/8 of this field is 0
                                // if aFont[N] needs to be reallocated before
                                // being used.
  XColor *apColor[N_COLOR];     // Information about all colors
  long colorUsed;               // bit N is 1 if color N is in use. Only
                                // applies to colors that aren't predefined
  int iDark[N_COLOR];           // Dark 3D shadow of color K is iDark[K]
  int iLight[N_COLOR];          // Light 3D shadow of color K is iLight[K]
  XColor *bgColor;              // Background color of the HTML document
  XColor *fgColor;              // Color of normal text. apColor[0]
  XColor *newLinkColor;         // Color of unvisitied links. apColor[1]
  XColor *oldLinkColor;         // Color of visitied links. apColor[2]
  XColor *selectionColor;       // Background color for selections
  GcCache aGcCache[N_CACHE_GC]; // A cache of GCs for general use
  int GcNextToFree;
  int lastGC;                   // Index of recently used GC
  OHtmlImage *imageList;        // A list of all images
  OImage *bgImage;              // Background image

  int formPadding;              // Amount to pad form elements by
  int overrideFonts;            // TRUE if we should override fonts
  int overrideColors;           // TRUE if we should override colors
  int underlineLinks;           // TRUE if we should underline hyperlinks
  int HasScript;                // TRUE if we can do scripts for this page
  int HasFrames;                // TRUE if we can do frames for this page
  int AddEndTags;               // TRUE if we add /LI etc.
  int TableBorderMin;           // Force tables to have min border size
  int varind;                   // Index suffix for unique global var name

  // Information about the selection

  int exportSelection;          // True if the selection is automatically
                                // exported to the clipboard

  // Miscellaneous information:

  int tableRelief;              // 3d effects on <TABLE>
  int ruleRelief;               // 3d effects on <HR>
  int rulePadding;              // extra pixels above and below <HR>
  char *zBase;                  // The base URI
  char *zBaseHref;              // zBase as modified by <BASE HREF=..> markup
  Cursor cursor;		// Current cursor for window, or None.
  int maxX, maxY;               // Maximum extent of any "paint" that appears
                                // on the virtual canvas. Used to compute 
                                // scrollbar positions.
  int dirtyLeft, dirtyTop;      // Top left corner of region to redraw. These
                                // are physical screen coordinates relative to
                                // the clip win, not main window.
  int dirtyRight, dirtyBottom;  // Bottom right corner of region to redraw
  int flags;                    // Various flags; see below for definitions.
  int idind;
  int inParse;                  // Prevent update if parsing
  char *zGoto;                  // Label to goto right after layout
  
  SHtmlExtensions *exts;        // Pointer to user extension data

  OStringHashTable uidTable;    // Hash table for some used string values
                                // like color names, etc.
  const char *_lastUri;         // Used in HandleMotion
  int _exiting;                 // True if the widget is being destroyed
};


// Flag bits "flags" field of the Html widget:
//
// REDRAW_PENDING         An idle handler has already been queued to 
//                        call the OXHtml::Redraw() method.
//
// GOT_FOCUS              This widget currently has input focus.
//
// HSCROLL                Horizontal scrollbar position needs to be
//                        recomputed.
//
// VSCROLL                Vertical scrollbar position needs to be
//                        recomputed.
//
// RELAYOUT               We need to reposition every element on the 
//                        virtual canvas. (This happens, for example,
//                        when the size of the widget changes and we
//                        need to recompute the line breaks.)
//
// RESIZE_ELEMENTS        We need to recompute the size of every element.
//                        This happens, for example, when the fonts
//                        change.
//
// REDRAW_FOCUS           We need to repaint the focus highlight border.
//
// REDRAW_TEXT            Everything in the clipping window needs to be redrawn.
//
// STYLER_RUNNING         There is a call to HtmlAddStyle() in process.
//                        Used to prevent a recursive call to HtmlAddStyle().
//
// INSERT_FLASHING        True if there is a timer scheduled that will toggle
//                        the state of the insertion cursor.
//
// REDRAW_IMAGES          One or more OHtmlImageMarkup objects have their
//                        redrawNeeded flag set.

#define REDRAW_PENDING	     0x000001
#define GOT_FOCUS            0x000002
#define HSCROLL              0x000004
#define VSCROLL              0x000008
#define RELAYOUT             0x000010
#define RESIZE_ELEMENTS      0x000020
#define REDRAW_FOCUS         0x000040
#define REDRAW_TEXT          0x000080
#define EXTEND_LAYOUT        0x000100
#define STYLER_RUNNING       0x000200
#define INSERT_FLASHING      0x000400
#define REDRAW_IMAGES        0x000800
#define ANIMATE_IMAGES       0x001000


// Macros to set, clear or test bits of the "flags" field.

#define HtmlHasFlag(A,F)      (((A)->flags&(F))==(F))
#define HtmlHasAnyFlag(A,F)   (((A)->flags&(F))!=0)
#define HtmlSetFlag(A,F)      ((A)->flags|=(F))
#define HtmlClearFlag(A,F)    ((A)->flags&=~(F))


// No coordinate is ever as big as this number

#define LARGE_NUMBER 100000000


// Default values for configuration options

#define DEF_HTML_BG_COLOR             DEF_FRAME_BG_COLOR
#define DEF_HTML_BG_MONO              DEF_FRAME_BG_MONO
#define DEF_HTML_EXPORT_SEL           1
#define DEF_HTML_FG                   DEF_BUTTON_FG
#define DEF_HTML_HIGHLIGHT_BG         DEF_BUTTON_HIGHLIGHT_BG
#define DEF_HTML_HIGHLIGHT            DEF_BUTTON_HIGHLIGHT
#define DEF_HTML_HIGHLIGHT_WIDTH      "0"
#define DEF_HTML_INSERT_OFF_TIME      300
#define DEF_HTML_INSERT_ON_TIME       600
#define DEF_HTML_PADX                 (HTML_INDENT / 4)
#define DEF_HTML_PADY                 (HTML_INDENT / 4)
#define DEF_HTML_RELIEF               "raised"
#define DEF_HTML_SELECTION_COLOR      "skyblue"
#define DEF_HTML_TAKE_FOCUS           "0"
#define DEF_HTML_UNVISITED            "blue2"
#define DEF_HTML_VISITED              "purple4"

#ifdef NAVIGATOR_TABLES

#define DEF_HTML_TABLE_BORDER             "0"
#define DEF_HTML_TABLE_CELLPADDING        "2"
#define DEF_HTML_TABLE_CELLSPACING        "5"
#define DEF_HTML_TABLE_BORDER_LIGHT_COLOR "gray80"
#define DEF_HTML_TABLE_BORDER_DARK_COLOR  "gray40"

#endif  // NAVIGATOR_TABLES


//----------------------------------------------------------------------

// Messages generated by the HTML widget

class OHtmlMessage : public OWidgetMessage {
public:
  OHtmlMessage(int t, int a, int i, const char *u, int rx, int ry) :
    OWidgetMessage(t, a, i) {
      uri = u;
      x_root = rx;
      y_root = ry;
    }

public:
  const char *uri;
  //ORectangle bbox;
  int x_root, y_root;
};


#endif  // __OXHTML_H
