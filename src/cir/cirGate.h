/****************************************************************************
  FileName     [ cirGate.h ]
  PackageName  [ cir ]
  Synopsis     [ Define basic gate data structures ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
 ****************************************************************************/

#ifndef CIR_GATE_H
#define CIR_GATE_H

#include <string>
#include <vector>
#include <set>
#include <iostream>
#include "cirDef.h"
#include "sat.h"

using namespace std;

// TODO: Feel free to define your own classes, variables, or functions.

class CirGate;

//------------------------------------------------------------------------
//   Define classes
//------------------------------------------------------------------------
class CirGate
{
    friend class CirMgr;
    friend class CirAigGate;
    friend class CirPoGate;
    public:
        CirGate() {}
        CirGate(int id = 0, int lineNum = 0): _id(id), _lineNum(lineNum){}
        virtual ~CirGate() {}

        // Basic access methods
        virtual string getTypeStr() const { return ""; }
        unsigned getLineNo() const { return _lineNum; }
        unsigned getId() const { return _id; }
        virtual bool isAig() const { return false; }
        virtual GateType getType() const  { return UNDEF_GATE; }

        // Printing functions
        virtual void printGate() const {}
        void reportGate() const;
        void reportFanin(int level) const;
        void reportFanout(int level) const;

        // DFS functions
        void netflow(bool[], vector<CirGate*>&) const;
        void faninFlow(int depth, int &level, bool neg, set<int> &set) const;
        void fanoutFlow(int depth, int &level, bool neg, set<int> &set) const;

    private:
        int _id;
        int _lineNum;
        vector<size_t> _fanin;
        vector<size_t> _fanout;
};

class ConstGate: public CirGate
{
    friend class CirMgr;
    public:
        ConstGate():CirGate(0, 0){}
        void printGate() const { cout << "CONST0" << endl;}
        bool isAig() const { return false; }
        string getTypeStr() const { return "CONST"; }
        GateType getType() const { return CONST_GATE;}
};

class CirAigGate: public CirGate
{
    friend class CirMgr;
    public:
        CirAigGate(int id = 0, int lineNum = 0):CirGate(id, lineNum) {}
        void printGate() const
        {     assert(getLineNo() > 0); // Not UNDEF_GATE
              cout << "AIG " << getId();
              for (unsigned i = 0, n = _fanin.size(); i < n; i++)
              {    cout << " ";
                   CirGate* ptr = (CirGate*)(_fanin[i] & ~(size_t)0x1);
                   if (ptr->getType() == UNDEF_GATE) cout << "*";
                   if (_fanin[i] & 1) cout << "!";
                   cout << ptr->getId();
              }
              cout << endl;
        }
        bool isAig() const { return true; }
        GateType getType() const { return (getLineNo())? AIG_GATE : UNDEF_GATE; }
        string getTypeStr() const { return (getLineNo()) ? "AIG" : "UNDEF"; }
};

class CirPiGate: public CirGate
{
     friend class CirMgr;
     friend class CirGate;
     public:
        CirPiGate(int id = 0, int lineNum = 0): CirGate(id, lineNum), _name(""){}
        string getTypeStr() const {return "PI";}
        GateType getType() const { return PI_GATE; }

        bool isAig() const { return false; }
        void printGate() const
        {   cout << "PI  " << getId();
            if (!_name.empty()) cout <<  " (" << _name << ")";
            cout << endl;
        }
    private:
        string _name;
};

class CirPoGate: public CirGate
{
    friend class CirMgr;
    friend class CirGate;
    public:
        CirPoGate(int id = 0, int lineNum = 0): CirGate(id, lineNum), _name(""){}
        string getTypeStr() const { return "PO"; }
        GateType getType() const { return PO_GATE;}
        void printGate() const
        {   cout << "PO  " << getId();
            for (unsigned i = 0, n = _fanin.size(); i < n; i++)
            {    cout << " ";
                 CirGate* ptr = (CirGate*)(_fanin[i] & ~(size_t)0x1);
                 if (ptr->getType() == UNDEF_GATE) cout << "*";
                 if (_fanin[i] & 1) cout << "!";
                 cout << ptr->getId();
            }
            if (!_name.empty()) cout <<  " (" << _name << ")";
            cout << endl;
        }
        bool isAig() const { return false; }
    private:
        string _name;
};
#endif // CIR_GATE_H
