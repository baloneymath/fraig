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
    SimKey(const SimKey& k) { _key = k._key; }
    size_t operator () () const { return key; }
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
}

void
CirMgr::fileSim(ifstream& patternFile)
{
  string line;
  size_t input;
  size_t nPatterns = 0;

  HashMap<SimKey, IDList> FECGrps(0);
  FECGrps.forceInsert(0x0, _dfsList);
  while (getline(patternFile, line)) {
    // check the length of patterns
    if (line.size() != _params[1]) {
      if (!line.empty()) {
        cerr << "Error: Pattern(" << line << ") length(" << line.size()
          << ") does not match the number of inputs(" << _params[1]
          << ") in a circuit!!\n";
      }
      break;
    }
    // check if the patterns contain some trash (ex. 00102001x300)
    size_t pos = find_first_not_of("01");
    if (pos != string::npos) {
      cerr << "Error: Pattern(" << line << ") contains a non-0/1 character(\'"
        << line[pos] << "\').\n";
      break;
    }
    // convert inputs patterns
    for (size_t i = 0; i < _params[1]; ++i) {
      input = input | (size_t)(line[i] == '1');
      if (i != _params[1]-1) input <<= 1;
    }
    ++nPatterns;

    // start to simulate
    FECGrps = simulate(FECGrps, input);
  }


}

/*************************************************/
/*   Private member functions about Simulation   */
/*************************************************/

HashMap<SimKey, IDList>
CirMgr::simulate(HashMap<SimKey, IDList> FECGrps, size_t input)
{
  // set simValue
  size_t simValue[_params[0] + 1];
  simValue[0] = 0; // const0
  for (size_t i = 0; i < _dfsList.size(); ++i) {
    if (_dfsList[i]->getType() == PI_GATE) {
      size_t value  = (size_t)( (input>>(_params[1]-i)) & 1);
      simValue[_dfsList[i]->getId()] = value;
    }
    else if (_dfsList[i]->getType() == AIG_GATE) {
      IDList& fanin = _dfsList[i]->_fanin;
      CirGate* in[2];
      in[0] = (CirGate*)(fanin[0] & ~(size_t)(0x1));
      in[1] = (CirGate*)(fanin[1] & ~(size_t)(0x1));
      size_t value = simValue[in[0]->getId()] & simValue[in[1]->getId()];
      simValue[_dfsList[i]->getId()] = value;
    }
    else if (_dfsList[i]->getType() == PO_GATE) {
      CirGate* in = (CirGate*)(_dfsList[i]->_fanin[0] & ~(size_t)(0x1));
      size_t value = simValue[in->getId()];
      simValue[_dfsList[i]->getId()] = value;
    }
  }
  // check FECGrps
  HashMap<SimKey, IDList> target;
  for (FECGrps::iterator it = FECGrps.begin(); it != FECGrps.end(); ++it) {
    HashMap<SimKey, IDList> newFECGrps(0);
    for (size_t i = 0; i < it.second.size(); ++i) {
      IDList temp;
      CirGate* gate = (CirGate*)(it.second[i] & ~(size_t)(0x1));
      size_t key = simValue[gate->getId()];
      if (newFECGrps.check(key, temp)) {
        temp.push_back((size_t)gate);
      }
      else {
        temp.push_back((size_t)gate);
        newFECGrps.forceInsert(key, temp);
      }
    }
    for (newFECGrps::iterator nt = newFECGrps.begin(); nt != newFECGrps.end(); ++nt) {
      if (nt.second.size() != 1) target.forceInsert(nt.first, nt.second);
    }
  }
  return target;
}
