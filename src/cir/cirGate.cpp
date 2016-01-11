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

unsigned CirGate::index = 0;
/**************************************/
/*   class CirGate member functions   */
/**************************************/
string unsToStr(unsigned n) {
  stringstream ss;
  ss << n;
  return ss.str();
}

string CirGate::getTypeStr() const {
  if (gateType == UNDEF_GATE)	return "UNDEF";
  if (gateType == PI_GATE)	return "PI";
  if (gateType == PO_GATE)	return "PO";
  if (gateType == AIG_GATE)	return "AIG";
  if (gateType == CONST_GATE)	return "CONST";
  if (gateType == TOT_GATE)	return "TOT";
  return "UNDEF";
}

void CirGate::printGate() const {
  //cout << index << ": gateId " << gateId << endl;
  if (flag)	return;
  if (gateType == UNDEF_GATE)	return;
  for (size_t i = 0; i < inputs.size(); ++i)
    getInput(i)->printGate(); 
  string info = "";
  if (gateType == CONST_GATE) info = "0";
  else {
    info = unsToStr(gateId);
    for (size_t i = 0; i < inputs.size(); ++i) {
      info += ' ';
      if (getInput(i)->getType() == UNDEF_GATE)	info += '*';
      if (isInv(i))	info += '!';
      info += unsToStr(getInput(i)->getGateId());
    }
    if (symbol != "")	info += " (" + symbol + ')';
  }
  cout 	<< '[' << index++ << "] " << setw(4) << left 
    << getTypeStr() << info << endl;
  flag = true;
}

void CirGate::reportGate() const {
  cout << "==================================================\n";
  stringstream ss;
  ss << "= " + getTypeStr() << '(' << getGateId() << ")";
  if (symbol != "")	ss << "\"" << symbol << "\"";
  ss << ", line " << getLineNo();
  cout << setw(49) << left << ss.str() << "=\n";
  cout << "==================================================\n";
}

void CirGate::reportFanin(int level) const {
  assert (level >= 0);
  myFanin(level, false);
  cirMgr->resetFlag();
}

void CirGate::myFanin(int level, bool inv) const {
  for (unsigned i = 0; i < index; ++i)	cout << "  ";
  if (inv)	cout << '!';
  cout << getTypeStr() << ' ' << gateId;
  if (level == 0)	cout << endl;
  else if (flag)	cout << " (*)\n";
  else {
    cout << endl;
    if (!inputs.empty())	flag = true;
    for (unsigned i = 0, size = inputs.size(); i < size; ++i) {
      ++index;
      getInput(i)->myFanin(level - 1, isInv(i));
    }
  }
  if (index != 0)	--index;
}

void CirGate::reportFanout(int level) const {
  assert (level >= 0);
  myFanout(level, false);
  cirMgr->resetFlag();
}

void CirGate::myFanout(int level, bool inv) const {
  for (unsigned i = 0; i < index; ++i)	cout << "  ";
  if (inv)	cout << '!';
  cout << getTypeStr() << ' ' << gateId;
  if (level == 0)	cout << endl;
  else if (flag) cout << " (*)\n";
  else {
    cout << endl;
    if(!outputs.empty())	flag = true;
    for (unsigned i = 0, size = outputs.size(); i < size; ++i) {
      ++index;
      const CirGate* g = getOutput(i);
      bool n_inv = false;
      unsigned j = 0;
      while (true) {
        CirGate* g2 = g->getInput(j);
        if (g2 == 0)	break;
        if (this == g2) {
          n_inv = g->isInv(j);
          break;
        }
        ++j;
      }
      getOutput(i)->myFanout(level - 1, n_inv);
    }
  }
  if (index != 0)	--index;
}

void CirGate::printAig(string& s, unsigned& cnt) const {
  if (flag) return;
  for (unsigned i = 0; i < inputs.size(); ++i)
    getInput(i)->printAig(s, cnt);
  if (gateType == AIG_GATE) {
    flag = true;
    stringstream ss;
    ss << gateId * 2
      << ' ' << getInput(0)->getGateId() * 2 + isInv(0)
      << ' ' << getInput(1)->getGateId() * 2 + isInv(1)
      << '\n';
    s += ss.str();
    ++cnt;
  }
}
