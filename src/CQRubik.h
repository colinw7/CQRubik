#include <QWidget>
#include <QGLWidget>

#include <CMatrix3D.h>

#include <iostream>

class CQGLControl;
class CQRubik2D;
class CQRubik3D;
class CQGLControlToolBar;
class CUndo;
class CGLTexture;
class CQWinWidget;

struct CRubikSideConnect {
  uint side;
  int  rotate;

  CRubikSideConnect(uint side1=0, int rotate1=0) :
   side(side1), rotate(rotate1) {
  }
};

struct CRubikPieceInd {
  uint side_num;
  uint side_col;
  uint side_row;

  CRubikPieceInd(uint side_num1=0, uint side_col1=0, uint side_row1=0) :
   side_num(side_num1), side_col(side_col1), side_row(side_row1) {
  }

  bool assertInd(const char *msg, uint side_num1, uint side_col1, uint side_row1) {
    if (side_num != side_num1 || side_col != side_col1 || side_row != side_row1) {
      std::cerr << msg << "# ";

      print(std::cerr);

      std::cerr << std::endl;

      return false;
    }

    return true;
  }

  void print(std::ostream &os=std::cout) {
    os << side_num << ": " << side_col << ", " << side_row;
  }
};

struct CRubikPiece {
  uint side;
  uint id;

  mutable QRect r;

  CRubikPiece(uint side1=0, uint id1=0) :
   side(side1), id(id1) {
  }

  CRubikPiece &operator=(const CRubikPiece &piece) {
    side = piece.side;
    id   = piece.id;
    r    = piece.r;

    return *this;
  }
};

struct CRubikSide {
  enum { SIDE_ROWS = 3 };
  enum { SIDE_COLS = 3 };

  std::string       name;
  CRubikPiece       pieces[SIDE_COLS][SIDE_ROWS];
  CRubikSideConnect side_l, side_r, side_u, side_d;

  mutable QRect r;

  CRubikSide() { }
};

struct CQRubikAnimateData {
  bool      animating;
  uint      side_num;
  bool      clockwise;
  bool      axis;
  uint      step;
  QPolygonF polygon;

  CQRubikAnimateData() :
   animating(false), side_num(0), clockwise(false), axis(false), step(0) {
  }
};

class CQRubik : public QWidget {
  Q_OBJECT

 public:
  enum { CUBE_SIDES   = 6 };
  enum { SIDE_LENGTH  = 3 };
  enum { SIDE_PIECES  = 9 };
  enum { SIDE_ROWS    = 3 };
  enum { SIDE_COLS    = 3 };

 public:
  CQRubik(QWidget *parent=NULL);

  bool getShade    () const { return shade_     ; }
  bool getNumber   () const { return number_    ; }
  bool getUndoGroup() const { return undo_group_; }
  bool getAnimate  () const { return animate_   ; }
  bool getValidate () const { return validate_  ; }
  bool getShow3    () const { return show3_     ; }

  void setShade    (bool shade     ) { shade_      = shade     ; }
  void setNumber   (bool number    ) { number_     = number    ; }
  void setUndoGroup(bool undo_group) { undo_group_ = undo_group; }
  void setAnimate  (bool animate   ) { animate_    = animate   ; }
  void setShow3    (bool show3     ) { show3_      = show3     ; }

  CUndo *getUndo() const { return undo_; }

  const CRubikPieceInd &getInd() const { return ind_; }
  int                   getDir() const { return dir_; }

  CQRubik2D *getTwoD  () const { return twod_  ; }
  CQRubik3D *getThreeD() const { return threed_; }

  void placeWidgets();

  void reset();

  void randomize();

  bool solve();
  bool solve1();

  void execute(const std::string &str);

  void moveSideLeft2 (uint side_num, uint side_row);
  void moveSideRight2(uint side_num, uint side_row);
  void moveSideDown2 (uint side_num, uint side_col);
  void moveSideUp2   (uint side_num, uint side_col);

  void moveSideLeft (uint side_num, uint side_row);
  void moveSideRight(uint side_num, uint side_row);
  void moveSideDown (uint side_num, uint side_col);
  void moveSideUp   (uint side_num, uint side_col);

  void rotateSide2(uint side_num);

  void rotateSide(uint side_num, bool clockwise);

  void animateRotateSide(uint side_num, bool clockwise);

  void animateRotateMiddleX(bool clockwise);
  void animateRotateMiddleY(bool clockwise);
  void animateRotateMiddleZ(bool clockwise);

  CRubikPieceInd findPiece(uint side_num, uint id);

  const CRubikSide &getSide(uint i) const { return sides_[i]; }

  QColor getColor(const CRubikPiece &piece);
  QColor getColor(uint value);

  const CRubikPiece &getPieceLeft (uint side_num, uint side_col, uint side_row) const;
  const CRubikPiece &getPieceRight(uint side_num, uint side_col, uint side_row) const;
  const CRubikPiece &getPieceUp   (uint side_num, uint side_col, uint side_row) const;
  const CRubikPiece &getPieceDown (uint side_num, uint side_col, uint side_row) const;

  bool validate();

  void movePosition(int key);
  void movePieces  (int key);
  void rotatePieces(int key);
  void moveSides   (int key);

  CQRubikAnimateData &getAnimateData() { return animateData_; }

  void paintEvent(QPaintEvent *) override;
  void resizeEvent(QResizeEvent *) override;

  void keyPressEvent(QKeyEvent *e) override;

  void waitAnimate();

  static bool decodeSideChar(char c, uint &n);
  static bool encodeSideChar(uint n, char &c);

 private:
  bool solveTopInd4();
  bool solveTopInd1();
  bool solveTopInd3();
  bool solveTopInd5();
  bool solveTopInd7();
  bool solveTopInd0();
  bool solveTopInd2();
  bool solveTopInd6();
  bool solveTopInd8();
  bool solveMidInd4();

  bool solveMidLeftInd3();
  bool solveMidLeftInd5();

  bool solveMidRightInd3();
  bool solveMidRightInd5();

  bool solveBottomCross();
  bool solveBottomCornerOrder();
  bool solveBottomCornerOrder1(bool check=false);
  bool solveBottomCornerOrient(bool check=false);
  void solveBottomCornerOrient1();
  void solveBottomCornerOrient2();
  bool solveBottomMiddles();
  void solveBottomMiddlesSub();
  void solveBottomMiddles1();
  void solveBottomMiddles2();

  void getPosLeft (uint side_num, uint side_col, uint side_row, int dir,
                   uint &side_num1, uint &side_col1, uint &side_row1, int &dir1) const;
  void getPosRight(uint side_num, uint side_col, uint side_row, int dir,
                   uint &side_num1, uint &side_col1, uint &side_row1, int &dir1) const;
  void getPosUp   (uint side_num, uint side_col, uint side_row, int dir,
                   uint &side_num1, uint &side_col1, uint &side_row1, int &dir1) const;
  void getPosDown (uint side_num, uint side_col, uint side_row, int dir,
                   uint &side_num1, uint &side_col1, uint &side_row1, int &dir1) const;

  void moveSidesLeft ();
  void moveSidesRight();
  void moveSidesDown ();
  void moveSidesUp   ();

  void moveSideLeft1 (uint side_num, uint side_row);
  void moveSideRight1(uint side_num, uint side_row);
  void moveSideDown1 (uint side_num, uint side_col);
  void moveSideUp1   (uint side_num, uint side_col);

  void rotateSide1(uint side_num, bool clockwise);

 private slots:
  void animateRotateSideSlot();

 private:
  CRubikSide          sides_[CUBE_SIDES];
  CRubikPieceInd      ind_;
  QColor              colors_[CUBE_SIDES];
  bool                shade_, number_, undo_group_, animate_, validate_, show3_;
  CQRubikAnimateData  animateData_;
  int                 dir_;
  CQRubik2D          *twod_;
  CQRubik3D          *threed_;
  CQWinWidget        *w2_;
  CQWinWidget        *w3_;
  CQGLControlToolBar *toolbar_;
  CUndo              *undo_;
};

class CQRubik2D : public QWidget {
  Q_OBJECT

 public:
  enum { DRAW_COLUMNS = 4 };
  enum { DRAW_ROWS    = 3 };

 public:
  CQRubik2D(CQRubik *rubik);

 private:
  void paintEvent(QPaintEvent *) override;

  void mousePressEvent(QMouseEvent *e) override;
  void keyPressEvent  (QKeyEvent *e) override;

 private:
  void drawSide(QPainter *p, uint i);

  void drawAnimation(QPainter *p, CQRubikAnimateData *animateData);

 private:
  CQRubik *rubik_;
};

class CQRubik3D : public QGLWidget {
  Q_OBJECT

 public:
  CQRubik3D(CQRubik *rubik);

  CQGLControlToolBar *createToolBar();

  void toggleTexture();

  void animateSide(CQRubikAnimateData *animateData);

 protected:
  void paintGL() override;

  void resizeGL(int width, int height) override;

  void mousePressEvent  (QMouseEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;
  void mouseMoveEvent   (QMouseEvent *event) override;

  void keyPressEvent(QKeyEvent *event) override;

  void drawSide(uint i);

  void drawSideCubes(uint i, bool *b=NULL, CQRubikAnimateData *animateData=NULL);

  void drawCube(double xc, double yc, double zc, double size, const QColor *c,
                const CMatrix3D &m);

 private:
  CQRubik     *rubik_;
  CQGLControl *control_;
  bool         showTexture_;
  CGLTexture  *texture_;
  uint         tId_;
};
