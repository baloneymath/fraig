/****************************************************************************
  FileName     [ cirFraig.cpp ]
  PackageName  [ cir ]
  Synopsis     [ Define cir FRAIG functions ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2012-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#include <cassert>
#include "cirMgr.h"
#include "cirGate.h"
#include "sat.h"
#include "myHashMap.h"
#include "util.h"

using namespace std;

// TODO: Please keep "CirMgr::strash()" and "CirMgr::fraig()" for cir cmd.
//       Feel free to define your own variables or functions

/*******************************/
/*   Global variable and enum  */
/*******************************/

/**************************************/
/*   Static varaibles and functions   */
/**************************************/

/*******************************************/
/*   Public member functions about fraig   */
/*******************************************/
// _floatList may be changed.
// _unusedList and _undefList won't be changed
class StrashKey
{
  public:
    StrashKey() { _fanin[0] = _fanin[1] = 0; }
    StrashKey(size_t fanin[2]) {
      _fanin[0] = fanin[0];
      _fanin[1] = fanin[1];
    }

    size_t operator() () const { return _fanin[0] + _fanin[1] + (_fanin[0]&_fanin[1]); }

    bool operator == (const StrashKey& k) const {
      return (_fanin[0] == k._fanin[0] && _fanin[1] == k._fanin[1]) ||
             (_fanin[0] == k._fanin[1] && _fanin[1] == k._fanin[0]);
    }

  private:
    size_t _fanin[2];
};
void
CirMgr::strash()
{
  HashMap<StrashKey, CirGate*> myMap( (size_t)(_dfsList.size() * 1.6));
  for (size_t i = 0; i < _dfsList.size(); ++i) {
    if (_dfsList[i]->getType() != AIG_GATE) continue;
    size_t fanin[2];
    fanin[0] = _dfsList[i]->_fanin[0];
    fanin[1] = _dfsList[i]->_fanin[1];
    CirGate* in[2];
    in[0] = (CirGate*)(fanin[0] & ~(size_t)(0x1));
    in[1] = (CirGate*)(fanin[1] & ~(size_t)(0x1));
    CirGate* temp;

    if (myMap.check(fanin, temp)) {
      merge(_dfsList[i], temp, 0, "Strashing: ");
      // remove some trash
      for (size_t j = 0; j < 2; ++j) {
        for (size_t k = 0; k < in[j]->_fanout.size(); ++k) {
          if( (CirGate*)(in[j]->_fanout[k] & ~(size_t)(0x1)) == _dfsList[i] )
            in[j]->_fanout.erase(in[j]->_fanout.begin()+k);
        }
        if (in[j]->getType() == UNDEF_GATE && in[j]->_fanout.size() == 0) {
         _gateList[in[j]->getId()] = NULL;
          delete in[j];
        }
      }
      _gateList[_dfsList[i]->getId()] = NULL;
      delete _dfsList[i];
      --_params[4];
    }
    else myMap.forceInsert(fanin, _dfsList[i]);
  }
  buildDFSList();
}

void
CirMgr::fraig()
{
  // initialize circuit
  SatSolver solver;
  solver.initialize();
  // generate proof model
  generateProofModel(solver);
  // proofing
  bool result;
  Var newVar;
  for (size_t i = 0; i < _dfsList.size(); ++i) {
    if (_dfsList[i] == NULL) continue;
    if (_dfsList[i]->_fecs == NULL) continue;
    IDList& fecs = (*_dfsList[i]->_fecs);
    for (size_t j = 0; j < fecs.size(); ++j) {
      for (size_t k = j+1; k < fecs.size(); ++k) {
        newVar = solver.newVar();
        CirGate* ptr[2];
        ptr[0] = getGate(fecs[j]/2);
        ptr[1] = getGate(fecs[k]/2);
        bool flag[2];
        flag[0] = (fecs[j]%2 == 1);
        flag[1] = (fecs[k]%2 == 1);
        solver.addXorCNF(newVar, ptr[0]->_var, flag[0], ptr[1]->_var, flag[1]);
        solver.assumeRelease();
        solver.assumeProperty(newVar, true);
        result = solver.assumpSolve();
        // merge
        if (!result) {
          --_params[4];

          if (ptr[0]->_simValue == (size_t)(0x0)) {
            merge(ptr[0], _gateList[0], 0, "Fraig: ");
            solver.initialize();
            buildDFSList();
            generateProofModel(solver);
          }
          if (ptr[1]->_simValue == (size_t)(0x0)) {
            merge(ptr[1], _gateList[0], 0, "Fraig: ");
            solver.initialize();
            buildDFSList();
            generateProofModel(solver);
          }
          else {
            merge(ptr[1], ptr[0], (size_t)flag[0]^flag[1], "Fraig: ");
            solver.initialize();
            buildDFSList();
            generateProofModel(solver);
          }
          // remove some NULL fanouts
          CirGate* in[2];
          in[0] = (CirGate*)(ptr[1]->_fanin[0] & ~(size_t)(0x1));
          in[1] = (CirGate*)(ptr[1]->_fanin[1] & ~(size_t)(0x1));
          for (size_t m = 0; m < 2; ++m) {
            for (size_t n = 0; n < in[m]->_fanout.size(); ++n) {
              if ( (CirGate*)(in[m]->_fanout[n] & ~(size_t)(0x1)) == ptr[1])
                in[m]->_fanout.erase(in[m]->_fanout.begin()+n);
            }
            if (in[m]->getType() == UNDEF_GATE && in[m]->_fanout.size() == 0) {
              _gateList[in[m]->getId()] = NULL;
              delete in[m];
            }
          }
          ////////////////////////////////////////////////////////////
          _gateList[ptr[1]->getId()] = NULL;
          delete ptr[1];
          fecs.erase(fecs.begin()+k);
          --j;
          cout << "Updating by UNSAT... Total #FEC Group = " << _fecList.size() << endl;
        }
        else {
        }
      }
    }
  }
  _fecList.clear();
  clearFECs();
  buildDFSList();
  optimize();
  strash();
  buildDFSList();
}

/********************************************/
/*   Private member functions about fraig   */
/********************************************/

void
CirMgr::reportResult(const SatSolver &s, bool result, CirGate* c)
{
  s.printStats();
  cout << (result? "SAT":"UNSAT") << endl;
  if (result) {
    cout << s.getValue(c->_var) << endl;
  }
}

void
CirMgr::clearFECs()
{
  for (size_t i = 0; i < _dfsList.size(); ++i) {
    if (_dfsList[i]->_fecs)  _dfsList[i]->_fecs = NULL;
  }
}

void
CirMgr::generateProofModel(SatSolver& solver)
{
  _gateList[0]->_var = solver.newVar();
  for (size_t i = 0; i < _dfsList.size(); ++i) {
    if (_dfsList[i]->getType() == PI_GATE || _dfsList[i]->getType() == AIG_GATE) {
      Var v = solver.newVar();
      _dfsList[i]->_var = v;
      if (_dfsList[i]->getType() == AIG_GATE) {
        CirGate* in[2];
        in[0] = (CirGate*)(_dfsList[i]->_fanin[0] & ~(size_t)(0x1));
        in[1] = (CirGate*)(_dfsList[i]->_fanin[1] & ~(size_t)(0x1));
        bool flag[2];
        flag[0] = (((_dfsList[i]->_fanin[0])&1) == 1);
        flag[1] = (((_dfsList[i]->_fanin[1])&1) == 1);
        solver.addAigCNF(_dfsList[i]->_var, in[0]->_var, flag[0], in[1]->_var, flag[1]);
      }
    }
  }

}
