#ifndef CGL_UTIL_H
#define CGL_UTIL_H

#include <CFont.h>
#include <CPoint3D.h>
#include <CRGBA.h>
#include <GL/gl.h>

class CGLTexture;

namespace CGLUtil {

void invertMatrix(const GLdouble *m, GLdouble *out);

void *fontToBitmap(CFontPtr font);

int fontWidth  (CFontPtr font);
int fontAscent (CFontPtr font);
int fontDescent(CFontPtr font);

int fontHeight(CFontPtr font);

void drawTexturePoint(const CPoint3D &point, CGLTexture *texture);

//---

struct CRGBAToFV {
  float fvalues[4];

  CRGBAToFV(const CRGBA &rgba) {
    fvalues[0] = rgba.getRedF  ();
    fvalues[1] = rgba.getGreenF();
    fvalues[2] = rgba.getBlueF ();
    fvalues[3] = rgba.getAlphaF();
  }
};

}

#endif
