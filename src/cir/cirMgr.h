/****************************************************************************
  FileName     [ cirMgr.h ]
  PackageName  [ cir ]
  Synopsis     [ Define circuit manager ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef CIR_MGR_H
#define CIR_MGR_H

#include <vector>
#include <string>
#include <fstream>
#include <iostream>

using namespace std;

// TODO: Feel free to define your own classes, variables, or functions.

#include "cirDef.h"

extern CirMgr *cirMgr;

class CirMgr
{
public:
   CirMgr() {}
   ~CirMgr() {
     /*if (!_piList.empty()) {
       for (size_t i = 0; i < _piList.size(); ++i)
         delete _piList[i];
     }
     if (!_poList.empty()) {
       for (size_t i = 0; i < _poList.size(); ++i)
         delete _poList[i];
     }
     if (!_aigList.empty()) {
       for (size_t i = 0; i < _aigList.size(); ++i)
         delete _aigList[i];
     }*/
   }

   // Access functions
   // return '0' if "gid" corresponds to an undefined gate.
   CirGate* getGate(unsigned gid) const;
   void DFS(CirGate*);

   // Member functions about circuit construction
   bool readCircuit(const string&);

   // Member functions about circuit optimization
   void sweep();
   void optimize();

   // Member functions about simulation
   void randomSim();
   void fileSim(ifstream&);
   void setSimLog(ofstream *logFile) { _simLog = logFile; }

   // Member functions about fraig
   void strash();
   void printFEC() const;
   void fraig();

   // Member functions about circuit reporting
   void printSummary() const;
   void printNetlist() const;
   void printPIs() const;
   void printPOs() const;
   void printFloatGates() const;
   void printFECPairs() const;
   void writeAag(ostream&) const;
   void writeGate(ostream&, CirGate*) const;

private:
   string                _name;
   GateList            _piList;
   GateList            _poList;
   GateList           _aigList;
   IdList              _idList;
   GateList          totalList;
   unsigned         M, I, O, A;
   GateList            DFSList;
   ofstream           *_simLog;

};

#endif // CIR_MGR_H
