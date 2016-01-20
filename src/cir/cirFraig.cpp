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
      CirMgr::merge(_dfsList[i], temp, 0, "Strashing: ");
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
  SatSolver solver;
  solver.initialize();
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

  bool result;
  Var newVar;
  for (size_t i = 0; i < _fecList.size(); ++i) {
    for (size_t j = 0; j < _fecList[i].size(); ++i) {
      for (size_t k = j+1; k < _fecList[i].size(); ++k) {
        newVar = solver.newVar();
        bool flag[2];
        flag[0] = (_fecList[i][j]%2 == 1);
        flag[1] = (_fecList[i][k]%2 == 1);
        solver.addXorCNF(newVar, _fecList[i][j], flag[0], _fecList[i][k], flag[1]);
        solver.assumeRelease();
        solver.assumeProperty(newVar, true);
        result = solver.assumpSolve();
        reportResult(solver);
      }
    }
  }
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
