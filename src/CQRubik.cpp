#include <CQRubik.h>
#include <CQApp.h>
#include <CQImage.h>
#include <CQGLControl.h>
#include <CGLTexture.h>
#include <CQWinWidget.h>
#include <CMathRand.h>
#include <CMatrix2D.h>
#include <CStrParse.h>
#include <CUndo.h>
#include <QPainter>
#include <QKeyEvent>
#include <QTimer>

class CQRubikUndoMoveData : public CUndoData {
 public:
  CQRubikUndoMoveData(CQRubik *rubik, uint side_num, char dir, uint pos) :
   rubik_(rubik), side_num_(side_num), dir_(dir), pos_(pos) {
  }

  bool exec() override {
    if (getState() == UNDO_STATE) {
      if      (dir_ == 'U') rubik_->moveSideDown (side_num_, pos_);
      else if (dir_ == 'D') rubik_->moveSideUp   (side_num_, pos_);
      else if (dir_ == 'L') rubik_->moveSideRight(side_num_, pos_);
      else if (dir_ == 'R') rubik_->moveSideLeft (side_num_, pos_);
    }
    else {
      if      (dir_ == 'U') rubik_->moveSideUp   (side_num_, pos_);
      else if (dir_ == 'D') rubik_->moveSideDown (side_num_, pos_);
      else if (dir_ == 'L') rubik_->moveSideLeft (side_num_, pos_);
      else if (dir_ == 'R') rubik_->moveSideRight(side_num_, pos_);
    }

    rubik_->waitAnimate();

    return true;
  }

 private:
  CQRubik *rubik_;
  uint     side_num_;
  char     dir_;
  uint     pos_;
};

class CQRubikUndoRotateData : public CUndoData {
 public:
  CQRubikUndoRotateData(CQRubik *rubik, uint side_num, bool clockwise) :
   rubik_(rubik), side_num_(side_num), clockwise_(clockwise) {
  }

  bool exec() override {
    if (getState() == UNDO_STATE) rubik_->rotateSide(side_num_, ! clockwise_);
    else                          rubik_->rotateSide(side_num_, clockwise_);

    rubik_->waitAnimate();

    return true;
  }

  void print() {
    char c;

    CQRubik::encodeSideChar(side_num_, c);

    std::cerr << c << (clockwise_ ? " " : "' ") << std::endl;
  }

 private:
  CQRubik *rubik_;
  uint     side_num_;
  bool     clockwise_;
};

int
main(int argc, char **argv)
{
  CQApp app(argc, argv);

  CQRubik rubik;

  rubik.resize(600, 600);

  rubik.placeWidgets();

  rubik.show();

  return app.exec();
}

CQRubik::
CQRubik(QWidget *parent) :
 QWidget(parent), shade_(true), number_(false), undo_group_(true),
 animate_(false), validate_(false), show3_(false), dir_(0)
{
  twod_   = new CQRubik2D(this);
  threed_ = new CQRubik3D(this);

  w2_ = new CQWinWidget(this);
  w3_ = new CQWinWidget(this);

  w2_->setFocusPolicy(Qt::StrongFocus);
  w3_->setFocusPolicy(Qt::StrongFocus);

  w2_->setChild(twod_);
  w3_->setChild(threed_);

  undo_ = new CUndo;

  reset();

  //------

  ind_ = CRubikPieceInd(2, 1, 1);

  //------

  colors_[0] = QColor(255,255,255);
  colors_[1] = QColor(255,  0,  0);
  colors_[2] = QColor(  0,255,  0);
  colors_[3] = QColor(255,127,  0);
  colors_[4] = QColor(255,255,  0);
  colors_[5] = QColor(  0,  0,255);

  twod_->setFocus();
}

void
CQRubik::
reset()
{
  for (uint i = 0; i < CUBE_SIDES; ++i) {
    CRubikSide &side = sides_[i];

    for (uint k = 0, id = 0; k < SIDE_COLS; ++k)
      for (uint j = 0; j < SIDE_ROWS; ++j, ++id)
        side.pieces[k][j] = CRubikPiece(i, id);
  }

  // Left Face
  sides_[0].name = "L";

  sides_[0].side_l = CRubikSideConnect(5,   0);
  sides_[0].side_r = CRubikSideConnect(2,   0);
  sides_[0].side_d = CRubikSideConnect(3,  90);
  sides_[0].side_u = CRubikSideConnect(1, -90);

  // Up Face
  sides_[1].name = "U";

  sides_[1].side_l = CRubikSideConnect(0,  90);
  sides_[1].side_r = CRubikSideConnect(4, -90);
  sides_[1].side_d = CRubikSideConnect(2,   0);
  sides_[1].side_u = CRubikSideConnect(5, 180);

  // Front Face
  sides_[2].name = "F";

  sides_[2].side_l = CRubikSideConnect(0,   0);
  sides_[2].side_r = CRubikSideConnect(4,   0);
  sides_[2].side_d = CRubikSideConnect(3,   0);
  sides_[2].side_u = CRubikSideConnect(1,   0);

  // Down Face
  sides_[3].name = "D";

  sides_[3].side_l = CRubikSideConnect(0, -90);
  sides_[3].side_r = CRubikSideConnect(4,  90);
  sides_[3].side_d = CRubikSideConnect(5, 180);
  sides_[3].side_u = CRubikSideConnect(2,   0);

  // Right Face
  sides_[4].name = "R";

  sides_[4].side_l = CRubikSideConnect(2,   0);
  sides_[4].side_r = CRubikSideConnect(5,   0);
  sides_[4].side_d = CRubikSideConnect(3, -90);
  sides_[4].side_u = CRubikSideConnect(1,  90);

  // Back Face
  sides_[5].name = "B";

  sides_[5].side_l = CRubikSideConnect(4,   0);
  sides_[5].side_r = CRubikSideConnect(0,   0);
  sides_[5].side_d = CRubikSideConnect(3, 180);
  sides_[5].side_u = CRubikSideConnect(1, 180);

  //-----

  undo_->clear();
}

void
CQRubik::
randomize()
{
  static const char *names[] =
    { "Move Up", "Move Down", "Move Left", "Move Right",
     "Rotate Clockwise", "Rotate Anti-Clockwise" };

  bool animate;

  std::swap(animate_, animate);

  uint num = 30;

  for (uint i = 0; i < num; ++i) {
    int side_num = CMathRand::randInRange(0, 5);
    int op       = CMathRand::randInRange(0, 5);
    int pos      = CMathRand::randInRange(0, 2);

    if      (op == 0) // move up
      moveSideUp   (side_num, pos);
    else if (op == 1) // move down
      moveSideDown (side_num, pos);
    else if (op == 2) // move left
      moveSideLeft (side_num, pos);
    else if (op == 3) // move right
      moveSideRight(side_num, pos);
    else if (op == 4)
      rotateSide(side_num, true);
    else
      rotateSide(side_num, false);

    if (! validate()) {
      std::cerr << "Invalid: " << names[op] << ": " << side_num << ":" << pos << std::endl;
      break;
    }
  }

  std::swap(animate_, animate);

  undo_->clear();
}

bool
CQRubik::
solve()
{
  undo_->clear();

  bool animate = false;

  std::swap(animate_, animate);

  bool rc = solve1();

  undo_->undoAll();

  animate = true;

  std::swap(animate_, animate);

  undo_->redoAll();

  std::swap(animate_, animate);

  return rc;
}

bool
CQRubik::
solve1()
{
  if (! solveTopInd4()) return false;

  if (! solveTopInd1()) return false;
  if (! solveTopInd3()) return false;
  if (! solveTopInd5()) return false;
  if (! solveTopInd7()) return false;

  // Top Corners
  if (! solveTopInd0()) return false;
  if (! solveTopInd2()) return false;
  if (! solveTopInd6()) return false;
  if (! solveTopInd8()) return false;

  // Middle Layer
  if (! solveMidInd4()) return false;

  if (! solveMidLeftInd3()) return false;
  if (! solveMidLeftInd5()) return false;

  if (! solveMidRightInd3()) return false;
  if (! solveMidRightInd5()) return false;

  // Last Layer
  moveSidesLeft(); moveSidesLeft();

  if (! solveBottomCross()) return false;

  if (! solveBottomCornerOrder()) return false;

  if (! solveBottomCornerOrient()) return false;

  if (! solveBottomMiddles()) return false;

  // Restore Original Top
  moveSidesLeft(); moveSidesLeft();

  return true;
}

bool
CQRubik::
solveTopInd4()
{
  // Green center to top
  CRubikPieceInd ind = findPiece(2, 4);

  uint side_num = ind.side_num;

  if (side_num == 2) return true;

  if (getUndoGroup()) undo_->startGroup();

  if      (side_num == 0) moveSideRight(side_num, 1); // execute("mFMR");
  else if (side_num == 1) moveSideDown (side_num, 1);
  else if (side_num == 3) moveSideUp   (side_num, 1);
  else if (side_num == 4) moveSideLeft (side_num, 1);
  else if (side_num == 5) moveSideLeft2(side_num, 1);

  if (getUndoGroup()) undo_->endGroup();

  ind = findPiece(2, 4);

  if (! ind.assertInd("top 4", 2, 1, 1)) return false;

  return true;
}

bool
CQRubik::
solveTopInd1()
{
  // Green Cross on top (piece numbers 1L 3T 5B 7R)
  CRubikPieceInd ind = findPiece(2, 1);

  uint side_num = ind.side_num;
  uint side_col = ind.side_col;
  uint side_row = ind.side_row;

  if (side_num == 2 && side_col == 0 && side_row == 1) return true;

  if (getUndoGroup()) undo_->startGroup();

  if (side_num != 2) {
    if      (side_col == 0) { }
    else if (side_col == 2) rotateSide2(side_num);
    else if (side_row == 0) rotateSide (side_num, true );
    else if (side_row == 2) rotateSide (side_num, false);

    if      (side_num == 0) {
      moveSideUp  (side_num, 0); moveSideLeft (2, 1);
      moveSideDown(side_num, 0); moveSideRight(2, 1);
    }
    else if (side_num == 1) {
      moveSideDown(side_num, 0);
    }
    else if (side_num == 3) {
      moveSideUp  (side_num, 0);
    }
    else if (side_num == 4) {
      rotateSide2(side_num);
      moveSideUp (side_num, 2); moveSideLeft (2, 1);
      moveSideUp (side_num, 2); moveSideRight(2, 1);
    }
    else if (side_num == 5) {
      rotateSide2(side_num);
      moveSideUp(side_num, 2);
      moveSideUp(side_num, 2);
    }
  }
  else {
    if      (side_col == 0) { }
    else if (side_col == 2) rotateSide2(side_num);
    else if (side_row == 0) rotateSide (side_num, true );
    else if (side_row == 2) rotateSide (side_num, false);
  }

  if (getUndoGroup()) undo_->endGroup();

  ind = findPiece(2, 4); if (! ind.assertInd("top 4", 2, 1, 1)) return false;
  ind = findPiece(2, 1); if (! ind.assertInd("top 1", 2, 0, 1)) return false;

  return true;
}

bool
CQRubik::
solveTopInd3()
{
  CRubikPieceInd ind = findPiece(2, 3);

  uint side_num = ind.side_num;
  uint side_col = ind.side_col;
  uint side_row = ind.side_row;

  if (side_num == 2 && side_col == 1 && side_row == 0) return true;

  if (getUndoGroup()) undo_->startGroup();

  if      (side_num != 2 && side_num != 0) {
    if      (side_row == 0) { }
    else if (side_row == 2) rotateSide2(side_num);
    else if (side_col == 0) rotateSide (side_num, false);
    else if (side_col == 2) rotateSide (side_num, true );

    if      (side_num == 0)
      moveSideRight(side_num, 0);
    else if (side_num == 1) {
      moveSideLeft (1, 0); moveSideUp  (2, 1);
      moveSideRight(1, 0); moveSideDown(2, 1);
    }
    else if (side_num == 3) {
      rotateSide(3, false); rotateSide(2, false);
      moveSideUp(3, 2); rotateSide(2, true);
    }
    else if (side_num == 4)
      moveSideLeft (side_num, 0);
    else if (side_num == 5)
      moveSideLeft2(side_num, 0);
  }
  else if (side_num == 0) {
    if      (side_row == 0)
      moveSideRight(0, 0);
    else if (side_row == 2) {
      rotateSide2(2); moveSideRight(0, 2); rotateSide2(2);
    }
    else if (side_col == 0) {
      moveSideUp(2, 1); moveSideUp(0, 0); moveSideDown(2, 1);
    }
    else if (side_col == 2) { // not possible
    }
  }
  else if (side_num == 2) {
    if      (side_row == 0) { }
    else if (side_row == 2) {
      moveSideDown(2, 1); moveSideLeft (3, 2); moveSideUp(2, 1); rotateSide2(2);
      moveSideDown(2, 1); moveSideRight(3, 2); moveSideUp(2, 1); rotateSide2(2);
    }
    else if (side_col == 0) { }
    else if (side_col == 2) {
      moveSideRight(2, 1); moveSideUp(2, 1);
      moveSideUp(4, 2);
      moveSideDown(2, 1); moveSideLeft(2, 1);
    }
  }

  if (getUndoGroup()) undo_->endGroup();

  ind = findPiece(2, 4); if (! ind.assertInd("top 4", 2, 1, 1)) return false;
  ind = findPiece(2, 1); if (! ind.assertInd("top 1", 2, 0, 1)) return false;
  ind = findPiece(2, 3); if (! ind.assertInd("top 3", 2, 1, 0)) return false;

  return true;
}

bool
CQRubik::
solveTopInd5()
{
  CRubikPieceInd ind = findPiece(2, 5);

  uint side_num = ind.side_num;
  uint side_col = ind.side_col;
  uint side_row = ind.side_row;

  if (side_num == 2 && side_col == 1 && side_row == 2) return true;

  if (getUndoGroup()) undo_->startGroup();

  if (side_num != 2 && side_num != 0 && side_num != 1) {
    if      (side_row == 2) { }
    else if (side_row == 0) rotateSide2(side_num);
    else if (side_col == 2) rotateSide (side_num, false);
    else if (side_col == 0) rotateSide (side_num, true );

    if      (side_num == 0)
      moveSideRight(side_num, 2);
    else if (side_num == 3) {
      moveSideRight(3, 2); moveSideDown(3, 1);
      moveSideLeft (3, 2); moveSideUp  (3, 1);
    }
    else if (side_num == 4)
      moveSideLeft (side_num, 2);
    else if (side_num == 5)
      moveSideLeft2(side_num, 2);
  }
  else if (side_num == 2) {
    if      (side_col == 0) { // not possible
    }
    else if (side_col == 2) {
      moveSideDown(2, 2); rotateSide(2, true); moveSideUp(2, 2); rotateSide(2, false);
    }
    else if (side_row == 0) { // not possible
    }
    else if (side_row == 2) { // already done
    }
  }
  else if (side_num == 0) {
    if      (side_row == 0) {
      rotateSide2(2); moveSideRight(0, 0); rotateSide2(2);
    }
    else if (side_row == 2)
      moveSideRight(0, 2);
    else if (side_col == 0) {
      rotateSide(2, false); rotateSide(0, true); rotateSide(2, true); moveSideRight(0, 2);
    }
    else if (side_col == 2) {
      rotateSide(2, true); moveSideRight(0, 0); rotateSide(2, false);
    }
  }
  else if (side_num == 1) {
    if      (side_row == 0) {
      rotateSide(2, true);
      moveSideRight(2, 1); moveSideRight(1, 0); moveSideLeft(2, 1);
      rotateSide(2, false);
    }
    else if (side_row == 2) { // not possible
    }
    else if (side_col == 0) {
      rotateSide(2, false); moveSideDown(1, 0); rotateSide(2, true);
    }
    else if (side_col == 2) {
      rotateSide(2, true); moveSideDown(1, 2); rotateSide(2, false);
    }
  }

  if (getUndoGroup()) undo_->endGroup();

  ind = findPiece(2, 4); if (! ind.assertInd("top 4", 2, 1, 1)) return false;
  ind = findPiece(2, 1); if (! ind.assertInd("top 1", 2, 0, 1)) return false;
  ind = findPiece(2, 3); if (! ind.assertInd("top 3", 2, 1, 0)) return false;
  ind = findPiece(2, 5); if (! ind.assertInd("top 5", 2, 1, 2)) return false;

  return true;
}

bool
CQRubik::
solveTopInd7()
{
  CRubikPieceInd ind = findPiece(2, 7);

  uint side_num = ind.side_num;
  uint side_col = ind.side_col;
  uint side_row = ind.side_row;

  if (side_num == 2 && side_col == 2 && side_row == 1) return true;

  if (getUndoGroup()) undo_->startGroup();

  if (side_num != 2 && side_num != 0 && side_num != 1 && side_num != 3) {
    if      (side_col == 2) { }
    else if (side_col == 0) rotateSide2(side_num);
    else if (side_row == 2) rotateSide (side_num, true );
    else if (side_row == 0) rotateSide (side_num, false);

    if      (side_num == 1)
      moveSideDown(side_num, 2);
    else if (side_num == 3)
      moveSideUp  (side_num, 2);
    else if (side_num == 4) {
      moveSideUp  (4, 2); moveSideRight(2, 1);
      moveSideDown(4, 2); moveSideLeft (2, 1);
    }
    else if (side_num == 5) {
      rotateSide2(5);
      moveSideRight(5, 1); moveSideUp(4, 2); moveSideUp(4, 2); moveSideLeft(5, 1);
    }
  }
  else if (side_num == 2) {
    // must be correct
  }
  else if (side_num == 0) {
    if      (side_col == 2) { // not possible
    }
    else if (side_col == 0) {
      moveSideUp  (0, 0); rotateSide2  (2   ); moveSideLeft(2, 1);
      moveSideDown(0, 0); moveSideRight(2, 1); rotateSide2 (2   );
    }
    else if (side_row == 2) {
      rotateSide(2, false); moveSideRight(0, 2); rotateSide(2, true);
    }
    else if (side_row == 0) {
      rotateSide(2, true); moveSideRight(0, 0); rotateSide(2, false);
    }
  }
  else if (side_num == 1) {
    if      (side_col == 2) {
      moveSideDown(1, 2);
    }
    else if (side_col == 0) {
      rotateSide(2, true); rotateSide2(1); rotateSide(2, false); moveSideDown(2, 2);
    }
    else if (side_row == 2) { // not possible
    }
    else if (side_row == 0) {
      moveSideRight(1, 0); rotateSide  (2, true); moveSideUp(2, 1);
      moveSideLeft (1, 0); moveSideDown(2, 1   ); rotateSide(2, false);
    }
  }
  else if (side_num == 3) {
    if      (side_col == 2) {
      moveSideUp(3, 2);
    }
    else if (side_col == 0) {
      rotateSide2(2); moveSideUp(3, 0); rotateSide2(2);
    }
    else if (side_row == 2) {
      rotateSide(2, false); rotateSide(3, true); rotateSide(2, true); moveSideUp(2, 2);
    }
    else if (side_row == 0) { // not possible
    }
  }

  if (getUndoGroup()) undo_->endGroup();

  ind = findPiece(2, 4); if (! ind.assertInd("top 4", 2, 1, 1)) return false;
  ind = findPiece(2, 1); if (! ind.assertInd("top 1", 2, 0, 1)) return false;
  ind = findPiece(2, 3); if (! ind.assertInd("top 3", 2, 1, 0)) return false;
  ind = findPiece(2, 5); if (! ind.assertInd("top 5", 2, 1, 2)) return false;
  ind = findPiece(2, 7); if (! ind.assertInd("top 7", 2, 2, 1)) return false;

  return true;
}

bool
CQRubik::
solveTopInd0()
{
  CRubikPieceInd ind = findPiece(2, 0);

  uint side_num = ind.side_num;
  uint side_col = ind.side_col;
  uint side_row = ind.side_row;

  if (side_num == 2 && side_col == 0 && side_row == 0) return true;

  if (getUndoGroup()) undo_->startGroup();

  if      (side_num == 0) {
    if (side_col == 2) {
      if      (side_row == 0) {
        moveSideLeft(0, 0); moveSideUp  (0, 0); moveSideRight(0, 0);
      }
      else if (side_row == 2) {
        moveSideLeft(0, 2); moveSideDown(0, 0); moveSideRight(0, 2);
      }

      side_col = 0;
    }

    if (side_col == 0) {
      if      (side_row == 0) {
        moveSideDown(0, 0); moveSideLeft(0, 0); moveSideUp(0, 0); moveSideRight(2, 0);
      }
      else if (side_row == 2) {
        moveSideLeft(1, 0); moveSideUp  (1, 0); moveSideRight2(1, 0); moveSideDown(1, 0);
      }
    }
  }
  else if (side_num == 1) {
    if (side_row == 2) {
      if      (side_col == 0) {
        moveSideUp(1, 0); moveSideLeft(1, 0); moveSideDown(1, 0);
      }
      else if (side_col == 2) {
        moveSideUp(1, 2); moveSideRight(1, 0); moveSideDown(1, 2);
      }

      side_row = 0;
    }

    if (side_row == 0) {
      if      (side_col == 0) {
        moveSideRight(1, 0); moveSideUp(1, 0); moveSideLeft(1, 0); moveSideDown(1, 0);
      }
      else if (side_col == 2) {
        moveSideUp(0, 0); moveSideLeft(0, 0);
        moveSideUp(0, 0); moveSideUp(0, 0); moveSideRight(0, 0);
      }
    }
  }
  else if (side_num == 2) {
    if      (side_col == 0 && side_row == 0) { // done
    }
    else if (side_col == 0 && side_row == 2) {
      moveSideLeft (0, 2); moveSideDown(0, 0);
      moveSideUp   (1, 0); moveSideRight(1, 0); moveSideRight(1, 0); moveSideDown(1, 0);
      moveSideRight(0, 2);
    }
    else if (side_col == 2 && side_row == 0) {
      moveSideUp(2, 2); rotateSide(2, true); moveSideDown(2, 0);
      moveSideRight2(1, 0); moveSideUp(2, 0);
      rotateSide(2, false); moveSideDown(2, 2);
    }
    else if (side_col == 2 && side_row == 2) {
      moveSideUp(2, 0); moveSideDown(2, 2); moveSideRight2(1, 0);
      moveSideDown(2, 0); moveSideUp(2, 2);
    }
  }
  else if (side_num == 3) {
    if (side_row == 0) {
      if      (side_col == 0) {
        moveSideDown(3, 0); moveSideLeft (3, 2); moveSideUp(3, 0);
      }
      else if (side_col == 2) {
        moveSideDown(3, 2); moveSideRight(3, 2); moveSideUp(3, 2);
      }

      side_row = 2;
    }

    if (side_row == 2) {
      if      (side_col == 0) {
        moveSideLeft(0, 0); moveSideUp(0, 0); moveSideRight(0, 0);
      }
      else if (side_col == 2) {
        moveSideUp  (1, 0); moveSideLeft2(1, 0); moveSideDown(1, 0);
      }
    }
  }
  else if (side_num == 4) {
    if (side_col == 0) {
      if      (side_row == 0) {
        moveSideRight(4, 0); moveSideUp  (4, 2); moveSideLeft(4, 0);
      }
      else if (side_row == 2) {
        moveSideRight(4, 2); moveSideDown(4, 2); moveSideLeft(4, 2);
      }

      side_col = 2;
    }

    if (side_col == 2) {
      if      (side_row == 0) {
        moveSideUp  (2, 0); moveSideLeft  (1, 0); moveSideDown (2, 0);
      }
      else if (side_row == 2) {
        moveSideLeft(2, 0); moveSideRight2(1, 0); moveSideRight(2, 0);
      }
    }
  }
  else if (side_num == 5) {
    // move to 5,0,0
    if      (side_col == 0 && side_row == 2) rotateSide(5, false);
    else if (side_col == 2 && side_row == 0) rotateSide(5, true);
    else if (side_col == 2 && side_row == 2) rotateSide2(5);

    moveSideRight(2, 0); moveSideDown(0, 0); moveSideLeft (2, 0);
    moveSideLeft (2, 0); moveSideUp  (0, 0); moveSideRight(2, 0);
  }

  if (getUndoGroup()) undo_->endGroup();

  ind = findPiece(2, 4); if (! ind.assertInd("top 4", 2, 1, 1)) return false;
  ind = findPiece(2, 1); if (! ind.assertInd("top 1", 2, 0, 1)) return false;
  ind = findPiece(2, 3); if (! ind.assertInd("top 3", 2, 1, 0)) return false;
  ind = findPiece(2, 5); if (! ind.assertInd("top 5", 2, 1, 2)) return false;
  ind = findPiece(2, 7); if (! ind.assertInd("top 7", 2, 2, 1)) return false;
  ind = findPiece(2, 0); if (! ind.assertInd("top 0", 2, 0, 0)) return false;

  return true;
}

bool
CQRubik::
solveTopInd2()
{
  CRubikPieceInd ind = findPiece(2, 2);

  uint side_num = ind.side_num;
  uint side_col = ind.side_col;
  uint side_row = ind.side_row;

  if (side_num == 2 && side_col == 0 && side_row == 2) return true;

  if (getUndoGroup()) undo_->startGroup();

  if      (side_num == 0) {
    if (side_col == 2) {
      if      (side_row == 0) { // not possible
        moveSideLeft(0, 0); moveSideUp  (0, 0); moveSideRight(0, 0);
      }
      else if (side_row == 2) {
        moveSideLeft(0, 2); moveSideDown(0, 0); moveSideRight(0, 2);
      }

      side_col = 0;
    }

    if (side_col == 0) {
      if      (side_row == 0) {
        moveSideLeft(3, 2); moveSideDown(3, 0); moveSideRight2(3, 2); moveSideUp   (3, 0);
      }
      else if (side_row == 2) {
        moveSideUp  (0, 0); moveSideLeft(0, 2); moveSideDown  (0, 0); moveSideRight(0, 2);
      }
    }
  }
  else if (side_num == 1) {
    if (side_row == 2) {
      if      (side_col == 0) { // not possible
      }
      else if (side_col == 2) {
        moveSideUp(1, 2); moveSideRight(1, 0); moveSideDown(1, 2);
      }

      side_row = 0;
    }

    if (side_row == 0) {
      if      (side_col == 0) {
        moveSideLeft(0, 2); moveSideDown(0, 0); moveSideRight(0, 2);
      }
      else if (side_col == 2) {
        moveSideDown(3, 0); moveSideLeft2(3, 2); moveSideUp(3, 0);
      }
    }
  }
  else if (side_num == 2) {
    if      (side_col == 0 && side_row == 0) { // not possible
    }
    else if (side_col == 0 && side_row == 2) { // done
    }
    else if (side_col == 2 && side_row == 0) {
      moveSideRight(2, 0); moveSideLeft(2, 2);
      moveSideUp2(0, 0);
      moveSideRight(2, 2); moveSideLeft(2, 0);
    }
    else if (side_col == 2 && side_row == 2) {
      moveSideDown(3, 2); moveSideRight(3, 2);
      moveSideLeft(0, 2); moveSideDown2(0, 0); moveSideRight(0, 2); moveSideUp(3, 2);
    }
  }
  else if (side_num == 3) {
    if (side_row == 0) {
      if      (side_col == 0) {
        moveSideDown(3, 0); moveSideLeft (3, 2); moveSideUp(3, 0);
      }
      else if (side_col == 2) {
        moveSideDown(3, 2); moveSideRight(3, 2); moveSideUp(3, 2);
      }

      side_row = 2;
    }

    if (side_row == 2) {
      if      (side_col == 0) {
        moveSideRight(3, 2); moveSideDown(3, 0); moveSideLeft (3, 2); moveSideUp   (3, 0);
      }
      else if (side_col == 2) {
        moveSideDown (0, 0); moveSideLeft(0, 2); moveSideDown2(0, 0); moveSideRight(0, 2);
      }
    }
  }
  else if (side_num == 4) {
    if (side_col == 0) {
      if      (side_row == 0) {
        moveSideRight(4, 0); moveSideUp  (4, 2); moveSideLeft(4, 0);
      }
      else if (side_row == 2) {
        moveSideRight(4, 2); moveSideDown(4, 2); moveSideLeft(4, 2);
      }

      side_col = 2;
    }

    if (side_col == 2) {
      if      (side_row == 0) {
        moveSideLeft(0, 2); moveSideDown2(0, 0); moveSideRight(0, 2);
      }
      else if (side_row == 2) {
        moveSideDown(3, 0); moveSideLeft (3, 2); moveSideUp   (3, 0);
      }
    }
  }
  else if (side_num == 5) {
    // move to 5,0,2
    if      (side_col == 0 && side_row == 0) rotateSide(5, true);
    else if (side_col == 2 && side_row == 0) rotateSide2(5);
    else if (side_col == 2 && side_row == 2) rotateSide(5, false);

    moveSideRight(2, 2); moveSideUp  (0, 0); moveSideLeft (2, 2);
    moveSideLeft (2, 2); moveSideDown(0, 0); moveSideRight(2, 2);
  }

  if (getUndoGroup()) undo_->endGroup();

  ind = findPiece(2, 4); if (! ind.assertInd("top 4", 2, 1, 1)) return false;
  ind = findPiece(2, 1); if (! ind.assertInd("top 1", 2, 0, 1)) return false;
  ind = findPiece(2, 3); if (! ind.assertInd("top 3", 2, 1, 0)) return false;
  ind = findPiece(2, 5); if (! ind.assertInd("top 5", 2, 1, 2)) return false;
  ind = findPiece(2, 7); if (! ind.assertInd("top 7", 2, 2, 1)) return false;
  ind = findPiece(2, 0); if (! ind.assertInd("top 0", 2, 0, 0)) return false;
  ind = findPiece(2, 2); if (! ind.assertInd("top 2", 2, 0, 2)) return false;

  return true;
}

bool
CQRubik::
solveTopInd6()
{
  CRubikPieceInd ind = findPiece(2, 6);

  uint side_num = ind.side_num;
  uint side_col = ind.side_col;
  uint side_row = ind.side_row;

  if (side_num == 2 && side_col == 2 && side_row == 0) return true;

  if (getUndoGroup()) undo_->startGroup();

  if      (side_num == 0) {
    if (side_col == 2) {
      if      (side_row == 0) { // not possible
      }
      else if (side_row == 2) { // not possible
      }

      side_col = 0;
    }

    if (side_col == 0) {
      if      (side_row == 0) {
        moveSideUp(1, 2); moveSideRight(1, 0); moveSideDown(1, 2);
      }
      else if (side_row == 2) {
        moveSideRight(4, 0); moveSideDown2(4, 2); moveSideLeft(4, 0);
      }
    }
  }
  else if (side_num == 1) {
    if (side_row == 2) {
      if      (side_col == 0) { // not possible
      }
      else if (side_col == 2) {
        moveSideUp(1, 2); moveSideRight(1, 0); moveSideDown(1, 2);
      }

      side_row = 0;
    }

    if (side_row == 0) {
      if      (side_col == 0) {
        moveSideUp  (4, 2); moveSideRight(4, 0); moveSideDown2(4, 2); moveSideLeft(4, 0);
      }
      else if (side_col == 2) {
        moveSideLeft(1, 0); moveSideUp   (1, 2); moveSideRight(1, 0); moveSideDown(1, 2);
      }
    }
  }
  else if (side_num == 2) {
    if      (side_col == 0 && side_row == 0) { // not possible
    }
    else if (side_col == 0 && side_row == 2) { // done
    }
    else if (side_col == 2 && side_row == 0) { // not possible
    }
    else if (side_col == 2 && side_row == 2) {
      moveSideRight(4, 2); moveSideDown  (4, 2);
      moveSideUp   (1, 2); moveSideRight2(1, 0); moveSideDown(1, 2);
      moveSideLeft (4, 2);
    }
  }
  else if (side_num == 3) {
    if (side_row == 0) {
      if      (side_col == 0) { // not possible
      }
      else if (side_col == 2) {
        moveSideDown(3, 2); moveSideRight(3, 2); moveSideUp(3, 2);
      }

      side_row = 2;
    }

    if (side_row == 2) {
      if      (side_col == 0) {
        moveSideUp(1, 2); moveSideRight2(1, 0); moveSideDown(1, 2);
      }
      else if (side_col == 2) {
        moveSideRight(4, 0); moveSideUp(4, 2); moveSideLeft(4, 0);
      }
    }
  }
  else if (side_num == 4) {
    if (side_col == 0) {
      if      (side_row == 0) {
        moveSideRight(4, 0); moveSideUp  (4, 2); moveSideLeft(4, 0);
      }
      else if (side_row == 2) {
        moveSideRight(4, 2); moveSideDown(4, 2); moveSideLeft(4, 2);
      }

      side_col = 2;
    }

    if (side_col == 2) {
      if      (side_row == 0) {
        moveSideDown(4, 2); moveSideRight(4, 0); moveSideUp(4, 2); moveSideLeft(4, 0);
      }
      else if (side_row == 2) {
        moveSideRight(1, 0); moveSideUp(1, 2); moveSideLeft2(1, 0); moveSideDown(1, 2);
      }
    }
  }
  else if (side_num == 5) {
    // move to 5,0,0
    if      (side_col == 0 && side_row == 2) rotateSide(5, false);
    else if (side_col == 2 && side_row == 0) rotateSide(5, true);
    else if (side_col == 2 && side_row == 2) rotateSide2(5);

    moveSideRight(4, 0); moveSideUp    (4, 2); moveSideLeft(4, 0);
    moveSideUp   (1, 2); moveSideRight2(1, 0); moveSideDown(1, 2);
  }

  if (getUndoGroup()) undo_->endGroup();

  ind = findPiece(2, 4); if (! ind.assertInd("top 4", 2, 1, 1)) return false;
  ind = findPiece(2, 1); if (! ind.assertInd("top 1", 2, 0, 1)) return false;
  ind = findPiece(2, 3); if (! ind.assertInd("top 3", 2, 1, 0)) return false;
  ind = findPiece(2, 5); if (! ind.assertInd("top 5", 2, 1, 2)) return false;
  ind = findPiece(2, 7); if (! ind.assertInd("top 7", 2, 2, 1)) return false;
  ind = findPiece(2, 0); if (! ind.assertInd("top 0", 2, 0, 0)) return false;
  ind = findPiece(2, 2); if (! ind.assertInd("top 2", 2, 0, 2)) return false;
  ind = findPiece(2, 6); if (! ind.assertInd("top 6", 2, 2, 0)) return false;

  return true;
}

bool
CQRubik::
solveTopInd8()
{
  CRubikPieceInd ind = findPiece(2, 8);

  uint side_num = ind.side_num;
  uint side_col = ind.side_col;
  uint side_row = ind.side_row;

  if (side_num == 2 && side_col == 2 && side_row == 2) return true;

  if (getUndoGroup()) undo_->startGroup();

  if      (side_num == 0) {
    if (side_col == 2) {
      if      (side_row == 0) { // not possible
      }
      else if (side_row == 2) { // not possible
      }

      side_col = 0;
    }

    if (side_col == 0) {
      if      (side_row == 0) {
        moveSideRight(4, 2); moveSideDown2(4, 2); moveSideLeft(4, 2);
      }
      else if (side_row == 2) {
        moveSideDown(3, 2); moveSideRight(3, 2); moveSideUp(3, 2);
      }
    }
  }
  else if (side_num == 1) {
    if (side_row == 2) {
      if      (side_col == 0) { // not possible
      }
      else if (side_col == 2) { // not possible
      }

      side_row = 0;
    }

    if (side_row == 0) {
      if      (side_col == 0) {
        moveSideDown(3, 2); moveSideLeft2(3, 2); moveSideUp(3, 2);
      }
      else if (side_col == 2) {
        moveSideRight(4, 2); moveSideDown(4, 2); moveSideLeft(4, 2);
      }
    }
  }
  else if (side_num == 2) {
    if      (side_col == 0 && side_row == 0) { // not possible
    }
    else if (side_col == 0 && side_row == 2) { // not possible
    }
    else if (side_col == 2 && side_row == 0) { // not possible
    }
    else if (side_col == 2 && side_row == 2) { // done
    }
  }
  else if (side_num == 3) {
    if (side_row == 0) {
      if      (side_col == 0) { // not possible
      }
      else if (side_col == 2) {
        moveSideDown(3, 2); moveSideRight(3, 2); moveSideUp(3, 2);
      }

      side_row = 2;
    }

    if (side_row == 2) {
      if      (side_col == 0) {
        moveSideDown(4, 2); moveSideRight(4, 2); moveSideUp2(4, 2); moveSideLeft(4, 2);
      }
      else if (side_col == 2) {
        moveSideLeft(3, 2); moveSideDown(3, 2); moveSideRight(3, 2); moveSideUp(3, 2);
      }
    }
  }
  else if (side_num == 4) {
    if (side_col == 0) {
      if      (side_row == 0) { // not possible
      }
      else if (side_row == 2) {
        moveSideRight(4, 2); moveSideDown(4, 2); moveSideLeft(4, 2);
      }

      side_col = 2;
    }

    if (side_col == 2) {
      if      (side_row == 0) {
        moveSideRight(3, 2); moveSideDown(3, 2); moveSideLeft2(3, 2); moveSideUp(3, 2);
      }
      else if (side_row == 2) {
        moveSideUp(4, 2); moveSideRight(4, 2); moveSideDown(4, 2); moveSideLeft(4, 2);
      }
    }
  }
  else if (side_num == 5) {
    // move to 5,0,2
    if      (side_col == 0 && side_row == 0) rotateSide(5, true);
    else if (side_col == 2 && side_row == 0) rotateSide2(5);
    else if (side_col == 2 && side_row == 2) rotateSide(5, false);

    moveSideRight(4, 2); moveSideDown  (4, 2); moveSideLeft(4, 2);
    moveSideDown (3, 2); moveSideRight2(3, 2); moveSideUp  (3, 2);
  }

  if (getUndoGroup()) undo_->endGroup();

  ind = findPiece(2, 4); if (! ind.assertInd("top 4", 2, 1, 1)) return false;
  ind = findPiece(2, 1); if (! ind.assertInd("top 1", 2, 0, 1)) return false;
  ind = findPiece(2, 3); if (! ind.assertInd("top 3", 2, 1, 0)) return false;
  ind = findPiece(2, 5); if (! ind.assertInd("top 5", 2, 1, 2)) return false;
  ind = findPiece(2, 7); if (! ind.assertInd("top 7", 2, 2, 1)) return false;
  ind = findPiece(2, 0); if (! ind.assertInd("top 0", 2, 0, 0)) return false;
  ind = findPiece(2, 2); if (! ind.assertInd("top 2", 2, 0, 2)) return false;
  ind = findPiece(2, 6); if (! ind.assertInd("top 6", 2, 2, 0)) return false;
  ind = findPiece(2, 8); if (! ind.assertInd("top 8", 2, 2, 2)) return false;

  return true;
}

bool
CQRubik::
solveMidInd4()
{
  CRubikPieceInd ind = findPiece(0, 4);

  uint side_num = ind.side_num;

  if (side_num == 0) return true;

  if (getUndoGroup()) undo_->startGroup();

  if      (side_num == 1) moveSideDown(0, 1);
  else if (side_num == 3) moveSideUp  (0, 1);
  else if (side_num == 4) moveSideUp2 (0, 1);

  if (getUndoGroup()) undo_->endGroup();

  ind = findPiece(2, 4); if (! ind.assertInd("top 4", 2, 1, 1)) return false;
  ind = findPiece(2, 1); if (! ind.assertInd("top 1", 2, 0, 1)) return false;
  ind = findPiece(2, 3); if (! ind.assertInd("top 3", 2, 1, 0)) return false;
  ind = findPiece(2, 5); if (! ind.assertInd("top 5", 2, 1, 2)) return false;
  ind = findPiece(2, 7); if (! ind.assertInd("top 7", 2, 2, 1)) return false;
  ind = findPiece(2, 0); if (! ind.assertInd("top 0", 2, 0, 0)) return false;
  ind = findPiece(2, 2); if (! ind.assertInd("top 2", 2, 0, 2)) return false;
  ind = findPiece(2, 6); if (! ind.assertInd("top 6", 2, 2, 0)) return false;
  ind = findPiece(2, 8); if (! ind.assertInd("top 8", 2, 2, 2)) return false;
  ind = findPiece(0, 4); if (! ind.assertInd("mid 4", 0, 1, 1)) return false;

  return true;
}

bool
CQRubik::
solveMidLeftInd3()
{
  CRubikPieceInd ind = findPiece(0, 3);

  uint side_num = ind.side_num;
  uint side_col = ind.side_col;
  uint side_row = ind.side_row;

  if (side_num == 0 && side_col == 1 && side_row == 0) return true;

  if (getUndoGroup()) undo_->startGroup();

  if      (side_num == 1) {
    if      (side_col == 0) {
      moveSideDown (0, 0); moveSideLeft(0, 0); moveSideUp  (0, 0); moveSideRight(0, 0);
      moveSideRight(1, 0); moveSideUp  (1, 0); moveSideLeft(1, 0); moveSideDown (1, 0);

      moveSideDown2(0, 0);

      side_num = 0; side_col = 0; side_row = 1;
    }
    else if (side_col == 2) {
      moveSideDown(4, 2); moveSideRight(4, 0); moveSideUp   (4, 2); moveSideLeft(4, 0);
      moveSideLeft(1, 0); moveSideUp   (1, 2); moveSideRight(1, 0); moveSideDown(1, 2);

      side_num = 0; side_col = 0; side_row = 1;
    }
    else if (side_row == 0) {
      moveSideDown(0, 0);

      side_num = 0; side_col = 0; side_row = 1;
    }
    else if (side_row == 2) { // not possible
    }
  }
  else if (side_num == 3) {
    if      (side_col == 0) {
      moveSideUp   (0, 0); moveSideLeft(0, 2); moveSideDown(0, 0); moveSideRight(0, 2);
      moveSideRight(3, 2); moveSideDown(3, 0); moveSideLeft(3, 2); moveSideUp   (3, 0);
      moveSideUp2  (0, 0);

      side_num = 0; side_col = 0; side_row = 1;
    }
    else if (side_col == 2) {
      moveSideUp  (4, 2); moveSideRight(4, 2); moveSideDown (4, 2); moveSideLeft(4, 2);
      moveSideLeft(3, 2); moveSideDown (3, 2); moveSideRight(3, 2); moveSideUp  (3, 2);

      side_num = 0; side_col = 0; side_row = 1;
    }
    else if (side_row == 0) { // not possible
    }
    else if (side_row == 2) {
      moveSideUp(0, 0);

      side_num = 0; side_col = 0; side_row = 1;
    }
  }
  else if (side_num == 4) {
    if      (side_col == 0) { // not possible
    }
    else if (side_col == 2) {
      moveSideDown2(0, 0);

      side_num = 0; side_col = 0; side_row = 1;
    }
    else if (side_row == 0) {
      moveSideLeft(1, 0); moveSideUp   (1, 2); moveSideRight(1, 0); moveSideDown(1, 2);
      moveSideDown(4, 2); moveSideRight(4, 0); moveSideUp   (4, 2); moveSideLeft(4, 0);

      moveSideUp(0, 0);

      side_num = 0; side_col = 0; side_row = 1;
    }
    else if (side_row == 2) {
      moveSideLeft(3, 2); moveSideDown (3, 2); moveSideRight(3, 2); moveSideUp  (3, 2);
      moveSideUp  (4, 2); moveSideRight(4, 2); moveSideDown (4, 2); moveSideLeft(4, 2);

      moveSideDown(0, 0);

      side_num = 0; side_col = 0; side_row = 1;
    }
  }

  if      (side_num == 0) {
    if (side_row == 2) {
      moveSideRight(3, 2); moveSideDown(3, 0); moveSideLeft(3, 2); moveSideUp   (3, 0);
      moveSideUp   (0, 0); moveSideLeft(0, 2); moveSideDown(0, 0); moveSideRight(0, 2);

      moveSideDown(0, 0);

      side_col = 0; side_row = 1;
    }

    if      (side_col == 0) {
      moveSideDown (0, 0); moveSideLeft(0, 0); moveSideUp  (0, 0); moveSideRight(0, 0);
      moveSideRight(1, 0); moveSideUp  (1, 0); moveSideLeft(1, 0); moveSideDown (1, 0);
    }
    else if (side_col == 2) { // not possible
    }
    else if (side_row == 0) { // done
    }
  }
  else if (side_num == 5) {
    // move to side_row 0
    if      (side_col == 0) {
      rotateSide(5, false);
    }
    else if (side_col == 2) {
      rotateSide(5, true);
    }
    else if (side_row == 2) {
      rotateSide2(5);
    }

    moveSideRight(1, 0); moveSideUp  (1, 0); moveSideLeft(1, 0); moveSideDown (1, 0);
    moveSideDown (0, 0); moveSideLeft(0, 0); moveSideUp  (0, 0); moveSideRight(0, 0);
  }

  if (getUndoGroup()) undo_->endGroup();

  ind = findPiece(2, 4); if (! ind.assertInd("top 4", 2, 1, 1)) return false;
  ind = findPiece(2, 1); if (! ind.assertInd("top 1", 2, 0, 1)) return false;
  ind = findPiece(2, 3); if (! ind.assertInd("top 3", 2, 1, 0)) return false;
  ind = findPiece(2, 5); if (! ind.assertInd("top 5", 2, 1, 2)) return false;
  ind = findPiece(2, 7); if (! ind.assertInd("top 7", 2, 2, 1)) return false;
  ind = findPiece(2, 0); if (! ind.assertInd("top 0", 2, 0, 0)) return false;
  ind = findPiece(2, 2); if (! ind.assertInd("top 2", 2, 0, 2)) return false;
  ind = findPiece(2, 6); if (! ind.assertInd("top 6", 2, 2, 0)) return false;
  ind = findPiece(2, 8); if (! ind.assertInd("top 8", 2, 2, 2)) return false;
  ind = findPiece(0, 4); if (! ind.assertInd("mid 4", 0, 1, 1)) return false;

  ind = findPiece(0, 3); if (! ind.assertInd("mid left 3", 0, 1, 0)) return false;

  return true;
}

bool
CQRubik::
solveMidLeftInd5()
{
  CRubikPieceInd ind = findPiece(0, 5);

  uint side_num = ind.side_num;
  uint side_col = ind.side_col;
  uint side_row = ind.side_row;

  if (side_num == 0 && side_col == 1 && side_row == 2) return true;

  if (getUndoGroup()) undo_->startGroup();

  if      (side_num == 1) {
    if      (side_col == 0) {
      moveSideDown (0, 0); moveSideLeft(0, 0); moveSideUp  (0, 0); moveSideRight(0, 0);
      moveSideRight(1, 0); moveSideUp  (1, 0); moveSideLeft(1, 0); moveSideDown (1, 0);

      moveSideDown2(0, 0);

      side_num = 0; side_col = 0; side_row = 1;
    }
    else if (side_col == 2) {
      moveSideDown(4, 2); moveSideRight(4, 0); moveSideUp   (4, 2); moveSideLeft(4, 0);
      moveSideLeft(1, 0); moveSideUp   (1, 2); moveSideRight(1, 0); moveSideDown(1, 2);

      side_num = 0; side_col = 0; side_row = 1;
    }
    else if (side_row == 0) {
      moveSideDown(0, 0);

      side_num = 0; side_col = 0; side_row = 1;
    }
    else if (side_row == 2) { // not possible
    }
  }
  else if (side_num == 3) {
    if      (side_col == 0) {
      moveSideUp   (0, 0); moveSideLeft(0, 2); moveSideDown(0, 0); moveSideRight(0, 2);
      moveSideRight(3, 2); moveSideDown(3, 0); moveSideLeft(3, 2); moveSideUp   (3, 0);
      moveSideUp2  (0, 0);

      side_num = 0; side_col = 0; side_row = 1;
    }
    else if (side_col == 2) {
      moveSideUp  (4, 2); moveSideRight(4, 2); moveSideDown (4, 2); moveSideLeft(4, 2);
      moveSideLeft(3, 2); moveSideDown (3, 2); moveSideRight(3, 2); moveSideUp  (3, 2);

      side_num = 0; side_col = 0; side_row = 1;
    }
    else if (side_row == 0) { // not possible
    }
    else if (side_row == 2) {
      moveSideUp(0, 0);

      side_num = 0; side_col = 0; side_row = 1;
    }
  }
  else if (side_num == 4) {
    if      (side_col == 0) { // not possible
    }
    else if (side_col == 2) {
      moveSideUp2(0, 0);

      side_num = 0; side_col = 0; side_row = 1;
    }
    else if (side_row == 0) {
      moveSideLeft(1, 0); moveSideUp   (1, 2); moveSideRight(1, 0); moveSideDown(1, 2);
      moveSideDown(4, 2); moveSideRight(4, 0); moveSideUp   (4, 2); moveSideLeft(4, 0);

      moveSideUp(0, 0);

      side_num = 0; side_col = 0; side_row = 1;
    }
    else if (side_row == 2) {
      moveSideLeft(3, 2); moveSideDown (3, 2); moveSideRight(3, 2); moveSideUp  (3, 2);
      moveSideUp  (4, 2); moveSideRight(4, 2); moveSideDown (4, 2); moveSideLeft(4, 2);

      moveSideDown(0, 0);

      side_num = 0; side_col = 0; side_row = 1;
    }
  }

  if      (side_num == 0) {
    if (side_row == 0) {
    }

    if      (side_col == 0) {
      moveSideUp   (0, 0); moveSideLeft(0, 2); moveSideDown(0, 0); moveSideRight(0, 2);
      moveSideRight(3, 2); moveSideDown(3, 0); moveSideLeft(3, 2); moveSideUp   (3, 0);
    }
    else if (side_col == 2) { // not possible
    }
    else if (side_row == 2) { // done
    }
  }
  else if (side_num == 5) {
    // move to side_row 2
    if      (side_col == 0) {
      rotateSide(5, true);
    }
    else if (side_col == 2) {
      rotateSide(5, false);
    }
    else if (side_row == 0) {
      rotateSide2(5);
    }

    moveSideRight(3, 2); moveSideDown(3, 0); moveSideLeft(3, 2); moveSideUp   (3, 0);
    moveSideUp   (0, 0); moveSideLeft(0, 2); moveSideDown(0, 0); moveSideRight(0, 2);
  }

  if (getUndoGroup()) undo_->endGroup();

  ind = findPiece(2, 4); if (! ind.assertInd("top 4", 2, 1, 1)) return false;
  ind = findPiece(2, 1); if (! ind.assertInd("top 1", 2, 0, 1)) return false;
  ind = findPiece(2, 3); if (! ind.assertInd("top 3", 2, 1, 0)) return false;
  ind = findPiece(2, 5); if (! ind.assertInd("top 5", 2, 1, 2)) return false;
  ind = findPiece(2, 7); if (! ind.assertInd("top 7", 2, 2, 1)) return false;
  ind = findPiece(2, 0); if (! ind.assertInd("top 0", 2, 0, 0)) return false;
  ind = findPiece(2, 2); if (! ind.assertInd("top 2", 2, 0, 2)) return false;
  ind = findPiece(2, 6); if (! ind.assertInd("top 6", 2, 2, 0)) return false;
  ind = findPiece(2, 8); if (! ind.assertInd("top 8", 2, 2, 2)) return false;
  ind = findPiece(0, 4); if (! ind.assertInd("mid 4", 0, 1, 1)) return false;

  ind = findPiece(0, 3); if (! ind.assertInd("mid left 3", 0, 1, 0)) return false;
  ind = findPiece(0, 5); if (! ind.assertInd("mid left 5", 0, 1, 2)) return false;

  return true;
}

bool
CQRubik::
solveMidRightInd3()
{
  CRubikPieceInd ind = findPiece(4, 3);

  uint side_num = ind.side_num;
  uint side_col = ind.side_col;
  uint side_row = ind.side_row;

  if (side_num == 4 && side_col == 1 && side_row == 0) return true;

  if (getUndoGroup()) undo_->startGroup();

  if      (side_num == 0) {
    if      (side_col == 0) {
      moveSideUp2(0, 0);

      side_num = 4; side_col = 2; side_row = 1;
    }
    else if (side_col == 2) { // not possible
    }
    else if (side_row == 0) { // not possible
    }
    else if (side_row == 2) { // not possible
    }
  }
  else if (side_num == 1) {
    if      (side_col == 0) { // not possible
    }
    else if (side_col == 2) {
      moveSideDown(4, 2); moveSideRight(4, 0); moveSideUp   (4, 2); moveSideLeft(4, 0);
      moveSideLeft(1, 0); moveSideUp   (1, 2); moveSideRight(1, 0); moveSideDown(1, 2);

      moveSideUp2(0, 0);

      side_num = 4; side_col = 2; side_row = 1;
    }
    else if (side_row == 0) {
      moveSideUp(0, 0);

      side_num = 4; side_col = 2; side_row = 1;
    }
    else if (side_row == 2) { // not possible
    }
  }
  else if (side_num == 3) {
    if      (side_col == 0) { // not possible
    }
    else if (side_col == 2) {
      moveSideUp  (4, 2); moveSideRight(4, 2); moveSideDown (4, 2); moveSideLeft(4, 2);
      moveSideLeft(3, 2); moveSideDown (3, 2); moveSideRight(3, 2); moveSideUp  (3, 2);

      moveSideUp2(0, 0);

      side_num = 4; side_col = 2; side_row = 1;
    }
    else if (side_row == 0) { // not possible
    }
    else if (side_row == 2) {
      moveSideDown(0, 0);

      side_num = 4; side_col = 2; side_row = 1;
    }
  }

  if      (side_num == 4) {
    if      (side_col == 0) { // not possible
    }
    else if (side_row == 0) { // done
    }
    else if (side_row == 2) {
      moveSideLeft(3, 2); moveSideDown (3, 2); moveSideRight(3, 2); moveSideUp  (3, 2);
      moveSideUp  (4, 2); moveSideRight(4, 2); moveSideDown (4, 2); moveSideLeft(4, 2);

      moveSideUp(0, 0);

      side_num = 4; side_col = 2; side_row = 1;
    }

    if (side_col == 2) {
      moveSideDown(4, 2); moveSideRight(4, 0); moveSideUp   (4, 2); moveSideLeft(4, 0);
      moveSideLeft(1, 0); moveSideUp   (1, 2); moveSideRight(1, 0); moveSideDown(1, 2);
    }
  }
  else if (side_num == 5) {
    // move to side_row 0
    if      (side_col == 0) {
      rotateSide(5, false);
    }
    else if (side_col == 2) {
      rotateSide(5, true);
    }
    else if (side_row == 2) {
      rotateSide2(5);
    }

    moveSideLeft(1, 0); moveSideUp   (1, 2); moveSideRight(1, 0); moveSideDown(1, 2);
    moveSideDown(4, 2); moveSideRight(4, 0); moveSideUp   (4, 2); moveSideLeft(4, 0);
  }

  if (getUndoGroup()) undo_->endGroup();

  ind = findPiece(2, 4); if (! ind.assertInd("top 4", 2, 1, 1)) return false;
  ind = findPiece(2, 1); if (! ind.assertInd("top 1", 2, 0, 1)) return false;
  ind = findPiece(2, 3); if (! ind.assertInd("top 3", 2, 1, 0)) return false;
  ind = findPiece(2, 5); if (! ind.assertInd("top 5", 2, 1, 2)) return false;
  ind = findPiece(2, 7); if (! ind.assertInd("top 7", 2, 2, 1)) return false;
  ind = findPiece(2, 0); if (! ind.assertInd("top 0", 2, 0, 0)) return false;
  ind = findPiece(2, 2); if (! ind.assertInd("top 2", 2, 0, 2)) return false;
  ind = findPiece(2, 6); if (! ind.assertInd("top 6", 2, 2, 0)) return false;
  ind = findPiece(2, 8); if (! ind.assertInd("top 8", 2, 2, 2)) return false;
  ind = findPiece(0, 4); if (! ind.assertInd("mid 4", 0, 1, 1)) return false;

  ind = findPiece(0, 3); if (! ind.assertInd("mid left 3", 0, 1, 0)) return false;
  ind = findPiece(0, 5); if (! ind.assertInd("mid left 5", 0, 1, 2)) return false;

  ind = findPiece(4, 3); if (! ind.assertInd("mid right 3", 4, 1, 0)) return false;

  return true;
}

bool
CQRubik::
solveMidRightInd5()
{
  CRubikPieceInd ind = findPiece(4, 5);

  uint side_num = ind.side_num;
  uint side_col = ind.side_col;
  uint side_row = ind.side_row;

  if (side_num == 4 && side_col == 1 && side_row == 2) return true;

  if (getUndoGroup()) undo_->startGroup();

  if      (side_num == 0) {
    if      (side_col == 0) {
      moveSideUp2(0, 0);

      side_num = 4; side_col = 2; side_row = 1;
    }
    else if (side_col == 2) { // not possible
    }
    else if (side_row == 0) { // not possible
    }
    else if (side_row == 2) { // not possible
    }
  }
  else if (side_num == 1) {
    if      (side_col == 0) { // not possible
    }
    else if (side_col == 2) { // not possible
    }
    else if (side_row == 0) {
      moveSideUp(0, 0);

      side_num = 4; side_col = 2; side_row = 1;
    }
    else if (side_row == 2) { // not possible
    }
  }
  else if (side_num == 3) {
    if      (side_col == 0) { // not possible
    }
    else if (side_col == 2) {
      moveSideUp  (4, 2); moveSideRight(4, 2); moveSideDown (4, 2); moveSideLeft(4, 2);
      moveSideLeft(3, 2); moveSideDown (3, 2); moveSideRight(3, 2); moveSideUp  (3, 2);

      moveSideUp2(0, 0);

      side_num = 4; side_col = 2; side_row = 1;
    }
    else if (side_row == 0) { // not possible
    }
    else if (side_row == 2) {
      moveSideDown(0, 0);

      side_num = 4; side_col = 2; side_row = 1;
    }
  }

  if      (side_num == 4) {
    if      (side_col == 0) { // not possible
    }
    else if (side_row == 0) { // not possible
    }
    else if (side_row == 2) { // done
    }

    if (side_col == 2) {
      moveSideUp  (4, 2); moveSideRight(4, 2); moveSideDown (4, 2); moveSideLeft(4, 2);
      moveSideLeft(3, 2); moveSideDown (3, 2); moveSideRight(3, 2); moveSideUp  (3, 2);
    }
  }
  else if (side_num == 5) {
    // move to side_row 2
    if      (side_col == 0) {
      rotateSide(5, true);
    }
    else if (side_col == 2) {
      rotateSide(5, false);
    }
    else if (side_row == 0) {
      rotateSide2(5);
    }

    moveSideLeft(3, 2); moveSideDown (3, 2); moveSideRight(3, 2); moveSideUp  (3, 2);
    moveSideUp  (4, 2); moveSideRight(4, 2); moveSideDown (4, 2); moveSideLeft(4, 2);
  }

  if (getUndoGroup()) undo_->endGroup();

  ind = findPiece(2, 4); if (! ind.assertInd("top 4", 2, 1, 1)) return false;
  ind = findPiece(2, 1); if (! ind.assertInd("top 1", 2, 0, 1)) return false;
  ind = findPiece(2, 3); if (! ind.assertInd("top 3", 2, 1, 0)) return false;
  ind = findPiece(2, 5); if (! ind.assertInd("top 5", 2, 1, 2)) return false;
  ind = findPiece(2, 7); if (! ind.assertInd("top 7", 2, 2, 1)) return false;
  ind = findPiece(2, 0); if (! ind.assertInd("top 0", 2, 0, 0)) return false;
  ind = findPiece(2, 2); if (! ind.assertInd("top 2", 2, 0, 2)) return false;
  ind = findPiece(2, 6); if (! ind.assertInd("top 6", 2, 2, 0)) return false;
  ind = findPiece(2, 8); if (! ind.assertInd("top 8", 2, 2, 2)) return false;
  ind = findPiece(0, 4); if (! ind.assertInd("mid 4", 0, 1, 1)) return false;

  ind = findPiece(0, 3); if (! ind.assertInd("mid left 3", 0, 1, 0)) return false;
  ind = findPiece(0, 5); if (! ind.assertInd("mid left 5", 0, 1, 2)) return false;

  ind = findPiece(4, 3); if (! ind.assertInd("mid right 3", 4, 1, 0)) return false;
  ind = findPiece(4, 5); if (! ind.assertInd("mid right 5", 4, 1, 2)) return false;

  return true;
}

bool
CQRubik::
solveBottomCross()
{
  CRubikSide &side = sides_[2];

  CRubikPiece &piece1 = side.pieces[0][1];
  CRubikPiece &piece3 = side.pieces[1][0];
  CRubikPiece &piece5 = side.pieces[1][2];
  CRubikPiece &piece7 = side.pieces[2][1];

  if (piece1.side == 5 && piece3.side == 5 &&
      piece5.side == 5 && piece7.side == 5) {
    return true;
  }

  if (getUndoGroup()) undo_->startGroup();

  if      ((piece1.side != 5 && piece7.side != 5 &&
            piece3.side == 5 && piece5.side == 5) ||
           (piece1.side == 5 && piece7.side == 5 &&
            piece3.side != 5 && piece5.side != 5)) {
    if (piece1.side != 5)
      rotateSide(2, false);

    rotateSide(3, false); rotateSide(4, false); rotateSide(2, false);
    rotateSide(4, true ); rotateSide(2, true ); rotateSide(3, true );
  }
  else if (piece1.side != 5 && piece3.side != 5 &&
           piece5.side != 5 && piece7.side != 5) {
    rotateSide(3, false); rotateSide(4, false); rotateSide(2, false);
    rotateSide(4, true ); rotateSide(2, true ); rotateSide(3, true );

    CRubikPiece &piece1 = side.pieces[0][1];
    CRubikPiece &piece3 = side.pieces[1][0];
    CRubikPiece &piece5 = side.pieces[1][2];
    CRubikPiece &piece7 = side.pieces[2][1];

    if      ((piece1.side != 5 && piece7.side != 5 &&
             piece3.side == 5 && piece5.side == 5) ||
            (piece1.side == 5 && piece7.side == 5 &&
             piece3.side != 5 && piece5.side != 5)) {
      // State 3

      if (piece1.side != 5)
        rotateSide(2, false);

      rotateSide(3, false); rotateSide(4, false); rotateSide(2, false);
      rotateSide(4, true ); rotateSide(2, true ); rotateSide(3, true );
    }
    else {
      // State 2
      if      (piece1.side != 5 && piece3.side != 5)
        rotateSide2(2);
      else if (piece1.side == 5 && piece3.side != 5)
        rotateSide(2, false);
      else if (piece1.side != 5 && piece3.side == 5)
        rotateSide(2, true);

      rotateSide(3, false); rotateSide(2, false); rotateSide(4, false);
      rotateSide(2, true ); rotateSide(4, true ); rotateSide(3, true );
    }
  }
  else {
    if      (piece1.side != 5 && piece3.side != 5)
      rotateSide2(2);
    else if (piece1.side == 5 && piece3.side != 5)
      rotateSide(2, false);
    else if (piece1.side != 5 && piece3.side == 5)
      rotateSide(2, true);

    rotateSide(3, false); rotateSide(2, false); rotateSide(4, false);
    rotateSide(2, true ); rotateSide(4, true ); rotateSide(3, true );
  }

  if (getUndoGroup()) undo_->endGroup();

  {
    CRubikPiece &piece1 = side.pieces[0][1];
    CRubikPiece &piece3 = side.pieces[1][0];
    CRubikPiece &piece5 = side.pieces[1][2];
    CRubikPiece &piece7 = side.pieces[2][1];

    if (piece1.side != 5 || piece3.side != 5 || piece5.side != 5 || piece7.side != 5)
      return false;
  }

  return true;
}

bool
CQRubik::
solveBottomCornerOrder()
{
  if (getUndoGroup()) undo_->startGroup();

  solveBottomCornerOrder1();

  if (getUndoGroup()) undo_->endGroup();

  return solveBottomCornerOrder1(true);
}

bool
CQRubik::
solveBottomCornerOrder1(bool check)
{
  CRubikSide &side = sides_[2];

  const CRubikPiece &piece0 = side.pieces[0][0];
  const CRubikPiece &piece2 = side.pieces[0][2];
  const CRubikPiece &piece6 = side.pieces[2][0];
  const CRubikPiece &piece8 = side.pieces[2][2];

  uint corner0, corner2, corner6, corner8;

  if (piece0.side == 5)
    corner0 = piece0.id;
  else {
    const CRubikPiece &piece0 = getPieceLeft(2, 0, 0);

    if (piece0.side == 5)
      corner0 = piece0.id;
    else {
      const CRubikPiece &piece0 = getPieceUp(2, 0, 0);

      corner0 = piece0.id;
    }
  }

  if (piece2.side == 5)
    corner2 = piece2.id;
  else {
    const CRubikPiece &piece2 = getPieceLeft(2, 0, 2);

    if (piece2.side == 5)
      corner2 = piece2.id;
    else {
      const CRubikPiece &piece2 = getPieceDown(2, 0, 2);

      corner2 = piece2.id;
    }
  }

  if (piece6.side == 5)
    corner6 = piece6.id;
  else {
    const CRubikPiece &piece6 = getPieceRight(2, 2, 0);

    if (piece6.side == 5)
      corner6 = piece6.id;
    else {
      const CRubikPiece &piece6 = getPieceUp(2, 2, 0);

      corner6 = piece6.id;
    }
  }

  if (piece8.side == 5)
    corner8 = piece8.id;
  else {
    const CRubikPiece &piece8 = getPieceRight(2, 2, 2);

    if (piece8.side == 5)
      corner8 = piece8.id;
    else {
      const CRubikPiece &piece8 = getPieceDown(2, 2, 2);

      corner8 = piece8.id;
    }
  }

  if      (corner2 == 0) {
    rotateSide(2, false);

    uint t = corner0; corner0 = corner2; corner2 = corner8; corner8 = corner6; corner6 = t;
  }
  else if (corner6 == 0) {
    rotateSide(2, true);

    uint t = corner0; corner0 = corner6; corner6 = corner8; corner8 = corner2; corner2 = t;
  }
  else if (corner8 == 0) {
    rotateSide2(2);

    uint t1 = corner0; corner0 = corner8; corner8 = t1;
    uint t2 = corner2; corner2 = corner6; corner6 = t2;
  }

  if (check) {
    return (corner2 == 2 && corner6 == 6);
  }

  //-----

  if      (corner2 == 2) {
    if   (corner6 == 6) { // 0 2 6 8
      // done
    }
    else { // 0 2 8 6
      // adjacent 6/8
      rotateSide(0, false); rotateSide(2, true); rotateSide(4, true ); rotateSide (2, false);
      rotateSide(0, true ); rotateSide(2, true); rotateSide(4, false); rotateSide2(2);
    }
  }
  else if (corner2 == 6) {
    if   (corner6 == 2) { // 0 6 2 8
      // swap 2/6

      rotateSide(0, false); rotateSide(2, true); rotateSide(4, true ); rotateSide (2, false);
      rotateSide(0, true ); rotateSide(2, true); rotateSide(4, false); rotateSide2(2);

      solveBottomCornerOrder1();
    }
    else { // 0 6 8 2
      // swap 0/6
      rotateSide2(2);

      rotateSide(0, false); rotateSide(2, true); rotateSide(4, true ); rotateSide (2, false);
      rotateSide(0, true ); rotateSide(2, true); rotateSide(4, false); rotateSide2(2);
    }
  }
  else {
    if   (corner6 == 2) { // 0 8 2 6
      rotateSide(2, false);

      // adjacent 0/8
      rotateSide(0, false); rotateSide(2, true); rotateSide(4, true ); rotateSide (2, false);
      rotateSide(0, true ); rotateSide(2, true); rotateSide(4, false); rotateSide2(2);
    }
    else { // 0 8 6 2
      rotateSide(2, true);

      // adjacent 2/8
      rotateSide(0, false); rotateSide(2, true); rotateSide(4, true ); rotateSide (2, false);
      rotateSide(0, true ); rotateSide(2, true); rotateSide(4, false); rotateSide2(2);
    }
  }

  return true;
}

bool
CQRubik::
solveBottomCornerOrient(bool check)
{
  CRubikSide &side = sides_[2];

  const CRubikPiece &piece0 = side.pieces[0][0];
  const CRubikPiece &piece2 = side.pieces[0][2];
  const CRubikPiece &piece6 = side.pieces[2][0];
  const CRubikPiece &piece8 = side.pieces[2][2];

  uint num = 0;

  if (piece0.side == 5) ++num;
  if (piece2.side == 5) ++num;
  if (piece6.side == 5) ++num;
  if (piece8.side == 5) ++num;

  if (check) return (num == 4);

  if (num == 4) return true;

  if (getUndoGroup()) undo_->startGroup();

  uint state = 0;

  if      (num == 0) {
    const CRubikPiece &upiece1 = getPieceUp  (2, 0, 0);
    const CRubikPiece &upiece2 = getPieceUp  (2, 2, 0);
    const CRubikPiece &dpiece1 = getPieceDown(2, 0, 2);
    const CRubikPiece &dpiece2 = getPieceDown(2, 2, 2);

    if ((upiece1.side != 5 && upiece2.side != 5 && dpiece1.side != 5 && dpiece2.side != 5) ||
       (upiece1.side == upiece2.side && dpiece1.side == dpiece2.side)) {
      if (upiece1.side != 5)
        rotateSide(2, false);

      state = 7;

      rotateSide(2, false);

      solveBottomCornerOrient2();

      rotateSide2(2);

      solveBottomCornerOrient2();
    }
    else {
      if      (upiece1.side != 5 && upiece2.side != 5)
        rotateSide(2, false);
      else if (upiece1.side == 5 && upiece2.side != 5)
        rotateSide2(2);
      else if (upiece1.side == 5 && upiece2.side == 5)
        rotateSide(2, true);

      state = 6;

      solveBottomCornerOrient2();

      rotateSide(2, false);

      solveBottomCornerOrient2();
    }
  }
  else if (num == 1) {
    if (piece2.side == 5) rotateSide (2, false);
    if (piece6.side == 5) rotateSide (2, true);
    if (piece8.side == 5) rotateSide2(2);

    const CRubikPiece &upiece = getPieceUp(2, 2, 0);

    if (upiece.side == 5) {
      // State 1
      state = 1;

      solveBottomCornerOrient1();
    }
    else {
      rotateSide(2, true);

      // State 2
      state = 2;

      solveBottomCornerOrient2();
    }
  }
  else if (num == 2) {
    if (piece0.side != piece8.side && piece2.side != piece6.side) {
      if      (piece0.side != 5 && piece2.side != 5) rotateSide2(2);
      else if (piece0.side != 5) rotateSide(2, false);
      else if (piece2.side != 5) rotateSide(2, true);

      const CRubikPiece &upiece = getPieceUp(2, 2, 0);

      if (upiece.side == 5) {
        // State 4
        state = 4;

        rotateSide2(2);

        solveBottomCornerOrient1();

        solveBottomCornerOrient2();
      }
      else {
        // State 3
        state = 3;

        rotateSide(2, false);

        solveBottomCornerOrient2();

        rotateSide2(2);

        solveBottomCornerOrient1();
      }
    }
    else {
      state = 5;

      if (piece0.side != 5) rotateSide(2, false);

      const CRubikPiece &upiece = getPieceUp(2, 2, 0);

      if (upiece.side != 5) rotateSide2(2);

      rotateSide(2, false);

      solveBottomCornerOrient1();

      rotateSide(2, false);

      solveBottomCornerOrient2();
    }
  }

  if (side.pieces[0][0].id != 0) rotateSide(2, false);
  if (side.pieces[0][0].id != 0) rotateSide(2, false);
  if (side.pieces[0][0].id != 0) rotateSide(2, false);

  if (getUndoGroup()) undo_->endGroup();

  if (! solveBottomCornerOrient(true)) {
    std::cerr << "State " << state << "(" << num << ")" << std::endl;
    return false;
  }

  return true;
}

bool
CQRubik::
solveBottomMiddles()
{
  CRubikSide &side = sides_[2];

  CRubikPiece &piece1 = side.pieces[0][1];
  CRubikPiece &piece3 = side.pieces[1][0];
  CRubikPiece &piece5 = side.pieces[1][2];
  CRubikPiece &piece7 = side.pieces[2][1];

  if (piece1.id == 1 && piece3.id == 3 && piece5.id == 5 && piece7.id == 7)
    return true;

  if (getUndoGroup()) undo_->startGroup();

  solveBottomMiddlesSub();

  if (getUndoGroup()) undo_->endGroup();

  return (piece1.id == 1 && piece3.id == 3 && piece5.id == 5 && piece7.id == 7);
}

void
CQRubik::
solveBottomMiddlesSub()
{
  CRubikSide &side = sides_[2];

  CRubikPiece &piece1 = side.pieces[0][1];
  CRubikPiece &piece3 = side.pieces[1][0];
  CRubikPiece &piece5 = side.pieces[1][2];
  CRubikPiece &piece7 = side.pieces[2][1];

  if (piece1.id == 1 && piece3.id == 3 && piece5.id == 5 && piece7.id == 7) return;

  if      (piece1.id == 1) { // L Side Correct
    if      (piece3.id == 3) {
      if (piece5.id == 5) {
      }
      else { // 7
      }
    }
    else if (piece3.id == 5) {
      if (piece5.id == 3) {
      }
      else { // 7
        // State 2
        solveBottomMiddles2();
      }
    }
    else { // 7
      if (piece5.id == 3) {
        // State 1
        solveBottomMiddles1();
      }
      else { // 5
      }
    }
  }
  else if (piece3.id == 3) { // T Side Correct
    rotateSide(2, true);

    if      (piece3.id == 1) {
      if (piece5.id == 5) {
        // State 2
        solveBottomMiddles2();
      }
      else { // 7
      }
    }
    else if (piece3.id == 5) {
      if (piece5.id == 3) {
      }
      else { // 7
        // State 1
        solveBottomMiddles1();
      }
    }
    else { // 7
      if (piece5.id == 1) {
      }
      else { // 5
      }
    }

    rotateSide(2, false);
  }
  else if (piece5.id == 5) { // B Side Correct
    rotateSide(2, false);

    if      (piece3.id == 1) {
      if (piece5.id == 3) {
      }
      else { // 7
      }
    }
    else if (piece3.id == 3) {
      if (piece5.id == 1) {
        // State 1
        solveBottomMiddles1();
      }
      else { // 7
      }
    }
    else { // 7
      if (piece5.id == 1) {
      }
      else { // 3
        // State 2
        solveBottomMiddles2();
      }
    }

    rotateSide(2, true);
  }
  else if (piece7.id == 7) { // R Side Correct
    rotateSide2(2);

    if      (piece3.id == 1) {
      if (piece5.id == 3) {
      }
      else { // 5
        // State 1
        solveBottomMiddles1();
      }
    }
    else if (piece3.id == 3) {
      if (piece5.id == 1) {
        // State 1
        solveBottomMiddles2();
      }
      else { // 5
      }
    }
    else { // 5
      if (piece5.id == 1) {
      }
      else { // 3
      }
    }

    rotateSide2(2);
  }
  else { // No Sides Correct
    if ((piece1.side == 7 && piece7.side == 1) ||
        (piece1.side == 3 && piece7.side == 5) ||
        (piece1.side == 5 && piece7.side == 3)) {
      solveBottomMiddles1();

      solveBottomMiddlesSub();
    }
    else {
      solveBottomMiddles1();

      solveBottomMiddlesSub();
    }
  }
}

void
CQRubik::
solveBottomMiddles1()
{
  rotateSide2(4); rotateSide(2, false); rotateSide(3, false); rotateSide(1, true );
  rotateSide2(4); rotateSide(3, true ); rotateSide(1, false); rotateSide(2, false);
  rotateSide2(4);
}

void
CQRubik::
solveBottomMiddles2()
{
  rotateSide2(4); rotateSide(2, true); rotateSide(3, false); rotateSide(1, true);
  rotateSide2(4); rotateSide(3, true); rotateSide(1, false); rotateSide(2, true);
  rotateSide2(4);
}

void
CQRubik::
solveBottomCornerOrient1()
{
  rotateSide(4, true); rotateSide (2, true); rotateSide(4, false); rotateSide (2, true);
  rotateSide(4, true); rotateSide2(2      ); rotateSide(4, false); rotateSide2(2);
}

void
CQRubik::
solveBottomCornerOrient2()
{
  rotateSide(4, false); rotateSide (2, false); rotateSide(4, true); rotateSide (2, false);
  rotateSide(4, false); rotateSide2(2       ); rotateSide(4, true); rotateSide2(2);
}

CRubikPieceInd
CQRubik::
findPiece(uint side_num, uint id)
{
  for (uint i = 0; i < CUBE_SIDES; ++i) {
    CRubikSide &side = sides_[i];

    for (uint j = 0; j < SIDE_PIECES; ++j) {
      uint ix = j % SIDE_LENGTH;
      uint iy = j / SIDE_LENGTH;

      if (side.pieces[ix][iy].side == side_num && side.pieces[ix][iy].id == id)
        return CRubikPieceInd(i, ix, iy);
    }
  }

  return CRubikPieceInd(0, 0, 0);
}

void
CQRubik::
paintEvent(QPaintEvent *)
{
  QPainter p(this);

  p.fillRect(rect(), QColor(140,140,140));
}

void
CQRubik::
resizeEvent(QResizeEvent *)
{
}

void
CQRubik::
placeWidgets()
{
  int s  = std::min(width(), height());
  int s1 = s/3;
  int b  = s1/10;

  if (! getShow3()) {
    int s2 = 2*s1;
    int s3 = 3*(s2 - 2*b)/4 + 2*b;

    w2_->resize(s2, s3);
    w2_->move  (b, height() - s3 - b);

    w3_->resize(s1, s1);
    w3_->move  (width() - s1 - b, b);

    //toolbar_->move(width() - s1 - b, b + s1);
  }
  else {
    int s2 = s1;
    int s3 = 3*(s2 - 2*b)/4 + 2*b;

    w2_->resize(s2, s3);
    w2_->move  (width() - s2 - b, b);

    w3_->resize(2*s3, 2*s3);
    w3_->move  (b, height() - 2*s3 - b);

    //toolbar_->move(b, height() - 2*s3 - b - toolbar_->height());
  }
}

void
CQRubik::
keyPressEvent(QKeyEvent *e)
{
  int key = e->key();

  if      (key == Qt::Key_A) {
    setAnimate(! getAnimate());

    getTwoD()->update(); getThreeD()->update();
  }
  else if (key == Qt::Key_R) {
    reset();

    getTwoD()->update(); getThreeD()->update();
  }
  else if (key == Qt::Key_M) {
    randomize();

    getTwoD()->update(); getThreeD()->update();
  }
  else if (key == Qt::Key_H) {
    setShade(! getShade());

    getTwoD()->update(); getThreeD()->update();
  }
  else if (key == Qt::Key_N) {
    setNumber(! getNumber());

    getTwoD()->update(); getThreeD()->update();
  }
  else if (key == Qt::Key_F) {
    setShow3(! getShow3());

    resizeEvent(NULL);

    getTwoD()->update(); getThreeD()->update();
  }
  else if (key == Qt::Key_S) {
    solve();

    getTwoD()->update(); getThreeD()->update();
  }
  else if (key == Qt::Key_T) {
    getThreeD()->toggleTexture();

    getTwoD()->update(); getThreeD()->update();
  }
  else if (key == Qt::Key_U) {
    setUndoGroup(! getUndoGroup());
  }
  else if (key == Qt::Key_X) {
    for (uint i = 0; i < 1000; ++i) {
      randomize();

      if (! solve()) break;
    }

    getTwoD()->update(); getThreeD()->update();
  }
  else if (e->modifiers() & Qt::ShiftModifier) {
    movePieces(key);
  }
  else if (e->modifiers() & Qt::ControlModifier) {
    if      (key == Qt::Key_Z) {
      getUndo()->undo();

      getTwoD()->update(); getThreeD()->update();
    }
    else if (key == Qt::Key_Y) {
      getUndo()->redo();

      getTwoD()->update(); getThreeD()->update();
    }
    else
      rotatePieces(key);
  }
  else if (e->modifiers() & Qt::AltModifier) {
    moveSides(key);
  }
  else {
    movePosition(key);
  }
}

void
CQRubik::
movePosition(int key)
{
  if      (key == Qt::Key_Left) {
    getPosLeft(ind_.side_num, ind_.side_col, ind_.side_row, dir_,
               ind_.side_num, ind_.side_col, ind_.side_row, dir_);
  }
  else if (key == Qt::Key_Right) {
    getPosRight(ind_.side_num, ind_.side_col, ind_.side_row, dir_,
                ind_.side_num, ind_.side_col, ind_.side_row, dir_);
  }
  else if (key == Qt::Key_Down) {
    getPosDown(ind_.side_num, ind_.side_col, ind_.side_row, dir_,
               ind_.side_num, ind_.side_col, ind_.side_row, dir_);
  }
  else if (key == Qt::Key_Up) {
    getPosUp(ind_.side_num, ind_.side_col, ind_.side_row, dir_,
             ind_.side_num, ind_.side_col, ind_.side_row, dir_);
  }
  else
    return;

  getTwoD()->update(); getThreeD()->update();
}

const CRubikPiece &
CQRubik::
getPieceLeft(uint side_num, uint side_col, uint side_row) const
{
  uint side_num1, side_col1, side_row1;
  int  dir1;

  getPosLeft(side_num, side_col, side_row, 0, side_num1, side_col1, side_row1, dir1);

  return sides_[side_num1].pieces[side_col1][side_row1];
}

const CRubikPiece &
CQRubik::
getPieceRight(uint side_num, uint side_col, uint side_row) const
{
  uint side_num1, side_col1, side_row1;
  int  dir1;

  getPosRight(side_num, side_col, side_row, 0, side_num1, side_col1, side_row1, dir1);

  return sides_[side_num1].pieces[side_col1][side_row1];
}

const CRubikPiece &
CQRubik::
getPieceDown(uint side_num, uint side_col, uint side_row) const
{
  uint side_num1, side_col1, side_row1;
  int  dir1;

  getPosDown(side_num, side_col, side_row, 0, side_num1, side_col1, side_row1, dir1);

  return sides_[side_num1].pieces[side_col1][side_row1];
}

const CRubikPiece &
CQRubik::
getPieceUp(uint side_num, uint side_col, uint side_row) const
{
  uint side_num1, side_col1, side_row1;
  int  dir1;

  getPosUp(side_num, side_col, side_row, 0, side_num1, side_col1, side_row1, dir1);

  return sides_[side_num1].pieces[side_col1][side_row1];
}

void
CQRubik::
getPosLeft(uint side_num, uint side_col, uint side_row, int dir,
           uint &side_num1, uint &side_col1, uint &side_row1, int &dir1) const
{
  side_num1 = side_num;
  side_col1 = side_col;
  side_row1 = side_row;
  dir1      = dir;

  const CRubikSide &side = sides_[side_num1];

  if (side_col1 > 0)
    --side_col1;
  else {
    side_num1 = side.side_l.side;

    dir1 += side.side_l.rotate;

    if (dir1 <= -180) dir1 += 360; else if (dir1 > 180) dir1 -= 360;

    if      (dir1 ==   0)   side_col1 = 2;
    else if (dir1 == 180) { side_col1 = 0; side_row1 = 2 - side_row1; }
    else if (dir1 ==  90) { side_col1 = side_row1    ; side_row1 = 0; }
    else if (dir1 == -90) { side_col1 = 2 - side_row1; side_row1 = 2; }
  }

  dir1 = 0;
}

void
CQRubik::
getPosRight(uint side_num, uint side_col, uint side_row, int dir,
            uint &side_num1, uint &side_col1, uint &side_row1, int &dir1) const
{
  side_num1 = side_num;
  side_col1 = side_col;
  side_row1 = side_row;
  dir1      = dir;

  const CRubikSide &side = sides_[side_num1];

  if (side_col1 < 2)
    ++side_col1;
  else {
    side_num1 = side.side_r.side;

    dir1 += side.side_r.rotate;

    if (dir1 <= -180) dir1 += 360; else if (dir1 > 180) dir1 -= 360;

    if      (dir1 ==   0)   side_col1 = 0;
    else if (dir1 == 180) { side_col1 = 2; side_row1 = 2 - side_row1; }
    else if (dir1 ==  90) { side_col1 = side_row1    ; side_row1 = 2; }
    else if (dir1 == -90) { side_col1 = 2 - side_row1; side_row1 = 0; }
  }

  dir1 = 0;
}

void
CQRubik::
getPosDown(uint side_num, uint side_col, uint side_row, int dir,
           uint &side_num1, uint &side_col1, uint &side_row1, int &dir1) const
{
  side_num1 = side_num;
  side_col1 = side_col;
  side_row1 = side_row;
  dir1      = dir;

  const CRubikSide &side = sides_[side_num1];

  if      (dir1 == 0) {
    if (side_row1 < 2)
      ++side_row1;
    else {
      side_num1 = side.side_d.side;

      dir1 += side.side_d.rotate;

      if (dir1 <= -180) dir1 += 360; else if (dir1 > 180) dir1 -= 360;

      if      (dir1 ==   0)   side_row1 = 0;
      else if (dir1 == 180) { side_row1 = 2; side_col1 = 2 - side_col1; }
      else if (dir1 ==  90) { side_row1 = 2 - side_col1; side_col1 = 0; }
      else if (dir1 == -90) { side_row1 = side_col1    ; side_col1 = 2; }
    }
  }
  else if (dir1 == 180) {
    if (side_row1 > 0)
      --side_row1;
    else {
      side_num1 = side.side_u.side;

      dir1 += side.side_u.rotate;

      if (dir1 <= -180) dir1 += 360; else if (dir1 > 180) dir1 -= 360;

      if      (dir1 ==   0) { side_row1 = 0; side_col1 = 2 - side_col1; }
      else if (dir1 == 180)   side_row1 = 2;
      else if (dir1 ==  90) { side_row1 =     side_col1; side_col1 = 0; }
      else if (dir1 == -90) { side_row1 = 2 - side_col1; side_col1 = 2; }
    }
  }
  else if (dir1 == 90) {
    if (side_col1 < 2)
      ++side_col1;
    else {
      side_num1 = side.side_r.side;

      dir1 += side.side_r.rotate;

      if (dir1 <= -180) dir1 += 360; else if (dir1 > 180) dir1 -= 360;

      if      (dir1 ==  90)   side_col1 = 0;
      else if (dir1 == -90)   side_col1 = 2;
      else if (dir1 ==   0) { side_col1 = 2 - side_row1; side_row1 = 0; }
      else if (dir1 == 180) { side_col1 =     side_row1; side_row1 = 2; }
    }
  }
  else if (dir1 == -90) {
    if (side_col1 > 0)
      --side_col1;
    else {
      side_num1 = side.side_l.side;

      dir1 += side.side_l.rotate;

      if (dir1 <= -180) dir1 += 360; else if (dir1 > 180) dir1 -= 360;

      if      (dir1 ==  90)   side_col1 = 0;
      else if (dir1 == -90)   side_col1 = 2;
      else if (dir1 ==   0) { side_col1 =     side_row1; side_row1 = 0; }
      else if (dir1 == 180) { side_col1 = 2 - side_row1; side_row1 = 2; }
    }
  }
}

void
CQRubik::
getPosUp(uint side_num, uint side_col, uint side_row, int dir,
         uint &side_num1, uint &side_col1, uint &side_row1, int &dir1) const
{
  side_num1 = side_num;
  side_col1 = side_col;
  side_row1 = side_row;
  dir1      = dir;

  const CRubikSide &side = sides_[side_num1];

  if      (dir1 == 0) {
    if (side_row1 > 0)
      --side_row1;
    else {
      side_num1 = side.side_u.side;

      dir1 += side.side_u.rotate;

      if  (dir1 <= -180) dir1 += 360; else if (dir1 > 180) dir1 -= 360;

      if      (dir1 ==   0)   side_row1 = 2;
      else if (dir1 == 180) { side_row1 = 0; side_col1 = 2 - side_col1; }
      else if (dir1 ==  90) { side_row1 = 2 - side_col1; side_col1 = 2; }
      else if (dir1 == -90) { side_row1 =     side_col1; side_col1 = 0; }
    }
  }
  else if (dir1 == 180) {
    if (side_row1 < 2)
      ++side_row1;
    else {
      side_num1 = side.side_d.side;

      dir1 += side.side_d.rotate;

      if (dir1 <= -180) dir1 += 360; else if (dir1 > 180) dir1 -= 360;

      if      (dir1 ==   0) { side_row1 = 2; side_col1 = 2 - side_col1; }
      else if (dir1 == 180)   side_row1 = 0;
      else if (dir1 ==  90) { side_row1 =     side_col1; side_col1 = 2; }
      else if (dir1 == -90) { side_row1 = 2 - side_col1; side_col1 = 0; }
    }
  }
  else if (dir1 == 90) {
    if (side_col1 > 0)
      --side_col1;
    else {
      side_num1 = side.side_l.side;

      dir1 += side.side_l.rotate;

      if (dir1 <= -180) dir1 += 360; else if (dir1 > 180) dir1 -= 360;

      if      (dir1 ==  90)   side_col1 = 2;
      else if (dir1 == -90)   side_col1 = 0;
      else if (dir1 ==   0) { side_col1 = 2 - side_row1; side_row1 = 2; }
      else if (dir1 == 180) { side_col1 =     side_row1; side_row1 = 0; }
    }
  }
  else if (dir1 == -90) {
    if (side_col1 < 2)
      ++side_col1;
    else {
      side_num1 = side.side_r.side;

      dir1 += side.side_r.rotate;

      if (dir1 <= -180) dir1 += 360; else if (dir1 > 180) dir1 -= 360;

      if      (dir1 ==  90)   side_col1 = 2;
      else if (dir1 == -90)   side_col1 = 0;
      else if (dir1 ==   0) { side_col1 =     side_row1; side_row1 = 2; }
      else if (dir1 == 180) { side_col1 = 2 - side_row1; side_row1 = 0; }
    }
  }
}

void
CQRubik::
movePieces(int key)
{
  if      (key == Qt::Key_Left)
    moveSideLeft (ind_.side_num, ind_.side_row);
  else if (key == Qt::Key_Right)
    moveSideRight(ind_.side_num, ind_.side_row);
  else if (key == Qt::Key_Down)
    moveSideDown (ind_.side_num, ind_.side_col);
  else if (key == Qt::Key_Up)
    moveSideUp   (ind_.side_num, ind_.side_col);
  else
    return;

  getTwoD()->update(); getThreeD()->update();
}

void
CQRubik::
rotatePieces(int key)
{
  if      (key == Qt::Key_Left)
    rotateSide(ind_.side_num, true);
  else if (key == Qt::Key_Right)
    rotateSide(ind_.side_num, false);
  else
    return;

  getTwoD()->update(); getThreeD()->update();
}

void
CQRubik::
moveSides(int key)
{
  if      (key == Qt::Key_Left)
    moveSidesLeft();
  else if (key == Qt::Key_Right)
    moveSidesRight();
  else if (key == Qt::Key_Down)
    moveSidesDown();
  else if (key == Qt::Key_Up)
    moveSidesUp();
  else
    return;

  getTwoD()->update(); getThreeD()->update();
}

void
CQRubik::
execute(const std::string &moveStr)
{
  // syntax:
  //  <side> (F,B,U,D,L,R) [']

  CStrParse parse(moveStr);

  parse.skipSpace();

  char side;

  while (! parse.readChar(&side)) {
    bool clockwise = true;
    uint turns     = 1;

    if      (parse.isChar('\''))
      clockwise = false;
    else if (parse.isChar('2'))
      turns = 2;

    uint iside;

    if (! decodeSideChar(side, iside))
      return;

    if      (turns == 1)
      rotateSide(iside, clockwise);
    else
      rotateSide2(iside);

    parse.skipSpace();
  }
}

bool
CQRubik::
decodeSideChar(char c, uint &n)
{
  static std::string sides = "LUFDRB";

  std::string::size_type iside = sides.find(c);

  if (iside == std::string::npos) return false;

  n = iside;

  return true;
}

bool
CQRubik::
encodeSideChar(uint n, char &c)
{
  static std::string sides = "LUFDRB";

  if (n >= 6) return false;

  c = sides[n];

  return true;
}

void
CQRubik::
moveSideLeft2(uint side_num, uint side_row)
{
  moveSideLeft(side_num, side_row);
  moveSideLeft(side_num, side_row);
}

void
CQRubik::
moveSideRight2(uint side_num, uint side_row)
{
  moveSideRight(side_num, side_row);
  moveSideRight(side_num, side_row);
}

void
CQRubik::
moveSideDown2(uint side_num, uint side_row)
{
  moveSideDown(side_num, side_row);
  moveSideDown(side_num, side_row);
}

void
CQRubik::
moveSideUp2(uint side_num, uint side_row)
{
  moveSideUp(side_num, side_row);
  moveSideUp(side_num, side_row);
}

void
CQRubik::
moveSideLeft(uint side_num, uint side_row)
{
  int side_num1 = side_num;

  CRubikSide &side1 = sides_[side_num1];

  if      (side_row == 0) {
    int tside_num = side1.side_u.side;

    rotateSide(tside_num, false);
  }
  else if (side_row == 1) {
    if (getAnimate()) {
      if (side_num == 0 || side_num == 2 || side_num == 4 || side_num == 5)
        animateRotateMiddleX(false);
      else
        animateRotateMiddleY(false);

      return;
    }

    moveSideLeft1(side_num, side_row);

    undo_->addUndo(new CQRubikUndoMoveData(this, side_num, 'L', side_row));
  }
  else if (side_row == 2) {
    int bside_num = side1.side_d.side;

    rotateSide(bside_num, true);
  }
}

void
CQRubik::
moveSideRight(uint side_num, uint side_row)
{
  int side_num1 = side_num;

  CRubikSide &side1 = sides_[side_num1];

  if      (side_row == 0) {
    int tside_num = side1.side_u.side;

    rotateSide(tside_num, true);
  }
  else if (side_row == 1) {
    if (getAnimate()) {
      if (side_num == 0 || side_num == 2 || side_num == 4 || side_num == 5)
        animateRotateMiddleX(true);
      else
        animateRotateMiddleY(true);

      return;
    }

    moveSideRight1(side_num, side_row);

    undo_->addUndo(new CQRubikUndoMoveData(this, side_num, 'R', side_row));
  }
  else if (side_row == 2) {
    int bside_num = side1.side_d.side;

    rotateSide(bside_num, false);
  }
}

void
CQRubik::
moveSideDown(uint side_num, uint side_col)
{
  int side_num1 = side_num;

  CRubikSide &side1 = sides_[side_num1];

  if      (side_col == 0) {
    uint lside_num = side1.side_l.side;

    rotateSide(lside_num, false);
  }
  else if (side_col == 1) {
    if (getAnimate()) {
      if      (side_num == 1 || side_num == 2 || side_num == 3)
        animateRotateMiddleZ(false);
      else if (side_num == 5)
        animateRotateMiddleZ(true);
      else
        animateRotateMiddleY(false);

      return;
    }

    moveSideDown1(side_num, side_col);

    undo_->addUndo(new CQRubikUndoMoveData(this, side_num, 'D', side_col));
  }
  else if (side_col == 2) {
    uint rside_num = side1.side_r.side;

    rotateSide(rside_num, true);
  }
}

void
CQRubik::
moveSideUp(uint side_num, uint side_col)
{
  int side_num1 = side_num;

  CRubikSide &side1 = sides_[side_num1];

  if      (side_col == 0) {
    uint lside_num = side1.side_l.side;

    rotateSide(lside_num, true);
  }
  else if (side_col == 1) {
    if (getAnimate()) {
      if      (side_num == 1 || side_num == 2 || side_num == 3)
        animateRotateMiddleZ(true);
      else if (side_num == 5)
        animateRotateMiddleZ(false);
      else
        animateRotateMiddleY(true);

      return;
    }

    moveSideUp1(side_num, side_col);

    undo_->addUndo(new CQRubikUndoMoveData(this, side_num, 'U', side_col));
  }
  else if (side_col == 2) {
    uint rside_num = side1.side_r.side;

    rotateSide(rside_num, false);
  }
}

void
CQRubik::
rotateSide2(uint side_num)
{
  rotateSide(side_num, true);
  rotateSide(side_num, true);
}

void
CQRubik::
rotateSide(uint side_num, bool clockwise)
{
  if (getAnimate()) {
    animateRotateSide(side_num, clockwise);
    return;
  }

  if (clockwise) {
    rotateSide1(side_num, true);

    CRubikSide &side = sides_[side_num];

    if      (side.side_l.rotate == 0)
      moveSideDown1(side.side_l.side, 2);
    else if (side.side_r.rotate == 0)
      moveSideUp1  (side.side_r.side, 0);
    else if (side.side_d.rotate == 0)
      moveSideRight1(side.side_d.side, 0);
    else if (side.side_u.rotate == 0)
      moveSideLeft1(side.side_u.side, 2);
  }
  else {
    rotateSide1(side_num, false);

    CRubikSide &side = sides_[side_num];

    if      (side.side_l.rotate == 0)
      moveSideUp1  (side.side_l.side, 2);
    else if (side.side_r.rotate == 0)
      moveSideDown1(side.side_r.side, 0);
    else if (side.side_d.rotate == 0)
      moveSideLeft1(side.side_d.side, 0);
    else if (side.side_u.rotate == 0)
      moveSideRight1(side.side_u.side, 2);
  }

  undo_->addUndo(new CQRubikUndoRotateData(this, side_num, clockwise));
}

void
CQRubik::
animateRotateSide(uint side_num, bool clockwise)
{
  animateData_.side_num  = side_num;
  animateData_.clockwise = clockwise;
  animateData_.axis      = false;
  animateData_.step      = 0;

  animateData_.animating = true;

  animateRotateSideSlot();
}

void
CQRubik::
animateRotateMiddleX(bool clockwise)
{
  animateData_.side_num  = 0;
  animateData_.clockwise = clockwise;
  animateData_.axis      = true;
  animateData_.step      = 0;

  animateData_.animating = true;

  animateRotateSideSlot();
}

void
CQRubik::
animateRotateMiddleY(bool clockwise)
{
  animateData_.side_num  = 1;
  animateData_.clockwise = clockwise;
  animateData_.axis      = true;
  animateData_.step      = 0;

  animateData_.animating = true;

  animateRotateSideSlot();
}

void
CQRubik::
animateRotateMiddleZ(bool clockwise)
{
  animateData_.side_num  = 2;
  animateData_.clockwise = clockwise;
  animateData_.axis      = true;
  animateData_.step      = 0;

  animateData_.animating = true;

  animateRotateSideSlot();
}

void
CQRubik::
animateRotateSideSlot()
{
  getTwoD()->update(); getThreeD()->update();

  ++animateData_.step;

  uint num = 10;

  if (animateData_.step <= num)
    QTimer::singleShot(50, this, SLOT(animateRotateSideSlot()));
  else {
    animateData_.animating = false;

    setAnimate(false);

    if      (! animateData_.axis)
      rotateSide(animateData_.side_num, animateData_.clockwise);
    else if (animateData_.side_num == 0) {
      if (! animateData_.clockwise)
        moveSideLeft (2, 1);
      else
        moveSideRight(2, 1);
    }
    else if (animateData_.side_num == 1) {
      if (animateData_.clockwise)
        moveSideUp  (0, 1);
      else
        moveSideDown(0, 1);
    }
    else if (animateData_.side_num == 2) {
      if (animateData_.clockwise)
        moveSideUp  (2, 1);
      else
        moveSideDown(2, 1);
    }

    setAnimate(true);
  }
}

void
CQRubik::
waitAnimate()
{
  while (animateData_.animating) {
    QApplication::processEvents();
  }
}

void
CQRubik::
moveSidesLeft()
{
  moveSideLeft(2, 0);
  moveSideLeft(2, 1);
  moveSideLeft(2, 2);
}

void
CQRubik::
moveSidesRight()
{
  moveSideRight(2, 0);
  moveSideRight(2, 1);
  moveSideRight(2, 2);
}

void
CQRubik::
moveSidesDown()
{
  moveSideDown(2, 0);
  moveSideDown(2, 1);
  moveSideDown(2, 2);
}

void
CQRubik::
moveSidesUp()
{
  moveSideUp(2, 0);
  moveSideUp(2, 1);
  moveSideUp(2, 2);
}

void
CQRubik::
moveSideLeft1(uint side_num, uint side_row)
{
  uint side_num1 = side_num;

  int dir1 = 0, dir2, dir3, dir4;

  int side_num2, side_num3, side_num4;

  side_num2 = sides_[side_num1].side_l.side; dir2 = dir1 + sides_[side_num1].side_l.rotate;

  if (dir2 <= -180) dir2 += 360; else if (dir2 > 180) dir2 -= 360;

  if      (dir2 == 0) {
    side_num3 = sides_[side_num2].side_l.side; dir3 = dir2 + sides_[side_num2].side_l.rotate;
  }
  else if (dir2 == 90) {
    side_num3 = sides_[side_num2].side_d.side; dir3 = dir2 + sides_[side_num2].side_d.rotate;
  }
  else if (dir2 == -90) {
    side_num3 = sides_[side_num2].side_u.side; dir3 = dir2 + sides_[side_num2].side_u.rotate;
  }
  else {
    side_num3 = sides_[side_num2].side_r.side; dir3 = dir2 + sides_[side_num2].side_r.rotate;
  }

  if (dir3 <= -180) dir3 += 360; else if (dir3 > 180) dir3 -= 360;

  if      (dir3 == 0) {
    side_num4 = sides_[side_num3].side_l.side; dir4 = dir3 + sides_[side_num3].side_l.rotate;
  }
  else if (dir3 == 90) {
    side_num4 = sides_[side_num3].side_d.side; dir4 = dir3 + sides_[side_num3].side_d.rotate;
  }
  else if (dir3 == -90) {
    side_num4 = sides_[side_num3].side_u.side; dir4 = dir3 + sides_[side_num3].side_u.rotate;
  }
  else {
    side_num4 = sides_[side_num3].side_r.side; dir4 = dir3 + sides_[side_num3].side_r.rotate;
  }

  if (dir4 <= -180) dir4 += 360; else if (dir4 > 180) dir4 -= 360;

  CRubikSide &side1 = sides_[side_num1];
  CRubikSide &side2 = sides_[side_num2];
  CRubikSide &side3 = sides_[side_num3];
  CRubikSide &side4 = sides_[side_num4];

  CRubikPiece t[15];

  uint j = 0;

  for (uint i = 0; i < 3; ++i, ++j)
    t[j] = side1.pieces[2 - i][side_row];

  for (uint i = 0; i < 3; ++i, ++j) {
    if      (dir2 ==   0) t[j] = side2.pieces[       2 - i][    side_row];
    else if (dir2 ==  90) t[j] = side2.pieces[    side_row][           i];
    else if (dir2 == 180) t[j] = side2.pieces[           i][2 - side_row];
    else                  t[j] = side2.pieces[2 - side_row][       2 - i];
  }

  for (uint i = 0; i < 3; ++i, ++j) {
    if      (dir3 ==   0) t[j] = side3.pieces[       2 - i][    side_row];
    else if (dir3 ==  90) t[j] = side3.pieces[    side_row][           i];
    else if (dir3 == 180) t[j] = side3.pieces[           i][2 - side_row];
    else                  t[j] = side3.pieces[2 - side_row][       2 - i];
  }

  for (uint i = 0; i < 3; ++i, ++j) {
    if      (dir4 ==   0) t[j] = side4.pieces[       2 - i][    side_row];
    else if (dir4 ==  90) t[j] = side4.pieces[    side_row][           i];
    else if (dir4 == 180) t[j] = side4.pieces[           i][2 - side_row];
    else                  t[j] = side4.pieces[2 - side_row][       2 - i];
  }

  //----

  for (int i = 11; i >= 0; --i)
    t[i + 3] = t[i];

  for (uint i = 12; i < 15; ++i)
    t[i - 12] = t[i];

  //----

  j = 0;

  for (uint i = 0; i < 3; ++i, ++j)
    side1.pieces[2 - i][side_row] = t[j];

  for (uint i = 0; i < 3; ++i, ++j) {
    if      (dir2 ==   0) side2.pieces[       2 - i][    side_row] = t[j];
    else if (dir2 ==  90) side2.pieces[    side_row][           i] = t[j];
    else if (dir2 == 180) side2.pieces[           i][2 - side_row] = t[j];
    else                  side2.pieces[2 - side_row][       2 - i] = t[j];
  }

  for (uint i = 0; i < 3; ++i, ++j) {
    if      (dir3 ==   0) side3.pieces[       2 - i][    side_row] = t[j];
    else if (dir3 ==  90) side3.pieces[    side_row][           i] = t[j];
    else if (dir3 == 180) side3.pieces[           i][2 - side_row] = t[j];
    else                  side3.pieces[2 - side_row][       2 - i] = t[j];
  }

  for (uint i = 0; i < 3; ++i, ++j) {
    if      (dir4 ==   0) side4.pieces[       2 - i][    side_row] = t[j];
    else if (dir4 ==  90) side4.pieces[    side_row][           i] = t[j];
    else if (dir4 == 180) side4.pieces[           i][2 - side_row] = t[j];
    else                  side4.pieces[2 - side_row][       2 - i] = t[j];
  }
}

void
CQRubik::
moveSideRight1(uint side_num, uint side_row)
{
  uint side_num1 = side_num;

  int dir1 = 0, dir2, dir3, dir4;

  int side_num2, side_num3, side_num4;

  side_num2 = sides_[side_num1].side_r.side; dir2 = dir1 + sides_[side_num1].side_r.rotate;

  if (dir2 <= -180) dir2 += 360; else if (dir2 > 180) dir2 -= 360;

  if      (dir2 == 0) {
    side_num3 = sides_[side_num2].side_r.side; dir3 = dir2 + sides_[side_num2].side_r.rotate;
  }
  else if (dir2 == 90) {
    side_num3 = sides_[side_num2].side_u.side; dir3 = dir2 + sides_[side_num2].side_u.rotate;
  }
  else if (dir2 == -90) {
    side_num3 = sides_[side_num2].side_d.side; dir3 = dir2 + sides_[side_num2].side_d.rotate;
  }
  else {
    side_num3 = sides_[side_num2].side_l.side; dir3 = dir2 + sides_[side_num2].side_l.rotate;
  }

  if (dir3 <= -180) dir3 += 360; else if (dir3 > 180) dir3 -= 360;

  if      (dir3 == 0) {
    side_num4 = sides_[side_num3].side_r.side; dir4 = dir3 + sides_[side_num3].side_r.rotate;
  }
  else if (dir3 == 90) {
    side_num4 = sides_[side_num3].side_u.side; dir4 = dir3 + sides_[side_num3].side_u.rotate;
  }
  else if (dir3 == -90) {
    side_num4 = sides_[side_num3].side_d.side; dir4 = dir3 + sides_[side_num3].side_d.rotate;
  }
  else {
    side_num4 = sides_[side_num3].side_l.side; dir4 = dir3 + sides_[side_num3].side_l.rotate;
  }

  if (dir4 <= -180) dir4 += 360; else if (dir4 > 180) dir4 -= 360;

  CRubikSide &side1 = sides_[side_num1];
  CRubikSide &side2 = sides_[side_num2];
  CRubikSide &side3 = sides_[side_num3];
  CRubikSide &side4 = sides_[side_num4];

  CRubikPiece t[15];

  uint j = 0;

  for (uint i = 0; i < 3; ++i, ++j)
    t[j] = side1.pieces[i][side_row];

  for (uint i = 0; i < 3; ++i, ++j) {
    if      (dir2 ==   0) t[j] = side2.pieces[           i][    side_row];
    else if (dir2 ==  90) t[j] = side2.pieces[    side_row][       2 - i];
    else if (dir2 == 180) t[j] = side2.pieces[       2 - i][2 - side_row];
    else                  t[j] = side2.pieces[2 - side_row][           i];
  }

  for (uint i = 0; i < 3; ++i, ++j) {
    if      (dir3 ==   0) t[j] = side3.pieces[           i][    side_row];
    else if (dir3 ==  90) t[j] = side3.pieces[    side_row][       2 - i];
    else if (dir3 == 180) t[j] = side3.pieces[       2 - i][2 - side_row];
    else                  t[j] = side3.pieces[2 - side_row][           i];
  }

  for (uint i = 0; i < 3; ++i, ++j) {
    if      (dir4 ==   0) t[j] = side4.pieces[           i][    side_row];
    else if (dir4 ==  90) t[j] = side4.pieces[    side_row][       2 - i];
    else if (dir4 == 180) t[j] = side4.pieces[       2 - i][2 - side_row];
    else                  t[j] = side4.pieces[2 - side_row][           i];
  }

  //----

  for (int i = 11; i >= 0; --i)
    t[i + 3] = t[i];

  for (uint i = 12; i < 15; ++i)
    t[i - 12] = t[i];

  //----

  j = 0;

  for (uint i = 0; i < 3; ++i, ++j)
    side1.pieces[i][side_row] = t[j];

  for (uint i = 0; i < 3; ++i, ++j) {
    if      (dir2 ==   0) side2.pieces[           i][    side_row] = t[j];
    else if (dir2 ==  90) side2.pieces[    side_row][       2 - i] = t[j];
    else if (dir2 == 180) side2.pieces[       2 - i][2 - side_row] = t[j];
    else                  side2.pieces[2 - side_row][           i] = t[j];
  }

  for (uint i = 0; i < 3; ++i, ++j) {
    if      (dir3 ==   0) side3.pieces[           i][    side_row] = t[j];
    else if (dir3 ==  90) side3.pieces[    side_row][       2 - i] = t[j];
    else if (dir3 == 180) side3.pieces[       2 - i][2 - side_row] = t[j];
    else                  side3.pieces[2 - side_row][           i] = t[j];
  }

  for (uint i = 0; i < 3; ++i, ++j) {
    if      (dir4 ==   0) side4.pieces[           i][    side_row] = t[j];
    else if (dir4 ==  90) side4.pieces[    side_row][       2 - i] = t[j];
    else if (dir4 == 180) side4.pieces[       2 - i][2 - side_row] = t[j];
    else                  side4.pieces[2 - side_row][           i] = t[j];
  }
}

void
CQRubik::
moveSideDown1(uint side_num, uint side_col)
{
  uint side_num1 = side_num;

  int dir1 = 0, dir2, dir3, dir4;

  int side_num2, side_num3, side_num4;

  side_num2 = sides_[side_num1].side_d.side; dir2 = dir1 + sides_[side_num1].side_d.rotate;

  if (dir2 <= -180) dir2 += 360; else if (dir2 > 180) dir2 -= 360;

  if      (dir2 == 0) {
    side_num3 = sides_[side_num2].side_d.side; dir3 = dir2 + sides_[side_num2].side_d.rotate;
  }
  else if (dir2 == -90) {
    side_num3 = sides_[side_num2].side_l.side; dir3 = dir2 + sides_[side_num2].side_l.rotate;
  }
  else if (dir2 == 90) {
    side_num3 = sides_[side_num2].side_r.side; dir3 = dir2 + sides_[side_num2].side_r.rotate;
  }
  else {
    side_num3 = sides_[side_num2].side_u.side; dir3 = dir2 + sides_[side_num2].side_u.rotate;
  }

  if (dir3 <= -180) dir3 += 360; else if (dir3 > 180) dir3 -= 360;

  if      (dir3 == 0) {
    side_num4 = sides_[side_num3].side_d.side; dir4 = dir3 + sides_[side_num3].side_d.rotate;
  }
  else if (dir3 == -90) {
    side_num4 = sides_[side_num3].side_l.side; dir4 = dir3 + sides_[side_num3].side_l.rotate;
  }
  else if (dir3 == 90) {
    side_num4 = sides_[side_num3].side_r.side; dir4 = dir3 + sides_[side_num3].side_r.rotate;
  }
  else {
    side_num4 = sides_[side_num3].side_u.side; dir4 = dir3 + sides_[side_num3].side_u.rotate;
  }

  if (dir4 <= -180) dir4 += 360; else if (dir4 > 180) dir4 -= 360;

  CRubikSide &side1 = sides_[side_num1];
  CRubikSide &side2 = sides_[side_num2];
  CRubikSide &side3 = sides_[side_num3];
  CRubikSide &side4 = sides_[side_num4];

  CRubikPiece t[15];

  uint j = 0;

  for (uint i = 0; i < 3; ++i, ++j)
    t[j] = side1.pieces[side_col][i];

  for (uint i = 0; i < 3; ++i, ++j) {
    if      (dir2 ==   0) t[j] = side2.pieces[    side_col][           i];
    else if (dir2 ==  90) t[j] = side2.pieces[           i][2 - side_col];
    else if (dir2 == 180) t[j] = side2.pieces[2 - side_col][       2 - i];
    else                  t[j] = side2.pieces[       2 - i][    side_col];
  }

  for (uint i = 0; i < 3; ++i, ++j) {
    if      (dir3 ==   0) t[j] = side3.pieces[    side_col][           i];
    else if (dir3 ==  90) t[j] = side3.pieces[           i][2 - side_col];
    else if (dir3 == 180) t[j] = side3.pieces[2 - side_col][       2 - i];
    else                  t[j] = side3.pieces[       2 - i][    side_col];
  }

  for (uint i = 0; i < 3; ++i, ++j) {
    if      (dir4 ==   0) t[j] = side4.pieces[    side_col][           i];
    else if (dir4 ==  90) t[j] = side4.pieces[           i][2 - side_col];
    else if (dir4 == 180) t[j] = side4.pieces[2 - side_col][       2 - i];
    else                  t[j] = side4.pieces[       2 - i][    side_col];
  }

  //----

  for (int i = 11; i >= 0; --i)
    t[i + 3] = t[i];

  for (uint i = 12; i < 15; ++i)
    t[i - 12] = t[i];

  //----

  j = 0;

  for (uint i = 0; i < 3; ++i, ++j)
    side1.pieces[side_col][i] = t[j];

  for (uint i = 0; i < 3; ++i, ++j) {
    if      (dir2 ==   0) side2.pieces[    side_col][           i] = t[j];
    else if (dir2 ==  90) side2.pieces[           i][2 - side_col] = t[j];
    else if (dir2 == 180) side2.pieces[2 - side_col][       2 - i] = t[j];
    else                  side2.pieces[       2 - i][    side_col] = t[j];
  }

  for (uint i = 0; i < 3; ++i, ++j) {
    if      (dir3 ==   0) side3.pieces[    side_col][           i] = t[j];
    else if (dir3 ==  90) side3.pieces[           i][2 - side_col] = t[j];
    else if (dir3 == 180) side3.pieces[2 - side_col][       2 - i] = t[j];
    else                  side3.pieces[       2 - i][    side_col] = t[j];
  }

  for (uint i = 0; i < 3; ++i, ++j) {
    if      (dir4 ==   0) side4.pieces[    side_col][           i] = t[j];
    else if (dir4 ==  90) side4.pieces[           i][2 - side_col] = t[j];
    else if (dir4 == 180) side4.pieces[2 - side_col][       2 - i] = t[j];
    else                  side4.pieces[       2 - i][    side_col] = t[j];
  }
}

void
CQRubik::
moveSideUp1(uint side_num, uint side_col)
{
  uint side_num1 = side_num;

  int dir1 = 0, dir2, dir3, dir4;

  int side_num2, side_num3, side_num4;

  side_num2 = sides_[side_num1].side_u.side; dir2 = dir1 + sides_[side_num1].side_u.rotate;

  if (dir2 <= -180) dir2 += 360; else if (dir2 > 180) dir2 -= 360;

  if      (dir2 == 0) {
    side_num3 = sides_[side_num2].side_u.side; dir3 = dir2 + sides_[side_num2].side_u.rotate;
  }
  else if (dir2 == 90) {
    side_num3 = sides_[side_num2].side_l.side; dir3 = dir2 + sides_[side_num2].side_l.rotate;
  }
  else if (dir2 == -90) {
    side_num3 = sides_[side_num2].side_r.side; dir3 = dir2 + sides_[side_num2].side_r.rotate;
  }
  else {
    side_num3 = sides_[side_num2].side_d.side; dir3 = dir2 + sides_[side_num2].side_d.rotate;
  }

  if (dir3 <= -180) dir3 += 360; else if (dir3 > 180) dir3 -= 360;

  if      (dir3 == 0) {
    side_num4 = sides_[side_num3].side_u.side; dir4 = dir3 + sides_[side_num3].side_u.rotate;
  }
  else if (dir3 == 90) {
    side_num4 = sides_[side_num3].side_l.side; dir4 = dir3 + sides_[side_num3].side_l.rotate;
  }
  else if (dir3 == -90) {
    side_num4 = sides_[side_num3].side_r.side; dir4 = dir3 + sides_[side_num3].side_r.rotate;
  }
  else {
    side_num4 = sides_[side_num3].side_d.side; dir4 = dir3 + sides_[side_num3].side_d.rotate;
  }

  if (dir4 <= -180) dir4 += 360; else if (dir4 > 180) dir4 -= 360;

  CRubikSide &side1 = sides_[side_num1];
  CRubikSide &side2 = sides_[side_num2];
  CRubikSide &side3 = sides_[side_num3];
  CRubikSide &side4 = sides_[side_num4];

  CRubikPiece t[15];

  uint j = 0;

  for (uint i = 0; i < 3; ++i, ++j)
    t[j] = side1.pieces[side_col][2 - i];

  for (uint i = 0; i < 3; ++i, ++j) {
    if      (dir2 ==   0) t[j] = side2.pieces[    side_col][       2 - i];
    else if (dir2 ==  90) t[j] = side2.pieces[       2 - i][2 - side_col];
    else if (dir2 == 180) t[j] = side2.pieces[2 - side_col][           i];
    else                  t[j] = side2.pieces[           i][    side_col];
  }

  for (uint i = 0; i < 3; ++i, ++j) {
    if      (dir3 ==   0) t[j] = side3.pieces[    side_col][       2 - i];
    else if (dir3 ==  90) t[j] = side3.pieces[       2 - i][2 - side_col];
    else if (dir3 == 180) t[j] = side3.pieces[2 - side_col][           i];
    else                  t[j] = side3.pieces[           i][    side_col];
  }

  for (uint i = 0; i < 3; ++i, ++j) {
    if      (dir4 ==   0) t[j] = side4.pieces[    side_col][       2 - i];
    else if (dir4 ==  90) t[j] = side4.pieces[       2 - i][2 - side_col];
    else if (dir4 == 180) t[j] = side4.pieces[2 - side_col][           i];
    else                  t[j] = side4.pieces[           i][    side_col];
  }

  //----

  for (int i = 11; i >= 0; --i)
    t[i + 3] = t[i];

  for (uint i = 12; i < 15; ++i)
    t[i - 12] = t[i];

  //----

  j = 0;

  for (uint i = 0; i < 3; ++i, ++j)
    side1.pieces[side_col][2 - i] = t[j];

  for (uint i = 0; i < 3; ++i, ++j) {
    if      (dir2 ==   0) side2.pieces[    side_col][       2 - i] = t[j];
    else if (dir2 ==  90) side2.pieces[       2 - i][2 - side_col] = t[j];
    else if (dir2 == 180) side2.pieces[2 - side_col][           i] = t[j];
    else                  side2.pieces[           i][    side_col] = t[j];
  }

  for (uint i = 0; i < 3; ++i, ++j) {
    if      (dir3 ==   0) side3.pieces[    side_col][       2 - i] = t[j];
    else if (dir3 ==  90) side3.pieces[       2 - i][2 - side_col] = t[j];
    else if (dir3 == 180) side3.pieces[2 - side_col][           i] = t[j];
    else                  side3.pieces[           i][    side_col] = t[j];
  }

  for (uint i = 0; i < 3; ++i, ++j) {
    if      (dir4 ==   0) side4.pieces[    side_col][       2 - i] = t[j];
    else if (dir4 ==  90) side4.pieces[       2 - i][2 - side_col] = t[j];
    else if (dir4 == 180) side4.pieces[2 - side_col][           i] = t[j];
    else                  side4.pieces[           i][    side_col] = t[j];
  }
}

void
CQRubik::
rotateSide1(uint side_num, bool clockwise)
{
  CRubikSide &side = sides_[side_num];

  CRubikPiece t;

  if (! clockwise) {
    t                 = side.pieces[2][1];
    side.pieces[2][1] = side.pieces[1][0];
    side.pieces[1][0] = side.pieces[0][1];
    side.pieces[0][1] = side.pieces[1][2];
    side.pieces[1][2] = t;

    t                 = side.pieces[2][0];
    side.pieces[2][0] = side.pieces[0][0];
    side.pieces[0][0] = side.pieces[0][2];
    side.pieces[0][2] = side.pieces[2][2];
    side.pieces[2][2] = t;
  }
  else {
    t                 = side.pieces[2][1];
    side.pieces[2][1] = side.pieces[1][2];
    side.pieces[1][2] = side.pieces[0][1];
    side.pieces[0][1] = side.pieces[1][0];
    side.pieces[1][0] = t;

    t                 = side.pieces[2][0];
    side.pieces[2][0] = side.pieces[2][2];
    side.pieces[2][2] = side.pieces[0][2];
    side.pieces[0][2] = side.pieces[0][0];
    side.pieces[0][0] = t;
  }
}

QColor
CQRubik::
getColor(const CRubikPiece &piece)
{
  if (getShade()) {
    QColor c = colors_[piece.side];

    double f = 0.75 + 0.25*(piece.id + 1)/9.0 ;

    return QColor(c.red()*f, c.green()*f, c.blue()*f);
  }
  else
    return colors_[piece.side];
}

QColor
CQRubik::
getColor(uint value)
{
  return colors_[value];
}

bool
CQRubik::
validate()
{
  for (uint i = 0; i < CUBE_SIDES; ++i) {
    CRubikSide &side = sides_[i];

    for (uint j = 0; j < SIDE_PIECES; ++j) {
      uint ix = j % SIDE_LENGTH;
      uint iy = j / SIDE_LENGTH;

      CRubikPiece &piece = side.pieces[ix][iy];

      uint side_num1, side_col1, side_row1;
      int  dir1;

      if      (ix == 0)
        getPosLeft (i, ix, iy, 0, side_num1, side_col1, side_row1, dir1);
      else if (ix == 2)
        getPosRight(i, ix, iy, 0, side_num1, side_col1, side_row1, dir1);
      else
        return true;

      uint side_num2, side_col2, side_row2;
      int  dir2;

      if      (iy == 0)
        getPosUp  (i, ix, iy, 0, side_num2, side_col2, side_row2, dir2);
      else if (iy == 2)
        getPosDown(i, ix, iy, 0, side_num2, side_col2, side_row2, dir2);
      else
        return true;

      CRubikSide &side1 = sides_[side_num1];
      CRubikSide &side2 = sides_[side_num2];

      CRubikPiece &piece1 = side1.pieces[side_col1][side_row1];
      CRubikPiece &piece2 = side2.pieces[side_col2][side_row2];

      uint id1 = piece1.id;
      uint id2 = piece2.id;

      if (id1 > id2) std::swap(id1, id2);

      // validate white side
      if      (piece.side == 0) {
        if      (piece.id == 0) {
          if (id1 != 0 || id2 != 6) {
            std::cerr << "Trap: " << i << ":" << ix << "," << iy << std::endl;
            return false;
          }
        }
        else if (piece.id == 2) {
          if (id1 != 2 || id2 != 8) {
            std::cerr << "Trap: " << i << ":" << ix << "," << iy << std::endl;
            return false;
          }
        }
        else if (piece.id == 6) {
          if (id1 != 0 || id2 != 2) {
            std::cerr << "Trap: " << i << ":" << ix << "," << iy << std::endl;
            return false;
          }
        }
        else if (piece.id == 8) {
          if (id1 != 0 || id2 != 2) {
            std::cerr << "Trap: " << i << ":" << ix << "," << iy << std::endl;
            return false;
          }
        }
      }
      // validate red side
      else if (piece.side == 1) {
        if      (piece.id == 0) {
          if (id1 != 0 || id2 != 6) {
            std::cerr << "Trap: " << i << ":" << ix << "," << iy << std::endl;
            return false;
          }
        }
        else if (piece.id == 2) {
          if (id1 != 0 || id2 != 6) {
            std::cerr << "Trap: " << i << ":" << ix << "," << iy << std::endl;
            return false;
          }
        }
        else if (piece.id == 6) {
          if (id1 != 0 || id2 != 6) {
            std::cerr << "Trap: " << i << ":" << ix << "," << iy << std::endl;
            return false;
          }
        }
        else if (piece.id == 8) {
          if (id1 != 0 || id2 != 6) {
            std::cerr << "Trap: " << i << ":" << ix << "," << iy << std::endl;
            return false;
          }
        }
      }
      // validate green side
      else if (piece.side == 2) {
        if      (piece.id == 0) {
          if (id1 != 2 || id2 != 6) {
            std::cerr << "Trap: " << i << ":" << ix << "," << iy << std::endl;
            return false;
          }
        }
        else if (piece.id == 2) {
          if (id1 != 0 || id2 != 8) {
            std::cerr << "Trap: " << i << ":" << ix << "," << iy << std::endl;
            return false;
          }
        }
        else if (piece.id == 6) {
          if (id1 != 0 || id2 != 8) {
            std::cerr << "Trap: " << i << ":" << ix << "," << iy << std::endl;
            return false;
          }
        }
        else if (piece.id == 8) {
          if (id1 != 2 || id2 != 6) {
            std::cerr << "Trap: " << i << ":" << ix << "," << iy << std::endl;
            return false;
          }
        }
      }
      // validate yellow side
      else if (piece.side == 4) {
        if      (piece.id == 0) {
          if (id1 != 6 || id2 != 8) {
            std::cerr << "Trap: " << i << ":" << ix << "," << iy << std::endl;
            return false;
          }
        }
        else if (piece.id == 2) {
          if (id1 != 6 || id2 != 8) {
            std::cerr << "Trap: " << i << ":" << ix << "," << iy << std::endl;
            return false;
          }
        }
        else if (piece.id == 6) {
          if (id1 != 0 || id2 != 6) {
            std::cerr << "Trap: " << i << ":" << ix << "," << iy << std::endl;
            return false;
          }
        }
        else if (piece.id == 8) {
          if (id1 != 2 || id2 != 8) {
            std::cerr << "Trap: " << i << ":" << ix << "," << iy << std::endl;
            return false;
          }
        }
      }
      // validate blue side
      else if (piece.side == 5) {
        if      (piece.id == 0) {
          if (id1 != 6 || id2 != 6) {
            std::cerr << "Trap: " << i << ":" << ix << "," << iy << std::endl;
            return false;
          }
        }
        else if (piece.id == 2) {
          if (id1 != 8 || id2 != 8) {
            std::cerr << "Trap: " << i << ":" << ix << "," << iy << std::endl;
            return false;
          }
        }
        else if (piece.id == 6) {
          if (id1 != 0 || id2 != 0) {
            std::cerr << "Trap: " << i << ":" << ix << "," << iy << std::endl;
            return false;
          }
        }
        else if (piece.id == 8) {
          if (id1 != 2 || id2 != 2) {
            std::cerr << "Trap: " << i << ":" << ix << "," << iy << std::endl;
            return false;
          }
        }
      }
    }
  }

  return true;
}

//------

CQRubik2D::
CQRubik2D(CQRubik *rubik) :
 QWidget(rubik), rubik_(rubik)
{
  setFocusPolicy(Qt::StrongFocus);
}

void
CQRubik2D::
paintEvent(QPaintEvent *)
{
  QPainter p(this);

  p.fillRect(rect(), QColor(180,180,180));

  p.setRenderHint(QPainter::Antialiasing, true);

  for (uint i = 0; i < CQRubik::CUBE_SIDES; ++i)
    drawSide(&p, i);

  if (rubik_->getValidate()) {
    const CRubikPieceInd &ind = rubik_->getInd();
    int                   dir = rubik_->getDir();

    int h = height();

    QString valid_str = (rubik_->validate() ? "Valid" : "Invalid");

    p.setPen(QColor(0,0,0));

    p.drawText(20, h - 20, QString("%1) %2,%3 : %4 - %5").
               arg(ind.side_num).arg(ind.side_col).
               arg(ind.side_row).arg(dir).arg(valid_str));
  }

  CQRubikAnimateData &animateData = rubik_->getAnimateData();

  if (animateData.animating)
    drawAnimation(&p, &animateData);
}

void
CQRubik2D::
drawSide(QPainter *p, uint i)
{
  static int x_pos[] = { 0, 1, 1, 1, 2, 3 };
  static int y_pos[] = { 1, 0, 1, 2, 1, 1 };

  const CRubikPieceInd &ind = rubik_->getInd();

  int w = width ();
  int h = height();

  int b  = std::min(w, h)/10;
  int ds = std::min((w - b)/DRAW_COLUMNS, (h - b)/DRAW_ROWS);
  int dp = ds/CQRubik::SIDE_LENGTH;

  int dy = 0;

  const CRubikSide &side = rubik_->getSide(i);

  int x = x_pos[i];
  int y = y_pos[i];

  side.r = QRect(x*ds + b/2, y*ds + b/2 + dy, ds, ds);

  for (uint j = 0; j < CQRubik::SIDE_PIECES; ++j) {
    uint ix = j % CQRubik::SIDE_LENGTH;
    uint iy = j / CQRubik::SIDE_LENGTH;

    const CRubikPiece &piece = side.pieces[ix][iy];

    int xo = x*ds + ix*dp + b/2;
    int yo = y*ds + iy*dp + b/2 + dy;

    piece.r = QRect(xo, yo, dp, dp);

    QColor c = rubik_->getColor(piece);

    p->setPen(QColor(0,0,0));
    p->setBrush(c);

    p->drawRect(piece.r);

    if (ind.side_num == i && ind.side_col == ix && ind.side_row == iy) {
      int xc = xo + dp/2;
      int yc = yo + dp/2;

      int dp1 = dp/3;

      p->setBrush(QColor(0,0,0));

      p->drawRect(xc - dp1/2, yc - dp1/2, dp1, dp1);
    }

    if (rubik_->getNumber()) {
      int xc = xo + dp/2;
      int yc = yo + dp/2;

      QColor c = rubik_->getColor(side.side_d.side);

      p->setPen(QColor(0,0,0));
      p->setBrush(c);

      p->drawText(xc, yc, QString("%1").arg(piece.id));
    }
  }
}

void
CQRubik2D::
drawAnimation(QPainter *p, CQRubikAnimateData *animateData)
{
  static int x_pos[] = { 0, 1, 1, 1, 2, 3 };
  static int y_pos[] = { 1, 0, 1, 2, 1, 1 };

  if (animateData->axis) return;

  int w = width ();
  int h = height();

  int b  = std::min(w, h)/10;
  int ds = std::min((w - b)/DRAW_COLUMNS, (h - b)/DRAW_ROWS);
  int dp = ds/CQRubik::SIDE_LENGTH;

  int dy = 0;

  const CRubikSide &side = rubik_->getSide(animateData->side_num);

  int x = x_pos[animateData->side_num];
  int y = y_pos[animateData->side_num];

  QRect r(x*ds + b/2, y*ds + b/2 + dy, ds, ds);

  double x1 = r.left  ();
  double y1 = r.bottom();
  double x2 = r.right ();
  double y2 = r.top   ();

  double xc = (x1 + x2)/2;
  double yc = (y1 + y2)/2;

  uint   num = 10;
  double da  = 90/num;

  if (animateData->clockwise) da = -da;

  double a = da*animateData->step;

  CMatrix2D m1, m2, m3;

  double ra = a*M_PI/180.0;

  m1.setTranslation(-xc, -yc);
  m2.setRotation(ra);
  m3.setTranslation( xc,  yc);

  CMatrix2D m = m3*m2*m1;

  for (uint j = 0; j < CQRubik::SIDE_PIECES; ++j) {
    uint ix = j % CQRubik::SIDE_LENGTH;
    uint iy = j / CQRubik::SIDE_LENGTH;

    const CRubikPiece &piece = side.pieces[ix][iy];

    int xo = x*ds + ix*dp + b/2;
    int yo = y*ds + iy*dp + b/2 + dy;

    QRect r(xo, yo, dp, dp);

    QColor c = rubik_->getColor(piece);

    p->setPen(QColor(0,0,0));
    p->setBrush(c);

    double x1 = r.left  ();
    double y1 = r.bottom();
    double x2 = r.right ();
    double y2 = r.top   ();

    double rx11, ry11, rx12, ry12, rx21, ry21, rx22, ry22;

    m.multiplyPoint(x1, y1, &rx11, &ry11);
    m.multiplyPoint(x2, y1, &rx12, &ry12);
    m.multiplyPoint(x2, y2, &rx22, &ry22);
    m.multiplyPoint(x1, y2, &rx21, &ry21);

    animateData->polygon.resize(4);

    animateData->polygon[0] = QPointF(rx11, ry11);
    animateData->polygon[1] = QPointF(rx12, ry12);
    animateData->polygon[2] = QPointF(rx22, ry22);
    animateData->polygon[3] = QPointF(rx21, ry21);

    p->drawPolygon(animateData->polygon);
  }
}

void
CQRubik2D::
mousePressEvent(QMouseEvent *)
{
}

void
CQRubik2D::
keyPressEvent(QKeyEvent *e)
{
  rubik_->keyPressEvent(e);
}

//------

CQRubik3D::
CQRubik3D(CQRubik *rubik) :
 QGLWidget(rubik), rubik_(rubik), showTexture_(false)
{
  setFocusPolicy(Qt::StrongFocus);

  control_ = new CQGLControl(this);

  control_->setDepthTest  (true);
  control_->setSmoothShade(true);

  texture_ = new CGLTexture;

  CImageFileSrc src("textures/EscherCubeFish_2048.gif");
//CImageFileSrc src("textures/Cubetexture.jpg");

  CImagePtr image = CImageMgrInst->createImage(src);

  image->convertToRGB();

  texture_->setImage(image);

  //tId_ = bindTexture(dynamic_cast<CQImage *>(image.get())->getQImage());

  //toolbar_ = threed_->createToolBar();
}

CQGLControlToolBar *
CQRubik3D::
createToolBar()
{
  CQGLControlToolBar *toolbar = control_->createToolBar();

  toolbar->setParent(this);

  return toolbar;
}

void
CQRubik3D::
toggleTexture()
{
  showTexture_ = ! showTexture_;
}

void
CQRubik3D::
paintGL()
{
  static GLfloat pos[4] = {5.0, 5.0, 10.0, 10.0};

  static float lambient[4] = {0.2, 0.2, 0.2, 1.0};
  static float ldiffuse[4] = {0.4, 0.4, 0.4, 1.0};

  glLightfv(GL_LIGHT0, GL_POSITION, pos);
  glEnable(GL_LIGHT0);

  glEnable(GL_COLOR_MATERIAL);
  glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

  glLightfv(GL_LIGHT0, GL_AMBIENT, lambient);
  glLightfv(GL_LIGHT0, GL_DIFFUSE, ldiffuse);

  //glEnable(GL_NORMALIZE);

  control_->getDepthTest  () ? glEnable(GL_DEPTH_TEST) : glDisable(GL_DEPTH_TEST);
  control_->getCullFace   () ? glEnable(GL_CULL_FACE)  : glDisable(GL_CULL_FACE);
  control_->getLighting   () ? glEnable(GL_LIGHTING)   : glDisable(GL_LIGHTING);
  control_->getOutline    () ? glPolygonMode(GL_FRONT_AND_BACK, GL_LINE) :
                               glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  control_->getFrontFace  () ? glFrontFace(GL_CW) : glFrontFace(GL_CCW);
  control_->getSmoothShade() ? glShadeModel(GL_SMOOTH) : glShadeModel(GL_FLAT);

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  bool b1[] = { 0, 1, 0, 0, 1, 0, 0, 1, 0 };
  bool b2[] = { 0, 0, 0, 0, 1, 0, 0, 0, 0 };
  bool b3[] = { 0, 0, 0, 1, 1, 1, 0, 0, 0 };

  CQRubikAnimateData &animateData = rubik_->getAnimateData();

  if (! animateData.animating) {
    drawSideCubes(0); // Left
    drawSideCubes(4); // Right

    drawSideCubes(1, b1); // Top
    drawSideCubes(3, b1); // Bottom

    drawSideCubes(2, b2); // Front
    drawSideCubes(5, b2); // Back
  }
  else {
    if (! animateData.axis) {
      if      (animateData.side_num == 0 || animateData.side_num == 4) {
        drawSideCubes(0, NULL, animateData.side_num == 0 ? &animateData : NULL); // Left
        drawSideCubes(4, NULL, animateData.side_num == 4 ? &animateData : NULL); // Right

        drawSideCubes(1, b1, NULL); // Top
        drawSideCubes(3, b1, NULL); // Bottom

        drawSideCubes(2, b2, NULL); // Front
        drawSideCubes(5, b2, NULL); // Back
      }
      else if (animateData.side_num == 1 || animateData.side_num == 3) {
        drawSideCubes(1, NULL, animateData.side_num == 1 ? &animateData : NULL); // Top
        drawSideCubes(3, NULL, animateData.side_num == 3 ? &animateData : NULL); // Bottom

        drawSideCubes(0, b1, NULL); // Left
        drawSideCubes(4, b1, NULL); // Right

        drawSideCubes(2, b2, NULL); // Front
        drawSideCubes(5, b2, NULL); // Back
      }
      else {
        drawSideCubes(2, NULL, animateData.side_num == 2 ? &animateData : NULL); // Front
        drawSideCubes(5, NULL, animateData.side_num == 5 ? &animateData : NULL); // Back

        drawSideCubes(0, b3, NULL); // Left
        drawSideCubes(4, b3, NULL); // Right

        drawSideCubes(1, b2, NULL); // Top
        drawSideCubes(3, b2, NULL); // Bottom
      }
    }
    else {
      if      (animateData.side_num == 0) {
        drawSideCubes(0, b1, &animateData);
        drawSideCubes(4, b1, &animateData);
        drawSideCubes(2, b2, &animateData);
        drawSideCubes(5, b2, &animateData);

        drawSideCubes(1); // Top
        drawSideCubes(3); // Bottom
      }
      else if (animateData.side_num == 1) {
        drawSideCubes(0, b3, &animateData);
        drawSideCubes(4, b3, &animateData);
        drawSideCubes(1, b2, &animateData);
        drawSideCubes(3, b2, &animateData);

        drawSideCubes(2); // Front
        drawSideCubes(5); // Back
      }
      else if (animateData.side_num == 2) {
        drawSideCubes(1, b1, &animateData);
        drawSideCubes(3, b1, &animateData);
        drawSideCubes(2, b2, &animateData);
        drawSideCubes(5, b2, &animateData);

        drawSideCubes(0); // Left
        drawSideCubes(4); // Right
      }
    }
  }
}

void
CQRubik3D::
drawSideCubes(uint i, bool *b, CQRubikAnimateData *animateData)
{
  static double x1_pos[] = { -1, -1, -1, -1,  1,  1 };
  static double y1_pos[] = {  1,  1,  1, -1,  1,  1 };
  static double z1_pos[] = { -1, -1,  1,  1,  1, -1 };
  static double x2_pos[] = { -1,  1,  1,  1,  1, -1 };
  static double y2_pos[] = { -1,  1, -1, -1, -1, -1 };
  static double z2_pos[] = {  1,  1,  1, -1, -1, -1 };
  static bool   flip[]   = {  1,  0,  0,  0,  1,  0 };

  double gap = 0.02;

  const CRubikSide &side = rubik_->getSide(i);

  double x1 = x1_pos[i], y1 = y1_pos[i], z1 = z1_pos[i];
  double x2 = x2_pos[i], y2 = y2_pos[i], z2 = z2_pos[i];

  double dx = x2 - x1;
  double dy = y2 - y1;
  double dz = z2 - z1;

  double dsx = dx/CQRubik::SIDE_LENGTH;
  double dsy = dy/CQRubik::SIDE_LENGTH;
  double dsz = dz/CQRubik::SIDE_LENGTH;

  CMatrix3D m;

  if (animateData) {
    double xc = (x1 + x2)/2.0;
    double yc = (y1 + y2)/2.0;
    double zc = (z1 + z2)/2.0;

    uint   num = 10;
    double da  = 90/num;

    if (! animateData->axis) {
      if (i == 0 || i == 3 || i == 5) {
        if (! animateData->clockwise) da = -da;
      }
      else {
        if (  animateData->clockwise) da = -da;
      }
    }
    else {
      if (animateData->side_num == 0) {
        if (  animateData->clockwise) da = -da;
      }
      else {
        if (! animateData->clockwise) da = -da;
      }
    }

    double a = da*animateData->step;

    double ra = a*M_PI/180.0;

    CMatrix3D m1, m2, m3;

    if (! animateData->axis) {
      m1.setTranslation(-xc, -yc, -zc);

      if      (fabs(dsx) < 1E-6)
        m2.setRotation(CMathGen::X_AXIS_3D, ra);
      else if (fabs(dsy) < 1E-6)
        m2.setRotation(CMathGen::Y_AXIS_3D, ra);
      else
        m2.setRotation(CMathGen::Z_AXIS_3D, ra);

      m3.setTranslation(xc, yc, zc);

      m = m3*m2*m1;
    }
    else {
      if      (animateData->side_num == 0)
        m2.setRotation(CMathGen::Y_AXIS_3D, ra);
      else if (animateData->side_num == 1)
        m2.setRotation(CMathGen::Z_AXIS_3D, ra);
      else
        m2.setRotation(CMathGen::X_AXIS_3D, ra);

      m = m2;
    }
  }
  else
    m.setIdentity();

  for (uint j = 0; j < CQRubik::SIDE_PIECES; ++j) {
    if (b && ! b[j]) continue;

    uint ix = j % CQRubik::SIDE_LENGTH;
    uint iy = j / CQRubik::SIDE_LENGTH;

    double xc = 0.0, yc = 0.0, zc = 0.0, s = 0.0;

    if      (fabs(dx) < 1E-6) {
      s = fabs(dsz);

      xc = x1 +          (i == 0 ? s/2 : -s/2);
      yc = y1 + ix*dsy + dsy/2;
      zc = z1 + iy*dsz + dsz/2;
    }
    else if (fabs(dy) < 1E-6) {
      s = fabs(dsx);

      xc = x1 + ix*dsx + dsx/2;
      yc = y1 +          (i == 1 ? -s/2 : s/2);
      zc = z1 + iy*dsz + dsz/2;
    }
    else if (fabs(dz) < 1E-6) {
      s = fabs(dsx);

      xc = x1 + ix*dsx + dsx/2;
      yc = y1 + iy*dsy + dsy/2;
      zc = z1 +          (i == 2 ? -s/2 : s/2);
    }

    if (flip[i]) { std::swap(ix, iy); }

    const CRubikPiece &piece = side.pieces[ix][iy];

    const CRubikPiece &piece1 = rubik_->getPieceLeft (i, ix, iy);
    const CRubikPiece &piece3 = rubik_->getPieceUp   (i, ix, iy);
    const CRubikPiece &piece5 = rubik_->getPieceDown (i, ix, iy);
    const CRubikPiece &piece7 = rubik_->getPieceRight(i, ix, iy);

    QColor c1 = (ix == 0 ? rubik_->getColor(piece1) : QColor(100,100,100));
    QColor c3 = (iy == 0 ? rubik_->getColor(piece3) : QColor(100,100,100));
    QColor c5 = (iy == 2 ? rubik_->getColor(piece5) : QColor(100,100,100));
    QColor c7 = (ix == 2 ? rubik_->getColor(piece7) : QColor(100,100,100));

    QColor c[6];

    if      (i == 0) {
      c[0] = rubik_->getColor(piece);
      c[1] = c3;
      c[2] = QColor(100,100,100);
      c[3] = c5;
      c[4] = c7;
      c[5] = c1;
    }
    else if (i == 1) {
      c[0] = c1;
      c[1] = rubik_->getColor(piece);
      c[2] = c7;
      c[3] = QColor(100,100,100);
      c[4] = c5;
      c[5] = c3;
    }
    else if (i == 2) {
      c[0] = c1;
      c[1] = c3;
      c[2] = c7;
      c[3] = c5;
      c[4] = rubik_->getColor(piece);
      c[5] = QColor(100,100,100);
    }
    else if (i == 3) {
      c[0] = c1;
      c[1] = QColor(100,100,100);
      c[2] = c7;
      c[3] = rubik_->getColor(piece);
      c[4] = c3;
      c[5] = c5;
    }
    else if (i == 4) {
      c[0] = QColor(100,100,100);
      c[1] = c3;
      c[2] = rubik_->getColor(piece);
      c[3] = c5;
      c[4] = c1;
      c[5] = c7;
    }
    else {
      c[0] = c7;
      c[1] = c3;
      c[2] = c1;
      c[3] = c5;
      c[4] = QColor(100,100,100);
      c[5] = rubik_->getColor(piece);
    }

    drawCube(xc, yc, zc, s - 2*gap, c, m);
  }
}

void
CQRubik3D::
drawCube(double xc, double yc, double zc, double size, const QColor *c, const CMatrix3D &m)
{
  static GLfloat cube_normal[6][3] = {
    {-1.0,  0.0,  0.0},
    { 0.0,  1.0,  0.0},
    { 1.0,  0.0,  0.0},
    { 0.0, -1.0,  0.0},
    { 0.0,  0.0,  1.0},
    { 0.0,  0.0, -1.0}
  };

  static GLint cube_faces[6][4] = {
    {0, 1, 2, 3},
    {3, 2, 6, 7},
    {7, 6, 5, 4},
    {4, 5, 1, 0},
    {5, 6, 2, 1},
    {7, 4, 0, 3}
  };

  GLfloat v[8][3];

  v[0][0] = v[1][0] = v[2][0] = v[3][0] = -size / 2;
  v[4][0] = v[5][0] = v[6][0] = v[7][0] =  size / 2;
  v[0][1] = v[1][1] = v[4][1] = v[5][1] = -size / 2;
  v[2][1] = v[3][1] = v[6][1] = v[7][1] =  size / 2;
  v[0][2] = v[3][2] = v[4][2] = v[7][2] = -size / 2;
  v[1][2] = v[2][2] = v[5][2] = v[6][2] =  size / 2;

  for (GLint i = 5; i >= 0; i--) {
    const QColor c1 = c[i];

    double r = c1.red  ()/255.0;
    double g = c1.green()/255.0;
    double b = c1.blue ()/255.0;

    glColor3d(r, g, b);

    glBegin(GL_POLYGON);

    glNormal3fv(&cube_normal[i][0]);

    double x1, y1, z1, x2, y2, z2, x3, y3, z3, x4, y4, z4;

    m.multiplyPoint(v[cube_faces[i][0]][0] + xc,
                    v[cube_faces[i][0]][1] + yc,
                    v[cube_faces[i][0]][2] + zc,
                    &x1, &y1, &z1);
    m.multiplyPoint(v[cube_faces[i][1]][0] + xc,
                    v[cube_faces[i][1]][1] + yc,
                    v[cube_faces[i][1]][2] + zc,
                    &x2, &y2, &z2);
    m.multiplyPoint(v[cube_faces[i][2]][0] + xc,
                    v[cube_faces[i][2]][1] + yc,
                    v[cube_faces[i][2]][2] + zc,
                    &x3, &y3, &z3);
    m.multiplyPoint(v[cube_faces[i][3]][0] + xc,
                    v[cube_faces[i][3]][1] + yc,
                    v[cube_faces[i][3]][2] + zc,
                    &x4, &y4, &z4);

    glVertex3f(x1, y1, z1);
    glVertex3f(x2, y2, z2);
    glVertex3f(x3, y3, z3);
    glVertex3f(x4, y4, z4);

    glEnd();
  }
}

void
CQRubik3D::
drawSide(uint i)
{
  static double x1_pos[] = { -1, -1, -1, -1,  1,  1 };
  static double y1_pos[] = {  1,  1,  1, -1,  1,  1 };
  static double z1_pos[] = { -1, -1,  1,  1,  1, -1 };
  static double x2_pos[] = { -1,  1,  1,  1,  1, -1 };
  static double y2_pos[] = { -1,  1, -1, -1, -1, -1 };
  static double z2_pos[] = {  1,  1,  1, -1, -1, -1 };
  static bool   flip[]   = {  1,  0,  0,  0,  1,  0 };

  static double tx1_pos[] = {  0.00, 0.33, 0.33, 0.33, 0.66, 0.66 };
  static double ty1_pos[] = {  0.33, 0.66, 0.33, 0.00, 0.33, 0.00 };
  static double tx2_pos[] = {  0.33, 0.66, 0.66, 0.66, 1.00, 1.00 };
  static double ty2_pos[] = {  0.66, 1.00, 0.66, 0.33, 0.66, 0.33 };

  double gap = 0.02;

  const CRubikSide &side = rubik_->getSide(i);

  double x1 = x1_pos[i], y1 = y1_pos[i], z1 = z1_pos[i];
  double x2 = x2_pos[i], y2 = y2_pos[i], z2 = z2_pos[i];

  double dx = x2 - x1;
  double dy = y2 - y1;
  double dz = z2 - z1;

  double dsx = dx/CQRubik::SIDE_LENGTH;
  double dsy = dy/CQRubik::SIDE_LENGTH;
  double dsz = dz/CQRubik::SIDE_LENGTH;

  double tx1 = tx1_pos[i], ty1 = ty1_pos[i];
  double tx2 = tx2_pos[i], ty2 = ty2_pos[i];

  double dxt = (tx2 - tx1)/CQRubik::SIDE_LENGTH;
  double dyt = (ty2 - ty1)/CQRubik::SIDE_LENGTH;

  for (uint j = 0; j < CQRubik::SIDE_PIECES; ++j) {
    uint ix = j % CQRubik::SIDE_LENGTH;
    uint iy = j / CQRubik::SIDE_LENGTH;

    double xo1, yo1, zo1, xo2, yo2, zo2;
    double txo1, tyo1, txo2, tyo2;

    if      (fabs(dsx) < 1E-6) {
      xo1 = x1; yo1 = y1 + ix*dsy + gap*dsy; zo1 = z1 + iy*dsz + gap*dsz;
      xo2 = x1; yo2 = yo1 + dsy - 2*gap*dsy; zo2 = zo1 + dsz - 2*gap*dsz;
    }
    else if (fabs(dsy) < 1E-6) {
      xo1 = x1 + ix*dsx + gap*dsx; yo1 = y1; zo1 = z1 + iy*dsz + gap*dsz;
      xo2 = xo1 + dsx - 2*gap*dsx; yo2 = y1; zo2 = zo1 + dsz - 2*gap*dsz;
    }
    else if (fabs(dsz) < 1E-6) {
      xo1 = x1 + ix*dsx + gap*dsx; yo1 = y1 + iy*dsy + gap*dsy; zo1 = z1;
      xo2 = xo1 + dsx - 2*gap*dsx; yo2 = yo1 + dsy - 2*gap*dsy; zo2 = z1;
    }

    txo1 = tx1 + ix*dxt; tyo1 = ty1 + iy*dyt;
    txo2 = txo1 + dxt  ; tyo2 = tyo1 + dyt  ;

    if (flip[i]) { std::swap(ix, iy); }

    if (! showTexture_) {
      QColor c = rubik_->getColor(side.pieces[ix][iy]);

      double r = c.red  ()/255.0;
      double g = c.green()/255.0;
      double b = c.blue ()/255.0;

      glColor3d(r, g, b);

      glBegin(GL_POLYGON);

      if      (fabs(dsx) < 1E-6) {
        glVertex3d(xo1, yo1, zo1);
        glVertex3d(xo1, yo2, zo1);
        glVertex3d(xo1, yo2, zo2);
        glVertex3d(xo1, yo1, zo2);
      }
      else if (fabs(dsy) < 1E-6) {
        glVertex3d(xo1, yo1, zo1);
        glVertex3d(xo2, yo1, zo1);
        glVertex3d(xo2, yo1, zo2);
        glVertex3d(xo1, yo1, zo2);
      }
      else if (fabs(dsz) < 1E-6) {
        glVertex3d(xo1, yo1, zo1);
        glVertex3d(xo2, yo1, zo1);
        glVertex3d(xo2, yo2, zo1);
        glVertex3d(xo1, yo2, zo1);
      }

      glEnd();
    }
    else {
      glColor4d(1.0, 1.0, 1.0, 1.0);

      if      (fabs(dsx) < 1E-6) {
        texture_->draw(xo1, yo1, zo1, xo1, yo2, zo2, txo1, tyo1, txo2, tyo2);
      }
      else if (fabs(dsy) < 1E-6) {
        texture_->draw(xo1, yo1, zo1, xo2, yo1, zo2, txo1, tyo1, txo2, tyo2);
      }
      else if (fabs(dsz) < 1E-6) {
        texture_->draw(xo1, yo1, zo1, xo2, yo2, zo1, txo1, tyo1, txo2, tyo2);
      }
    }
  }

  if (! showTexture_) {
    glColor3d(0.4, 0.4, 0.4);

    glBegin(GL_POLYGON);

    if      (fabs(dsx) < 1E-6) {
      double dx1 = (x1 > 0 ? -gap : gap);

      glVertex3d(x1 + dx1, y1, z1);
      glVertex3d(x1 + dx1, y2, z1);
      glVertex3d(x1 + dx1, y2, z2);
      glVertex3d(x1 + dx1, y1, z2);
    }
    else if (fabs(dsy) < 1E-6) {
      double dy1 = (y1 > 0 ? -gap : gap);

      glVertex3d(x1, y1 + dy1, z1);
      glVertex3d(x2, y1 + dy1, z1);
      glVertex3d(x2, y1 + dy1, z2);
      glVertex3d(x1, y1 + dy1, z2);
    }
    else if (fabs(dsz) < 1E-6) {
      double dz1 = (z1 > 0 ? -gap : gap);

      glVertex3d(x1, y1, z1 + dz1);
      glVertex3d(x2, y1, z1 + dz1);
      glVertex3d(x2, y2, z1 + dz1);
      glVertex3d(x1, y2, z1 + dz1);
    }

    glEnd();
  }
}

void
CQRubik3D::
animateSide(CQRubikAnimateData *animateData)
{
  static double x1_pos[] = { -1, -1, -1, -1,  1,  1 };
  static double y1_pos[] = {  1,  1,  1, -1,  1,  1 };
  static double z1_pos[] = { -1, -1,  1,  1,  1, -1 };
  static double x2_pos[] = { -1,  1,  1,  1,  1, -1 };
  static double y2_pos[] = { -1,  1, -1, -1, -1, -1 };
  static double z2_pos[] = {  1,  1,  1, -1, -1, -1 };
  static bool   flip[]   = {  1,  0,  0,  0,  1,  0 };

  static double tx1_pos[] = {  0.00, 0.33, 0.33, 0.33, 0.66, 0.66 };
  static double ty1_pos[] = {  0.33, 0.66, 0.33, 0.00, 0.33, 0.00 };
  static double tx2_pos[] = {  0.33, 0.66, 0.66, 0.66, 1.00, 1.00 };
  static double ty2_pos[] = {  0.66, 1.00, 0.66, 0.33, 0.66, 0.33 };

  double gap = 0.02;

  const CRubikSide &side = rubik_->getSide(animateData->side_num);

  double x1 = x1_pos[animateData->side_num];
  double y1 = y1_pos[animateData->side_num];
  double z1 = z1_pos[animateData->side_num];
  double x2 = x2_pos[animateData->side_num];
  double y2 = y2_pos[animateData->side_num];
  double z2 = z2_pos[animateData->side_num];

  double dx = x2 - x1;
  double dy = y2 - y1;
  double dz = z2 - z1;

  double dsx = dx/CQRubik::SIDE_LENGTH;
  double dsy = dy/CQRubik::SIDE_LENGTH;
  double dsz = dz/CQRubik::SIDE_LENGTH;

  double xc = (x1 + x2)/2.0;
  double yc = (y1 + y2)/2.0;
  double zc = (z1 + z2)/2.0;

  uint   num = 10;
  double da  = 90/num;

  if (animateData->clockwise) da = -da;

  double a = da*animateData->step;

  CMatrix3D m1, m2, m3;

  double ra = a*M_PI/180.0;

  m1.setTranslation(-xc, -yc, -zc);

  if      (fabs(dsx) < 1E-6)
    m2.setRotation(CMathGen::X_AXIS_3D, ra);
  else if (fabs(dsy) < 1E-6)
    m2.setRotation(CMathGen::Y_AXIS_3D, ra);
  else
    m2.setRotation(CMathGen::Z_AXIS_3D, ra);

  m3.setTranslation(xc, yc, zc);

  CMatrix3D m = m3*m2*m1;

  double tx1 = tx1_pos[animateData->side_num], ty1 = ty1_pos[animateData->side_num];
  double tx2 = tx2_pos[animateData->side_num], ty2 = ty2_pos[animateData->side_num];

  double dxt = (tx2 - tx1)/CQRubik::SIDE_LENGTH;
  double dyt = (ty2 - ty1)/CQRubik::SIDE_LENGTH;

  for (uint j = 0; j < CQRubik::SIDE_PIECES; ++j) {
    uint ix = j % CQRubik::SIDE_LENGTH;
    uint iy = j / CQRubik::SIDE_LENGTH;

    const CRubikPiece &piece = side.pieces[ix][iy];

    double xo1, yo1, zo1, xo2, yo2, zo2;
    double txo1, tyo1, txo2, tyo2;

    if      (fabs(dsx) < 1E-6) {
      xo1 = x1; yo1 = y1 + ix*dsy + gap*dsy; zo1 = z1 + iy*dsz + gap*dsz;
      xo2 = x1; yo2 = yo1 + dsy - 2*gap*dsy; zo2 = zo1 + dsz - 2*gap*dsz;
    }
    else if (fabs(dsy) < 1E-6) {
      xo1 = x1 + ix*dsx + gap*dsx; yo1 = y1; zo1 = z1 + iy*dsz + gap*dsz;
      xo2 = xo1 + dsx - 2*gap*dsx; yo2 = y1; zo2 = zo1 + dsz - 2*gap*dsz;
    }
    else if (fabs(dsz) < 1E-6) {
      xo1 = x1 + ix*dsx + gap*dsx; yo1 = y1 + iy*dsy + gap*dsy; zo1 = z1;
      xo2 = xo1 + dsx - 2*gap*dsx; yo2 = yo1 + dsy - 2*gap*dsy; zo2 = z1;
    }

    txo1 = tx1 + ix*dxt; tyo1 = ty1 + iy*dyt;
    txo2 = txo1 + dxt  ; tyo2 = tyo1 + dyt  ;

    if (flip[animateData->side_num]) { std::swap(ix, iy); }

    double rx11, ry11, rz11, rx12, ry12, rz12, rx21, ry21, rz21, rx22, ry22, rz22;

    if      (fabs(dsx) < 1E-6) {
      m.multiplyPoint(xo1, yo1, zo1, &rx11, &ry11, &rz11);
      m.multiplyPoint(xo1, yo2, zo1, &rx12, &ry12, &rz12);
      m.multiplyPoint(xo1, yo2, zo2, &rx22, &ry22, &rz22);
      m.multiplyPoint(xo1, yo1, zo2, &rx21, &ry21, &rz21);
    }
    else if (fabs(dsy) < 1E-6) {
      m.multiplyPoint(xo1, yo1, zo1, &rx11, &ry11, &rz11);
      m.multiplyPoint(xo2, yo1, zo1, &rx12, &ry12, &rz12);
      m.multiplyPoint(xo2, yo1, zo2, &rx22, &ry22, &rz22);
      m.multiplyPoint(xo1, yo1, zo2, &rx21, &ry21, &rz21);
    }
    else if (fabs(dsz) < 1E-6) {
      m.multiplyPoint(xo1, yo1, zo1, &rx11, &ry11, &rz11);
      m.multiplyPoint(xo2, yo1, zo1, &rx12, &ry12, &rz12);
      m.multiplyPoint(xo2, yo2, zo1, &rx22, &ry22, &rz22);
      m.multiplyPoint(xo1, yo2, zo1, &rx21, &ry21, &rz21);
    }

    if (! showTexture_) {
      QColor c = rubik_->getColor(piece);

      double r = c.red  ()/255.0;
      double g = c.green()/255.0;
      double b = c.blue ()/255.0;

      glColor3d(r, g, b);

      glBegin(GL_POLYGON);

      glVertex3d(rx11, ry11, rz11);
      glVertex3d(rx12, ry12, rz12);
      glVertex3d(rx22, ry22, rz22);
      glVertex3d(rx21, ry21, rz21);

      glEnd();
    }
    else {
      glColor4d(1.0, 1.0, 1.0, 1.0);

      texture_->draw(rx11, ry11, rz11, rx22, ry22, rz22, txo2, tyo2);
    }
  }
}

void
CQRubik3D::
resizeGL(int width, int height)
{
  control_->handleResize(width, height);
}

void
CQRubik3D::
mousePressEvent(QMouseEvent *e)
{
  control_->handleMousePress(e);

  update();
}

void
CQRubik3D::
mouseReleaseEvent(QMouseEvent *e)
{
  control_->handleMouseRelease(e);

  update();
}

void
CQRubik3D::
mouseMoveEvent(QMouseEvent *e)
{
  control_->handleMouseMotion(e);

  update();
}

void
CQRubik3D::
keyPressEvent(QKeyEvent *e)
{
  rubik_->keyPressEvent(e);
}
