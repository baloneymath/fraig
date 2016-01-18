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
#include <set>
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

//  --------- READING FUCNTIONS ----------------
//  This part is a copy and paste from others

#define BUFFER_SIZE 1024
#define BUFFER_SIZE_SAFE (BUFFER_SIZE - 1)


enum ParseState {
    STATE_INITIAL,
    STATE_HEADER,
    STATE_PI,
    STATE_PO,
    STATE_AIG,
    STATE_SYMBOL,
    STATE_FINISHED,

    STATE_DUMMY
};
static ParseState state = STATE_DUMMY;


// due to some errors are universal but state-dependent,
// when reading or consuming fails, this function is called
// to make an approperiate error message based on internal state
// these "stateful" errors are:
//   * ILLEGAL_NUM
//   * MISSING_NUM
//   * MISSING_IDENTIFIER
//   * MISSING_DEF
// this function is only intended to emit an error,
// do not retreat or touch the buffer here!!
static void emitStatefulError(CirParseError pe) {
    static const string headerErrMsg[5] = { "vars", "PIs", "latches", "POs", "AIGs" };
    bool doSuffix = false;
    switch (state) {
        case STATE_HEADER: errMsg = "number of " + headerErrMsg[errInt]; break;
        case STATE_PI: errMsg = "PI"; doSuffix = true; break;
        case STATE_PO: errMsg = "PO"; doSuffix = true; break;
        case STATE_AIG: errMsg = "AIG"; doSuffix = true; break;
        case STATE_SYMBOL:
                        if (pe == ILLEGAL_NUM)
                            errMsg = "symbol index";
                        else if (pe == MISSING_IDENTIFIER)
                            errMsg = "symbolic name";
                        break;
        default: break;
    }
    if (pe == MISSING_NUM)
        if (doSuffix) errMsg += " literal ID";

    throw pe;
}

static inline bool isTerminatingChar(char c) {
    return (c == ' ' || c == '\n');
}

static inline bool isDigit(char c) {
    return (c >= '0' && c <= '9');
}


static char readChar(istream& f) {
    char ch = f.get();
    if (ch < 0) return ch; // EOF; do not raise error
    if (ch < 32 && ch != '\n') {
        errInt = (int)ch;
        throw ILLEGAL_WSPACE;
    }
    colNo++;
    return ch;
}

// unget a character from the stream
// update colNo accordingly (sorry, only within a line)
static bool retreatChar(istream& f) {
    if (colNo == 0) return false;
    colNo--;
    f.unget();
    return true;
}

// count colNo backward BASED ON THE INTERNAL BUFFER
// dangerous!! retreating more than once causes troubles
static bool rejectBuffer() {
    size_t bp = 0;
    while (buf[bp] != '\0') {
        bp++;
        if (bp >= BUFFER_SIZE_SAFE) return false;
    }
    if (colNo < bp) {
        // should not happen
        colNo = 0;
        return false;
    }
    colNo -= bp;
    return true;
}

// read an unsigned integer until a space
// ### exceptions: EXTRA_SPACE ###
// ### stateful  : MISSING_DEF, ILLEGAL_NUM, MISSING_NUM ###
static unsigned readUint(istream& f) {
    size_t bp = 0;
    char ch = '\0';
    while (bp < BUFFER_SIZE_SAFE) {
        ch = readChar(f);
        if (isTerminatingChar(ch)) {
            retreatChar(f);
            if (bp == 0 && ch == ' ')
                throw EXTRA_SPACE;
            break;
        } else if (ch < 0) {
            emitStatefulError(MISSING_DEF);
            break;
        }
        if (!isDigit(ch))
            emitStatefulError(ILLEGAL_NUM);

        buf[bp++] = ch;
    }
    if (bp == 0) {
        // read number failed
        emitStatefulError(MISSING_NUM);
    }
    buf[bp] = '\0';
    // cout << "read num " << buf << endl;
    return atoi(buf);
}

// read a string (can contain spaces) until line end
// if readWord is set, stop at a "terminating" char (ie. a space)
// ### stateful: MISSING_IDENTIFIER ###
static string readStr(istream& f, bool readWord = false, unsigned maxlen = 0) {
    size_t bp = 0;
    char ch = '\0';
    if (maxlen > BUFFER_SIZE_SAFE) maxlen = BUFFER_SIZE_SAFE;
    while (maxlen == 0 || bp < maxlen) {
        ch = readChar(f);
        if (ch == '\n' || (readWord && isTerminatingChar(ch))) {
            retreatChar(f);
            break;
        }
        buf[bp++] = ch;
    }
    if (bp == 0) {
        // read string failed
        emitStatefulError(MISSING_IDENTIFIER);
    }
    buf[bp] = '\0';
    string rtn(buf);
    // cout << "read str [" << buf << "]" << endl;
    return rtn;
}

// a space after the header is then excepted
// return whether this check is passed
// if not, more string should be read before crashing
// ### exceptions: MISSING_IDENTIFIER, EXTRA_SPACE, ILLEGAL_IDENTIFIER ###
static bool consumeAagHeader(istream& f) {
    static const string AAG_HEADER = "aag";
    char ch = f.peek();
    if (ch < 0) {
        errMsg = AAG_HEADER;
        throw MISSING_IDENTIFIER;
    }
    if (f.peek() == ' ') throw EXTRA_SPACE;

    string str = readStr(f, true, 3);
    if (str != AAG_HEADER)
        throw ILLEGAL_IDENTIFIER;

    return isTerminatingChar(f.peek());
}

// ### exceptions: MISSING_SPACE ###
static bool consumeSpace(istream& f) {
    char ch = f.get();
    if (ch != ' ') throw MISSING_SPACE;
    colNo++;
    return true;
}

// ### exceptions: MISSING_NEWLINE ###
static bool consumeNewline(istream& f) {
    char ch = f.get();
    if (ch != '\n') throw MISSING_NEWLINE;
    colNo = 0;
    lineNo++;
    return true;
}

// ### exceptions: MAX_LIT_ID, REDEF_CONST, REDEF_GATE, CANNOT_INVERTED ###
static void checkLiteralID(CirMgr* mgr, unsigned gid, bool checkEven, bool checkUnique = true) {
    if (gid / 2 > mgr->_maxNum()) {
        errInt = gid; rejectBuffer();
        throw MAX_LIT_ID;
    }
    if (checkUnique) {
        errGate = mgr->getGate(gid / 2);
        if (gid / 2 == 0) {
            errInt = gid; rejectBuffer();
            throw REDEF_CONST;
        } else if (errGate != 0 && errGate->getType() != UNDEF_GATE) {
            errInt = gid;
            throw REDEF_GATE;
        }
    }
    if (checkEven && gid % 2 != 0) {
        errInt = gid; rejectBuffer();
        throw CANNOT_INVERTED;
    }
}

/**************************************************************/
/*   class CirMgr member functions for circuit construction   */
/**************************************************************/
bool
CirMgr::readCircuit(const string& fileName)
{   ifstream f(fileName.c_str());
    if (!f.is_open())
    {   cerr << "Cannot open design \"" << fileName << "\"!!" << endl;
        return false;
    }

    bool ok = true;
    try
    {   lineNo = colNo = 0;
        // Header
        state = STATE_HEADER;
        if (!consumeAagHeader(f))
        {   if(isDigit(f.peek()))   throw MISSING_SPACE;
            errMsg =  "aag" + readStr(f, true);
            throw ILLEGAL_IDENTIFIER;
        }

        try
        {   for (errInt = 0; errInt < 5; ++errInt)
            {   consumeSpace(f);
                _params[errInt] = readUint(f);
            }
        }
        catch (CirParseError err)
        {   if (err == ILLEGAL_NUM)
            {   retreatChar(f);
                errMsg = errMsg + "(" + readStr(f, true) + ")";
            }
            throw err;
        }

        consumeNewline(f); lineNo--;  // Assert a newline, check Err first

        if (_params[0] < _params[1] + _params[2] + _params[4])  // M < I + L + A
        {   errInt = _params[0];
            errMsg = "Num of variables";
            throw NUM_TOO_SMALL;
        }
        // No latch please.
        if (_params[2])
        {   errMsg = "latches";
            throw ILLEGAL_NUM;
        }

        lineNo++; // Header Finished
        _gateList.resize(_params[0]+ _params[3]+1);   // Reserve Space first
        _piList.reserve(_params[1]);
        _poList.reserve(_params[3]);
        _gateList[0] = new ConstGate();

        // Reading some input
        state = STATE_PI;
        for (int i = 0; i < _params[1]; i++)
        {   unsigned lid = readUint(f);
            checkLiteralID(this, lid, true);
            _piList.push_back(new CirPiGate(lid/2, lineNo+1));
            _gateList[lid/2] = _piList[i];
            consumeNewline(f);
        }
        // READING SOME OUTPUT
        state = STATE_PO;
        for (int i = 0; i < _params[3]; i++)
        {   unsigned lid = readUint(f);
            checkLiteralID(this, lid, false, false);
            _poList.push_back(new CirPoGate(_params[0]+1+i, lineNo+1));  // Output gate ID is larger.
            _gateList[_params[0]+1+i] = _poList[i];

            if (_gateList[lid/2] == 0) // If gate is not yet created, do it.
                _gateList[lid/2] = new CirAigGate(lid/2, 0);

            // Add the pointer into the fanin list
            // Last Bit represent invert.
            _poList[i]->_fanin.push_back((size_t)_gateList[lid/2] | (size_t) (lid & 1));
            _gateList[lid/2]->_fanout.push_back((size_t) _poList[i] | (size_t) (lid & 1));

            consumeNewline(f);
        }

        // OK Let's do AIGGate
        state = STATE_AIG;
        for (int i = 0; i < _params[4]; i++)
        {   unsigned rhs, lhs1, lhs2;
            rhs = readUint(f);
            checkLiteralID(this, rhs, true);
            consumeSpace(f);

            lhs1 = readUint(f);
            checkLiteralID(this, lhs1, false, false);
            consumeSpace(f);

            lhs2 = readUint(f);
            checkLiteralID(this, lhs2, false, false);

            // Not yet created even as UNDEF_GATE
            if (_gateList[rhs/2] == 0) _gateList[rhs/2] = new CirAigGate(rhs/2, lineNo+1);
            if (_gateList[lhs1/2] == 0) _gateList[lhs1/2] = new CirAigGate(lhs1/2, 0);
            if (_gateList[lhs2/2] == 0) _gateList[lhs2/2] = new CirAigGate(lhs2/2, 0);

            // Change from UNDEF_GATE to AIGGate
            _gateList[rhs/2]->_lineNum = lineNo+1;

            // Handle lhs1 Fanin and Fanout
            _gateList[rhs/2]->_fanin.push_back((size_t) _gateList[lhs1/2] | (size_t) (lhs1 & 1));
            _gateList[lhs1/2]->_fanout.push_back((size_t) _gateList[rhs/2] | (size_t) (lhs1 & 1));

            // Handle lhs2 Fanin and Fanout
            _gateList[rhs/2]->_fanin.push_back((size_t) _gateList[lhs2/2] | (size_t) (lhs2 & 1));
            _gateList[lhs2/2]->_fanout.push_back((size_t) _gateList[rhs/2] | (size_t) (lhs2 & 1));
            consumeNewline(f);
        }

        // Symbols
        state = STATE_SYMBOL;

        int ls;   // PI or PO
        while (true)
        {   ls = 0;

            char symbolType = readChar(f);
            string c;
            switch(symbolType)
            {   case 'i': ls = 1; break;
                case 'o': ls = 2; break;
                case 'c':
                    consumeNewline(f);
                    while (getline(f, c))
                        _comments.push_back(c);
                    break;
                case -1: break;  // EOF
                case ' ': retreatChar(f); throw EXTRA_SPACE; break;
                case '\n': retreatChar(f); errMsg = "symbol type"; throw MISSING_IDENTIFIER; break;
                default: retreatChar(f); errMsg = symbolType; throw ILLEGAL_SYMBOL_TYPE; break;
            }

            if (!ls) break;

            unsigned cnt;
            try { cnt = readUint(f); }
            catch (CirParseError err)
            {   if (err == ILLEGAL_NUM)
                {   retreatChar(f);
                    errMsg = errMsg + "(" + readStr(f, true) + ")";
                }
                throw err;
            }
            consumeSpace(f);

            if (ls == 1)
            {   if (cnt >= _piList.size())
                {   errMsg = "PI index";
                    errInt = cnt;
                    throw NUM_TOO_BIG;
                }
            }
            else if (ls == 2)
            {   if (cnt >= _poList.size())
                {   errMsg = "PO index";
                    errInt = cnt;
                    throw NUM_TOO_BIG;
                }
            }

            try { errMsg = readStr(f); }
            catch (CirParseError err)
            {   if(err == ILLEGAL_WSPACE) throw ILLEGAL_SYMBOL_NAME;
                throw err;
            }

            if (ls == 1)
            {   errGate = _piList[cnt];
                if (!_piList[cnt]->_name.empty())
                {   errMsg = symbolType;
                    errInt = cnt;
                    throw REDEF_SYMBOLIC_NAME;
                }
                _piList[cnt]->_name = errMsg;
            }
            else if (ls == 2)
            {   errGate = _poList[cnt];
                if (!_poList[cnt]->_name.empty())
                {   errMsg = symbolType;
                    errInt = cnt;
                    throw REDEF_SYMBOLIC_NAME;
                }
                _poList[cnt]->_name = errMsg;
            }

            consumeNewline(f);
        }

        state = STATE_FINISHED;
    }
    catch (CirParseError err)
    {   ok = false;
        parseError(err);
    }

    buildDFSList();// Create DFS List



    f.close();
    return true;
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
void CirMgr::buildDFSList()
{   _dfsList.clear();
    bool flag[_params[0]+_params[3]+1];
    memset(flag, 0, sizeof(flag));
    for (int i = 0, n = _poList.size(); i < n; ++i)
        _poList[i]->netflow(flag, _dfsList);
}

void
CirMgr::printSummary() const
{   int total = 0;
    cout << "Circuit Statistics" << endl;
    cout << "==================" << endl;
    cout << "  PI" <<  right << setw(12) << _params[1]<< endl;
    total += _params[1];
    cout << "  PO" <<  right << setw(12) << _params[3]<< endl;
    total += _params[3];
    cout << "  AIG" << right << setw(11) << _params[4] << endl;
    total += _params[4];
    cout << "------------------" << endl;
    cout << "  Total" << right << setw(9) << total << endl;
}

void
CirMgr::printNetlist() const
{   cout << endl;
    for (unsigned i = 0, n = _dfsList.size(); i < n; ++i)
    {    cout << "[" << i << "] ";
         _dfsList[i]->printGate();
    }
}

void
CirMgr::printPIs() const
{
    cout << "PIs of the circuit:";
    for (size_t i = 0; i < _piList.size(); ++i)
        cout << " " << _piList[i]->_id;
    cout << endl;
}

void
CirMgr::printPOs() const
{
    cout << "POs of the circuit:";
    for (size_t i = 0; i < _poList.size(); ++i)
        cout << " " << _poList[i]->_id;
    cout << endl;
}

void
CirMgr::printFloatGates() const
{   set<int> p, q;
    for (size_t i = 0; i < _gateList.size(); ++i)
        if (_gateList[i]->getType() == UNDEF_GATE)
        {   for (size_t j = 0; j < _gateList[i]->_fanout.size(); ++j)
            {   CirGate* out = (CirGate*)(_gateList[i]->_fanout[j] & ~(size_t)(0x1));
                p.insert(out->_id);
            }
        }
        else if ( _gateList[i]->getType() == AIG_GATE && _gateList[i]->_fanout.size() == 0)
            q.insert(_gateList[i]->_id);
    if (p.size())
    {   cout << "Gates with floating fanin(s):";
        for (std::set<int>::iterator it=p.begin(); it!=p.end(); ++it)
            cout << " " << *it;
        cout << endl;
    }
    if (q.size())
    {   cout << "Gates defined but not used:";
        for (std::set<int>::iterator it=q.begin(); it!=q.end(); ++it)
            cout << " " << *it;
        cout << endl;
    } } 
void CirMgr::printFECPairs() const
{
}

void
CirMgr::writeAag(ostream& outfile) const
{   // header
    outfile << "aag";
    for (int i = 0; i < 5; ++i) outfile << " " << _params[i];
    outfile << endl;

    // PI
    for (size_t i = 0; i < _piList.size(); ++i)
        outfile << _piList[i]->_id*2 << endl;

    // PO
    for (size_t i = 0; i < _poList.size(); ++i)
    {   CirGate* ptr = (CirGate*)(_poList[i]->_fanin[0] & ~(size_t)(0x1));
        outfile << ptr->_id*2 + (_poList[i]->_fanin[0] & 1) << endl;
    }

    // AIG, only those in DFSList
    for (size_t i = 0, s = _dfsList.size(); i < s; ++i)
        if (_dfsList[i]->getType() == AIG_GATE)
        {   outfile << (_dfsList[i]->getId() << 1);
            for (unsigned j = 0, n = _dfsList[i]->_fanin.size(); j < n; ++j)
            {   CirGate* ptr = (CirGate*)(_dfsList[i]->_fanin[j] & ~(size_t)0x1);
                outfile << " " << ptr->_id*2 + (_dfsList[i]->_fanin[j] & 1);
            }
            outfile << endl;
        }

    for (size_t i = 0; i < _piList.size(); ++i)
    {   if (_piList[i]->_name != "")
            outfile << "i" << i << " " << _piList[i]->_name << endl;
    }
    for (size_t i = 0; i < _poList.size(); ++i)
    {   if (_poList[i]->_name != "")
            outfile << "o" << i << " " << _poList[i]->_name << endl;
    }
    if (_comments.size())
    {   outfile << "c" << endl;
        for (size_t i = 0; i < _comments.size(); ++i)
            outfile << _comments[i] << endl;
    }

}

void
CirMgr::writeGate(ostream& outfile, CirGate *g) const
{
}
