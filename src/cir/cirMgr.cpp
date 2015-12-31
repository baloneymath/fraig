/****************************************************************************
  FileName     [ cirMgr.cpp ]
  PackageName  [ cir ]
  Synopsis     [ Define cir manager functions ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
 ****************************************************************************/

#include <iostream>
#include <iomanip>
#include <algorithm>
#include <cstdio>
#include <ctype.h>
#include <cassert>
#include <cstring>
#include "cirMgr.h"
#include "cirGate.h"
#include "util.h"

using namespace std;

// TODO: Implement memeber functions for class CirMgr

/*******************************/
/*   Global variable and enum  */
/*******************************/
CirMgr* cirMgr = 0;

enum CirParseError {
  EXTRA_SPACE,
  MISSING_SPACE,
  ILLEGAL_WSPACE,
  ILLEGAL_NUM,
  ILLEGAL_IDENTIFIER,
  ILLEGAL_SYMBOL_TYPE,
  ILLEGAL_SYMBOL_NAME,
  MISSING_NUM,
  MISSING_IDENTIFIER,
  MISSING_NEWLINE,
  MISSING_DEF,
  CANNOT_INVERTED,
  MAX_LIT_ID,
  REDEF_GATE,
  REDEF_SYMBOLIC_NAME,
  REDEF_CONST,
  NUM_TOO_SMALL,
  NUM_TOO_BIG,

  DUMMY_END
};

/**************************************/
/*   Static varaibles and functions   */
/**************************************/
static unsigned lineNo = 0;  // in printint, lineNo needs to ++
static unsigned colNo  = 0;  // in printing, colNo needs to ++
static char buf[1024];
static string errMsg;
static int errInt;
static CirGate *errGate;

static bool
parseError(CirParseError err)
{
  switch (err) {
    case EXTRA_SPACE:
      cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
        << ": Extra space character is detected!!" << endl;
      break;
    case MISSING_SPACE:
      cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
        << ": Missing space character!!" << endl;
      break;
    case ILLEGAL_WSPACE: // for non-space white space character
      cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
        << ": Illegal white space char(" << errInt
        << ") is detected!!" << endl;
      break;
    case ILLEGAL_NUM:
      cerr << "[ERROR] Line " << lineNo+1 << ": Illegal "
        << errMsg << "!!" << endl;
      break;
    case ILLEGAL_IDENTIFIER:
      cerr << "[ERROR] Line " << lineNo+1 << ": Illegal identifier \""
        << errMsg << "\"!!" << endl;
      break;
    case ILLEGAL_SYMBOL_TYPE:
      cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
        << ": Illegal symbol type (" << errMsg << ")!!" << endl;
      break;
    case ILLEGAL_SYMBOL_NAME:
      cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
        << ": Symbolic name contains un-printable char(" << errInt
        << ")!!" << endl;
      break;
    case MISSING_NUM:
      cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
        << ": Missing " << errMsg << "!!" << endl;
      break;
    case MISSING_IDENTIFIER:
      cerr << "[ERROR] Line " << lineNo+1 << ": Missing \""
        << errMsg << "\"!!" << endl;
      break;
    case MISSING_NEWLINE:
      cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
        << ": A new line is expected here!!" << endl;
      break;
    case MISSING_DEF:
      cerr << "[ERROR] Line " << lineNo+1 << ": Missing " << errMsg
        << " definition!!" << endl;
      break;
    case CANNOT_INVERTED:
      cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
        << ": " << errMsg << " " << errInt << "(" << errInt/2
        << ") cannot be inverted!!" << endl;
      break;
    case MAX_LIT_ID:
      cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
        << ": Literal \"" << errInt << "\" exceeds maximum valid ID!!"
        << endl;
      break;
    case REDEF_GATE:
      cerr << "[ERROR] Line " << lineNo+1 << ": Literal \"" << errInt
        << "\" is redefined, previously defined as "
        << errGate->getTypeStr() << " in line " << errGate->getLineNo()
        << "!!" << endl;
      break;
    case REDEF_SYMBOLIC_NAME:
      cerr << "[ERROR] Line " << lineNo+1 << ": Symbolic name for \""
        << errMsg << errInt << "\" is redefined!!" << endl;
      break;
    case REDEF_CONST:
      cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
        << ": Cannot redefine const (" << errInt << ")!!" << endl;
      break;
    case NUM_TOO_SMALL:
      cerr << "[ERROR] Line " << lineNo+1 << ": " << errMsg
        << " is too small (" << errInt << ")!!" << endl;
      break;
    case NUM_TOO_BIG:
      cerr << "[ERROR] Line " << lineNo+1 << ": " << errMsg
        << " is too big (" << errInt << ")!!" << endl;
      break;
    default: break;
  }
  return false;
}

/**************************************************************/
/*   class CirMgr member functions for circuit construction   */
/**************************************************************/
CirGate*
CirMgr::getGate(unsigned gid) const {
   for (size_t i = 0; i < totalList.size(); ++i) {
     if (totalList[i]->getId() == gid && totalList[i]->getType() != UNDEF_GATE)
       return totalList[i];
   }
   return NULL;
}

void
CirMgr::DFS(CirGate* c) {
   c->setMark(true);
   GateList fanin = c->getFanin();
   for (size_t i = 0; i < fanin.size(); ++i) {
     if (fanin[i] && !fanin[i]->getMark()) {
       if (fanin[i]->getType() != UNDEF_GATE)
         DFS(fanin[i]);
     }
   }
   DFSList.push_back(c);
}

bool
CirMgr::readCircuit(const string& fileName)
{
   fstream file;
   if (file.is_open()) return false;
   file.open(fileName.c_str(), fstream::in);
   _name = fileName;
   string line;
   vector<string> tokens;
   // header line
   getline(file, line);
   string token;
   size_t n = myStrGetTok(line, token);
   while (token.size()) {
     tokens.push_back(token);
     n = myStrGetTok(line, token, n);
   }
   if (tokens.empty()) return false;
   if (tokens[0] != "aag" || tokens[3] != "0") return false;
   M = atoi(tokens[1].c_str());
   I = atoi(tokens[2].c_str());
   O = atoi(tokens[4].c_str());
   A = atoi(tokens[5].c_str());
   if (M < I + A) return false;
   // Gate dat
   // CONST
   CirGate* const0 = new CirGate(CONST_GATE, GateList(), vector<bool>(), GateList());
   totalList.push_back(const0);
   // PI
   for (size_t i = 0; i < I; ++i) {
     getline(file, line);
     unsigned id = atoi(line.c_str()) / 2;
     CirGate* temp = new CirGate(PI_GATE, GateList(), vector<bool>(), GateList(), id, i+2);
     _piList.push_back(temp);
     totalList.push_back(temp);
   }
   // PO
   for (size_t i = 0; i < O; ++i) {
     getline(file, line);
     unsigned id = M + i + 1;
     GateList fanin;
     size_t out = atoi(line.c_str());
     fanin.push_back( (CirGate*)( out/2 ) );

     vector<bool> inIsInv;
     if (out % 2 != 0) inIsInv.push_back(true);
     else inIsInv.push_back(false);

     CirGate* temp = new CirGate(PO_GATE, fanin, inIsInv, GateList(), id, i+I+2);
     _poList.push_back(temp);
     totalList.push_back(temp);
   }
   // AIG
   for (size_t i = 0; i < A; ++i) {
     getline(file, line);
     string token;
     vector<string> tokens;
     size_t n = myStrGetTok(line, token);
     while (token.size()) {
       tokens.push_back(token);
       n = myStrGetTok(line, token, n);
     }
     unsigned id = atoi(tokens[0].c_str()) / 2;
     GateList fanin;
     unsigned id1 = atoi(tokens[1].c_str()) / 2;
     unsigned id2 = atoi(tokens[2].c_str()) / 2;

     if (!getGate(id1)) { CirGate* G1 = new CirGate(UNDEF_GATE, GateList(), vector<bool>(), GateList(), id1); fanin.push_back(G1); }
     else if (id1 == 0) { fanin.push_back(const0); }
     else { CirGate* G1 = getGate(id1); fanin.push_back(G1); }
     if (!getGate(id2)) { CirGate* G2 = new CirGate(UNDEF_GATE, GateList(), vector<bool>(), GateList(), id2); fanin.push_back(G2); }
     else if (id2 == 0) { fanin.push_back(const0); }
     else { CirGate* G2 = getGate(id2); fanin.push_back(G2); }

     vector<bool> inIsInv;
     if (atoi(tokens[1].c_str()) % 2 != 0) inIsInv.push_back(true);
     else inIsInv.push_back(false);
     if (atoi(tokens[2].c_str()) % 2 != 0) inIsInv.push_back(true);
     else inIsInv.push_back(false);

     CirGate* temp = new CirGate(AIG_GATE, fanin, inIsInv, GateList(), id, i+I+O+2);
     _aigList.push_back(temp);
     totalList.push_back(temp);
   }
   // set PO fanins
   for (size_t i = 0; i < _poList.size(); ++i) {
     GateList& temp = _poList[i]->_faninList;
     for (size_t j = 0; j < temp.size(); ++j) {
       temp[j] = getGate((size_t)(temp[j]));
     }
   }
   // set AIG fanins
   for (size_t i = 0; i < _aigList.size(); ++i) {
     GateList &fanin = _aigList[i]->_faninList;
     for (size_t j = 0; j < fanin.size(); ++j) {
       if (fanin[j]->getType() == UNDEF_GATE)
         for (size_t k = 1; k < _aigList.size(); ++k) {
           if (fanin[j]->getId() == _aigList[k]->getId())
             fanin[j] = _aigList[k];
         }
     }
   }
   // set CONST0 fanouts
   for (size_t i = _piList.size(); i < totalList.size(); ++i) {
     GateList fanin = totalList[i]->getFanin();
     if (fanin.size() > 0 && fanin[0]->getId() == 0)
       totalList[0]->_fanoutList.push_back(totalList[i]);
     if (fanin.size() == 2 && fanin[1]->getId() == 0)
       totalList[0]->_fanoutList.push_back(totalList[i]);
   }
   // set PI fanouts
   for (size_t i = 0; i < _piList.size(); ++i) {
     for (size_t j = _piList.size(); j < totalList.size(); ++j) {
       GateList fanin = totalList[j]->getFanin();
       if (fanin.size() > 0 && fanin[0]->getId() == _piList[i]->getId())
         _piList[i]->_fanoutList.push_back(totalList[j]);
       if (fanin.size() == 2 && fanin[1]->getId() == _piList[i]->getId())
         _piList[i]->_fanoutList.push_back(totalList[j]);
     }
   }
   // set AIG fanouts
   for (size_t i = 0; i < _aigList.size(); ++i) {
     for (size_t j = _piList.size(); j < totalList.size(); ++j) {
       GateList fanin = totalList[j]->getFanin();
       if (fanin.size() > 0 && fanin[0]->getId() == _aigList[i]->getId())
         _aigList[i]->_fanoutList.push_back(totalList[j]);
       if (fanin.size() == 2 && fanin[1]->getId() == _aigList[i]->getId())
         _aigList[i]->_fanoutList.push_back(totalList[j]);
     }
   }

   // Build DFSList
   for (size_t i = 0; i < _poList.size(); ++i) {
     DFS(_poList[i]);
   }
   // Reset marks
   for (size_t i = 0; i < totalList.size(); ++i) {
     totalList[i]->setMark(false);
   }
   // names
   while (getline(file, line)) {
     if (line == "c") break;
     string token;
     vector<string> tokens;
     size_t n = myStrGetTok(line, token);
     while (token.size()) {
       tokens.push_back(token);
       n = myStrGetTok(line, token, n);
     }
     // input symbols
     string target = tokens[0];
     unsigned ID = atoi(target.substr(1).c_str());
     if (target[0] == 'i') {
       _piList[ID]->setName(tokens[1]);
     }
     // output symbols
     else if (target[0] == 'o') {
       _poList[ID]->setName(tokens[1]);
     }
   }
   return true;

/**********************************************************/
/*   class CirMgr member functions for circuit printing   */
/**********************************************************/
/*********************
  Circuit Statistics
  ==================
  PI          20
  PO          12
  AIG        130
  ------------------
  Total      162
 *********************/
void
CirMgr::printSummary() const
{
  size_t nPi, nPo, nAig, nTol;
  nPi = _piList.size();
  nPo = _poList.size();
  nAig = _aigList.size();
  nTol = nPi + nPo + nAig;
  cout << endl;
  cout << "Circuit Statistics" << endl;
  cout << "==================" << endl;
  cout << "  PI" << setw(12) << nPi << endl;
  cout << "  PO" << setw(12) << nPo << endl;
  cout << "  AIG" << setw(11) << nAig << endl;
  cout << "------------------" << endl;
  cout << "  Total" << setw(9) << nTol << endl;
}

void
CirMgr::printNetlist() const
{
  cout << endl;
  for (size_t i = 0; i < DFSList.size(); ++i) {
    if (DFSList[i]->getType() == UNDEF_GATE) continue;
    cout << '[' << lineNo << ']' << ' ';
    // CONST0
    if (DFSList[i]->getType() == CONST_GATE) cout << "CONST0" << endl;
    // AIG
    else if (DFSList[i]->getType() == AIG_GATE) {
      GateList fanin = DFSList[i]->getFanin();
      vector<bool> isInv = DFSList[i]->getIsInv();
      cout << "AIG " << DFSList[i]->getId() << ' ';
      if (fanin[0]->getType() == UNDEF_GATE) cout << '*';
      if (isInv[0]) cout << '!' << fanin[0]->getId() << ' ';
      else cout << fanin[0]->getId() << ' ';
      if (fanin[1]->getType() == UNDEF_GATE) cout << '*';
      if (isInv[1]) cout << '!' << fanin[1]->getId() << ' ';
      else cout << fanin[1]->getId() << ' ';
      if (DFSList[i]->getName() != "")
        cout << '(' << DFSList[i]->getName() << ')' << endl;
      else cout << endl;
    }
    // PI
    else if (DFSList[i]->getType() == PI_GATE) {
      cout << "PI" << "  ";
      cout << DFSList[i]->getId() << ' ';
      if (DFSList[i]->getName() != "")
        cout << '(' << DFSList[i]->getName() << ')' << endl;
      else cout << endl;
    }
    // PO
    else if (DFSList[i]->getType() == PO_GATE){
      GateList fanin = DFSList[i]->getFanin();
      vector<bool> isInv = DFSList[i]->getIsInv();
      cout << "PO" << "  ";
      cout << DFSList[i]->getId() << ' ';
      if (fanin[0]->getType() == UNDEF_GATE) cout << '*';
      if (isInv[0]) cout << '!' << fanin[0]->getId() << ' ';
      else cout << fanin[0]->getId() << ' ';
      if (DFSList[i]->getName() != "")
        cout << '(' << DFSList[i]->getName() << ')' << endl;
      else cout << endl;
    }
    lineNo++;
  }
  lineNo = 0;
}

void
CirMgr::printPIs() const
{
  cout << "PIs of the circuit:";
  for (size_t i = 0; i < _piList.size(); ++i) {
    cout << ' ' << _piList[i]->getId();
  }
  cout << endl;
}

void
CirMgr::printPOs() const
{
  cout << "POs of the circuit:";
  for (size_t i = 0; i < _poList.size(); ++i) {
    cout << ' ' << _poList[i]->getId();
  }
  cout << endl;
}

void
CirMgr::printFloatGates() const
{
  bool flag1 = false;
  for (size_t i = 0; i < totalList.size(); ++i) {
    GateList fanin = totalList[i]->getFanin();
    if (fanin.size() > 0 && fanin[0]->getType() == UNDEF_GATE)
      flag1 = true;
    if (fanin.size() == 2 && fanin[1]->getType() == UNDEF_GATE)
      flag1 = true;
  }
  if (flag1) cout << "Gates with floating fanin(s)  :";
  for (size_t i = 0; i < totalList.size(); ++i) {
    GateList fanin = totalList[i]->getFanin();
    if (fanin.size() > 0 && fanin[0]->getType() == UNDEF_GATE)
      cout << ' ' << fanin[0]->getId();
    if (fanin.size() == 2 && fanin[1]->getType() == UNDEF_GATE)
      cout << ' ' << fanin[1]->getId();
  }
  if (flag1) cout << endl;

  bool flag2 = false;
  for (size_t i = 1; i < totalList.size(); ++i) {
    bool flag = false;
    for (size_t j = 0; j < DFSList.size(); ++j) {
      if (totalList[i] == DFSList[j]) flag = true;
    }
    if (!flag) flag2 = true;
  }
  if (flag2) cout << "Gates defined but not unsed  :";
  for (size_t i = 1; i < totalList.size(); ++i) {
    bool flag = false;
    for (size_t j = 0; j < DFSList.size(); ++j) {
      if (totalList[i] == DFSList[j]) flag = true;
    }
    if (!flag) cout << ' ' << totalList[i]->getId();
  }
  if (flag2) cout << endl;
}

void
CirMgr::printFECPairs() const
{//TODO
}

void
CirMgr::writeAag(ostream& outfile) const
{
  unsigned newA = 0;
  for (size_t i = 0; i < DFSList.size(); ++i) {
    if (DFSList[i]->getType() == AIG_GATE) newA++;
  }
  // header
  outfile << "aag " << M << ' ' << I << ' ';
  outfile << 0 << ' ' << O << ' ' << newA << endl;
  // PI
  for (size_t i = 0; i < _piList.size(); ++i) {
    if (_piList[i]->getType() == PI_GATE) {
      outfile << _piList[i]->getId()*2 << endl;
    }
  }
  // PO
  for (size_t i = 0; i < _poList.size(); ++i) {
    if (_poList[i]->getType() == PO_GATE) {
      GateList fanin = _poList[i]->getFanin();
      vector<bool> inIsInv = _poList[i]->getIsInv();
      if (inIsInv[0]) outfile << fanin[0]->getId()*2 + 1 << endl;
      else outfile << fanin[0]->getId()*2 << endl;
    }
  }
  // AIG
  for (size_t i = 0; i < DFSList.size(); ++i) {
    if (DFSList[i]->getType() == AIG_GATE) {
      GateList fanin = DFSList[i]->getFanin();
      vector<bool> inIsInv = DFSList[i]->getIsInv();
      outfile << DFSList[i]->getId()*2 << ' ';
      if (inIsInv[0]) outfile << fanin[0]->getId()*2 + 1 << ' ';
      else outfile << fanin[0]->getId()*2 << ' ';
      if (inIsInv[1]) outfile << fanin[1]->getId()*2 + 1 << endl;
      else outfile << fanin[1]->getId()*2 << endl;
    }
  }
  // Symbols
  for (size_t i = 0; i < _piList.size(); ++i) {
    if (_piList[i]->getName() != "")
      outfile << 'i' << i << ' ' << _piList[i]->getName() << endl;
  }
  for (size_t i = 0; i < _poList.size(); ++i) {
    if (_poList[i]->getName() != "")
      outfile << 'o' << i << ' ' << _poList[i]->getName() << endl;
  }
  // Comment
  outfile << 'c' << endl;
  outfile << "AAG output by Hao Chen" << endl;
}

void
CirMgr::writeGate(ostream& outfile, CirGate *g) const
{//TODO
}

