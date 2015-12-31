/****************************************************************************
  FileName     [ cirGate.cpp ]
  PackageName  [ cir ]
  Synopsis     [ Define class CirAigGate member functions ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#include <iostream>
#include <iomanip>
#include <sstream>
#include <stdarg.h>
#include <cassert>
#include "cirGate.h"
#include "cirMgr.h"
#include "util.h"

using namespace std;

// TODO: Keep "CirGate::reportGate()", "CirGate::reportFanin()" and
//       "CirGate::reportFanout()" for cir cmds. Feel free to define
//       your own variables and functions.

extern CirMgr *cirMgr;

/**************************************/
/*   class CirGate member functions   */
/**************************************/
void
CirGate::ResetInMark(CirGate* c)
{
   c->setMark(false);
   GateList fanin = c->getFanin();
   for (size_t i = 0; i < fanin.size(); ++i) {
     if (fanin[i] && fanin[i]->getMark())
       ResetInMark(fanin[i]);
   }
}
void
CirGate::ResetOutMark(CirGate* c)
{
  c->setMark(false);
  GateList fanout = c->getFanout();
  for (size_t i = 0; i < fanout.size(); ++i) {
    if (fanout[i] && fanout[i]->getMark())
      ResetOutMark(fanout[i]);
  }
}

void
CirGate::reportGate() const
{
  size_t sizeID = 0;
  unsigned t = _id;
  if (t == 0) sizeID++;
  while (t != 0) {
    t /= 10;
    sizeID++;
  }
  size_t sizeL = 0;
  unsigned temp = _lineNo;
  if (temp == 0) sizeL++;
  while (temp != 0) {
    temp /= 10;
    sizeL++;
  }
  size_t num = 50 - 2 - getTypeStr().size() - 2 - sizeL - 7 - sizeID;
  size_t num2 = 50 - 2 - getTypeStr().size() - 2 - sizeID - _name.size() -2 -7 -sizeL;
  cout << "==================================================" << endl;
  cout << "= " << getTypeStr() << '(' << _id << ')';
  if (_name != "") cout << '"' << _name << '"' << ',' << " line " << _lineNo << setw(num2) << '=' << endl;
  else cout << ',' << " line " << _lineNo << setw(num) << '=' << endl;
  cout << "==================================================" << endl;
}

void
CirGate::__reportFanin(int level, int spaces, bool inv)
{
   if (inv) cout << '!';
   cout << getTypeStr() << ' ' << getId();
   if (level != 0 && this->getMark() && _faninList.size() != 0)
     cout << " (*)" << endl;
   else if (level > 0) {
     cout << endl;
     for (size_t i = 0; i < _faninList.size(); ++i) {
       for (int j = 0; j < spaces; ++j) cout << "  ";
       _faninList[i]->__reportFanin(level-1, spaces+1, this->_faninIsInv[i]);
     }
     this->setMark(true);
   }
   else cout << endl;
}

void
CirGate::reportFanin(int level)
{
   assert (level >= 0);
   cout << getTypeStr() << ' ' << getId() << endl;
   for (size_t i = 0; i < _faninList.size(); ++i) {
     cout << "  ";
     _faninList[i]->__reportFanin(level-1, 2, this->_faninIsInv[i]);
   }
   this->setMark(true);
   ResetInMark(this);
}

void
CirGate::__reportFanout(int level, int spaces, bool inv)
{
  if (inv) cout << '!';
  cout << getTypeStr() << ' ' << getId();
  if (level != 0 && this->getMark() && _fanoutList.size() != 0)
    cout << " (*)" << endl;
  else if (level > 0) {
    cout << endl;
    for (size_t i = 0; i < _fanoutList.size(); ++i) {
      for (int j = 0; j < spaces; ++j) cout << "  ";
        if (this->_fanoutList[i]->_faninList[0] == this)
          _fanoutList[i]->__reportFanout(level-1, spaces+1, this->_fanoutList[i]->_faninIsInv[0]);
        else
          _fanoutList[i]->__reportFanout(level-1, spaces+1, this->_fanoutList[i]->_faninIsInv[1]);
    }
    this->setMark(true);
  }
  else cout << endl;
}

void
CirGate::reportFanout(int level)
{
   assert (level >= 0);
   cout << getTypeStr() << ' ' << getId() << endl;
   for (size_t i = 0; i < _fanoutList.size(); ++i) {
     cout << "  ";
     if (this->_fanoutList[i]->_faninList[0] == this)
       _fanoutList[i]->__reportFanout(level-1, 2, this->_fanoutList[i]->_faninIsInv[0]);
     else
       _fanoutList[i]->__reportFanout(level-1, 2, this->_fanoutList[i]->_faninIsInv[1]);
   }
   this->setMark(true);
   ResetOutMark(this);
}