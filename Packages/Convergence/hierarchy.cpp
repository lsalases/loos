/*
  hierarchy

  Based on Zhang, Bhatt, and Zuckerman; JCTC, DOI: 10.1021/ct1002384
  and code provided by the Zuckerman Lab
  (http://www.ccbb.pitt.edu/Faculty/zuckerman/software.html)


  Given a trajectory whose structures have been binned into states via
  reference structures, computes the MFPT between states and then
  constructs a hierarchy of states based on exchange rates

*/


/*

  This file is part of LOOS.

  LOOS (Lightweight Object-Oriented Structure library)
  Copyright (c) 2010, Tod D. Romo
  Department of Biochemistry and Biophysics
  School of Medicine & Dentistry, University of Rochester

  This package (LOOS) is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation under version 3 of the License.

  This package is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

// @cond TOOLS_INTERNAL

#include <loos.hpp>
#include <boost/format.hpp>


using namespace std;
using namespace loos;

typedef pair<uint, uint>     uPair;
typedef vector<uint>         vUint;
typedef vector<vUint>        vvUint;



// Simple class to sort pairs of numbers based on a third (the rate)
struct RatePair {
  RatePair(const double d, const uint a, const uint b) :
    rate(d), pair(a,b) { }

  bool operator<(const RatePair& x) const {
    return(rate > x.rate);
  }

  double rate;
  uPair pair;
};



// @endcond TOOLS_INTERNAL

// Debugging generates a lot of information about the internal state
// of the code.  This was used to validate its operation with respect
// to Dan's PERL code.
const bool debugging = false;



double mfpt(const vector<uint>& assign, const uint x, const uint y) {
  double fpt = 0.0;
  uint n = 0;

  bool state = false;
  uint start = 0;
  for (uint j=0; j<assign.size(); ++j) {
    if (!state) {
      if (assign[j] == x) {
        start = j;
        state = true;
      }
    } else {
      if (assign[j] == y) {
        fpt += (j - start);
        ++n;
        state = false;
      }
    }
  }


  return(n != 0 ? static_cast<double>(n)/fpt : 0.0);
}



DoubleMatrix computeRates(const string& fname) {
  ifstream ifs(fname.c_str());
  if (!ifs) {
      cerr << "Error- unable to open " << fname << endl;
      exit(-1);
  }
  

  vector<uint> assignments = readVector<uint>(ifs);
  uint nbins = 0;
  for (vector<uint>::iterator i = assignments.begin(); i != assignments.end(); ++i)
    if (*i > 0 && static_cast<uint>(*i) > nbins)
      nbins = *i;
  
  ++nbins;   // Bins are 0-based

  DoubleMatrix M(nbins, nbins);
  for (uint j=0; j<nbins; ++j)
    for (uint i=0; i<nbins; ++i) {
      if (i == j)
        continue;
      M(j, i) = mfpt(assignments, j, i);
    }

  for (uint j=0; j<nbins-1; ++j)
    for (uint i=j+1; i<nbins; ++i)
      if (M(j, i) > 0.0 && M(i, j) > 0.0) {
        double d = (M(j, i) + M(i, j)) / 2.0;
        M(j, i) = d;
      } else
        M(j, i) = 0.0;

  return(M);
}




vector<uPair>  sortRates(const DoubleMatrix& M) {
  vector<RatePair> rates;
  for (uint j=0; j<M.cols()-1; ++j)
    for (uint i=j+1; i<M.cols(); ++i)
      if (M(j, i) != 0.0)
        rates.push_back(RatePair(M(j, i), j, i));
      
  sort(rates.begin(), rates.end());

  vector<uPair> pairs;
  if (debugging)
    cerr << "DEBUG> PAIR_BEGIN\n";

  for (vector<RatePair>::iterator i = rates.begin(); i != rates.end(); ++i) {
    pairs.push_back(i->pair);
    if (debugging)
      cerr << boost::format("%d %d = %f\n") % i->pair.first % i->pair.second % i->rate;
  }
  
  if (debugging)
    cerr << "DEBUG> PAIR_END\n";

  return(pairs);
}



void dumpMatrix(ostream& os, const vvUint& M) {
  os << M.size() << endl;
  for (uint j=0; j<M.size(); ++j) {
    os << M[j].size() << "\t";
    copy(M[j].begin(), M[j].end(), ostream_iterator<uint>(os, "\t"));
    os << endl;
  }
}


// Yes, this really needs to be commented...  Maybe in the next release.
vvUint cluster(const vector<uPair>& pairs) {
  vvUint states;
  vUint list;

  list.push_back(pairs[0].first);
  list.push_back(pairs[0].second);
  states.push_back(list);

  // Skip the last pair so we have 2 states...

  for (uint i=1; i<pairs.size()-1; ++i) {
    if (debugging)
      cerr << boost::format("DEBUG> i=%d, first=%d, second=%d\n") % i % pairs[i].first % pairs[i].second;

    bool flag1 = false, flag2 = false;
    uint bin1_state = 0, bin1_element = 0;
    uint bin2_state = 0, bin2_element = 0;

    for (uint j=0; j<states.size(); ++j)
      for (uint k=0; k<states[j].size(); ++k) {
        if (states[j][k] == pairs[i].first) {
          bin1_state = j;
          bin1_element = k;
          flag1 = true;
        }
        if (states[j][k] == pairs[i].second) {
          bin2_state = j;
          bin2_element = k;
          flag2 = true;
        }
      }

    if (debugging)
      cerr << boost::format("DEBUG> flag1=%d, flag2=%d\n") % flag1 % flag2;

    if (flag1 && flag2) {
      uint big = bin1_state;
      uint small = bin2_state;
      if (bin1_state < bin2_state) {
        big = bin2_state;
        small = bin1_state;
      }

      if (debugging)
        cerr << boost::format("DEBUG> small=%d, big=%d\n") % small % big;
      
      bool flag3 = false;
      for (uint w = 0; w<states[big].size() && !flag3; ++w)
        for (uint z=0; z<states[small].size() && !flag3; ++z) {
          bool failed = true;
          for (uint y=0; y<=i; ++y)
            if ( (states[small][z] == pairs[y].first && states[big][w] == pairs[y].second)
                 || (states[small][z] == pairs[y].second && states[big][w] == pairs[y].first) ) {
              failed = false;
              break;
            }
          if (failed) {
            flag3 = true;
            if (debugging)
              cerr << boost::format("DEBUG> Check failed for w=%d, z=%d\n") % w % z;
          }
        }


      if (!flag3) {

        if (debugging)
          cerr << "DEBUG> *Merging states*\n";

        copy(states[big].begin(), states[big].end(), back_inserter(states[small]));
        states.erase(states.begin() + big);

      }

    } else if (flag1) {

      bool failed = false;
        
      for (uint p=0; p<states[bin1_state].size() && !failed; ++p) {
        if (p == bin1_element)
          continue;

        failed = true;
        for (uint q=0; q<i; ++q)
          if ( (states[bin1_state][p] == pairs[q].first && pairs[i].second == pairs[q].second)
               || (states[bin1_state][p] == pairs[q].second && pairs[i].second == pairs[q].first) ) {
            failed = false;
            break;
          }
      }

      if (debugging)
        cerr << boost::format("DEBUG> [1] failed=%d\n") % failed;

      if (!failed)
        states[bin1_state].push_back(pairs[i].second);

    } else if (flag2) {


      bool failed = false;
        
      for (uint p=0; p<states[bin2_state].size() && !failed; ++p) {
        if (p == bin2_element)
          continue;

        failed = true;
        for (uint q=0; q<i; ++q)
          if ( (states[bin2_state][p] == pairs[q].first && pairs[i].first == pairs[q].second)
               || (states[bin2_state][p] == pairs[q].second && pairs[i].first == pairs[q].first) ) {
            failed = false;
            break;
          }
      }


      if (debugging)
        cerr << boost::format("DEBUG> [1] failed=%d\n") % failed;


      if (!failed)
        states[bin2_state].push_back(pairs[i].first);

    } else {
      if (debugging)
        cerr << "DEBUG> Adding new state.\n";

      vUint newlist;
      newlist.push_back(pairs[i].first);
      newlist.push_back(pairs[i].second);
      states.push_back(newlist);
    }

    if (debugging) {
      dumpMatrix(cerr,states);
      cerr << "DEBUG> --------------------------------------\n";
    }
    
    

  }

  if (debugging)
    cerr << "DEBUG> final states = " << states.size() << endl;
  return(states);
}


void findOrphans(vvUint& states, const uint max_states) {
  vUint seen;

  if (debugging)
    cerr << "DEBUG> Finding orphans- ";

  for (vvUint::iterator v = states.begin(); v != states.end(); ++v)
    copy(v->begin(), v->end(), back_inserter(seen));

  vUint unseen;
  for (uint i = 0; i<max_states; ++i)
    if (find(seen.begin(), seen.end(), i) == seen.end())
      unseen.push_back(i);

  if (debugging)
    cerr << "found " << unseen.size() << endl;

  if (! unseen.empty())
    states.push_back(unseen);
}


string fullHelpMessage(void) {
  string msg =
    "\n"
    "SYNOPSIS\n"
    "\tPerform a hierarchical clustering needed to determine effective sample size\n"
    "\n"
    "DESCRIPTION\n"
    "\n"
    "\tThis tool implements the hierarchical clustering algorithm as part of determining\n"
    "effective sample size described in Zhang, Batt, and Zuckerman, JCTC (2010) 6:3048-57.\n"
    "\n"
    "EXAMPLES\n"
    "\n"
    "\thierarchy assignments.asc >zuckerman.states\n"
    "\n"
    "SEE ALSO\n"
    "\tufidpick, assign_frames, neff, effsize.pl\n";

  return(msg);
}



int main(int argc, char *argv[]) {
  if (argc == 1) {
    cout << "Usage- " << argv[0] << " assignments_file\n";
    exit(0);
  }
  
  string hdr = invocationHeader(argc, argv);
  int k = 1;
  DoubleMatrix M = computeRates(argv[k++]);
  vector<uPair> pairs = sortRates(M);
  if (pairs.empty()) {
      cerr << "Error- hierarchy failed to compute rates.  Double-check how the assignments\n"
	   << "       file was generated.  Did you use ufidpick to pick the fiducials?\n"
	   << "       Is the cutoff reasonable?  Is the selection correct (use model-select\n"
	   << "       to confirm)?\n";
      exit(-10);
  }
  
  vvUint states = cluster(pairs);

  // Not all states will get clustered, so manually search for
  // "orphaned" ones and add them in...
  findOrphans(states, M.rows());


  cout << "# " << hdr << endl;
  dumpMatrix(cout, states);


  // If this happens, you are likely very undersampled
  if (states.size() != 2)
    cerr << boost::format("Warning- clustering finished with %d states.\n") % states.size();
}
