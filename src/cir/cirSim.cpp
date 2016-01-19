/****************************************************************************
  FileName     [ cirSim.cpp ]
  PackageName  [ cir ]
  Synopsis     [ Define cir simulation functions ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#include <fstream>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <cassert>
#include "cirMgr.h"
#include "cirGate.h"
#include "util.h"


using namespace std;

// TODO: Keep "CirMgr::randimSim()" and "CirMgr::fileSim()" for cir cmd.
//       Feel free to define your own variables or functions

/*******************************/
/*   Global variable and enum  */
/*******************************/
class SimKey
{
  public:
    SimKey() { _key = 0; }
    SimKey(const size_t &i) { _key = i; }
    SimKey(const SimKey& k) { _key = k._key; }
    size_t operator () () const { return _key; }
    bool operator == (const SimKey& k) const { return _key == k._key; }
  private:
    size_t _key;
};

/**************************************/
/*   Static varaibles and functions   */
/**************************************/

/************************************************/
/*   Public member functions about Simulation   */
/************************************************/
void
CirMgr::randomSim()
{
  vector<size_t>* pattern;
  pattern = new vector<size_t>[_params[1]];
}

void
CirMgr::fileSim(ifstream& patternFile)
{
  string line;
  vector<string> lines;
  vector<size_t>* pattern;
  size_t nPatterns = 0;

  pattern = new vector<size_t>[_params[1]];
  patternFile >> line;
  while (patternFile.good()) {
    // check the length of patterns
    if (line.size() != (size_t)_params[1]) {
      if (!line.empty()) {
        cerr << "\nError: Pattern(" << line << ") length(" << line.size()
          << ") does not match the number of inputs(" << _params[1]
          << ") in a circuit!!\n";
      }
      return;
    }
    // check if the patterns contain some trash (ex. 00102001x300)
    size_t pos = line.find_first_not_of("01");
    if (pos != string::npos) {
      cerr << "\nError: Pattern(" << line << ") contains a non-0/1 character(\'"
        << line[pos] << "\').\n";
      return;
    }
    for (size_t i = 0; i < (size_t)_params[1]; ++i) {
      if (nPatterns%64 == 0) pattern[i].push_back((size_t)(0x0));
      pattern[i][nPatterns/64] = (pattern[i][nPatterns/64] << 1) | (size_t)(line[i] == '1');
    }
    // convert inputs patterns
    lines.push_back(line);
    ++nPatterns;
    patternFile >> line;
  }
  // start to simulate
  for (size_t i = 0; i < _dfsList.size(); ++i)
    _dfsList[i]->_simValue = (size_t)(0x0);
  if (!nPatterns) return;
  else {
    simulate(pattern, nPatterns);
    cout << nPatterns << " patterns simulatd.\n";
  }
  delete pattern;
}

/*************************************************/
/*   Private member functions about Simulation   */
/*************************************************/

void
CirMgr::simulate(vector<size_t>* pattern, size_t nPatterns)
{
  // initialize _fecList;
  IDList temp;
  for (size_t i = 0; i < _dfsList.size(); ++i)
    temp.push_back(_dfsList[i]->getId());
  vector<IDList>& FECGrps = _fecList;
  FECGrps.push_back(temp);
  // set simValue
  for (size_t i = 0; i < pattern[0].size(); ++i) {
    for (size_t j = 0; j < _piList.size(); ++j)
      _piList[j]->_simValue = pattern[j][i];
    for (size_t j = 0; j < _dfsList.size(); ++j) {
      if (_dfsList[j]->getType() == AIG_GATE) {
        CirGate* in[2];
        in[0] = (CirGate*)(_dfsList[j]->_fanin[0] & ~(size_t)(0x1));
        in[1] = (CirGate*)(_dfsList[j]->_fanin[1] & ~(size_t)(0x1));
        size_t value[2];
        value[0] = (((_dfsList[j]->_fanin[0])&1) == 1)? ~(in[0]->_simValue):in[0]->_simValue;
        value[1] = (((_dfsList[j]->_fanin[1])&1) == 1)? ~(in[1]->_simValue):in[1]->_simValue;
        _dfsList[j]->_simValue = value[0] & value[1];
      }
      else if (_dfsList[j]->getType() == PO_GATE) {
        CirGate* in = (CirGate*)(_dfsList[j]->_fanin[0] & ~(size_t)(0x1));
        _dfsList[j]->_simValue = ((_dfsList[j]->_fanin[0]&1) == 1)? ~(in->_simValue):in->_simValue;
      }
    }
    // write _simLog
    if (_simLog != NULL) {
      size_t mask = (size_t)(0x1);
      for (size_t j = 0; j < _piList.size(); ++j) {
        if (i != pattern[0].size()-1) {
          for (size_t k = 0; k < 64; ++k)
            (*_simLog) << ((mask<<k) & (_piList[j]->_simValue));
        }
        else {
          for (size_t k = 0; k < nPatterns%64; ++k)
            (*_simLog) << ((mask<<k)&(_piList[j]->_simValue));
        }
      }
      (*_simLog) << ' ';
      for (size_t j = 0; j < _poList.size(); ++j) {
        if (i != pattern[0].size()-1) {
          for (size_t k = 0; k < 64; ++k)
            (*_simLog) << ((mask<<k)&(_poList[j]->_simValue));
        }
        else {
          for (size_t k = 0; k < nPatterns%64; ++k)
            (*_simLog) << ((mask<<k)&(_poList[j]->_simValue));
        }
      }
    }
    // build FECs
    size_t m = FECGrps.size();
    for (size_t j = 0; j < m; ++j) {
      cout << "111111111111111\n";
      HashMap<SimKey, IDList> newFECGrps(FECGrps.size());
      size_t n = FECGrps[0].size();
      for (size_t k = 0; k < n; ++k) {
        IDList temp;
        cout << "2222222222222\n";
        CirGate* gate = getGate(FECGrps[0][k]);
        temp.push_back(gate->getId());
        if (newFECGrps.check(gate->_simValue, temp)) {
          newFECGrps.replaceInsert(gate->_simValue, temp);
        }
        else if (newFECGrps.check(~(gate->_simValue), temp)) {
          newFECGrps.replaceInsert(gate->_simValue, temp);
        }
        else {
          newFECGrps.forceInsert(gate->_simValue, temp);
        }
      }
      FECGrps.erase(FECGrps.begin()); // remove old groups
      for (HashMap<SimKey, IDList>::iterator nt = newFECGrps.begin(); nt != newFECGrps.end(); ++nt)
        if ((*nt).second.size() != 1) {FECGrps.push_back((*nt).second);cout << "33333333333\n";}
    }

  }
}
