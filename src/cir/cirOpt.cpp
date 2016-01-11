/****************************************************************************
  FileName     [ cirSim.cpp ]
  PackageName  [ cir ]
  Synopsis     [ Define cir optimization functions ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#include <cassert>
#include "cirMgr.h"
#include "cirGate.h"
#include "util.h"

using namespace std;

// TODO: Please keep "CirMgr::sweep()" and "CirMgr::optimize()" for cir cmd.
//       Feel free to define your own variables or functions

/*******************************/
/*   Global variable and enum  */
/*******************************/

/**************************************/
/*   Static varaibles and functions   */
/**************************************/

/**************************************************/
/*   Public member functions about optimization   */
/**************************************************/
// Remove unused gates
// DFS list should NOT be changed
// UNDEF, float and unused list may be changed
void
CirMgr::sweep()
{
  for (size_t i = 0; i < gateList.size(); ++i) {
    bool flag = false;
    if (gateList[i] == NULL) continue;
    if (gateList[i]->getType() == PO_GATE) continue;
    else if (gateList[i]->getType() == CONST_GATE) flag = true;
    else if (gateList[i]->getType() == PI_GATE) flag = true;
    else if (gateList[i]->getType() == AIG_GATE) {
      flag = check(gateList[i]);
      if (!flag) --ands;
    }
    else if (gateList[i]->getType() == UNDEF_GATE) {
      GateList& out = gateList[i]->outputs;
      for (size_t j = 0; j < out.size(); ++j)
        flag = check(out[j]);
    }
    if (!flag) {
      cout << "Sweeping: " << gateList[i]->getTypeStr();
      cout << '(' << gateList[i]->getGateId() << ") removed..." << endl;
      delete gateList[i];
      gateList[i] = NULL;
    }
    else {
      GateList& out = gateList[i]->outputs;
      for (size_t j = 0; j < out.size(); ++j)
        if (!check(out[j])) out.erase(out.begin()+j);
    }
  }
}

bool
CirMgr::check(CirGate* g)
{
  for (size_t k = 0; k < DFSList.size(); ++k) {
    if (g == DFSList[k]) return true;
  }
  return false;
}

// Recursively simplifying from POs;
// _dfsList needs to be reconstructed afterwards
// UNDEF gates may be delete if its fanout becomes empty...
void
CirMgr::optimize()
{
  for (size_t i = 0; i < DFSList.size(); ++i) {
    
  }
}

/***************************************************/
/*   Private member functions about optimization   */
/***************************************************/
