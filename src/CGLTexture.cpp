#include <CGLTexture.h>

#if 0
#include <glad/glad.h>
#endif
#include <GL/glut.h>

namespace {

bool checkError() {
  // check texture generated
  GLenum err = glGetError();

  if (err != GL_NO_ERROR) {
    std::cerr << "OpenGL Error: " << gluErrorString(err) << "\n";
    return false;
  }

  return true;
}

}

//---

CGLTexture::
CGLTexture()
{
}

CGLTexture::
CGLTexture(const CImagePtr &image)
{
  setImage(image);
}

CGLTexture::
~CGLTexture()
{
  if (valid_ && glIsTexture(id_))
    glDeleteTextures(1, &id_);
}

bool
CGLTexture::
load(const std::string &fileName, bool flip)
{
  if (! CFile::exists(fileName) || ! CFile::isRegular(fileName)) {
    std::cerr << "Error: Invalid texture file '" << fileName << "'\n";
    return false;
  }

  CImageFileSrc src(fileName);

  auto image = CImageMgrInst->createImage(src);

  if (! image) {
    std::cerr << "Error: Failed to read image from '" << fileName << "'\n";
    return false;
  }

  return load(image, flip);
}

bool
CGLTexture::
load(CImagePtr image, bool flip)
{
  return init(image, flip);
}

void
CGLTexture::
setImage(const CImagePtr &image)
{
  init(image, false);
}

#if 0
bool
CGLTexture::
setTarget(int w, int h)
{
  if (! valid_ || w != targetWidth_ ||  h != targetHeight_) {
    targetWidth_  = w;
    targetHeight_ = h;

    // The framebuffer, which regroups 0, 1, or more textures, and 0 or 1 depth buffer.
    if (frameBufferId_ == 0)
      glGenFramebuffers(1, &frameBufferId_);

    glBindFramebuffer(GL_FRAMEBUFFER, frameBufferId_);
    if (! checkError()) return false;

    // The texture we're going to render to
    if (id_ == 0) {
      glGenTextures(1, &id_);
      if (! checkError()) return false;
    }

    // generate texture
    glBindTexture(GL_TEXTURE_2D, id_);
    if (! checkError()) return false;

    // Give an empty image to OpenGL ( the last "0" )
    // no difference for GL_RGBA and GL_RGB
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, targetWidth_, targetHeight_,
                 /*border*/0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    if (! checkError()) return false;

    // Poor filtering (need min filter to avoid mip map use - not set)
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

//  glBindTexture(GL_TEXTURE_2D, 0);
//  if (! checkError()) return false;

    if (depthRenderBuffer_ == 0)
      glGenRenderbuffers(1, &depthRenderBuffer_);

    glBindRenderbuffer(GL_RENDERBUFFER, depthRenderBuffer_);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, targetWidth_, targetHeight_);
//  glBindRenderbuffer(GL_RENDERBUFFER, 0);

    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                              GL_RENDERBUFFER, depthRenderBuffer_);

    // attach it to currently bound framebuffer object
    //glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, id_, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, id_, 0);

    // generate render buffer
    // Set the list of draw buffers.
    GLenum drawBuffers[1] = { GL_COLOR_ATTACHMENT0 };
    glDrawBuffers(1, drawBuffers); // "1" is the size of DrawBuffers

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
      std::cerr << "Framebuffer error\n";
      return false;
    }

    valid_ = true;
  }

  return true;
}
#endif

bool
CGLTexture::
init(CImagePtr image, bool flip)
{
  if (! image) {
    std::cerr << "Invalid image data\n";
    return false;
  }

  image_ = image;

  image_->convertToRGB();

  if (flip)
    image_ = image_->flippedH();

  auto w = image_->getWidth ();
  auto h = image_->getHeight();

  //------

  //glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

  // allocate texture id
  glGenTextures(1, &id_);
  if (! checkError()) return false;

  valid_ = true;

  // set texture type
  glBindTexture(GL_TEXTURE_2D, id_);
  if (! checkError()) return false;

  if (wrapType() == WrapType::CLAMP) {
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  }
  else {
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  }
  if (! checkError()) return false;

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  if (! checkError()) return false;

  // select modulate to mix texture with color for shading
  //glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
  //if (! checkError()) return false;

  // build our texture mipmaps
  GLint internalFormat = (useAlpha() ? GL_RGBA : GL_RGB);

  if (CMathGen::isPowerOf(2, w) && CMathGen::isPowerOf(2, h)) {
#if 1
    // Hardware mipmap generation
    glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP_SGIS, GL_TRUE);
    if (! checkError()) return false;

    glHint(GL_GENERATE_MIPMAP_HINT_SGIS, GL_NICEST);
    if (! checkError()) return false;
#endif

    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, int(w), int(h), 0,
                 GL_BGRA, GL_UNSIGNED_BYTE, image_->getData());
    if (! checkError()) return false;

#if 0
    glGenerateMipmap(GL_TEXTURE_2D);
    if (! checkError()) return false;
#endif
  }
  else {
    // No hardware mipmap generation support, fall back to the
    // good old gluBuild2DMipmaps function
    gluBuild2DMipmaps(GL_TEXTURE_2D, internalFormat, int(w), int(h),
                      GL_BGRA, GL_UNSIGNED_BYTE, image_->getData());
    if (! checkError()) return false;
  }

  return true;
}

#if 0
void
CGLTexture::
bindTo(GLenum num) const
{
  glActiveTexture(num);

  bind();
}
#endif

void
CGLTexture::
bind() const
{
#if 0
  if (frameBufferId_ > 0) {
    functions_->glBindFramebuffer(GL_FRAMEBUFFER, 0);
  }
  else
    glBindTexture(GL_TEXTURE_2D, id_);
#else
  glBindTexture(GL_TEXTURE_2D, id_);
#endif
}

void
CGLTexture::
unbind() const
{
#if 0
  if (frameBufferId_ > 0) {
    functions_->glBindFramebuffer(GL_FRAMEBUFFER, 0);
  }
  else
    glBindTexture(GL_TEXTURE_2D, 0);
#else
  glBindTexture(GL_TEXTURE_2D, 0);
#endif
}

#if 0
void
CGLTexture::
bindBuffer() const
{
  glEnable(GL_TEXTURE_2D);

  if (frameBufferId_)
    glBindTexture(GL_TEXTURE_2D, id_);
}

void
CGLTexture::
unbindBuffer() const
{
  if (frameBufferId_)
    glBindTexture(GL_TEXTURE_2D, 0);
}
#endif

void
CGLTexture::
enable(bool b)
{
  if (b)
    glEnable(GL_TEXTURE_2D);
  else
    glDisable(GL_TEXTURE_2D);
}

void
CGLTexture::
draw()
{
  draw(0.0, 0.0, 1.0, 1.0);
}

void
CGLTexture::
draw(double x1, double y1, double x2, double y2)
{
  draw(x1, y1, 0, x2, y2, 0, 0, 0, 1, 1);
}

void
CGLTexture::
draw(double x1, double y1, double z1, double x2, double y2, double z2,
     double tx1, double ty1, double tx2, double ty2)
{
  //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glEnable(GL_TEXTURE_2D);

  bind();

  // select modulate to mix texture with color for shading
  //glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

  glBegin(GL_QUADS);

  if      (fabs(z2 - z1) < 1E-3) {
    glTexCoord2d(tx1, ty1); glVertex3d(x1, y1, z1);
    glTexCoord2d(tx1, ty2); glVertex3d(x1, y2, z1);
    glTexCoord2d(tx2, ty2); glVertex3d(x2, y2, z1);
    glTexCoord2d(tx2, ty1); glVertex3d(x2, y1, z1);
  }
  else if (fabs(y2 - y1) < 1E-3) {
    glTexCoord2d(tx1, ty1); glVertex3d(x1, y1, z1);
    glTexCoord2d(tx1, ty2); glVertex3d(x1, y1, z2);
    glTexCoord2d(tx2, ty2); glVertex3d(x2, y1, z2);
    glTexCoord2d(tx2, ty1); glVertex3d(x2, y1, z1);
  }
  else {
    glTexCoord2d(tx1, ty1); glVertex3d(x1, y1, z1);
    glTexCoord2d(tx1, ty2); glVertex3d(x1, y2, z1);
    glTexCoord2d(tx2, ty2); glVertex3d(x1, y2, z2);
    glTexCoord2d(tx2, ty1); glVertex3d(x1, y1, z2);
  }

  glEnd();

  glDisable(GL_TEXTURE_2D);
}

#if 0
void
CGLTexture::
displayFramebufferTexture(ShaderProgram *program, int vertexId)
{
  if (! notInitialized) {
    // initialize shader and vao w/ NDC vertex coordinates at top-right of the screen
    [...]
  }

  glActiveTexture(GL_TEXTURE0);

  program->bind();

  bind();

  glBindVertexArray(vertexId);

  glDrawArrays(GL_TRIANGLES, 0, 6);

  glBindVertexArray(0);

  unbind();

  program->unbind();
}
#endif
