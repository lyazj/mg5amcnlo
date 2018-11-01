// Merging.cc is a part of the DIRE plugin to the PYTHIA event generator.
// Copyright (C) 2018 Stefan Prestel.

#include "Dire/Merging.h"
#include "Dire/DireSpace.h"
#include "Dire/DireTimes.h"

namespace Pythia8 {

//==========================================================================

// The Merging class.

//--------------------------------------------------------------------------


// Check colour/flavour correctness of state.

bool validEvent( const Event& event ) {

  bool validColour  = true;
  bool validCharge  = true;
  bool validMomenta = true;
  double mTolErr=1e-2;

  // Check charge sum in initial and final state
  double initCharge = event[3].charge() + event[4].charge();
  double finalCharge = 0.0;
  for(int i = 0; i < event.size(); ++i)
    if (event[i].isFinal()) finalCharge += event[i].charge();
  if (abs(initCharge-finalCharge) > 1e-12) validCharge = false;

  // Check that overall pT is vanishing.
  Vec4 pSum(0.,0.,0.,0.);
  for ( int i = 0; i < event.size(); ++i) {
    //if ( i ==3 || i == 4 )    pSum -= event[i].p();
    if ( event[i].status() == -21 ) pSum -= event[i].p();
    if ( event[i].isFinal() )       pSum += event[i].p();
  }
  if ( abs(pSum.px()) > mTolErr || abs(pSum.py()) > mTolErr) {
    validMomenta = false;
  }

  if ( event[3].status() == -21
    && (abs(event[3].px()) > mTolErr || abs(event[3].py()) > mTolErr)){
    validMomenta = false;
  }
  if ( event[4].status() == -21
    && (abs(event[4].px()) > mTolErr || abs(event[4].py()) > mTolErr)){
    validMomenta = false;
  }

  return (validColour && validCharge && validMomenta);

}

//--------------------------------------------------------------------------

// Initialise Merging class

void MyMerging::init(){
  // Reset minimal tms value.
  tmsNowMin             = infoPtr->eCM();

  if (!myLHEF3Ptr) myLHEF3Ptr = new LHEF3FromPythia8(0, settingsPtr,
    infoPtr, particleDataPtr, 15, false);

}

//--------------------------------------------------------------------------

// Function to print information.
void MyMerging::statistics() {

  // Recall switch to enfore merging scale cut.
  bool enforceCutOnLHE  = settingsPtr->flag("Merging:enforceCutOnLHE");
  // Recall merging scale value.
  double tmsval         = mergingHooksPtr->tms();
  bool printBanner      = enforceCutOnLHE && tmsNowMin > TMSMISMATCH*tmsval
                        && tmsval > 0.;
  // Reset minimal tms value.
  tmsNowMin             = infoPtr->eCM();

  if (settingsPtr->flag("Dire:doMOPS") ) printBanner = false;

  if (!printBanner) return;

  // Header.
  cout << "\n *-------  PYTHIA Matrix Element Merging Information  ------"
       << "-------------------------------------------------------*\n"
       << " |                                                            "
       << "                                                     |\n";
  // Print warning if the minimal tms value of any event was significantly
  // above the desired merging scale value.
  cout << " | Warning in MyMerging::statistics: All Les Houches events"
       << " significantly above Merging:TMS cut. Please check.       |\n";

  // Listing finished.
  cout << " |                                                            "
       << "                                                     |\n"
       << " *-------  End PYTHIA Matrix Element Merging Information -----"
       << "-----------------------------------------------------*" << endl;
}

//--------------------------------------------------------------------------

void MyMerging::storeInfos() {

  // Clear previous information.
  clearInfos();

  // Store information on every possible last clustering.
  for ( int i = 0 ; i < int(myHistory->children.size()); ++i) {

    //// Get all clustering variables.
    map<string,double> stateVars;
    int rad = myHistory->children[i]->clusterIn.radPos();
    int emt = myHistory->children[i]->clusterIn.emtPos();
    int rec = myHistory->children[i]->clusterIn.recPos();
    bool isFSR = myHistory->showers->timesPtr->isTimelike(myHistory->state, rad, emt, rec, "");
    if (isFSR)
      stateVars = myHistory->showers->timesPtr->getStateVariables(myHistory->state,rad,emt,rec,"");
    else
      stateVars = myHistory->showers->spacePtr->getStateVariables(myHistory->state,rad,emt,rec,"");
    double t    = stateVars["t"];
    double mass = myHistory->children[i]->clusterIn.mass();

    // Just store pT for now.
    stoppingScalesSave.push_back(t);
    radSave.push_back(rad);
    emtSave.push_back(emt);
    recSave.push_back(rec);
    mDipSave.push_back(mass);

    isFSR = myHistory->showers->timesPtr->isTimelike(myHistory->state, rec, emt, rad, "");
    if (isFSR)
      stateVars = myHistory->showers->timesPtr->getStateVariables(myHistory->state,rec,emt,rad,"");
    else
      stateVars = myHistory->showers->spacePtr->getStateVariables(myHistory->state,rec,emt,rad,"");
    t = stateVars["t"];

    stoppingScalesSave.push_back(t);
    radSave.push_back(rec);
    emtSave.push_back(emt);
    recSave.push_back(rad);
    mDipSave.push_back(mass);

    //cout << "Emission of "
    // <<  myHistory->state[myHistory->children[i]->clusterIn.emtPos()].id()
    // << " at pT "
    // << myHistory->children[i]->clusterIn.pT() << endl;
    //
    //// Just store pT for now.
    //stoppingScalesSave.push_back(myHistory->children[i]->clusterIn.pT());
    //radSave.push_back(myHistory->children[i]->clusterIn.radPos());
    //emtSave.push_back(myHistory->children[i]->clusterIn.emtPos());
    //recSave.push_back(myHistory->children[i]->clusterIn.recPos());
    //mDipSave.push_back(myHistory->children[i]->clusterIn.mass());

  }

}

//--------------------------------------------------------------------------

void MyMerging::getStoppingInfo(double scales [100][100],
  double masses [100][100]) {

  int posOffest=2;
  for (unsigned int i=0; i < radSave.size(); ++i){
    scales[radSave[i]-posOffest][recSave[i]-posOffest] = stoppingScalesSave[i];
    masses[radSave[i]-posOffest][recSave[i]-posOffest] = mDipSave[i];
  }

}

double MyMerging::generateSingleSudakov ( double pTbegAll, 
  double pTendAll, double m2dip, int idA, int type, double s, double x) {

/*
// For testing.
cout << "\n\n\n current type " << type << endl;
isr->noEmissionProbability( pTbegAll, pTendAll, m2dip, 1, -1, s, x);
isr->noEmissionProbability( pTbegAll, pTendAll, m2dip, 1,  1, s, x);
fsr->noEmissionProbability( pTbegAll, pTendAll, m2dip, 1,  1, s, x);
fsr->noEmissionProbability( pTbegAll, pTendAll, m2dip, 1, -1, s, x);
isr->noEmissionProbability( pTbegAll, pTendAll, m2dip,-1, -1, s, x);
isr->noEmissionProbability( pTbegAll, pTendAll, m2dip,-1,  1, s, x);
fsr->noEmissionProbability( pTbegAll, pTendAll, m2dip,-1,  1, s, x);
fsr->noEmissionProbability( pTbegAll, pTendAll, m2dip,-1, -1, s, x);
isr->noEmissionProbability( pTbegAll, pTendAll, m2dip,21, -1, s, x);
isr->noEmissionProbability( pTbegAll, pTendAll, m2dip,21,  1, s, x);
fsr->noEmissionProbability( pTbegAll, pTendAll, m2dip,21,  1, s, x);
fsr->noEmissionProbability( pTbegAll, pTendAll, m2dip,21, -1, s, x);
abort();*/

  // II
  if (type == 1) {
    return isr->noEmissionProbability( pTbegAll, pTendAll, m2dip, idA,
      -1, s, x);
  // FF
  } else if (type == 2) {
    return fsr->noEmissionProbability( pTbegAll, pTendAll, m2dip, idA,
      1, s, x);
  // IF
  } else if (type == 3) {
    return isr->noEmissionProbability( pTbegAll, pTendAll, m2dip, idA,
      1, s, x);
  // FI
  } else if (type == 4) {
    return fsr->noEmissionProbability( pTbegAll, pTendAll, m2dip, idA,
      -1, s, x);
  }

  return 1.;

}


//--------------------------------------------------------------------------

// Function to perform CKKW-L merging on this event.

/*int MyMerging::genSud( Event& process) {

  double startingScale = infoPtr->scalup();
  double s = pow2(infoPtr->eCM());

  int iz=0, ig = 0;
  for (int i = process.size()-1; i > 0; --i)
    if (process[i].idAbs() == 23) { iz = i; break; }
  for (int i = process.size()-1; i > 0; --i)
    if (process[i].colType() != 0) { ig = i; break; }
  double stoppingScale = process[iz].pT();

  double m2dip = process[iz].m2Calc();

  int idA = process[3].id();
  int idB = process[4].id();
  int type = -1;
  double xA = 2.*process[3].e()/infoPtr->eCM();
  double xB = 2.*process[4].e()/infoPtr->eCM();

  double sbi = -2.*process[3].p()*process[ig].p();
  double sai = -2.*process[4].p()*process[ig].p();
  double sab =  2.*process[3].p()*process[4].p();
  double zA = 1 + sbi/sab;
  double zB = 1 + sai/sab;

zA = isr->z_II(process[3], process[ig], process[4]);

  // Calculate CS variables.
double pT2    = isr->pT2_II(process[3], process[ig], process[4]);
double Q2     = 2.*process[3].p()*process[4].p()
                - 2.*process[3].p()*process[ig].p()
                - 2.*process[ig].p()*process[4].p();
double kappa2 = pT2 / Q2;
double xCS    = (zA*(1-zA)- kappa2)/(1-zA);

stoppingScale = sqrt(pT2);

//process.list();
//cout << scientific << setprecision(4) << "z=" << zA << " xOld= " << xCS*xA << endl;

  double w1 = isr->noEmissionProbability( startingScale, stoppingScale, m2dip, idA,
    type, s, xCS*xA);

//cout << scientific << setprecision(4) << " x= " << zA*xA << " " << zB*xA << "\t\t" << zB*xB << endl;

//  double w2 = isr->noEmissionProbability( startingScale, stoppingScale, m2dip, idB,
//    type, s, zB*xB);

  double w2 = 1.;
if (process[ig].idAbs() < 9 && process[3].id() != 21) w1= 1.;

  bool includeWGT = mergingHooksPtr->includeWGTinXSEC();
  // Save the weight of the event for histogramming.
  if (!includeWGT) mergingHooksPtr->setWeightCKKWL(w1*w2);
  // Update the event weight.
  double norm = (abs(infoPtr->lhaStrategy()) == 4) ? 1/1e9 : 1.;
  if ( includeWGT) infoPtr->updateWeight(infoPtr->weight()*w1*w2*norm);

  // Done
  return 1;

}*/



//--------------------------------------------------------------------------

// Function to steer different merging prescriptions.

int MyMerging::mergeProcess(Event& process){

    // Clear all previous event-by-event information.
    clearInfos();
//stoppingScalesSave.clear();
//stoppingScalesSave.push_back(-1.0);
//stoppingScalesSave.push_back(1.0);
//stoppingScalesSave.push_back(2.0);
//stoppingScalesSave.push_back(3.0);

//  int ig =0;
//  for (int i = process.size()-1; i > 0; --i)
//    if (process[i].colType() != 0) { ig = i; break; }
//  if (process[ig].id() != 21) return 0;

  //process.list(true,true);
  //infoPtr->scales->list(cout);

  int vetoCode = 1;

  // Reinitialise hard process.
  mergingHooksPtr->hardProcess->clear();
  string processNow = settingsPtr->word("Merging:Process");
  mergingHooksPtr->hardProcess->initOnProcess(processNow, particleDataPtr);

  // Remove whitespace from process string
  while(processNow.find(" ", 0) != string::npos)
    processNow.erase(processNow.begin()+processNow.find(" ",0));
  mergingHooksPtr->processSave = processNow;

  mergingHooksPtr->doUserMergingSave
    = settingsPtr->flag("Merging:doUserMerging");
  mergingHooksPtr->doMGMergingSave
    = settingsPtr->flag("Merging:doMGMerging");
  mergingHooksPtr->doKTMergingSave
    = settingsPtr->flag("Merging:doKTMerging");
  mergingHooksPtr->doPTLundMergingSave
    = settingsPtr->flag("Merging:doPTLundMerging");
  mergingHooksPtr->doCutBasedMergingSave
    = settingsPtr->flag("Merging:doCutBasedMerging");
  mergingHooksPtr->doNL3TreeSave
    = settingsPtr->flag("Merging:doNL3Tree");
  mergingHooksPtr->doNL3LoopSave
    = settingsPtr->flag("Merging:doNL3Loop");
  mergingHooksPtr->doNL3SubtSave
    = settingsPtr->flag("Merging:doNL3Subt");
  mergingHooksPtr->doUNLOPSTreeSave
    = settingsPtr->flag("Merging:doUNLOPSTree");
  mergingHooksPtr->doUNLOPSLoopSave
    = settingsPtr->flag("Merging:doUNLOPSLoop");
  mergingHooksPtr->doUNLOPSSubtSave
    = settingsPtr->flag("Merging:doUNLOPSSubt");
  mergingHooksPtr->doUNLOPSSubtNLOSave
    = settingsPtr->flag("Merging:doUNLOPSSubtNLO");
  mergingHooksPtr->doUMEPSTreeSave
    = settingsPtr->flag("Merging:doUMEPSTree");
  mergingHooksPtr->doUMEPSSubtSave
    = settingsPtr->flag("Merging:doUMEPSSubt");
  mergingHooksPtr->nReclusterSave
    = settingsPtr->mode("Merging:nRecluster");

  mergingHooksPtr->hasJetMaxLocal  = false;
  mergingHooksPtr->nJetMaxLocal
    = mergingHooksPtr->nJetMaxSave;
  mergingHooksPtr->nJetMaxNLOLocal
    = mergingHooksPtr->nJetMaxNLOSave;
  mergingHooksPtr->nRequestedSave
    = settingsPtr->mode("Merging:nRequested");

  // Reset to default merging scale.
  mergingHooksPtr->tms(mergingHooksPtr->tmsCut());

  // Ensure that merging weight is not counted twice.
  bool includeWGT = mergingHooksPtr->includeWGTinXSEC();


//return genSud(process);


  // Possibility to apply merging scale to an input event.
  bool applyTMSCut = settingsPtr->flag("Merging:doXSectionEstimate");
  if ( applyTMSCut && cutOnProcess(process) ) {
    if (includeWGT) infoPtr->updateWeight(0.);
    return -1;
  }

  // Done if only a cut should be applied.
  if ( applyTMSCut ) return 1;

  if (settingsPtr->flag("Dire:doMerging") ){

    int nPartons = 0;

    // Do not include resonance decay products in the counting.
    Event newp( mergingHooksPtr->bareEvent( process, false) );
    // Get the maximal quark flavour counted as "additional" parton.
    int nQuarksMerge = settingsPtr->mode("Merging:nQuarksMerge");
    // Loop through event and count.
    for(int i=0; i < int(newp.size()); ++i)
      if ( newp[i].isFinal()
        && newp[i].colType()!= 0
        && ( newp[i].id() == 21 || newp[i].idAbs() <= nQuarksMerge))
        nPartons++;

    if (settingsPtr->word("Merging:process").compare("pp>aj") == 0)
      nPartons -= 1;
    if (settingsPtr->word("Merging:process").compare("pp>jj") == 0)
      nPartons -= 2;

    // Set number of requested partons.
    if (!settingsPtr->flag("Dire:doMcAtNloDelta"))
      settingsPtr->mode("Merging:nRequested", nPartons);







    settingsPtr->mode("Merging:nRequested", nPartons);






    mergingHooksPtr->hasJetMaxLocal  = false;
    mergingHooksPtr->nJetMaxLocal
      = mergingHooksPtr->nJetMaxSave;
    mergingHooksPtr->nJetMaxNLOLocal
      = mergingHooksPtr->nJetMaxNLOSave;
    mergingHooksPtr->nRequestedSave
      = settingsPtr->mode("Merging:nRequested");

    // Reset to default merging scale.
    mergingHooksPtr->tms(mergingHooksPtr->tmsCut());

    bool allowReject = settingsPtr->flag("Merging:applyVeto");

    // For ME corrections, only do mergingHooksPtr reinitialization here,
    // and do not perform any veto.
    if ( settingsPtr->flag("Dire:doMECs")) return 1;

    bool foundHistories = generateHistories(process);
    int returnCode = (foundHistories) ? 1 : 0;

    if ( settingsPtr->flag("Dire:doGenerateSubtractions"))
      calculateSubtractions();

    //int returnCode(1);
    //double RNpath(rndmPtr->flat());
    bool useAll = settingsPtr->flag("Dire:doMOPS");

//cout << __PRETTY_FUNCTION__ << " " << __LINE__ << endl;

    double RNpath = getPathIndex(useAll);
    if ( (settingsPtr->flag("Dire:doMOPS") && returnCode > 0)
      || settingsPtr->flag("Dire:doGenerateMergingWeights") )
      returnCode = calculateWeights(RNpath, useAll);

//cout << __PRETTY_FUNCTION__ << " " << __LINE__ << endl;

    //if (!generateHistories(process) ) return -1;
    //return calculateWeights(rndmPtr->flat());

    //if (returnCode > 0) returnCode = getStartingConditions( RNpath, process);
    int tmp_code = getStartingConditions( RNpath, process);
    if (returnCode > 0) returnCode = tmp_code;

    if (returnCode == 0) mergingHooksPtr->setWeightCKKWL(0.);

    if (!allowReject && returnCode < 1) returnCode=1;

    // Store information before leaving.
    if (foundHistories) storeInfos();

    if ( settingsPtr->flag("Dire:doMOPS") ) { 
      if (returnCode < 1) mergingHooksPtr->setWeightCKKWL(0.);
      return returnCode;
    }

    // Veto if we do not want to do event generation.
    if (settingsPtr->flag("Dire:doExitAfterMerging")) return -1;

    return 1;
  }

  // Possibility to perform CKKW-L merging on this event.
  if ( mergingHooksPtr->doCKKWLMerging() )
    vetoCode = mergeProcessCKKWL(process);

  // Possibility to perform UMEPS merging on this event.
  if ( mergingHooksPtr->doUMEPSMerging() )
     vetoCode = mergeProcessUMEPS(process);

  // Possibility to perform NL3 NLO merging on this event.
  if ( mergingHooksPtr->doNL3Merging() )
    vetoCode = mergeProcessNL3(process);

  // Possibility to perform UNLOPS merging on this event.
  if ( mergingHooksPtr->doUNLOPSMerging() )
    vetoCode = mergeProcessUNLOPS(process);

  return vetoCode;

}

//--------------------------------------------------------------------------

// Function to perform CKKW-L merging on this event.

int MyMerging::mergeProcessCKKWL( Event& process) {

  // Ensure that merging hooks to not veto events in the trial showers.
  mergingHooksPtr->doIgnoreStep(true);
  // For pp > h, allow cut on state, so that underlying processes
  // can be clustered to gg > h
  if ( mergingHooksPtr->getProcessString().compare("pp>h") == 0 )
    mergingHooksPtr->allowCutOnRecState(true);

  // Construct all histories. 
  // This needs to be done because MECs can depend on unordered paths if
  // these unordered paths are ordered up to some point.
  mergingHooksPtr->orderHistories(false);

  // Ensure that merging weight is not counted twice.
  bool includeWGT = mergingHooksPtr->includeWGTinXSEC();

  // Reset weight of the event.
  double wgt = 1.0;
  mergingHooksPtr->setWeightCKKWL(1.);
  mergingHooksPtr->muMI(-1.);

  // Prepare process record for merging. If Pythia has already decayed
  // resonances used to define the hard process, remove resonance decay
  // products.
  Event newProcess( mergingHooksPtr->bareEvent( process, true) );
  // Reset any incoming spins for W+-.
  if (mergingHooksPtr->doWeakClustering())
    for (int i = 0;i < newProcess.size();++i)
      newProcess[i].pol(9);
  // Store candidates for the splitting V -> qqbar'.
  mergingHooksPtr->storeHardProcessCandidates( newProcess);

  // Check if event passes the merging scale cut.
  //double tmsval = mergingHooksPtr->tms();
  // Get merging scale in current event.
  //double tmsnow = mergingHooksPtr->tmsNow( newProcess );
  // Calculate number of clustering steps.
  int nSteps = mergingHooksPtr->getNumberOfClusteringSteps( newProcess, true);

  double tmsnow = mergingHooksPtr->tmsNow( newProcess );

  // Check if hard event cut should be applied later.
  bool allowReject = settingsPtr->flag("Merging:applyVeto");

  // Too few steps can be possible if a chain of resonance decays has been
  // removed. In this case, reject this event, since it will be handled in
  // lower-multiplicity samples.
  int nRequested = mergingHooksPtr->nRequested();

  // Store hard event cut information, reset veto information.
  mergingHooksPtr->setHardProcessInfo(nSteps, tmsnow);
  mergingHooksPtr->setEventVetoInfo(-1, -1.);

  if (nSteps < nRequested && allowReject) {
    if (!includeWGT) mergingHooksPtr->setWeightCKKWL(0.);
    if ( includeWGT) infoPtr->updateWeight(0.);
    return -1;
  }

  // Reset the minimal tms value, if necessary.
  tmsNowMin     = (nSteps == 0) ? 0. : min(tmsNowMin, tmsnow);

  // Set dummy process scale.
  newProcess.scale(0.0);
  // Generate all histories.
  MyHistory FullHistory( nSteps, 0.0, newProcess, MyClustering(), mergingHooksPtr,
            (*beamAPtr), (*beamBPtr), particleDataPtr, infoPtr,
            trialPartonLevelPtr, fsr, isr, psweights, coupSMPtr, true, true, 
            true, true, 1.0, 1.0, 1.0, 0);

  // Project histories onto desired branches, e.g. only ordered paths.
  FullHistory.projectOntoDesiredHistories();

  // Setup to choose shower starting conditions randomly.
  double sumAll(0.), sumFullAll(0.);
  for ( map<double, MyHistory*>::iterator it = FullHistory.goodBranches.begin();
    it != FullHistory.goodBranches.end(); ++it ) {
    sumAll     += it->second->prodOfProbs;
    sumFullAll += it->second->prodOfProbsFull;
  }
  // Store a double with which to access each of the paths.
  double lastp(0.);
  vector<double> path_index;
  for ( map<double, MyHistory*>::iterator it = FullHistory.goodBranches.begin();
      it != FullHistory.goodBranches.end(); ++it ) {
      // Double to access path.
      double indexNow =  (lastp + 0.5*(it->first - lastp))/sumAll;
      path_index.push_back(indexNow);
      lastp = it->first;
  }
  // Randomly pick path.
  int sizeBranches = FullHistory.goodBranches.size();
  int iPosRN = (sizeBranches > 0)
             ? rndmPtr->pick(
                 vector<double>(sizeBranches, 1./double(sizeBranches)) )
             : 0;
  double RN  = (sizeBranches > 0) ? path_index[iPosRN] : rndmPtr->flat();

  // Setup the selected path. Needed for
  FullHistory.select(RN)->setSelectedChild();

  // Do not apply cut if the configuration could not be projected onto an
  // underlying born configuration.
  bool applyCut = allowReject
                && nSteps > 0 && FullHistory.select(RN)->nClusterings() > 0;

  Event core( FullHistory.lowestMultProc(RN) );
  // Set event-specific merging scale cut. Q2-dependent for DIS.
  if ( mergingHooksPtr->getProcessString().compare("e+p>e+j") == 0
    || mergingHooksPtr->getProcessString().compare("e-p>e-j") == 0 ) {

    // Set dynamical merging scale for DIS
    if (FullHistory.isDIS2to2(core)) {
      int iInEl(0), iOutEl(0);
      for ( int i=0; i < core.size(); ++i )
        if ( core[i].idAbs() == 11 ) {
          if ( core[i].status() == -21 ) iInEl  = i;
          if ( core[i].isFinal() )       iOutEl = i;
        }
      double Q      = sqrt( -(core[iInEl].p() - core[iOutEl].p() ).m2Calc());
      double tmsCut = mergingHooksPtr->tmsCut();
      double tmsEvt = tmsCut / sqrt( 1. + pow( tmsCut/ ( 0.5*Q ), 2)  );
      mergingHooksPtr->tms(tmsEvt);

    } else if (FullHistory.isMassless2to2(core)) {
      double mT(1.);
      for ( int i=0; i < core.size(); ++i )
        if ( core[i].isFinal() ) mT *= core[i].mT();
      double Q      = sqrt(mT);
      double tmsCut = mergingHooksPtr->tmsCut();
      double tmsEvt = tmsCut / sqrt( 1. + pow( tmsCut/ ( 0.5*Q ), 2)  );
      mergingHooksPtr->tms(tmsEvt);
    }
  }
  double tmsval = mergingHooksPtr->tms();

  // Enfore merging scale cut if the event did not pass the merging scale
  // criterion.
  bool enforceCutOnLHE  = settingsPtr->flag("Merging:enforceCutOnLHE");
  if ( enforceCutOnLHE && applyCut && tmsnow < tmsval ) {
    string message="Warning in MyMerging::mergeProcessCKKWL: Les Houches Event";
    message+=" fails merging scale cut. Reject event.";
    infoPtr->errorMsg(message);
    if (!includeWGT) mergingHooksPtr->setWeightCKKWL(0.);
    if ( includeWGT) infoPtr->updateWeight(0.);
    return -1;
  }

  // Check if more steps should be taken.
  int nFinalP(0), nFinalW(0), nFinalZ(0);
  for ( int i = 0; i < core.size(); ++i )
    if ( core[i].isFinal() ) {
      if ( core[i].colType() != 0 ) nFinalP++;
      if ( core[i].idAbs() == 24 )  nFinalW++;
      if ( core[i].idAbs() == 23 )  nFinalZ++;
    }
  bool complete = (FullHistory.select(RN)->nClusterings() == nSteps) ||
    ( mergingHooksPtr->doWeakClustering() && nFinalP == 2 && nFinalW+nFinalZ == 0);
  if ( !complete ) {
    string message="Warning in MyMerging::mergeProcessCKKWL: No clusterings";
    message+=" found. History incomplete.";
    infoPtr->errorMsg(message);
  }

  // Calculate CKKWL reweighting for all paths.
  double wgtsum(0.);
  lastp = 0.;
  for ( map<double, MyHistory*>::iterator it = FullHistory.goodBranches.begin();
      it != FullHistory.goodBranches.end(); ++it ) {

      // Double to access path.
      double indexNow =  (lastp + 0.5*(it->first - lastp))/sumAll;
      lastp = it->first;

      // Probability of path.
      double probPath = it->second->prodOfProbsFull/sumFullAll;

      FullHistory.select(indexNow)->setSelectedChild();

      // Calculate CKKWL weight:
      double w = FullHistory.weightTREE( trialPartonLevelPtr,
        mergingHooksPtr->AlphaS_FSR(), mergingHooksPtr->AlphaS_ISR(),
        mergingHooksPtr->AlphaEM_FSR(), mergingHooksPtr->AlphaEM_ISR(),
        indexNow);

      wgtsum += probPath*w;
  }

  wgt = wgtsum;

  // Event with production scales set for further (trial) showering
  // and starting conditions for the shower.
  FullHistory.getStartingConditions( RN, process );
  // If necessary, reattach resonance decay products.
  mergingHooksPtr->reattachResonanceDecays(process);

  // Allow to dampen histories in which the lowest multiplicity reclustered
  // state does not pass the lowest multiplicity cut of the matrix element.
  double dampWeight = mergingHooksPtr->dampenIfFailCuts(
           FullHistory.lowestMultProc(RN) );
  // Save the weight of the event for histogramming. Only change the
  // event weight after trial shower on the matrix element
  // multiplicity event (= in doVetoStep).
  wgt *= dampWeight;

  // Save the weight of the event for histogramming.
  if (!includeWGT) mergingHooksPtr->setWeightCKKWL(wgt);

  // Update the event weight.
  double norm = (abs(infoPtr->lhaStrategy()) == 4) ? 1/1e9 : 1.;
  if ( includeWGT) infoPtr->updateWeight(infoPtr->weight()*wgt*norm);

  // Allow merging hooks to veto events from now on.
  mergingHooksPtr->doIgnoreStep(false);

  // If no-emission probability is zero.
  if ( allowReject && wgt == 0. ) return 0;

  // Done
  return 1;

}

//--------------------------------------------------------------------------

// Function to perform UMEPS merging on this event.

int MyMerging::mergeProcessUMEPS( Event& process) {

  // Initialise which part of UMEPS merging is applied.
  bool doUMEPSTree                = settingsPtr->flag("Merging:doUMEPSTree");
  bool doUMEPSSubt                = settingsPtr->flag("Merging:doUMEPSSubt");
  // Save number of looping steps
  mergingHooksPtr->nReclusterSave = settingsPtr->mode("Merging:nRecluster");
  int nRecluster                  = settingsPtr->mode("Merging:nRecluster");

  // Ensure that merging hooks does not remove emissions.
  mergingHooksPtr->doIgnoreEmissions(true);
  // For pp > h, allow cut on state, so that underlying processes
  // can be clustered to gg > h
  if ( mergingHooksPtr->getProcessString().compare("pp>h") == 0 )
    mergingHooksPtr->allowCutOnRecState(true);
  // For now, prefer construction of ordered histories.
  mergingHooksPtr->orderHistories(true);

  // Ensure that merging weight is not counted twice.
  bool includeWGT = mergingHooksPtr->includeWGTinXSEC();

  // Reset any incoming spins for W+-.
  if (mergingHooksPtr->doWeakClustering())
    for (int i = 0;i < process.size();++i)
      process[i].pol(9);

  // Reset weights of the event.
  double wgt   = 1.;
  mergingHooksPtr->setWeightCKKWL(1.);
  mergingHooksPtr->muMI(-1.);

  // Prepare process record for merging. If Pythia has already decayed
  // resonances used to define the hard process, remove resonance decay
  // products.
  Event newProcess( mergingHooksPtr->bareEvent( process, true) );
  // Store candidates for the splitting V -> qqbar'.
  mergingHooksPtr->storeHardProcessCandidates( newProcess );

  // Check if event passes the merging scale cut.
  double tmsval   = mergingHooksPtr->tms();
  // Get merging scale in current event.
  double tmsnow  = mergingHooksPtr->tmsNow( newProcess );
  // Calculate number of clustering steps.
  int nSteps = mergingHooksPtr->getNumberOfClusteringSteps( newProcess, true);
  int nRequested = mergingHooksPtr->nRequested();

  // Too few steps can be possible if a chain of resonance decays has been
  // removed. In this case, reject this event, since it will be handled in
  // lower-multiplicity samples.
  if (nSteps < nRequested) {
    if (!includeWGT) mergingHooksPtr->setWeightCKKWL(0.);
    if ( includeWGT) infoPtr->updateWeight(0.);
    return -1;
  }

  // Reset the minimal tms value, if necessary.
  tmsNowMin      = (nSteps == 0) ? 0. : min(tmsNowMin, tmsnow);

  // Get random number to choose a path.
  double RN = rndmPtr->flat();
  // Set dummy process scale.
  newProcess.scale(0.0);
  // Generate all histories.
  MyHistory FullHistory( nSteps, 0.0, newProcess, MyClustering(), mergingHooksPtr,
            (*beamAPtr), (*beamBPtr), particleDataPtr, infoPtr,
            trialPartonLevelPtr, fsr, isr, psweights, coupSMPtr, true, true, true, true, 1.0, 1.0, 1.0, 0);
  // Project histories onto desired branches, e.g. only ordered paths.
  FullHistory.projectOntoDesiredHistories();

  // Do not apply cut if the configuration could not be projected onto an
  // underlying born configuration.
  bool applyCut = nSteps > 0 && FullHistory.select(RN)->nClusterings() > 0;

  // Enfore merging scale cut if the event did not pass the merging scale
  // criterion.
  bool enforceCutOnLHE  = settingsPtr->flag("Merging:enforceCutOnLHE");
  if ( enforceCutOnLHE && applyCut && tmsnow < tmsval ) {
    string message="Warning in MyMerging::mergeProcessUMEPS: Les Houches Event";
    message+=" fails merging scale cut. Reject event.";
    infoPtr->errorMsg(message);
    if (!includeWGT) mergingHooksPtr->setWeightCKKWL(0.);
    if ( includeWGT) infoPtr->updateWeight(0.);
    return -1;
  }

  // Check reclustering steps to correctly apply MPI.
  int nPerformed = 0;
  if ( nSteps > 0 && doUMEPSSubt
    && !FullHistory.getFirstClusteredEventAboveTMS( RN, nRecluster,
          newProcess, nPerformed, false ) ) {
    // Discard if the state could not be reclustered to a state above TMS.
    if (!includeWGT) mergingHooksPtr->setWeightCKKWL(0.);
    if ( includeWGT) infoPtr->updateWeight(0.);
    return -1;
  }

  mergingHooksPtr->nMinMPI(nSteps - nPerformed);

  // Calculate CKKWL weight:
  // Perform reweighting with Sudakov factors, save alpha_s ratios and
  // PDF ratio weights.
  if ( doUMEPSTree ) {
    wgt = FullHistory.weight_UMEPS_TREE( trialPartonLevelPtr,
      mergingHooksPtr->AlphaS_FSR(), mergingHooksPtr->AlphaS_ISR(),
      mergingHooksPtr->AlphaEM_FSR(), mergingHooksPtr->AlphaEM_ISR(), RN);
  } else {
    wgt = FullHistory.weight_UMEPS_SUBT( trialPartonLevelPtr,
      mergingHooksPtr->AlphaS_FSR(), mergingHooksPtr->AlphaS_ISR(),
      mergingHooksPtr->AlphaEM_FSR(), mergingHooksPtr->AlphaEM_ISR(), RN);
  }

  // Event with production scales set for further (trial) showering
  // and starting conditions for the shower.
  if ( doUMEPSTree ) FullHistory.getStartingConditions( RN, process );
  // Do reclustering (looping) steps.
  else FullHistory.getFirstClusteredEventAboveTMS( RN, nRecluster, process,
    nPerformed, true );

  // Allow to dampen histories in which the lowest multiplicity reclustered
  // state does not pass the lowest multiplicity cut of the matrix element
  double dampWeight = mergingHooksPtr->dampenIfFailCuts(
           FullHistory.lowestMultProc(RN) );
  // Save the weight of the event for histogramming. Only change the
  // event weight after trial shower on the matrix element
  // multiplicity event (= in doVetoStep)
  wgt *= dampWeight;

  // Save the weight of the event for histogramming.
  if (!includeWGT) mergingHooksPtr->setWeightCKKWL(wgt);

  // Update the event weight.
  double norm = (abs(infoPtr->lhaStrategy()) == 4) ? 1/1e9 : 1.;
  if ( includeWGT) infoPtr->updateWeight(infoPtr->weight()*wgt*norm);

  // Set QCD 2->2 starting scale different from arbitrary scale in LHEF!
  // --> Set to minimal mT of partons.
  int nFinal = 0;
  double muf = process[0].e();
  for ( int i=0; i < process.size(); ++i )
  if ( process[i].isFinal()
    && (process[i].colType() != 0 || process[i].id() == 22 ) ) {
    nFinal++;
    muf = min( muf, abs(process[i].mT()) );
  }

  // For pure QCD dijet events (only!), set the process scale to the
  // transverse momentum of the outgoing partons.
  // Calculate number of clustering steps.
  int nStepsNew = mergingHooksPtr->getNumberOfClusteringSteps( process );
  if ( nStepsNew == 0
    && ( mergingHooksPtr->getProcessString().compare("pp>jj") == 0
      || mergingHooksPtr->getProcessString().compare("pp>aj") == 0) )
    process.scale(muf);

  // Reset hard process candidates (changed after clustering a parton).
  mergingHooksPtr->storeHardProcessCandidates( process );
  // If necessary, reattach resonance decay products.
  mergingHooksPtr->reattachResonanceDecays(process);

  // Allow merging hooks to remove emissions from now on.
  mergingHooksPtr->doIgnoreEmissions(false);

  // If no-emission probability is zero.
  if ( wgt == 0. ) return 0;

  // Done
  return 1;

}

//--------------------------------------------------------------------------

// Function to perform NL3 NLO merging on this event.

int MyMerging::mergeProcessNL3( Event& process) {

  // Initialise which part of NL3 merging is applied.
  bool doNL3Tree = settingsPtr->flag("Merging:doNL3Tree");
  bool doNL3Loop = settingsPtr->flag("Merging:doNL3Loop");
  bool doNL3Subt = settingsPtr->flag("Merging:doNL3Subt");

  // Ensure that hooks (NL3 part) to not remove emissions.
  mergingHooksPtr->doIgnoreEmissions(true);
  // Ensure that hooks (CKKWL part) to not veto events in trial showers.
  mergingHooksPtr->doIgnoreStep(true);
  // For pp > h, allow cut on state, so that underlying processes
  // can be clustered to gg > h
  if ( mergingHooksPtr->getProcessString().compare("pp>h") == 0)
    mergingHooksPtr->allowCutOnRecState(true);
  // For now, prefer construction of ordered histories.
  mergingHooksPtr->orderHistories(true);

  // Reset weight of the event
  double wgt      = 1.;
  mergingHooksPtr->setWeightCKKWL(1.);
  // Reset the O(alphaS)-term of the CKKW-L weight.
  double wgtFIRST = 0.;
  mergingHooksPtr->setWeightFIRST(0.);
  mergingHooksPtr->muMI(-1.);

  // Prepare process record for merging. If Pythia has already decayed
  // resonances used to define the hard process, remove resonance decay
  // products.
  Event newProcess( mergingHooksPtr->bareEvent( process, true) );
  // Store candidates for the splitting V -> qqbar'
  mergingHooksPtr->storeHardProcessCandidates( newProcess);

  // Check if event passes the merging scale cut.
  double tmsval  = mergingHooksPtr->tms();
  // Get merging scale in current event.
  double tmsnow  = mergingHooksPtr->tmsNow( newProcess );
  // Calculate number of clustering steps
  int nSteps = mergingHooksPtr->getNumberOfClusteringSteps( newProcess, true);
  int nRequested = mergingHooksPtr->nRequested();

  // Too few steps can be possible if a chain of resonance decays has been
  // removed. In this case, reject this event, since it will be handled in
  // lower-multiplicity samples.
  if (nSteps < nRequested) {
    mergingHooksPtr->setWeightCKKWL(0.);
    mergingHooksPtr->setWeightFIRST(0.);
    return -1;
  }

  // Reset the minimal tms value, if necessary.
  tmsNowMin = (nSteps == 0) ? 0. : min(tmsNowMin, tmsnow);

  // Enfore merging scale cut if the event did not pass the merging scale
  // criterion.
  bool enforceCutOnLHE  = settingsPtr->flag("Merging:enforceCutOnLHE");
  if ( enforceCutOnLHE && nSteps > 0 && nSteps == nRequested
    && tmsnow < tmsval ) {
    string message="Warning in MyMerging::mergeProcessNL3: Les Houches Event";
    message+=" fails merging scale cut. Reject event.";
    infoPtr->errorMsg(message);
    mergingHooksPtr->setWeightCKKWL(0.);
    mergingHooksPtr->setWeightFIRST(0.);
    return -1;
  }

  // Get random number to choose a path.
  double RN = rndmPtr->flat();
  // Set dummy process scale.
  newProcess.scale(0.0);
  // Generate all histories
  MyHistory FullHistory( nSteps, 0.0, newProcess, MyClustering(), mergingHooksPtr,
            (*beamAPtr), (*beamBPtr), particleDataPtr, infoPtr,
            trialPartonLevelPtr, fsr, isr, psweights, coupSMPtr, true, true, true, true, 1.0, 1.0, 1.0, 0);
  // Project histories onto desired branches, e.g. only ordered paths.
  FullHistory.projectOntoDesiredHistories();

  // Discard states that cannot be projected unto a state with one less jet.
  if ( nSteps > 0 && doNL3Subt
    && FullHistory.select(RN)->nClusterings() == 0 ){
    mergingHooksPtr->setWeightCKKWL(0.);
    mergingHooksPtr->setWeightFIRST(0.);
    return -1;
  }

  // Potentially recluster real emission jets for powheg input containing
  // "too many" jets, i.e. real-emission kinematics.
  bool containsRealKin = nSteps > nRequested && nSteps > 0;

  // Perform one reclustering for real emission kinematics, then apply merging
  // scale cut on underlying Born kinematics.
  if ( containsRealKin ) {
    Event dummy = Event();
    // Initialise temporary output of reclustering.
    dummy.clear();
    dummy.init( "(hard process-modified)", particleDataPtr );
    dummy.clear();
    // Recluster once.
    if ( !FullHistory.getClusteredEvent( RN, nSteps, dummy )) {
      mergingHooksPtr->setWeightCKKWL(0.);
      mergingHooksPtr->setWeightFIRST(0.);
      return -1;
    }
    double tnowNew  = mergingHooksPtr->tmsNow( dummy );
    // Veto if underlying Born kinematics do not pass merging scale cut.
    if ( enforceCutOnLHE && nSteps > 0 && nRequested > 0
      && tnowNew < tmsval ) {
      mergingHooksPtr->setWeightCKKWL(0.);
      mergingHooksPtr->setWeightFIRST(0.);
      return -1;
    }
  }

  // Remember number of jets, to include correct MPI no-emission probabilities.
  if ( doNL3Subt || containsRealKin ) mergingHooksPtr->nMinMPI(nSteps - 1);
  else mergingHooksPtr->nMinMPI(nSteps);

  // Calculate weight
  // Do LO or first part of NLO tree-level reweighting
  if( doNL3Tree ) {
    // Perform reweighting with Sudakov factors, save as ratios and
    // PDF ratio weights
    wgt = FullHistory.weightTREE( trialPartonLevelPtr,
      mergingHooksPtr->AlphaS_FSR(), mergingHooksPtr->AlphaS_ISR(),
      mergingHooksPtr->AlphaEM_FSR(), mergingHooksPtr->AlphaEM_ISR(), RN);
  } else if( doNL3Loop || doNL3Subt ) {
    // No reweighting, just set event scales properly and incorporate MPI
    // no-emission probabilities.
    wgt = FullHistory.weightLOOP( trialPartonLevelPtr, RN);
  }

  // Event with production scales set for further (trial) showering
  // and starting conditions for the shower
  if ( !doNL3Subt && !containsRealKin )
    FullHistory.getStartingConditions(RN, process);
  // For sutraction of nSteps-additional resolved partons from
  // the nSteps-1 parton phase space, recluster the last parton
  // in nSteps-parton events, and sutract later
  else {
    // Function to return the reclustered event
    if ( !FullHistory.getClusteredEvent( RN, nSteps, process )) {
      mergingHooksPtr->setWeightCKKWL(0.);
      mergingHooksPtr->setWeightFIRST(0.);
      return -1;
    }
  }

  // Allow to dampen histories in which the lowest multiplicity reclustered
  // state does not pass the lowest multiplicity cut of the matrix element
  double dampWeight = mergingHooksPtr->dampenIfFailCuts(
           FullHistory.lowestMultProc(RN) );
  // Save the weight of the event for histogramming. Only change the
  // event weight after trial shower on the matrix element
  // multiplicity event (= in doVetoStep)
  wgt *= dampWeight;

  // For tree level samples in NL3, rescale with k-Factor
  if (doNL3Tree ){
    // Find k-factor
    double kFactor = 1.;
    if( nSteps > mergingHooksPtr->nMaxJetsNLO() )
      kFactor = mergingHooksPtr->kFactor( mergingHooksPtr->nMaxJetsNLO() );
    else kFactor = mergingHooksPtr->kFactor(nSteps);
    // For NLO merging, rescale CKKW-L weight with k-factor
    wgt *= kFactor;
  }

  // Save the weight of the event for histogramming
  mergingHooksPtr->setWeightCKKWL(wgt);

  // Check if we need to subtract the O(\alpha_s)-term. If the number
  // of additional partons is larger than the number of jets for
  // which loop matrix elements are available, do standard CKKW-L
  bool doOASTree = doNL3Tree && nSteps <= mergingHooksPtr->nMaxJetsNLO();

  // Now begin NLO part for tree-level events
  if ( doOASTree ) {
    // Calculate the O(\alpha_s)-term of the CKKWL weight
    wgtFIRST = FullHistory.weightFIRST( trialPartonLevelPtr,
      mergingHooksPtr->AlphaS_FSR(), mergingHooksPtr->AlphaS_ISR(),
      mergingHooksPtr->AlphaEM_FSR(), mergingHooksPtr->AlphaEM_ISR(), RN,
      rndmPtr );
    // If necessary, also dampen the O(\alpha_s)-term
    wgtFIRST *= dampWeight;
    // Set the subtractive weight to the value calculated so far
    mergingHooksPtr->setWeightFIRST(wgtFIRST);
    // Subtract the O(\alpha_s)-term from the CKKW-L weight
    // If PDF contributions have not been included, subtract these later
    wgt = wgt - wgtFIRST;
  }

  // Set qcd 2->2 starting scale different from arbirtrary scale in LHEF!
  // --> Set to pT of partons
  double pT = 0.;
  for( int i=0; i < process.size(); ++i)
    if(process[i].isFinal() && process[i].colType() != 0) {
      pT = sqrt(pow(process[i].px(),2) + pow(process[i].py(),2));
      break;
    }
  // For pure QCD dijet events (only!), set the process scale to the
  // transverse momentum of the outgoing partons.
  if ( nSteps == 0
    && mergingHooksPtr->getProcessString().compare("pp>jj") == 0)
    process.scale(pT);

  // Reset hard process candidates (changed after clustering a parton).
  mergingHooksPtr->storeHardProcessCandidates( process );
  // If necessary, reattach resonance decay products.
  mergingHooksPtr->reattachResonanceDecays(process);

  // Allow merging hooks (NL3 part) to remove emissions from now on.
  mergingHooksPtr->doIgnoreEmissions(false);
  // Allow merging hooks (CKKWL part) to veto events from now on.
  mergingHooksPtr->doIgnoreStep(false);

  // Done
  return 1;

}

//--------------------------------------------------------------------------

// Function to perform UNLOPS merging on this event.

int MyMerging::mergeProcessUNLOPS( Event& process) {

  // Initialise which part of UNLOPS merging is applied.
  bool nloTilde         = settingsPtr->flag("Merging:doUNLOPSTilde");
  bool doUNLOPSTree     = settingsPtr->flag("Merging:doUNLOPSTree");
  bool doUNLOPSLoop     = settingsPtr->flag("Merging:doUNLOPSLoop");
  bool doUNLOPSSubt     = settingsPtr->flag("Merging:doUNLOPSSubt");
  bool doUNLOPSSubtNLO  = settingsPtr->flag("Merging:doUNLOPSSubtNLO");
  // Save number of looping steps
  mergingHooksPtr->nReclusterSave = settingsPtr->mode("Merging:nRecluster");
  int nRecluster        = settingsPtr->mode("Merging:nRecluster");

  // Ensure that merging hooks to not remove emissions
  mergingHooksPtr->doIgnoreEmissions(true);
  // For now, prefer construction of ordered histories.
  mergingHooksPtr->orderHistories(true);
  // For pp > h, allow cut on state, so that underlying processes
  // can be clustered to gg > h
  if ( mergingHooksPtr->getProcessString().compare("pp>h") == 0)
    mergingHooksPtr->allowCutOnRecState(true);

  // Reset weight of the event.
  double wgt      = 1.;
  mergingHooksPtr->setWeightCKKWL(1.);
  // Reset the O(alphaS)-term of the UMEPS weight.
  double wgtFIRST = 0.;
  mergingHooksPtr->setWeightFIRST(0.);
  mergingHooksPtr->muMI(-1.);

  // Prepare process record for merging. If Pythia has already decayed
  // resonances used to define the hard process, remove resonance decay
  // products.
  Event newProcess( mergingHooksPtr->bareEvent( process, true) );
  // Store candidates for the splitting V -> qqbar'
  mergingHooksPtr->storeHardProcessCandidates( newProcess );

  // Check if event passes the merging scale cut.
  double tmsval  = mergingHooksPtr->tms();
  // Get merging scale in current event.
  double tmsnow  = mergingHooksPtr->tmsNow( newProcess );
  // Calculate number of clustering steps
  int nSteps = mergingHooksPtr->getNumberOfClusteringSteps( newProcess, true);
  int nRequested = mergingHooksPtr->nRequested();

  // Too few steps can be possible if a chain of resonance decays has been
  // removed. In this case, reject this event, since it will be handled in
  // lower-multiplicity samples.
  if (nSteps < nRequested) {
    string message="Warning in MyMerging::mergeProcessUNLOPS: Les Houches Event";
    message+=" after removing decay products does not contain enough partons.";
    infoPtr->errorMsg(message);
    mergingHooksPtr->setWeightCKKWL(0.);
    mergingHooksPtr->setWeightFIRST(0.);
    return -1;
  }

  // Reset the minimal tms value, if necessary.
  tmsNowMin = (nSteps == 0) ? 0. : min(tmsNowMin, tmsnow);

  // Get random number to choose a path.
  double RN = rndmPtr->flat();
  // Set dummy process scale.
  newProcess.scale(0.0);
  // Generate all histories
  MyHistory FullHistory( nSteps, 0.0, newProcess, MyClustering(), mergingHooksPtr,
            (*beamAPtr), (*beamBPtr), particleDataPtr, infoPtr,
            trialPartonLevelPtr, fsr, isr, psweights, coupSMPtr, true, true, true, true, 1.0, 1.0, 1.0, 0);
  // Project histories onto desired branches, e.g. only ordered paths.
  FullHistory.projectOntoDesiredHistories();

  // Do not apply cut if the configuration could not be projected onto an
  // underlying born configuration.
  bool applyCut = nSteps > 0 && FullHistory.select(RN)->nClusterings() > 0;

  // Enfore merging scale cut if the event did not pass the merging scale
  // criterion.
  bool enforceCutOnLHE  = settingsPtr->flag("Merging:enforceCutOnLHE");
  if ( enforceCutOnLHE && applyCut && nSteps == nRequested
    && tmsnow < tmsval && tmsval > 0.) {
    string message="Warning in MyMerging::mergeProcessUNLOPS: Les Houches";
    message+=" Event fails merging scale cut. Reject event.";
    infoPtr->errorMsg(message);
    mergingHooksPtr->setWeightCKKWL(0.);
    mergingHooksPtr->setWeightFIRST(0.);
    return -1;
  }

  // Potentially recluster real emission jets for powheg input containing
  // "too many" jets, i.e. real-emission kinematics.
  bool containsRealKin = nSteps > nRequested && nSteps > 0;
  if ( containsRealKin ) nRecluster += nSteps - nRequested;

  // Remove real emission events without underlying Born configuration from
  // the loop sample, since such states will be taken care of by tree-level
  // samples.
  bool allowIncompleteReal =
    settingsPtr->flag("Merging:allowIncompleteHistoriesInReal");
  if ( doUNLOPSLoop && containsRealKin && !allowIncompleteReal
    && FullHistory.select(RN)->nClusterings() == 0 ) {
    mergingHooksPtr->setWeightCKKWL(0.);
    mergingHooksPtr->setWeightFIRST(0.);
    return -1;
  }

  // Discard if the state could not be reclustered to any state above TMS.
  int nPerformed = 0;
  if ( nSteps > 0 && !allowIncompleteReal
    && ( doUNLOPSSubt || doUNLOPSSubtNLO || containsRealKin )
    && !FullHistory.getFirstClusteredEventAboveTMS( RN, nRecluster,
          newProcess, nPerformed, false ) ) {
    mergingHooksPtr->setWeightCKKWL(0.);
    mergingHooksPtr->setWeightFIRST(0.);
    return -1;
  }

  // Check reclustering steps to correctly apply MPI.
  mergingHooksPtr->nMinMPI(nSteps - nPerformed);

  // Perform one reclustering for real emission kinematics, then apply
  // merging scale cut on underlying Born kinematics.
  if ( containsRealKin ) {
    Event dummy = Event();
    // Initialise temporary output of reclustering.
    dummy.clear();
    dummy.init( "(hard process-modified)", particleDataPtr );
    dummy.clear();
    // Recluster once.
    FullHistory.getClusteredEvent( RN, nSteps, dummy );
    double tnowNew  = mergingHooksPtr->tmsNow( dummy );
    // Veto if underlying Born kinematics do not pass merging scale cut.
    if ( enforceCutOnLHE && nSteps > 0 && nRequested > 0
      && tnowNew < tmsval && tmsval > 0.) {
      string message="Warning in MyMerging::mergeProcessUNLOPS: Les Houches";
      message+=" Event fails merging scale cut. Reject event.";
      infoPtr->errorMsg(message);
      mergingHooksPtr->setWeightCKKWL(0.);
      mergingHooksPtr->setWeightFIRST(0.);
      return -1;
    }
  }

  // New UNLOPS strategy based on UN2LOPS.
  bool doUNLOPS2 = false;
  int depth = (!doUNLOPS2) ? -1 : ( (containsRealKin) ? nSteps-1 : nSteps);

  // Calculate weights.
  // Do LO or first part of NLO tree-level reweighting
  if( doUNLOPSTree ) {
    // Perform reweighting with Sudakov factors, save as ratios and
    // PDF ratio weights
    wgt = FullHistory.weight_UNLOPS_TREE( trialPartonLevelPtr,
            mergingHooksPtr->AlphaS_FSR(), mergingHooksPtr->AlphaS_ISR(),
            mergingHooksPtr->AlphaEM_FSR(), mergingHooksPtr->AlphaEM_ISR(),
            RN, depth);
  } else if( doUNLOPSLoop ) {
    // Set event scales properly, reweight for new UNLOPS
    wgt = FullHistory.weight_UNLOPS_LOOP( trialPartonLevelPtr,
            mergingHooksPtr->AlphaS_FSR(), mergingHooksPtr->AlphaS_ISR(),
            mergingHooksPtr->AlphaEM_FSR(), mergingHooksPtr->AlphaEM_ISR(),
            RN, depth);
  } else if( doUNLOPSSubtNLO ) {
    // Set event scales properly, reweight for new UNLOPS
    wgt = FullHistory.weight_UNLOPS_SUBTNLO( trialPartonLevelPtr,
            mergingHooksPtr->AlphaS_FSR(), mergingHooksPtr->AlphaS_ISR(),
            mergingHooksPtr->AlphaEM_FSR(), mergingHooksPtr->AlphaEM_ISR(),
            RN, depth);
  } else if( doUNLOPSSubt ) {
    // Perform reweighting with Sudakov factors, save as ratios and
    // PDF ratio weights
    wgt = FullHistory.weight_UNLOPS_SUBT( trialPartonLevelPtr,
            mergingHooksPtr->AlphaS_FSR(), mergingHooksPtr->AlphaS_ISR(),
            mergingHooksPtr->AlphaEM_FSR(), mergingHooksPtr->AlphaEM_ISR(),
            RN, depth);
  }

  // Event with production scales set for further (trial) showering
  // and starting conditions for the shower.
  if (!doUNLOPSSubt && !doUNLOPSSubtNLO && !containsRealKin )
    FullHistory.getStartingConditions(RN, process);
  // Do reclustering (looping) steps.
  else FullHistory.getFirstClusteredEventAboveTMS( RN, nRecluster, process,
    nPerformed, true );

  // Allow to dampen histories in which the lowest multiplicity reclustered
  // state does not pass the lowest multiplicity cut of the matrix element
  double dampWeight = mergingHooksPtr->dampenIfFailCuts(
           FullHistory.lowestMultProc(RN) );
  // Save the weight of the event for histogramming. Only change the
  // event weight after trial shower on the matrix element
  // multiplicity event (= in doVetoStep)
  wgt *= dampWeight;

  // For tree-level or subtractive sammples, rescale with k-Factor
  if ( doUNLOPSTree || doUNLOPSSubt ){
    // Find k-factor
    double kFactor = 1.;
    if ( nSteps > mergingHooksPtr->nMaxJetsNLO() )
      kFactor = mergingHooksPtr->kFactor( mergingHooksPtr->nMaxJetsNLO() );
    else kFactor = mergingHooksPtr->kFactor(nSteps);
    // For NLO merging, rescale CKKW-L weight with k-factor
    wgt *= (nRecluster == 2 && nloTilde) ? 1. : kFactor;
  }

  // Save the weight of the event for histogramming
  mergingHooksPtr->setWeightCKKWL(wgt);

  // Check if we need to subtract the O(\alpha_s)-term. If the number
  // of additional partons is larger than the number of jets for
  // which loop matrix elements are available, do standard UMEPS.
  int nMaxNLO     = mergingHooksPtr->nMaxJetsNLO();
  bool doOASTree  = doUNLOPSTree && nSteps <= nMaxNLO;
  bool doOASSubt  = doUNLOPSSubt && nSteps <= nMaxNLO+1 && nSteps > 0;

  // Now begin NLO part for tree-level events
  if ( doOASTree || doOASSubt ) {

    // Decide on which order to expand to.
    int order = ( nSteps > 0 && nSteps <= nMaxNLO) ? 1 : -1;

    // Exclusive inputs:
    // Subtract only the O(\alpha_s^{n+0})-term from the tree-level
    // subtraction, if we're at the highest NLO multiplicity (nMaxNLO).
    if ( nloTilde && doUNLOPSSubt && nRecluster == 1
      && nSteps == nMaxNLO+1 ) order = 0;

    // Exclusive inputs:
    // Do not remove the O(as)-term if the number of reclusterings
    // exceeds the number of NLO jets, or if more clusterings have
    // been performed.
    if (nloTilde && doUNLOPSSubt && ( nSteps > nMaxNLO+1
      || (nSteps == nMaxNLO+1 && nPerformed != nRecluster) ))
        order = -1;

    // Calculate terms in expansion of the CKKW-L weight.
    wgtFIRST = FullHistory.weight_UNLOPS_CORRECTION( order,
      trialPartonLevelPtr, mergingHooksPtr->AlphaS_FSR(),
      mergingHooksPtr->AlphaS_ISR(), mergingHooksPtr->AlphaEM_FSR(),
      mergingHooksPtr->AlphaEM_ISR(), RN, rndmPtr );

    // Exclusive inputs:
    // Subtract the O(\alpha_s^{n+1})-term from the tree-level
    // subtraction, not the O(\alpha_s^{n+0})-terms.
    if ( nloTilde && doUNLOPSSubt && nRecluster == 1
      && nPerformed == nRecluster && nSteps <= nMaxNLO )
      wgtFIRST += 1.;

    // If necessary, also dampen the O(\alpha_s)-term
    wgtFIRST *= dampWeight;

    // Set the subtractive weight to the value calculated so far
    mergingHooksPtr->setWeightFIRST(wgtFIRST);
    // Subtract the O(\alpha_s)-term from the CKKW-L weight
    // If PDF contributions have not been included, subtract these later
    // New UNLOPS based on UN2LOPS.
    if (doUNLOPS2 && order > -1) wgt = -wgt*(wgtFIRST-1.);
    else if (order > -1) wgt = wgt - wgtFIRST;

  }

  // Set QCD 2->2 starting scale different from arbitrary scale in LHEF!
  // --> Set to minimal mT of partons.
  int nFinal = 0;
  double muf = process[0].e();
  for ( int i=0; i < process.size(); ++i )
  if ( process[i].isFinal()
    && (process[i].colType() != 0 || process[i].id() == 22 ) ) {
    nFinal++;
    muf = min( muf, abs(process[i].mT()) );
  }
  // For pure QCD dijet events (only!), set the process scale to the
  // transverse momentum of the outgoing partons.
  if ( nSteps == 0 && nFinal == 2
    && ( mergingHooksPtr->getProcessString().compare("pp>jj") == 0
      || mergingHooksPtr->getProcessString().compare("pp>aj") == 0) )
    process.scale(muf);

  // Reset hard process candidates (changed after clustering a parton).
  mergingHooksPtr->storeHardProcessCandidates( process );

  // Check if resonance structure has been changed
  //  (e.g. because of clustering W/Z/gluino)
  vector <int> oldResonance;
  for ( int i=0; i < newProcess.size(); ++i )
//    if ( newProcess[i].status() == 22 )
    if ( newProcess[i].isFinal()
      && particleDataPtr->isResonance(newProcess[i].id()))
      oldResonance.push_back(newProcess[i].id());
  vector <int> newResonance;
  for ( int i=0; i < process.size(); ++i )
//    if ( process[i].status() == 22 )
    if ( process[i].isFinal()
      && particleDataPtr->isResonance(process[i].id()))
      newResonance.push_back(process[i].id());
  // Compare old and new resonances
  for ( int i=0; i < int(oldResonance.size()); ++i )
    for ( int j=0; j < int(newResonance.size()); ++j )
      if ( newResonance[j] == oldResonance[i] ) {
        oldResonance[i] = 99;
        break;
      }
  bool hasNewResonances = (newResonance.size() != oldResonance.size());
  for ( int i=0; i < int(oldResonance.size()); ++i )
    hasNewResonances = (oldResonance[i] != 99);

  // If necessary, reattach resonance decay products.
  if (!hasNewResonances) mergingHooksPtr->reattachResonanceDecays(process);

  // Allow merging hooks to remove emissions from now on.
  mergingHooksPtr->doIgnoreEmissions(false);

  // If no-emission probability is zero.
  if ( wgt == 0. ) return 0;

  // If the resonance structure of the process has changed due to reclustering,
  // redo the resonance decays in Pythia::next()
  if (hasNewResonances) return 2;

  // Done
  return 1;

}

//--------------------------------------------------------------------------

// Function to set up all histories for an event.

bool MyMerging::generateHistories( const Event& process) {

  // Input not valid.
  if (!validEvent(process)) {
    cout << "Warning in MyMerging::generateHistories: Input event "
         << "has invalid flavour or momentum structure, thus reject. " << endl;
    return false;
  }

  // Clear previous history.
  if (myHistory) delete myHistory;

  // For now, prefer construction of ordered histories.
  if ( settingsPtr->flag("Dire:doMOPS") )
    mergingHooksPtr->orderHistories(false);
  else
    mergingHooksPtr->orderHistories(true);

  // For pp > h, allow cut on state, so that underlying processes
  // can be clustered to gg > h
  if ( mergingHooksPtr->getProcessString().compare("pp>h") == 0)
    mergingHooksPtr->allowCutOnRecState(true);

  // Prepare process record for merging. If Pythia has already decayed
  // resonances used to define the hard process, remove resonance decay
  // products.
  Event newProcess( mergingHooksPtr->bareEvent( process, true) );
  // Store candidates for the splitting V -> qqbar'
  mergingHooksPtr->storeHardProcessCandidates( newProcess );

  // Calculate number of clustering steps
  int nSteps = mergingHooksPtr->getNumberOfClusteringSteps( newProcess, true);

  // Set dummy process scale.
  newProcess.scale(0.0);
  // Generate all histories
  myHistory = new MyHistory( nSteps, 0.0, newProcess, MyClustering(), mergingHooksPtr,
            (*beamAPtr), (*beamBPtr), particleDataPtr, infoPtr,
            trialPartonLevelPtr, fsr, isr, psweights, coupSMPtr, true, true, true, true, 1.0, 1.0, 1.0, 0);
  // Project histories onto desired branches, e.g. only ordered paths.
  bool foundHistories = myHistory->projectOntoDesiredHistories();

  // Done
  return (settingsPtr->flag("Dire:doMOPS") ? foundHistories : true);

}

//--------------------------------------------------------------------------

double MyMerging::getPathIndex( bool useAll) {

  if (!useAll) return rndmPtr->flat();

  // Setup to choose shower starting conditions randomly.
  double sumAll(0.);
  for ( map<double, MyHistory*>::iterator it = myHistory->goodBranches.begin();
    it != myHistory->goodBranches.end(); ++it ) {
    sumAll     += it->second->prodOfProbs;
  }
  // Store a double with which to access each of the paths.
  double lastp(0.);
  vector<double> path_index;
  for ( map<double, MyHistory*>::iterator it = myHistory->goodBranches.begin();
      it != myHistory->goodBranches.end(); ++it ) {
      // Double to access path.
      double indexNow =  (lastp + 0.5*(it->first - lastp))/sumAll;
      path_index.push_back(indexNow);
      lastp = it->first;
  }
  // Randomly pick path.
  int sizeBranches = myHistory->goodBranches.size();
  int iPosRN = (sizeBranches > 0)
             ? rndmPtr->pick(
                 vector<double>(sizeBranches, 1./double(sizeBranches)) )
             : 0;
  double RN  = (sizeBranches > 0) ? path_index[iPosRN] : rndmPtr->flat();

  return RN;
}

//--------------------------------------------------------------------------

// Function to set up all histories for an event.

bool MyMerging::calculateSubtractions() {

  // Store shower subtractions.
  clearSubtractions();
  for ( int i = 0 ; i < int(myHistory->children.size()); ++i) {

    // Need to reattach resonance decays, if necessary.
    Event psppoint = myHistory->children[i]->state;
    // Reset hard process candidates (changed after clustering a parton).
    mergingHooksPtr->storeHardProcessCandidates( psppoint );

    // Check if resonance structure has been changed
    //  (e.g. because of clustering W/Z/gluino)
    vector <int> oldResonance;
    for ( int n=0; n < myHistory->state.size(); ++n )
//      if ( myHistory->state[n].status() == 22 )
      if ( myHistory->state[n].isFinal()
        && particleDataPtr->isResonance(myHistory->state[n].id()))
        oldResonance.push_back(myHistory->state[n].id());
    vector <int> newResonance;
    for ( int n=0; n < psppoint.size(); ++n )
//      if ( psppoint[n].status() == 22 )
      if ( psppoint[n].isFinal()
        && particleDataPtr->isResonance(psppoint[n].id()))
        newResonance.push_back(psppoint[n].id());
    // Compare old and new resonances
    for ( int n=0; n < int(oldResonance.size()); ++n )
      for ( int m=0; m < int(newResonance.size()); ++m )
        if ( newResonance[m] == oldResonance[n] ) {
          oldResonance[n] = 99;
          break;
        }
    bool hasNewResonances = (newResonance.size() != oldResonance.size());
    for ( int n=0; n < int(oldResonance.size()); ++n )
      hasNewResonances = (oldResonance[n] != 99);

    // If necessary, reattach resonance decay products.
    if (!hasNewResonances) mergingHooksPtr->reattachResonanceDecays(psppoint);
    else {
      cout << "Warning in MyMerging::calculateSubtractions: Resonance "
           << "structure changed due to clustering. Cannot attach decay "
           << "products correctly." << endl;
    }

    double prob = myHistory->children[i]->clusterProb;

    // Switch from 4pi to 8pi convention
    prob *= 2.;

    // Get clustering variables.
    map<string,double> stateVars;
    int rad = myHistory->children[i]->clusterIn.radPos();
    int emt = myHistory->children[i]->clusterIn.emtPos();
    int rec = myHistory->children[i]->clusterIn.recPos();

    bool isFSR = myHistory->showers->timesPtr->isTimelike(myHistory->state, rad, emt, rec, "");
    if (isFSR)
      stateVars = myHistory->showers->timesPtr->getStateVariables(myHistory->state,rad,emt,rec,"");
    else
      stateVars = myHistory->showers->spacePtr->getStateVariables(myHistory->state,rad,emt,rec,"");

    double z = stateVars["z"];
    double t = stateVars["t"];

    double m2dip = abs(-2.*myHistory->state[emt].p()*myHistory->state[rad].p()
                      -2.*myHistory->state[emt].p()*myHistory->state[rec].p()
                       +2.*myHistory->state[rad].p()*myHistory->state[rec].p());
    double kappa2 = t/m2dip;
    double xCS        = (z*(1-z) - kappa2) / (1 -z);

    // For II dipoles, scale with 1/xCS.
    prob *= 1./xCS;

    // Multiply with ME correction.
    prob *= myHistory->MECnum/myHistory->MECden;

    // Attach point to list of shower subtractions.
    appendSubtraction( prob, psppoint);

  }

  // Restore stored hard process candidates
  mergingHooksPtr->storeHardProcessCandidates(  myHistory->state );

  // Done
  return true;

}

//--------------------------------------------------------------------------

// Function to calulate the weights used for UNLOPS merging.

int MyMerging::calculateWeights( double RNpath, bool useAll ) {

  // Initialise which part of UNLOPS merging is applied.
  bool nloTilde         = settingsPtr->flag("Merging:doUNLOPSTilde");
  bool doUNLOPSTree     = settingsPtr->flag("Merging:doUNLOPSTree");
  bool doUNLOPSLoop     = settingsPtr->flag("Merging:doUNLOPSLoop");
  bool doUNLOPSSubt     = settingsPtr->flag("Merging:doUNLOPSSubt");
  bool doUNLOPSSubtNLO  = settingsPtr->flag("Merging:doUNLOPSSubtNLO");
  // Save number of looping steps
  mergingHooksPtr->nReclusterSave = settingsPtr->mode("Merging:nRecluster");
  int nRecluster        = settingsPtr->mode("Merging:nRecluster");

  // Ensure that merging hooks to not remove emissions
  mergingHooksPtr->doIgnoreEmissions(true);
  mergingHooksPtr->setWeightCKKWL(1.);
  mergingHooksPtr->setWeightFIRST(0.);

  // Reset weight of the event.
  double wgt      = 1.;
  // Reset the O(alphaS)-term of the UMEPS weight.
  double wgtFIRST = 0.;
  mergingHooksPtr->muMI(-1.);

  // Check if event passes the merging scale cut.
  double tmsval  = mergingHooksPtr->tms();

  bool allowReject = settingsPtr->flag("Merging:applyVeto");

  if ( settingsPtr->flag("Dire:doMOPS")) tmsval = 0.;

  // Get merging scale in current event.
  double tmsnow  = mergingHooksPtr->tmsNow( myHistory->state );
  // Calculate number of clustering steps
  int nSteps = mergingHooksPtr->getNumberOfClusteringSteps( myHistory->state, true);
  int nRequested = mergingHooksPtr->nRequested();

  if ( settingsPtr->flag("Dire:doMOPS") && nSteps == 0) { return 1; }

  // Too few steps can be possible if a chain of resonance decays has been
  // removed. In this case, reject this event, since it will be handled in
  // lower-multiplicity samples.
  if (nSteps < nRequested) {
    string message="Warning in MyMerging::calculateWeights: Les Houches Event";
    message+=" after removing decay products does not contain enough partons.";
    infoPtr->errorMsg(message);
    if (allowReject) return -1;
    //return -1;
  }

  // Reset the minimal tms value, if necessary.
  tmsNowMin = (nSteps == 0) ? 0. : min(tmsNowMin, tmsnow);

  // Do not apply cut if the configuration could not be projected onto an
  // underlying born configuration.
  bool applyCut = nSteps > 0 && myHistory->select(RNpath)->nClusterings() > 0;

  // Enfore merging scale cut if the event did not pass the merging scale
  // criterion.
  bool enforceCutOnLHE  = settingsPtr->flag("Merging:enforceCutOnLHE");
  if ( enforceCutOnLHE && applyCut && nSteps == nRequested
    && tmsnow < tmsval && tmsval > 0.) {
    string message="Warning in MyMerging::calculateWeights: Les Houches";
    message+=" Event fails merging scale cut. Reject event.";
    infoPtr->errorMsg(message);
    if (allowReject) return -1;
    //return -1;
  }

//cout << nRequested << " " << nSteps << endl;

  // Potentially recluster real emission jets for powheg input containing
  // "too many" jets, i.e. real-emission kinematics.
  bool containsRealKin = nSteps > nRequested && nSteps > 0;
  if ( containsRealKin ) nRecluster += nSteps - nRequested;

  // Remove real emission events without underlying Born configuration from
  // the loop sample, since such states will be taken care of by tree-level
  // samples.
  bool allowIncompleteReal =
    settingsPtr->flag("Merging:allowIncompleteHistoriesInReal");
  if ( doUNLOPSLoop && containsRealKin && !allowIncompleteReal
    && myHistory->select(RNpath)->nClusterings() == 0 ) {
    if (allowReject) return -1;
    //return -1;
  }

  // Discard if the state could not be reclustered to any state above TMS.
  int nPerformed = 0;
  if ( nSteps > 0 && !allowIncompleteReal
    && ( doUNLOPSSubt || doUNLOPSSubtNLO || containsRealKin )
    && !myHistory->getFirstClusteredEventAboveTMS( RNpath, nRecluster,
          myHistory->state, nPerformed, false ) ) {
    if (allowReject) return -1;
    //return -1;
  }

  // Check reclustering steps to correctly apply MPI.
  mergingHooksPtr->nMinMPI(nSteps - nPerformed);

  // Perform one reclustering for real emission kinematics, then apply
  // merging scale cut on underlying Born kinematics.
  if ( containsRealKin ) {
    Event dummy = Event();
    // Initialise temporary output of reclustering.
    dummy.clear();
    dummy.init( "(hard process-modified)", particleDataPtr );
    dummy.clear();
    // Recluster once.
    myHistory->getClusteredEvent( RNpath, nSteps, dummy );
    double tnowNew  = mergingHooksPtr->tmsNow( dummy );
    // Veto if underlying Born kinematics do not pass merging scale cut.
    if ( enforceCutOnLHE && nSteps > 0 && nRequested > 0
      && tnowNew < tmsval && tmsval > 0.) {
      string message="Warning in MyMerging::calculateWeights: Les Houches";
      message+=" Event fails merging scale cut. Reject event.";
      infoPtr->errorMsg(message);
      if (allowReject) return -1;
      //return -1;
    }
  }

  // Setup to choose shower starting conditions randomly.
  double sumAll(0.), sumFullAll(0.);
  for ( map<double, MyHistory*>::iterator it = myHistory->goodBranches.begin();
    it != myHistory->goodBranches.end(); ++it ) {
    sumAll     += it->second->prodOfProbs;
    sumFullAll += it->second->prodOfProbsFull;
  }

  // New UNLOPS strategy based on UN2LOPS.
  bool doUNLOPS2 = false;
  int depth = (!doUNLOPS2) ? -1 : ( (containsRealKin) ? nSteps-1 : nSteps);

//  if (settingsPtr->flag("Dire:doMcAtNloDelta"))
//    depth = (containsRealKin) ? 1 : 0;

  if (settingsPtr->flag("Dire:doMcAtNloDelta"))
    depth = (nSteps>0) ? 1 : 0;


  if (!useAll) {

  // Calculate weights.
  if ( settingsPtr->flag("Dire:doMOPS") )
    wgt = myHistory->weightMOPS( trialPartonLevelPtr,
            mergingHooksPtr->AlphaS_FSR(), mergingHooksPtr->AlphaS_ISR(),
            mergingHooksPtr->AlphaEM_FSR(), mergingHooksPtr->AlphaEM_ISR(),
            RNpath);
  else if ( settingsPtr->flag("Dire:doMcAtNloDelta") )
    wgt = myHistory->weightMcAtNloDelta( trialPartonLevelPtr,
            mergingHooksPtr->AlphaS_FSR(), mergingHooksPtr->AlphaS_ISR(),
            mergingHooksPtr->AlphaEM_FSR(), mergingHooksPtr->AlphaEM_ISR(),
            RNpath, depth);
  else if ( mergingHooksPtr->doCKKWLMerging() )
    wgt = myHistory->weightTREE( trialPartonLevelPtr,
            mergingHooksPtr->AlphaS_FSR(), mergingHooksPtr->AlphaS_ISR(),
            mergingHooksPtr->AlphaEM_FSR(), mergingHooksPtr->AlphaEM_ISR(),
            RNpath);
  else if (  mergingHooksPtr->doUMEPSTreeSave )
    wgt = myHistory->weight_UMEPS_TREE( trialPartonLevelPtr,
            mergingHooksPtr->AlphaS_FSR(), mergingHooksPtr->AlphaS_ISR(),
            mergingHooksPtr->AlphaEM_FSR(), mergingHooksPtr->AlphaEM_ISR(),
            RNpath);
  else if ( mergingHooksPtr->doUMEPSSubtSave )
    wgt = myHistory->weight_UMEPS_SUBT( trialPartonLevelPtr,
            mergingHooksPtr->AlphaS_FSR(), mergingHooksPtr->AlphaS_ISR(),
            mergingHooksPtr->AlphaEM_FSR(), mergingHooksPtr->AlphaEM_ISR(),
            RNpath);
  else if ( mergingHooksPtr->doUNLOPSTreeSave )
    wgt = myHistory->weight_UNLOPS_TREE( trialPartonLevelPtr,
            mergingHooksPtr->AlphaS_FSR(), mergingHooksPtr->AlphaS_ISR(),
            mergingHooksPtr->AlphaEM_FSR(), mergingHooksPtr->AlphaEM_ISR(),
            RNpath, depth);
  else if ( mergingHooksPtr->doUNLOPSLoopSave )
    wgt = myHistory->weight_UNLOPS_LOOP( trialPartonLevelPtr,
            mergingHooksPtr->AlphaS_FSR(), mergingHooksPtr->AlphaS_ISR(),
            mergingHooksPtr->AlphaEM_FSR(), mergingHooksPtr->AlphaEM_ISR(),
            RNpath, depth);
  else if ( mergingHooksPtr->doUNLOPSSubtNLOSave )
    wgt = myHistory->weight_UNLOPS_SUBTNLO( trialPartonLevelPtr,
            mergingHooksPtr->AlphaS_FSR(), mergingHooksPtr->AlphaS_ISR(),
            mergingHooksPtr->AlphaEM_FSR(), mergingHooksPtr->AlphaEM_ISR(),
            RNpath, depth);
  else if ( mergingHooksPtr->doUNLOPSSubtSave )
    wgt = myHistory->weight_UNLOPS_SUBT( trialPartonLevelPtr,
            mergingHooksPtr->AlphaS_FSR(), mergingHooksPtr->AlphaS_ISR(),
            mergingHooksPtr->AlphaEM_FSR(), mergingHooksPtr->AlphaEM_ISR(),
            RNpath, depth);

  // For tree-level or subtractive sammples, rescale with k-Factor
  if ( doUNLOPSTree || doUNLOPSSubt ){
    // Find k-factor
    double kFactor = 1.;
    if ( nSteps > mergingHooksPtr->nMaxJetsNLO() )
      kFactor = mergingHooksPtr->kFactor( mergingHooksPtr->nMaxJetsNLO() );
    else kFactor = mergingHooksPtr->kFactor(nSteps);
    // For NLO merging, rescale CKKW-L weight with k-factor
    wgt *= (nRecluster == 2 && nloTilde) ? 1. : kFactor;
  }

  } else if (useAll && settingsPtr->flag("Dire:doMOPS") ) {
    // Calculate CKKWL reweighting for all paths.
    double wgtsum(0.);
    double lastp(0.);

    for ( map<double, MyHistory*>::iterator it = myHistory->goodBranches.begin();
      it != myHistory->goodBranches.end(); ++it ) {

      // Double to access path.
      double indexNow =  (lastp + 0.5*(it->first - lastp))/sumAll;
      lastp = it->first;

      // Probability of path.
      double probPath = it->second->prodOfProbsFull/sumFullAll;

      myHistory->select(indexNow)->setSelectedChild();

      // Calculate CKKWL weight:
      double w = myHistory->weightMOPS( trialPartonLevelPtr,
        mergingHooksPtr->AlphaS_FSR(), mergingHooksPtr->AlphaS_ISR(),
        mergingHooksPtr->AlphaEM_FSR(), mergingHooksPtr->AlphaEM_ISR(),
        indexNow);

      wgtsum += probPath*w;
    }
    wgt = wgtsum;

  }

  mergingHooksPtr->setWeightCKKWL(wgt);

//cout << wgt << endl;

  // Check if we need to subtract the O(\alpha_s)-term. If the number
  // of additional partons is larger than the number of jets for
  // which loop matrix elements are available, do standard UMEPS.
  int nMaxNLO     = mergingHooksPtr->nMaxJetsNLO();
  bool doOASTree  = doUNLOPSTree && nSteps <= nMaxNLO;
  bool doOASSubt  = doUNLOPSSubt && nSteps <= nMaxNLO+1 && nSteps > 0;

  // Now begin NLO part for tree-level events
  if ( doOASTree || doOASSubt ) {

    // Decide on which order to expand to.
    int order = ( nSteps > 0 && nSteps <= nMaxNLO) ? 1 : -1;

    // Exclusive inputs:
    // Subtract only the O(\alpha_s^{n+0})-term from the tree-level
    // subtraction, if we're at the highest NLO multiplicity (nMaxNLO).
    if ( nloTilde && doUNLOPSSubt && nRecluster == 1
      && nSteps == nMaxNLO+1 ) order = 0;

    // Exclusive inputs:
    // Do not remove the O(as)-term if the number of reclusterings
    // exceeds the number of NLO jets, or if more clusterings have
    // been performed.
    if (nloTilde && doUNLOPSSubt && ( nSteps > nMaxNLO+1
      || (nSteps == nMaxNLO+1 && nPerformed != nRecluster) ))
        order = -1;

    // Calculate terms in expansion of the CKKW-L weight.
    wgtFIRST = myHistory->weight_UNLOPS_CORRECTION( order,
      trialPartonLevelPtr, mergingHooksPtr->AlphaS_FSR(),
      mergingHooksPtr->AlphaS_ISR(), mergingHooksPtr->AlphaEM_FSR(),
      mergingHooksPtr->AlphaEM_ISR(), RNpath, rndmPtr );

    // Exclusive inputs:
    // Subtract the O(\alpha_s^{n+1})-term from the tree-level
    // subtraction, not the O(\alpha_s^{n+0})-terms.
    if ( nloTilde && doUNLOPSSubt && nRecluster == 1
      && nPerformed == nRecluster && nSteps <= nMaxNLO )
      wgtFIRST += 1.;

    // Subtract the O(\alpha_s)-term from the CKKW-L weight
    // If PDF contributions have not been included, subtract these later
    // New UNLOPS based on UN2LOPS.
    if (doUNLOPS2 && order > -1) wgt = -wgt*(wgtFIRST-1.);
    else if (order > -1) wgt = wgt - wgtFIRST;

  }

  // If no-emission probability is zero.
  if ( allowReject && wgt == 0. ) return 0;
  //if ( wgt == 0. ) return 0;

  // Done
  return 1;

}

//--------------------------------------------------------------------------

// Function to perform UNLOPS merging on this event.

int MyMerging::getStartingConditions( double RNpath, Event& process) {

  // Initialise which part of UNLOPS merging is applied.
  bool doUNLOPSSubt     = settingsPtr->flag("Merging:doUNLOPSSubt");
  bool doUNLOPSSubtNLO  = settingsPtr->flag("Merging:doUNLOPSSubtNLO");
  // Save number of looping steps
  mergingHooksPtr->nReclusterSave = settingsPtr->mode("Merging:nRecluster");
  int nRecluster        = settingsPtr->mode("Merging:nRecluster");

  // Calculate number of clustering steps
  int nSteps = mergingHooksPtr->getNumberOfClusteringSteps( myHistory->state, true);
  int nRequested = mergingHooksPtr->nRequested();

  // Potentially recluster real emission jets for powheg input containing
  // "too many" jets, i.e. real-emission kinematics.
  bool containsRealKin = nSteps > nRequested && nSteps > 0;
  if ( containsRealKin ) nRecluster += nSteps - nRequested;

  // Event with production scales set for further (trial) showering
  // and starting conditions for the shower.
  int nPerformed = 0;
  if (!doUNLOPSSubt && !doUNLOPSSubtNLO && !containsRealKin )
    myHistory->getStartingConditions(RNpath, process);
  // Do reclustering (looping) steps.
  else myHistory->getFirstClusteredEventAboveTMS( RNpath, nRecluster, process,
    nPerformed, true );

  // Set QCD 2->2 starting scale different from arbitrary scale in LHEF!
  // --> Set to minimal mT of partons.
  int nFinal = 0;
  double muf = process[0].e();
  for ( int i=0; i < process.size(); ++i )
  if ( process[i].isFinal()
    && (process[i].colType() != 0 || process[i].id() == 22 ) ) {
    nFinal++;
    muf = min( muf, abs(process[i].mT()) );
  }
  // For pure QCD dijet events (only!), set the process scale to the
  // transverse momentum of the outgoing partons.
  if ( nSteps == 0 && nFinal == 2
    && ( mergingHooksPtr->getProcessString().compare("pp>jj") == 0
      || mergingHooksPtr->getProcessString().compare("pp>aj") == 0) )
    process.scale(muf);

  // Reset hard process candidates (changed after clustering a parton).
  mergingHooksPtr->storeHardProcessCandidates( process );

  // Check if resonance structure has been changed
  //  (e.g. because of clustering W/Z/gluino)
  vector <int> oldResonance;
  for ( int i=0; i < myHistory->state.size(); ++i )
//    if ( myHistory->state[i].status() == 22 )
    if ( myHistory->state[i].isFinal()
      && particleDataPtr->isResonance(myHistory->state[i].id()))
      oldResonance.push_back(myHistory->state[i].id());
  vector <int> newResonance;
  for ( int i=0; i < process.size(); ++i )
//    if ( process[i].status() == 22 )
    if ( process[i].isFinal()
      && particleDataPtr->isResonance(process[i].id()))
      newResonance.push_back(process[i].id());
  // Compare old and new resonances
  for ( int i=0; i < int(oldResonance.size()); ++i )
    for ( int j=0; j < int(newResonance.size()); ++j )
      if ( newResonance[j] == oldResonance[i] ) {
        oldResonance[i] = 99;
        break;
      }
  bool hasNewResonances = (newResonance.size() != oldResonance.size());
  for ( int i=0; i < int(oldResonance.size()); ++i )
    hasNewResonances = (oldResonance[i] != 99);

  // If necessary, reattach resonance decay products.
  if (!hasNewResonances) mergingHooksPtr->reattachResonanceDecays(process);

  // Allow merging hooks to remove emissions from now on.
  mergingHooksPtr->doIgnoreEmissions(false);

  // If the resonance structure of the process has changed due to reclustering,
  // redo the resonance decays in Pythia::next()
  if (hasNewResonances) return 2;

  // Done
  return 1;

}

//--------------------------------------------------------------------------

// Function to apply the merging scale cut on an input event.

bool MyMerging::cutOnProcess( Event& process) {

  // Save number of looping steps
  mergingHooksPtr->nReclusterSave = settingsPtr->mode("Merging:nRecluster");

  // For now, prefer construction of ordered histories.
  mergingHooksPtr->orderHistories(true);
  // For pp > h, allow cut on state, so that underlying processes
  // can be clustered to gg > h
  if ( mergingHooksPtr->getProcessString().compare("pp>h") == 0)
    mergingHooksPtr->allowCutOnRecState(true);

  // Reset any incoming spins for W+-.
  if (mergingHooksPtr->doWeakClustering())
    for (int i = 0;i < process.size();++i)
      process[i].pol(9);

  // Prepare process record for merging. If Pythia has already decayed
  // resonances used to define the hard process, remove resonance decay
  // products.
  Event newProcess( mergingHooksPtr->bareEvent( process, true) );
  // Store candidates for the splitting V -> qqbar'
  mergingHooksPtr->storeHardProcessCandidates( newProcess );

  // Check if event passes the merging scale cut.
  double tmsval  = mergingHooksPtr->tms();
  // Get merging scale in current event.
  double tmsnow  = mergingHooksPtr->tmsNow( newProcess );
  // Calculate number of clustering steps
  int nSteps = mergingHooksPtr->getNumberOfClusteringSteps( newProcess, true);

  // Too few steps can be possible if a chain of resonance decays has been
  // removed. In this case, reject this event, since it will be handled in
  // lower-multiplicity samples.
  int nRequested = mergingHooksPtr->nRequested();
  if (nSteps < nRequested) return true;

  // Reset the minimal tms value, if necessary.
  tmsNowMin = (nSteps == 0) ? 0. : min(tmsNowMin, tmsnow);

  // Potentially recluster real emission jets for powheg input containing
  // "too many" jets, i.e. real-emission kinematics.
  bool containsRealKin = nSteps > nRequested && nSteps > 0;

  // Get random number to choose a path.
  double RN = rndmPtr->flat();
  // Set dummy process scale.
  newProcess.scale(0.0);
  // Generate all histories
  MyHistory FullHistory( nSteps, 0.0, newProcess, MyClustering(), mergingHooksPtr,
            (*beamAPtr), (*beamBPtr), particleDataPtr, infoPtr,
            trialPartonLevelPtr, fsr, isr, psweights, coupSMPtr, true, true, true, true, 1.0, 1.0, 1.0, 0);
  // Project histories onto desired branches, e.g. only ordered paths.
  FullHistory.projectOntoDesiredHistories();

  // Remove real emission events without underlying Born configuration from
  // the loop sample, since such states will be taken care of by tree-level
  // samples.
  bool allowIncompleteReal =
    settingsPtr->flag("Merging:allowIncompleteHistoriesInReal");
  if ( containsRealKin && !allowIncompleteReal
    && FullHistory.select(RN)->nClusterings() == 0 )
    return true;

  // Cut if no history passes the cut on the lowest-multiplicity state.
  double dampWeight = mergingHooksPtr->dampenIfFailCuts(
           FullHistory.lowestMultProc(RN) );
  if ( dampWeight == 0. ) return true;

  // Do not apply cut if the configuration could not be projected onto an
  // underlying born configuration.
  if ( nSteps > 0 && FullHistory.select(RN)->nClusterings() == 0 )
    return false;

  // Now enfore merging scale cut if the event did not pass the merging scale
  // criterion.
  if ( nSteps > 0 && nSteps == nRequested && tmsnow < tmsval && tmsval > 0.) {
    string message="Warning in MyMerging::cutOnProcess: Les Houches Event";
    message+=" fails merging scale cut. Reject event.";
    infoPtr->errorMsg(message);
    return true;
  }

  // Check if more steps should be taken.
  int nFinalP = 0;
  int nFinalW = 0;
  Event coreProcess = Event();
  coreProcess.clear();
  coreProcess.init( "(hard process-modified)", particleDataPtr );
  coreProcess.clear();
  coreProcess = FullHistory.lowestMultProc(RN);
  for ( int i = 0; i < coreProcess.size(); ++i )
    if ( coreProcess[i].isFinal() ) {
      if ( coreProcess[i].colType() != 0 )
        nFinalP++;
      if ( coreProcess[i].idAbs() == 24 )
        nFinalW++;
    }

  bool complete = (FullHistory.select(RN)->nClusterings() == nSteps) ||
    ( mergingHooksPtr->doWeakClustering() && nFinalP == 2 && nFinalW == 0 );

  if ( !complete ) {
    string message="Warning in MyMerging::cutOnProcess: No clusterings";
    message+=" found. History incomplete.";
    infoPtr->errorMsg(message);
  }

  // Done if no real-emission jets are present.
  if ( !containsRealKin ) return false;

  // Now cut on events that contain an additional real-emission jet.
  // Perform one reclustering for real emission kinematics, then apply merging
  // scale cut on underlying Born kinematics.
  if ( containsRealKin ) {
    Event dummy = Event();
    // Initialise temporary output of reclustering.
    dummy.clear();
    dummy.init( "(hard process-modified)", particleDataPtr );
    dummy.clear();
    // Recluster once.
    FullHistory.getClusteredEvent( RN, nSteps, dummy );
    double tnowNew  = mergingHooksPtr->tmsNow( dummy );
    // Veto if underlying Born kinematics do not pass merging scale cut.
    if ( nSteps > 0 && nRequested > 0 && tnowNew < tmsval && tmsval > 0.) {
      string message="Warning in MyMerging::cutOnProcess: Les Houches Event";
      message+=" fails merging scale cut. Reject event.";
      infoPtr->errorMsg(message);
      return true;
    }
  }

  // Done if only interested in cross section estimate after cuts.
  return false;

}

//==========================================================================

} // end namespace Pythia8