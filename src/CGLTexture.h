#ifndef CGL_TEXTURE_H
#define CGL_TEXTURE_H

#include <CImageLib.h>
#include <GL/gl.h>

class CGLTexture {
 public:
  enum class WrapType {
    CLAMP,
    REPEAT
  };

 public:
  CGLTexture();
  CGLTexture(const CImagePtr &image);

 ~CGLTexture();

  const WrapType &wrapType() const { return wrapType_; }
  void setWrapType(const WrapType &v) { wrapType_ = v; }

  bool useAlpha() const { return useAlpha_; }
  void setUseAlpha(bool b) { useAlpha_ = b; }

  bool isFlipped() const { return flipped_; }
  void setFlipped(bool b) { flipped_ = b; }

  bool load(const std::string &fileName, bool flip=false);
  bool load(CImagePtr image, bool flip=false);

  const CImagePtr &getImage() const { return image_; }
  void setImage(const CImagePtr &image);

  uint getId() const { return id_; }

  const std::string &getName() const { return name_; }
  void setName(const std::string &s) { name_ = s; }

#if 0
  bool setTarget(int w, int h);
#endif

  void enable(bool b);
//void bindTo(GLenum num) const;
  void bind() const;
  void unbind() const;

#if 0
  void bindBuffer() const;
  void unbindBuffer() const;
#endif

  void draw();
  void draw(double x1, double y1, double x2, double y2);
  void draw(double x1, double y1, double z1, double x2, double y2, double z2,
            double tx1=0.0, double ty1=0.0, double tx2=1.0, double ty2=1.0);

 private:
  CGLTexture(const CGLTexture &);

  CGLTexture &operator=(const CGLTexture &);

  bool init(CImagePtr image, bool flip);

 private:
  CImagePtr image_;

  uint        id_       { 0 };
  std::string name_;
  bool        valid_    { false };
  WrapType    wrapType_ { WrapType::REPEAT };
  bool        useAlpha_ { true };
  bool        flipped_  { false };

#if 0
  GLuint frameBufferId_ { 0 };
  GLuint depthRenderBuffer_ { 0 };

  int targetWidth_  { -1 };
  int targetHeight_ { -1 };
#endif
};

//---

class CGLTextureMgr {
 public:
  static CGLTextureMgr *getInstance() {
    static CGLTextureMgr *instance;

    if (! instance)
      instance = new CGLTextureMgr;

    return instance;
  }

  CGLTexture *load(const std::string &fileName, bool flip=false) {
    // Look for the texture in the registry
    auto p = texture_map_.find(fileName);

    if (p != texture_map_.end())
      return (*p).second;

    // If not found, load the texture
    auto *texture = new CGLTexture;

    if (! texture->load(fileName, flip)) {
      delete texture;
      return NULL;
    }

    // The texture has been successfully loaded, register it.
    texture_map_[fileName] = texture;

    return texture;
  }

 private:
  using TextureMap = std::map<std::string,CGLTexture *>;

  TextureMap texture_map_;
};

#endif
