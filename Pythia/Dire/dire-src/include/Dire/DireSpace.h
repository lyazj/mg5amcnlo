// DireSpace.h is a part of the DIRE plugin to the PYTHIA event generator.
// Copyright (C) 2016 Stefan Prestel.

// Header file for the spacelike initial-state showers.
// DireSpaceEnd: radiating dipole end in ISR.
// DireSpace: handles the showering description.

#ifndef Pythia8_DireSpace_H
#define Pythia8_DireSpace_H

#define DIRE_SPACE_VERSION "2.000"

#include "Pythia8/Basics.h"
#include "Pythia8/Pythia.h"
#include "Pythia8/SpaceShower.h"
#include "Pythia8/BeamParticle.h"
#include "Pythia8/Event.h"
#include "Pythia8/Info.h"
#include "Pythia8/ParticleData.h"
#include "Pythia8/PartonSystems.h"
#include "Pythia8/PythiaStdlib.h"
#include "Pythia8/Settings.h"
#include "Pythia8/StandardModel.h"
#include "Pythia8/UserHooks.h"
#include "Pythia8/MergingHooks.h"
#include "Pythia8/WeakShowerMEs.h"

#include "Dire/Basics.h"
#include "Dire/SplittingLibrary.h"
#include "Dire/WeightContainer.h"

namespace Pythia8 {

//==========================================================================

// Data on radiating dipole ends, only used inside DireSpace.

class DireSpaceEnd {
  
public:

  // Constructor.
  DireSpaceEnd( int systemIn = 0, int sideIn = 0, int iRadiatorIn = 0,
    int iRecoilerIn = 0, double pTmaxIn = 0., int colTypeIn = 0,
    int chgTypeIn = 0, int weakTypeIn = 0,  int MEtypeIn = 0,
    bool normalRecoilIn = true, int weakPolIn = 0,
    vector<int> iSpectatorIn = vector<int>(),
    vector<double> massIn = vector<double>() ) :
    system(systemIn), side(sideIn), iRadiator(iRadiatorIn),
    iRecoiler(iRecoilerIn), pTmax(pTmaxIn), colType(colTypeIn),
    chgType(chgTypeIn), weakType(weakTypeIn), MEtype(MEtypeIn),
    normalRecoil(normalRecoilIn), weakPol(weakPolIn), nBranch(0),
    pT2Old(0.), zOld(0.5), mass(massIn), iSpectator(iSpectatorIn) { 
    idDaughter = idMother = idSister = iFinPol = 0;
    x1  = x2 = m2Dip = pT2 = z = xMo = Q2 = mSister = m2Sister = pT2corr
        = pT2Old = zOld = asymPol = sa1 = xa = pT2start = pT2stop = 0.;
    phi = phia1 = -1.;
  }

  // Explicit copy constructor. 
  DireSpaceEnd( const DireSpaceEnd& dip )
    : system(dip.system), side(dip.side), iRadiator(dip.iRadiator),
      iRecoiler(dip.iRecoiler), pTmax(dip.pTmax), colType(dip.colType),
      chgType(dip.chgType), weakType(dip.weakType), MEtype(dip.MEtype),
      normalRecoil(dip.normalRecoil), weakPol(dip.weakPol),
      nBranch(dip.nBranch), idDaughter(dip.idDaughter), idMother(dip.idMother),
      idSister(dip.idSister), iFinPol(dip.iFinPol), x1(dip.x1), x2(dip.x2),
      m2Dip(dip.m2Dip), pT2(dip.pT2), z(dip.z), xMo(dip.xMo), Q2(dip.Q2),
      mSister(dip.mSister), m2Sister(dip.m2Sister), pT2corr(dip.pT2corr),
      pT2Old(dip.pT2Old), zOld(dip.zOld), asymPol(dip.asymPol), phi(dip.phi),
      pT2start(dip.pT2start), pT2stop(dip.pT2stop), sa1(dip.sa1), xa(dip.xa),
      phia1(dip.phia1), mass(dip.mass), iSpectator(dip.iSpectator) {}

  // Store values for trial emission.
  void store( int idDaughterIn, int idMotherIn, int idSisterIn,
    double x1In, double x2In, double m2DipIn, double pT2In, double zIn,
    double sa1In, double xaIn, double xMoIn, double Q2In, double mSisterIn,
    double m2SisterIn, double pT2corrIn, double phiIn = -1.,
    double phia1In = 1.) {
    idDaughter = idDaughterIn; idMother = idMotherIn;
    idSister = idSisterIn; x1 = x1In; x2 = x2In; m2Dip = m2DipIn;
    pT2 = pT2In; z = zIn; sa1 = sa1In; xa = xaIn; xMo = xMoIn; Q2 = Q2In;
    mSister = mSisterIn; m2Sister = m2SisterIn; pT2corr = pT2corrIn;
    phi = phiIn; phia1 = phia1In; }
 
  // Basic properties related to evolution and matrix element corrections.
  int    system, side, iRadiator, iRecoiler;
  double pTmax;
  int    colType, chgType, weakType, MEtype;
  bool   normalRecoil;
  int    weakPol;

  // Properties specific to current trial emission.
  int    nBranch, idDaughter, idMother, idSister, iFinPol;
  double x1, x2, m2Dip, pT2, z, xMo, Q2, mSister, m2Sister, pT2corr,
         pT2Old, zOld, asymPol, phi, pT2start, pT2stop;

  // Properties of 1->3 splitting.
  double sa1, xa, phia1;

  // Stored masses.
  vector<double> mass;

  // Extended list of recoilers.
  vector<int> iSpectator;

} ;
 
//==========================================================================

// The DireSpace class does spacelike showers.

class DireSpace : public Pythia8::SpaceShower {

public:

  // Constructor.
  DireSpace() {}

  DireSpace(Pythia8::Pythia* pythiaPtr) {
    beamOffset        = 0;
    pTdampFudge       = 0.;
    infoPtr           = &pythiaPtr->info;
    particleDataPtr   = &pythiaPtr->particleData;
    partonSystemsPtr  = &pythiaPtr->partonSystems;
    rndmPtr           = &pythiaPtr->rndm;
    settingsPtr       = &pythiaPtr->settings;
    userHooksPtr      = 0;
    mergingHooksPtr   = pythiaPtr->mergingHooksPtr;
    splittingsPtr     = 0;
    weights           = 0;
    printBanner       = true;
    nWeightsSave      = 0;
    dipEnd.reserve(1000000);

    // Other variables
    nMPI = 0;

  }

  // Destructor.
  virtual ~DireSpace() {}

  // Initialize generation. Possibility to force re-initialization by hand.
  virtual void init(BeamParticle* beamAPtrIn, BeamParticle* beamBPtrIn);

  bool initSplits() {
    if (splittingsPtr) splits = splittingsPtr->getSplittings();
    return (splits.size() > 0);
  }

  // Initialize various pointers.
  // (Separated from rest of init since not virtual.)
  void reinitPtr(Info* infoPtrIn, Settings* settingsPtrIn,
       ParticleData* particleDataPtrIn, Rndm* rndmPtrIn,
       PartonSystems* partonSystemsPtrIn, UserHooks* userHooksPtrIn,
       MergingHooks* mergingHooksPtrIn, SplittingLibrary* splittingsPtrIn) {
       infoPtr = infoPtrIn;
       settingsPtr = settingsPtrIn;
       particleDataPtr = particleDataPtrIn;
       rndmPtr = rndmPtrIn;
       partonSystemsPtr = partonSystemsPtrIn;
       userHooksPtr = userHooksPtrIn;
       mergingHooksPtr = mergingHooksPtrIn;
       splittingsPtr = splittingsPtrIn;
  }

  void setWeightContainerPtr(WeightContainer* weightsIn) { weights = weightsIn;}

  // Find whether to limit maximum scale of emissions, and whether to dampen.
  virtual bool limitPTmax( Event& event, double Q2Fac = 0.,
    double Q2Ren = 0.);

  // Potential enhancement factor of pTmax scale for hardest emission.
  virtual double enhancePTmax() const {return pTmaxFudge;}

  // Prepare system for evolution; identify ME.
  virtual void prepare( int iSys, Event& event, bool limitPTmaxIn = true);

  // Update dipole list after each FSR emission.
  // Usage: update( iSys, event).
  virtual void update( int , Event&, bool = false);

  // Update dipole list after initial-initial splitting.
  void updateAfterII( int iSysSelNow, int sideNow, int iDipSelNow,
    int eventSizeOldNow, int systemSizeOldNow, Event& event, int iDaughter,
    int iMother, int iSister, int iNewRecoiler, double pT2, double xNew);

  // Update dipole list after initial-initial splitting.
  void updateAfterIF( int iSysSelNow, int sideNow, int iDipSelNow,
    int eventSizeOldNow, int systemSizeOldNow, Event& event, int iDaughter,
    int iRecoiler, int iMother, int iSister, int iNewRecoiler, int iNewOther,
    double pT2, double xNew);

  // Select next pT in downwards evolution.
  virtual double pTnext( Event& event, double pTbegAll, double pTendAll,
    int nRadIn = -1, bool = false);

  // Setup branching kinematics.
  virtual bool branch( Event& event);

  bool branch_II( Event& event, bool = false,
    //const SplitInfo& split = SplitInfo());
    SplitInfo* split = NULL);
  bool branch_IF( Event& event, bool = false,
    //const SplitInfo& split = SplitInfo());
    SplitInfo* split = NULL);

  // Setup clustering kinematics.
  pair <Event, pair<int,int> > clustered_internal( const Event& state,
    int iRad, int iEmt, int iRecAft, string name);
  virtual Event clustered( const Event& state, int iRad, int iEmt, int iRecAft,
    string name) {
    return clustered_internal(state,iRad, iEmt, iRecAft, name).first; }
  bool cluster_II( const Event& state, int iRad,
    int iEmt, int iRecAft, int idRadBef, Particle& radBef, Particle& recBef,
    Event& partialState);
  bool cluster_IF( const Event& state, int iRad,
    int iEmt, int iRecAft, int idRadBef, Particle& radBef, Particle& recBef,
    Event& partialState);

  // Return ordering variable.
  // From Pythia version 8.215 onwards no longer virtual.
  double pT2Space ( const Particle& rad, const Particle& emt,
    const Particle& rec) {
    if (rec.isFinal()) return pT2_IF(rad,emt,rec);
    return pT2_II(rad,emt,rec);
  }

  double pT2_II ( const Particle& rad, const Particle& emt,
    const Particle& rec);
  double pT2_IF ( const Particle& rad, const Particle& emt,
    const Particle& rec);

  // Return auxiliary variable.
  // From Pythia version 8.215 onwards no longer virtual.
  double zSpace ( const Particle& rad, const Particle& emt,
    const Particle& rec) {
    if (rec.isFinal()) return z_IF(rad,emt,rec);
    return z_II(rad,emt,rec);
  }

  double z_II ( const Particle& rad, const Particle& emt,
    const Particle& rec);
  double z_IF ( const Particle& rad, const Particle& emt,
    const Particle& rec);

  // From Pythia version 8.218 onwards.
  // Return the evolution variable.
  // Usage: getStateVariables( const Event& event,  int iRad, int iEmt, 
  //                   int iRec, string name)
  // Important note:
  // - This map must contain an entry for the shower evolution variable, 
  //   specified with key "t".
  // - This map must contain an entry for the shower evolution variable from
  //   which the shower would be restarted after a branching. This entry
  //   must have key "tRS", 
  // - This map must contain an entry for the argument of \alpha_s used
  //   for the branching. This entry must have key "scaleAS". 
  // - This map must contain an entry for the argument of the PDFs used
  //   for the branching. This entry must have key "scalePDF". 
  virtual map<string, double> getStateVariables (const Event& state,
    int rad, int emt, int rec, string name);

  // From Pythia version 8.215 onwards.
  // Check if attempted clustering is handled by timelike shower
  // Usage: isSpacelike( const Event& event,  int iRad, int iEmt, 
  //                   int iRec, string name)
  virtual bool isSpacelike(const Event& state, int iRad, int, int, string)
    { return !state[iRad].isFinal(); }

  // From Pythia version 8.215 onwards.
  // Return a string identifier of a splitting.
  // Usage: getSplittingName( const Event& event, int iRad, int iEmt, int iRec)
  //string getSplittingName( const Event& state, int iRad, int iEmt , int )
  //  { return splittingsPtr->getSplittingName(state,iRad,iEmt).front(); }
  virtual vector<string> getSplittingName( const Event& state, int iRad, int iEmt,int)
    { return splittingsPtr->getSplittingName(state,iRad,iEmt); }

  // From Pythia version 8.215 onwards.
  // Return the splitting probability.
  // Usage: getSplittingProb( const Event& event, int iRad, int iEmt, int iRec)
  virtual double getSplittingProb( const Event& state, int iRad,
    int iEmt, int iRecAft, string);

  virtual bool allowedSplitting( const Event& state, int iRad, int iEmt);

  virtual vector<int> getRecoilers( const Event& state, int iRad, int iEmt, string name);

  // Return Jacobian for initial-initial phase space factorisation.
  double jacobian_II(double z, double pT2, double m2dip, double = 0.,
    double = 0., double = 0., double = 0., double = 0.) {
    // Calculate CS variables.
    double kappa2 = pT2 / m2dip;
    double xCS    = (z*(1-z)- kappa2)/(1-z);
    double vCS    = kappa2/(1-z); 
    return 1./xCS * ( 1. - xCS - vCS) / (1.- xCS - 2.*vCS);
  }

  // Return Jacobian for initial-final phase space factorisation.
  double jacobian_IF(double z, double pT2, double m2dip, double = 0.,
    double = 0., double = 0., double = 0., double = 0.) {
    // Calculate CS variables.
    double xCS = z;
    double uCS = (pT2/m2dip)/(1-z);
    return 1./xCS * ( 1. - uCS) / (1. - 2.*uCS);
  }

  // Auxiliary function to return the position of a particle.
  // Should go int Event class eventually!
  int FindParticle( const Particle& particle, const Event& event,
    bool checkStatus = true );

  // Print dipole list; for debug mainly.
  virtual void list() const;

  Event makeHardEvent( int iSys, const Event& state, bool isProcess = false );
  //bool hasME(const Event& event);

  // Check colour/flavour correctness of state.
  bool validEvent( const Event& state, bool isProcess = false );

  // Check that mother-daughter-relations are correctly set.
  bool validMotherDaughter( const Event& state );

  // Find index colour partner for input colour.
  int FindCol(int col, vector<int> iExc, const Event& event, int type,
    int iSys = -1);

  // Pointers to the two incoming beams.
  BeamParticle*  getBeamA () { return beamAPtr; }
  BeamParticle*  getBeamB () { return beamBPtr; }

  // Pointer to Standard Model couplings.
  CoupSM* getCoupSM () { return coupSMPtr; }

  // Function to calculate the correct alphaS/2*Pi value, including
  // renormalisation scale variations + threshold matching.
  double alphasNow( double pT2, double renormMultFacNow = 1., int iSys = 0 );

private:

  // Number of times the same error message is repeated, unless overridden.
  static const int TIMESTOPRINT;

  // Allow conversion from mb to pb.
  static const double CONVERTMB2PB;

  // Colour factors.
  static const double CA, CF, TR, NC;

  // Store common beam quantities.
  int    idASave, idBSave;

protected:

  // Store properties to be returned by methods.
  int    iSysSel;
  double pTmaxFudge;

private:

  // Constants: could only be changed in the code itself.
  static const int    MAXLOOPTINYPDF;
  static const double MCMIN, MBMIN, CTHRESHOLD, BTHRESHOLD, EVALPDFSTEP, 
         TINYPDF, TINYKERNELPDF, TINYPT2, HEAVYPT2EVOL, HEAVYXEVOL, 
         EXTRASPACEQ, LAMBDA3MARGIN, PT2MINWARN, LEPTONXMIN, LEPTONXMAX, 
         LEPTONPT2MIN, LEPTONFUDGE, HEADROOMQ2Q, HEADROOMQ2G, 
         HEADROOMG2G, HEADROOMG2Q, TINYMASS;
  static const double G2QQPDFPOW1, G2QQPDFPOW2; 

  // Initialization data, normally only set once.
  bool   doQCDshower, doQEDshowerByQ, doQEDshowerByL, useSamePTasMPI,
         doMEcorrections, doMEafterFirst, doPhiPolAsym,
         doPhiIntAsym, doRapidityOrder, useFixedFacScale, doSecondHard,
         canVetoEmission, hasUserHooks, alphaSuseCMW, printBanner, doTrialNow;
  int    pTmaxMatch, pTdampMatch, alphaSorder, alphaSnfmax,
         nQuarkIn, enhanceScreening, nFinalMax, kernelOrder, kernelOrderMPI,
         nWeightsSave, nMPI;
  double pTdampFudge, mc, mb, m2c, m2b, m2cPhys, m2bPhys, renormMultFac,
         factorMultFac, fixedFacScale2, alphaSvalue, alphaS2pi, Lambda3flav,
         Lambda4flav, Lambda5flav, Lambda3flav2, Lambda4flav2, Lambda5flav2,
         pT0Ref, ecmRef, ecmPow, pTmin, sCM, eCM, pT0, pT20,
         pT2min, m2min, mTolErr, pTmaxFudgeMPI, strengthIntAsym;
  double alphaS2piOverestimate;
  bool usePDFalphas, usePDFmasses, useSummedPDF;
  bool useGlobalMapIF, forceMassiveMap;

  // alphaStrong and alphaEM calculations.
  AlphaStrong alphaS;

  // Some current values.
  bool   sideA, dopTlimit1, dopTlimit2, dopTdamp;
  int    iNow, iRec, idDaughter, nRad, idResFirst, idResSecond;
  double xDaughter, x1Now, x2Now, m2Dip, m2Rec, pT2damp, pTbegRef, pdfScale2;

  // List of emissions in different sides in different systems:
  vector<int> nRadA,nRadB;

  // All dipole ends
  vector<DireSpaceEnd> dipEnd;

  // Pointers to the current and hardest (so far) dipole ends.
  int iDipNow, iSysNow;
  DireSpaceEnd* dipEndNow;
  int iDipSel;
  DireSpaceEnd* dipEndSel;

  void setupQCDdip( int iSys, int side, int colTag, int colSign,
    const Event& event, int MEtype, bool limitPTmaxIn);

  void getQCDdip( int iRad, int colTag, int colSign,
    const Event& event, vector<DireSpaceEnd>& dipEnds);

  virtual int system() const { return iSysSel;}
 
  // Evolve a QCD dipole end.
  void pT2nextQCD( double pT2begDip, double pT2endDip,
    DireSpaceEnd& dip, Event& event);
  bool pT2nextQCD_II( double pT2begDip, double pT2endDip,
    DireSpaceEnd& dip, Event& event);
  bool pT2nextQCD_IF( double pT2begDip, double pT2endDip,
    DireSpaceEnd& dip, Event& event);
  bool zCollNextQCD( DireSpaceEnd* dip, double zMin, double zMax,
    double tMin = 0., double tMax = 0.);
  bool virtNextQCD( DireSpaceEnd* dip, double tMin, double tMax,
    double zMin =-1., double zMax =-1.);

  // Function to determine how often the integrated overestimate should be
  // recalculated.
  double evalpdfstep(int idRad, double pT2, double m2cp = -1.,
    double m2bp = -1.) {

    return 1.;

    double ret = 0.1;
    if (m2cp < 0.) m2cp = particleDataPtr->m0(4);
    if (m2bp < 0.) m2bp = particleDataPtr->m0(5);
    // More steps close to the thresholds.
    if ( abs(idRad) == 4 && pT2 < 1.2*m2cp && pT2 > m2cp) ret = 1.0;
    if ( abs(idRad) == 5 && pT2 < 1.2*m2bp && pT2 > m2bp) ret = 1.0;
    return ret;
  }

  // Evolve a QCD dipole end near heavy quark threshold region.
  void pT2nearQCDthreshold( BeamParticle& beam, double m2Massive,
    double m2Threshold, double xMaxAbs, double zMinAbs,
    double zMaxMassive);

  SplittingLibrary* splittingsPtr;

  // Number of proposed splittings in hard scattering systems.
  map<int,int> nProposedPT;

  // Return headroom factors for integrated/differential overestimates.
  double overheadFactors( string, int, bool, double, double);
  double enhanceOverestimateFurther( string, int, double );

  // Function to fill map of integrated overestimates.
  void getNewOverestimates( int, DireSpaceEnd*, const Event&, double,
    double, double, double, multimap<double,string>& );

  // Function to sum all integrated overestimates.
  void addNewOverestimates( multimap<double,string>, double&);

  // Function to attach the correct alphaS weights to the kernels.
  void alphasReweight(double pT2, int iSys, double& weight,
    double& fullWeight, double& overWeight, double renormMultFacNow);

  // Function to evaluate the accept-probability, including picking of z.
  void getNewSplitting( const Event&, DireSpaceEnd*, double, double, double,
    double, double, int, string, int&, int&, double&, double&, 
    map<string,double>&, double&);

  //double generateMEC ( const Event& state, const int type, Splitting* split);
  pair<double,double> generateMEC ( const Event& state, const int type, Splitting* split);

  // Get particle masses.
  double getMass(int id, int strategy, double mass = 0.) {
    BeamParticle& beam = (abs(beamAPtr->id()) == 2212) ? *beamAPtr : *beamBPtr;
    double mRet = 0.;
    if (strategy == 1) mRet = particleDataPtr->m0(id);
    if (strategy == 2 &&  usePDFmasses) mRet = beam.mQuarkPDF(id);
    if (strategy == 2 && !usePDFmasses) mRet = particleDataPtr->m0(id);
    if (strategy == 3) mRet = mass;
    if (mRet < TINYMASS) mRet = 0.;
    return pow2(max(0.,mRet));
  }

  // Check if variables are in allowed phase space.
  bool inAllowedPhasespace(int kinType, double z, double pT2, double m2dip,
    double xOld, int splitType = 0, double m2RadBef = 0.,
    double m2r = 0.,  double m2s = 0., double m2e = 0.,
    vector<double> aux = vector<double>());

  // Kallen function and derived quantities. Helpers for massive phase
  // space mapping.
  double lABC(double a, double b, double c) { return pow2(a-b-c) - 4.*b*c;}
  double bABC(double a, double b, double c) { 
    double ret = 0.;
    if      ((a-b-c) > 0.) ret = sqrt(lABC(a,b,c));
    else if ((a-b-c) < 0.) ret =-sqrt(lABC(a,b,c));
    else                   ret = 0.;
    return ret; }
  double gABC(double a, double b, double c) { return 0.5*(a-b-c+bABC(a,b,c));}

  // Function to attach the correct alphaS weights to the kernels.
  // Auxiliary function to get number of flavours.
  double getNF(double pT2);

  // Auxiliary functions to get beta function coefficients.
  double beta0 (double NF)
    { return 11./6.*CA - 2./3.*NF*TR; }
  double beta1 (double NF)
    { return 17./6.*pow2(CA) - (5./3.*CA+CF)*NF*TR; }
  double beta2 (double NF)
    { return 2857./432.*pow(CA,3)
    + (-1415./216.*pow2(CA) - 205./72.*CA*CF + pow2(CF)/4.) *TR*NF
    + ( 79.*CA + 66.*CF)/108.*pow2(TR*NF); }

  // Identifier of the splitting
  string splittingNowName, splittingSelName;

  // Weighted shower book-keeping.
  map<string, map<double,double> > acceptProbability;
  map<string, multimap<double,double> > rejectProbability;

public:

  WeightContainer* weights;

private:

  bool doVariations;

  // List of splitting kernels.
  map<string, Splitting* > splits;

};
 
//==========================================================================

} // end namespace Pythia8

#endif