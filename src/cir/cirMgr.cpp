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

void getTokens (const string& option, vector<string>& tokens) {
  string token;
  size_t n = myStrGetTok(option, token);
  while (token.size()) {
    tokens.push_back(token);
    n = myStrGetTok(option, token, n);
  }
}

unsigned str2Unsigned (const string& str) {
  unsigned num = 0;
  for (size_t i = 0; i < str.size(); ++i) {
    if (isdigit(str[i])) {
      num *= 10;
      num += int(str[i] - '0');
    }
    else return num;
  }
  return num;
}
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
CirMgr::~CirMgr() {
  for (GateList::iterator it = gateList.begin(); it != gateList.end(); ++it)
    if((*it) != 0)	delete (*it);
  gateList.clear();
}

CirGate* CirMgr::getGate(unsigned gid) const {
  if (gid >= vars + outs + 1)	return 0;
  return gateList[gid];
}

CirGate* CirMgr::createUndef(unsigned gid) {
  gateList[gid] = new CirGate(UNDEF_GATE, 0, gid);
  return getGate(gid);
}

void CirMgr::resetFlag() const {
  for (unsigned i = 0, size = vars + outs + 1; i < size; ++i) {
    CirGate *g = getGate(i);
    if (g != 0)	g->flag = false;
  }
}

  bool
CirMgr::readCircuit(const string& fileName)
{
  ifstream ifs(fileName.c_str(), ifstream::in);
  if (! ifs.is_open())	return false;
  string line;
  char c;
  vector<string> circuit;
  if (!ifs.is_open())	return false;
  while (ifs.get(c)) {
    if (c == '\n')	{
      circuit.push_back(line);
      line = "";
    }
    else line += c;
  }
  if (line != "")	circuit.push_back(line);
  ifs.close();

  if (circuit.empty())	return false;

  vector<string> tokens;
  getTokens(circuit[0], tokens);
  if (tokens.size() != 6)	{
    cout << circuit[0] << endl;
    cout << fileName << " error format, first line size: " << tokens.size() << endl;
    return false;
  }
  vars = str2Unsigned(tokens[1]);	ins = str2Unsigned(tokens[2]);
  outs = str2Unsigned(tokens[4]);	ands = str2Unsigned(tokens[5]);
  if (circuit.size() <= (unsigned)(ins + outs + 1)) {
    cout << fileName << " error format, circuit size: " << circuit.size() << endl;
    return false;
  }
  gateList.resize(vars + outs + 1, 0);
  gateList[0] = new CirGate(CONST_GATE, 0, 0);
  // gateList.push_back(new CirGate(CONST_GATE, 0, 0));

  for (unsigned i = 0; i < ins; ++i) {
    unsigned id = str2Unsigned(circuit[i + 1]);
    gateList[id / 2] = new CirGate(PI_GATE, i + 2, id / 2);
    // gateList.push_back(new CirGate(PI_GATE, i + 2, id / 2));
  }
  for (unsigned i = ins + outs; i < ins + outs + ands; ++i) {
    unsigned id = str2Unsigned(circuit[i + 1]);
    gateList[id / 2] = new CirGate(AIG_GATE, i + 2, id / 2);
    // gateList.push_back(new CirGate(AIG_GATE, i + 2, id / 2));
  }
  for (unsigned i = ins + outs; i < ins + outs + ands; ++i) {
    vector<string> temps;
    getTokens(circuit[i + 1], temps);
    CirGate* g1 = getGate(str2Unsigned(temps[0]) / 2);
    unsigned n1 = str2Unsigned(temps[1]), n2 = str2Unsigned(temps[2]);
    CirGate *g2 = getGate(n1 / 2);
    if (g2 == 0)	g2 = createUndef(n1 / 2);
    CirGate *g3 = getGate(n2 / 2);
    if (g3 == 0)	g3 = createUndef(n2 / 2);
    g1->addInput(g2, n1 % 2);
    g2->addOutput(g1);
    g1->addInput(g3, n2 % 2);
    g3->addOutput(g1);
  }
  for (unsigned i = ins; i < ins + outs; ++i) {
    unsigned n = str2Unsigned(circuit[i + 1]);
    CirGate 	*g = new CirGate(PO_GATE, i + 2, vars + i + 1 - ins),
                *gi = getGate(n / 2);
    if (gi == 0)	createUndef(n / 2);
    g->addInput(gi, n % 2);
    gi->addOutput(g);

    gateList[vars + i + 1 - ins] = g;
    // gateList.push_back(g);
  }
  unsigned symbol = ins + outs + ands + 1, listSize = circuit.size();
  while (symbol < listSize) {
    string s = circuit[symbol++];
    if (s == "c")	break;
    bool input;
    if (s[0] == 'i')	input = true;
    else if (s[0] == 'o')	input = false;
    else {
      cout << "error: " << s << endl;
      break;
    }
    tokens.clear();
    getTokens(s, tokens);
    if (tokens.size() != 2)	cout << "error: " << s << endl;
    unsigned ith = str2Unsigned(tokens[0].substr(1));
    if (input)	getGate(str2Unsigned(circuit[ith + 1]) / 2)->setSymbol(tokens[1]);
    else			getGate(vars + ith + 1)->setSymbol(tokens[1]);
    // if (input)	gateList[ith + 1]->setSymbol(tokens[1]);
    // else			gateList[ith + 1 + ins + ands]->setSymbol(tokens[1]);
  }
  for (size_t i = 0; i < gateList.size(); ++i) {
    if (gateList[i] && gateList[i]->getType() == PO_GATE)
      DFS(gateList[i]);
  }
  resetFlag();

  return true;
}

void
CirMgr::DFS(CirGate* c) {
  c->flag = true;
  GateList in = c->getInputs();
  for (size_t i = 0; i < in.size(); ++i) {
    if (in[i] && !in[i]->flag) {
      if (in[i]->getType() != UNDEF_GATE)
        DFS(in[i]);
    }
  }
  DFSList.push_back(c);
}

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
  cout 	<< "\nCircuit Statistics\n"
    << "==================\n"
    << "  PI" << setw(12) << right << ins << "\n"
    << "  PO" << setw(12) << right << outs << "\n"
    << "  AIG" << setw(11) << right << ands << "\n"
    << "------------------\n"
    << "  Total" << setw(9) << right << ins + outs + ands << '\n';
}

void
CirMgr::printNetlist() const
{
  cout << endl;
  for (size_t i = 0; i < DFSList.size(); ++i) {
    DFSList[i]->printGate();
  }
}

void 
CirMgr::printPIs() const
{
  cout << "PIs of the circuit:";
  for (size_t i = 0, size = vars + outs + 1, count = 0; i < size && count < ins; ++i) {
    CirGate *g = getGate(i);
    if (g != 0 && g->getType() == PI_GATE)
      cout << " " << g->getGateId();
  }
  cout << endl;
}

void
CirMgr::printPOs() const
{
  cout << "POs of the circuit:";
  for (size_t i = vars + 1, size = vars + outs + 1; i < size; ++i) {
    CirGate *g = getGate(i);
    if (g != 0 && g->getType() == PO_GATE)
      cout << " " << g->getGateId();
  }

  cout << endl;
}

void
CirMgr::printFloatGates() const
{
  vector<unsigned> temp;
  for (size_t i = 0, size = vars + outs + 1; i < size; ++i) {
    CirGate* g = getGate(i);
    if (g == 0)	continue;
    if (g->getType() == PO_GATE && g->getInput(0)->getType() == UNDEF_GATE)
      temp.push_back(g->getGateId());
    else if (g->getType() == AIG_GATE &&
        ( g->getInput(0)->getType() == UNDEF_GATE || 
          g->getInput(1)->getType() == UNDEF_GATE ))
      temp.push_back(g->getGateId());
  }
  if (!temp.empty()) {
    cout << "Gates with floating fanin(s):";
    for (size_t i = 0, size = temp.size(); i < size; ++i)
      cout << ' ' << temp[i];
    cout << endl;
  }
  temp.clear();

  for (size_t i = 0, size = vars + outs + 1; i < size; ++i) {
    CirGate* g = getGate(i);
    if (g == 0)	continue;
    if ((g->getType() == PI_GATE || g->getType() == AIG_GATE)
        && g->getOutput(0) == 0)
      temp.push_back(g->getGateId());
  }
  if (!temp.empty()) {
    cout << "Gates defined but not used  :";
    for (size_t i = 0, size = temp.size(); i < size; ++i)
      cout << ' ' << temp[i];
    cout << endl;
  }
}

void
CirMgr::printFECPairs() const
{//TODO
}

void
CirMgr::writeAag(ostream& outfile) const
{
  string s_aig = "";
  unsigned aig = 0, count = 0;
  for (unsigned i = vars + 1; i <= vars + outs + 1; ++i) {
    CirGate *g = getGate(i);
    if (g != 0 && g->getType() == PO_GATE)
      g->printAig(s_aig, aig);
  }
  resetFlag();

  outfile	<< "aag " << vars << ' ' << ins << " 0 " << outs
    << ' ' << aig << '\n';
  vector<unsigned> ios;
  ios.resize(ins);
  for (unsigned i = 1; i <= ins && count < ins; ++i) {
    CirGate *g = getGate(i);
    if (g != 0 && g->getType() == PI_GATE) {
      ios[g->getLineNo() - 2] = g->getGateId();
      ++count;
    }
  }
  if (count != ins)	outfile << "count: " << count << ", ins: " << ins << endl;
  for (unsigned i = 0; i < ins; ++i)	outfile << ios[i] * 2 << '\n';

  for (unsigned i = vars + 1; i < vars + outs + 1; ++i) {
    CirGate *g = getGate(i);
    if (g == 0)	continue;
    unsigned id = g->getInput(0)->getGateId() * 2;
    if (g->isInv(0))	++id;
    outfile << id << '\n';
  }
  outfile << s_aig;
  for (unsigned i = 0; i < ins; ++i) {
    string s = getGate(ios[i])->getSymbol();
    if (s != "")	outfile << 'i' << i << ' ' << getGate(ios[i])->getSymbol() << '\n';
  }
  for (unsigned i = vars + 1; i < vars + outs + 1; ++i) {
    if (getGate(i) == 0 || getGate(i)->getSymbol() == "") continue;
    outfile << 'o' << i - vars - 1<< ' ' << getGate(i)->getSymbol() << '\n';
  }
  // Comment
  outfile << 'c' << endl;
  outfile << "AAG output by Hao Chen" << endl;
}

void
CirMgr::writeGate(ostream& outfile, CirGate *g) const
{//TODO
}

