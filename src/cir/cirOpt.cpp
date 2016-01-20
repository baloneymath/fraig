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
  for (size_t i = 0; i < _gateList.size(); ++i) {
    bool flag = false;
    if (_gateList[i] == NULL) continue;
    if (_gateList[i]->getType() == PO_GATE) continue;
    else if (_gateList[i]->getType() == CONST_GATE) flag = true;
    else if (_gateList[i]->getType() == PI_GATE) flag = true;
    else if (_gateList[i]->getType() == AIG_GATE) {
      flag = check(_gateList[i]);
      if (!flag) --_params[4];
    }
    else if (_gateList[i]->getType() == UNDEF_GATE) {
      IDList& fanout = _gateList[i]->_fanout;
      flag = true;
      for (size_t j = 0; j < fanout.size(); ++j)
        if (!check((CirGate*)(fanout[j] & ~(size_t)(0x1)))) flag = false;
    }


    if (!flag) {
      cout << "Sweeping: " << _gateList[i]->getTypeStr();
      cout << '(' << _gateList[i]->getId() << ") removed..." << endl;
      delete _gateList[i];
      _gateList[i] = NULL;
    }
    else {
      IDList& fanout = _gateList[i]->_fanout;
      for (size_t j = 0; j < fanout.size(); ++j)
        if (!check((CirGate*)(fanout[j] & ~(size_t)(0x1))))
          fanout.erase(fanout.begin()+(j--));
    }
  }
}

bool
CirMgr::check(CirGate* g)
{
  for (size_t k = 0; k < _dfsList.size(); ++k) {
    if (_dfsList[k] == NULL) continue;
    if (g == _dfsList[k]) return true;
  }
  return false;
}

// Recursively simplifying from POs;
// _dfsList needs to be reconstructed afterwards
// UNDEF gates may be delete if its fanout becomes empty...
void
CirMgr::optimize()
{
  for (size_t i =0; i < _dfsList.size(); ++i) {
    if (_dfsList[i]->getType() != AIG_GATE) continue;
    IDList& fanin = _dfsList[i]->_fanin;
    CirGate* in[2];
    in[0] = (CirGate*)(fanin[0] & ~(size_t)(0x1));
    in[1] = (CirGate*)(fanin[1] & ~(size_t)(0x1));

    // one of the fanin is 0
    if (fanin[0] == (size_t)_gateList[0] || fanin[1] == (size_t)_gateList[0]) {
      merge(_dfsList[i], _gateList[0], 0, "Simplifying: ");
    }
    // one of the fanin is 1
    else if (in[0] == _gateList[0] || in[1] == _gateList[0]) {
      if (in[0] == _gateList[0]) merge(_dfsList[i], in[1], fanin[1]&1, "Simplifying: ");
      else merge(_dfsList[i], in[0], fanin[0]&1, "Simplifying: ");
    }
    // both fanins are the same
    else if (fanin[0] == fanin[1]) {
      merge(_dfsList[i], in[0], fanin[0]&1, "Simplifying: ");
    }
    // one fanin is the inverse of the other
    else if (in[0] == in[1]) {
      merge(_dfsList[i], _gateList[0], 0, "Simplifying: ");
    }
    // none of the cases above
    else continue;

    for (size_t j = 0; j < fanin.size(); ++j) {
      // clear the NULL fanouts of fanins
      for (size_t k = 0; k < in[j]->_fanout.size(); ++k) {
        if ((CirGate*)(in[j]->_fanout[k] & ~(size_t)(0x1)) == _dfsList[i])
          in[j]->_fanout.erase(in[j]->_fanout.begin()+k);
      }
      // remove the UNDEF_GATE with NULL fanout
      if (in[j]->getType() == UNDEF_GATE && in[j]->_fanout.size() == 0) {
        _gateList[in[j]->getId()] = NULL;
        delete in[j];
      }
    }
    // remove the Gate which's merged
    _gateList[_dfsList[i]->getId()] = NULL;
    delete _dfsList[i];
    --_params[4];
  }
  // rebuildDFS
  buildDFSList();
}

/***************************************************/
/*   Private member functions about optimization   */
/***************************************************/

// inv determine on the condition of optimization
// inv is going to make the inverse bit right
// so it need to use a XOR compute with the fanout's fanin's inverse bit
void
CirMgr::merge(CirGate* old, CirGate* New, size_t inv, string messege)
{
  for (size_t i = 0; i < old->_fanout.size(); ++i) {
    CirGate* out = (CirGate*)(old->_fanout[i] & ~(size_t)(0x1));
    IDList& outs_in = out->_fanin;
    for (size_t j = 0; j < outs_in.size(); ++j) {
      if ( (CirGate*)(outs_in[j] & ~(size_t)(0x1)) == old) {
        outs_in[j] = ( (size_t)New | (size_t)((inv&1) ^ (outs_in[j]&1)) );
        New->_fanout.push_back( (size_t)out | (size_t)(outs_in[j]&1) );// cannot XOR again
      }
    }
  }
  cout << messege << New->getId() << " merging "
    << (inv? "!":"") << old->getId() << "...\n";
}
