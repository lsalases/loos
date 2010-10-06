
/*
  
  (c) 2009,2010 Tod D. Romo, Grossfield Lab, URMC...

  Perform a bootstrap analysis of a trajectory


  usage- boot-bcom model trajectory selection modes replicates [0|seed] block-list

  Notes:
    o if modes = 0, then use all available modes
*/


#include <loos.hpp>
#include "bcomlib.hpp"

using namespace std;
using namespace loos;
using namespace Convergence;


const bool debug = false;


typedef vector<AtomicGroup>                               vGroup;
typedef boost::tuple<RealMatrix, RealMatrix, RealMatrix>  SVDResult;


struct Datum {
  Datum(const double avg, const double var, const double power) : avg_coverlap(avg),
                                                                  var_coverlap(var),
                                                                  avg_power(power) { }


  double avg_coverlap;
  double var_coverlap;
  double avg_power;
};




const bool length_normalize = true;



vector<uint> pickFrames(const uint nframes, const uint blocksize) {
  
  boost::uniform_int<uint> imap(0,nframes-1);
  boost::variate_generator< base_generator_type&, boost::uniform_int<uint> > rng(rng_singleton(), imap);
  vector<uint> picks;

  for (uint i=0; i<blocksize; ++i)
    picks.push_back(rng());

  return(picks);
}


void dumpPicks(const vector<uint>& picks) {
  cerr << "Picks:\n";
  for (vector<uint>::const_iterator ci = picks.begin(); ci != picks.end(); ++ci)
    cerr << "\t" << *ci << endl;
}


vGroup subgroup(const vGroup& A, const vector<uint>& picks) {
  vGroup B;
  
  for (vector<uint>::const_iterator ci = picks.begin(); ci != picks.end(); ++ci)
    B.push_back(A[*ci]);

  return(B);
}



double sum(const RealMatrix& v) {
  double s = 0.0;
  for (uint j=0; j<v.rows(); ++j)
    s += v[j];

  return(s);
}


template<class ExtractPolicy>
Datum blocker(const RealMatrix& Ua, const RealMatrix sa, const vGroup& ensemble, const uint blocksize, uint repeats, ExtractPolicy& policy) {


  
  double sa_sum = sum(sa);
  vector<double> coverlaps;
  vector<double> powers;

  for (uint i=0; i<repeats; ++i) {
    vector<uint> picks = pickFrames(ensemble.size(), blocksize);
    
    if (debug) {
      cerr << "***Block " << blocksize << ", replica " << i << ", picks " << picks.size() << endl;
      dumpPicks(picks);
    }
    
    vGroup subset = subgroup(ensemble, picks);
    boost::tuple<RealMatrix, RealMatrix> pca_result = pca(subset, policy);
    RealMatrix s = boost::get<0>(pca_result);
    RealMatrix U = boost::get<1>(pca_result);

    if (length_normalize)
      for (uint j=0; j<s.rows(); ++j)
        s[j] /= blocksize;

    powers.push_back(sa_sum / sum(s));
    coverlaps.push_back(covarianceOverlap(sa, Ua, s, U));
  }

  TimeSeries<double> coverlaps_t(coverlaps);
  TimeSeries<double> powt(powers);
  return( Datum(coverlaps_t.average(), coverlaps_t.variance(), powt.average()) );

}




int main(int argc, char *argv[]) {
  string hdr = invocationHeader(argc, argv);
  int k=1;

  if (argc != 8) {
    cerr << "Usage- " << argv[0] << " model traj sel replicates [0|seed] [1=local avg|0=global avg] blocks\n";
    exit(0);
  }

  AtomicGroup model = createSystem(argv[k++]);
  pTraj traj = createTrajectory(argv[k++], model);

  AtomicGroup subset = selectAtoms(model, argv[k++]);
  int nreps = strtol(argv[k++],0, 10);
  uint seed = strtol(argv[k++],0, 10);
  if (seed == 0)
    seed = randomSeedRNG();
  else
    rng_singleton().seed(static_cast<ulong>(seed));

  int local_flag = atoi(argv[k++]);

  vector<uint> blocksizes = parseRangeList<uint>(argv[k++]);

  vector<AtomicGroup> ensemble;
  readTrajectory(ensemble, subset, traj);

  // First, get the complete PCA result...
  boost::tuple<std::vector<XForm>, greal, int> ares = iterativeAlignment(ensemble);
  AtomicGroup avg = averageStructure(ensemble);
  NoAlignPolicy policy(avg, local_flag);
  boost::tuple<RealMatrix, RealMatrix> res = pca(ensemble, policy);

  RealMatrix Us = boost::get<0>(res);
  RealMatrix UA = boost::get<1>(res);


  if (length_normalize)
    for (uint i=0; i<Us.rows(); ++i)
      Us[i] /= traj->nframes();


  
  cout << "# " << hdr << endl;
  cout << "# Config flags: length_normalize=" << length_normalize << endl;
  cout << "# Alignment converged to " << boost::get<1>(ares) << " in " << boost::get<2>(ares) << " iterations\n";
  cout << "# seed = " << seed << endl;
  cout << "# n\tCoverlap\tVariance\tAvg Pow Ratio\n";
  // Now iterate over all requested block sizes...

  PercentProgress watcher;
  ProgressCounter<PercentTrigger, EstimatingCounter> slayer(PercentTrigger(0.1), EstimatingCounter(blocksizes.size()));
  slayer.attach(&watcher);
  slayer.start();


  for (vector<uint>::iterator i = blocksizes.begin(); i != blocksizes.end(); ++i) {
    Datum result = blocker(UA, Us, ensemble, *i, nreps, policy);
    cout << *i << "\t" << result.avg_coverlap << "\t" << result.var_coverlap << "\t" << result.avg_power << endl;
    slayer.update();
  }

  slayer.finish();
}
