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
  vector<string> lines;
  while (getline(patternFile, line)) {
    // check the length of patterns
    if (line.size() != _params[1]) {
      if (!line.empty()) {
        cerr << "\nError: Pattern(" << line << ") length(" << line.size()
          << ") does not match the number of inputs(" << _params[1]
          << ") in a circuit!!\n";
      }
      break;
    }
    // check if the patterns contain some trash (ex. 00102001x300)
    size_t pos = find_first_not_of("01");
    if (pos != string::npos) {
      cerr << "\nError: Pattern(" << line << ") contains a non-0/1 character(\'"
        << line[pos] << "\').\n";
    }
  }


}

/*************************************************/
/*   Private member functions about Simulation   */
/*************************************************/
