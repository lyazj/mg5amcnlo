// Driver for Pythia 8. Reads an input file dynamically created on
// the basis of the inputs specified in MCatNLO_MadFKS_PY8.Script 
#include "Pythia8/Pythia.h"
#include "Pythia8/Pythia8ToHepMC.h"
#include "HepMC/GenEvent.h"
#include "HepMC/IO_GenEvent.h"
#include "HepMC/IO_BaseClass.h"
#include "HepMC/IO_HEPEVT.h"
#include "HepMC/HEPEVT_Wrapper.h"
#include "fstream"

using namespace Pythia8;

extern "C" {
    extern struct {
       double EVWGT;
    } cevwgt_;
}
#define cevwgt cevwgt_

extern "C" { 
  void pyabeg_();
  void pyaend_(int&);
  void pyanal_();
}

int main() {
  Pythia pythia;
  string inputname="Pythia8.cmd",outputname="Pythia8.hep";

  pythia.readFile(inputname.c_str());
  pythia.init();
  pyabeg_();

  int nAbort=10;
  int nPrintLHA=1;
  int iAbort=0;
  int iPrintLHA=0;
  int nstep=5000;
  int iEventtot=pythia.mode("Main:numberOfEvents");
  int iEventshower=pythia.mode("Main:spareMode1");
  string evt_norm=pythia.word("Main:spareWord1");
  int iEventtot_norm=iEventtot;
  if (evt_norm == "average"){
    iEventtot_norm=1;
  }

  HepMC::IO_BaseClass *_hepevtio;
  HepMC::Pythia8ToHepMC ToHepMC;
  HepMC::IO_GenEvent ascii_io(outputname.c_str(), std::ios::out);

  for (int iEvent = 0; ; ++iEvent) {
    if (!pythia.next()) {
      if (++iAbort < nAbort) continue;
      break;
    }
    if (iEvent >= iEventshower) break;
    if (pythia.info.isLHA() && iPrintLHA < nPrintLHA) {
      pythia.LHAeventList();
      pythia.info.list();
      pythia.process.list();
      pythia.event.list();
      ++iPrintLHA;
    }

    HepMC::GenEvent* hepmcevt = new HepMC::GenEvent();
    ToHepMC.fill_next_event( pythia, hepmcevt );

    //define the IO_HEPEVT
    _hepevtio = new HepMC::IO_HEPEVT;
    _hepevtio->write_event(hepmcevt);
    
    //event weight
    cevwgt.EVWGT=hepmcevt->weights()[0];

    //call the FORTRAN analysis for this event
    pyanal_();

    if (iEvent % nstep == 0 && iEvent >= 100){
      pyaend_(iEventtot_norm);
    }
    delete hepmcevt;
  }
  pyaend_(iEventtot_norm);

  pythia.stat();
  return 0;
}