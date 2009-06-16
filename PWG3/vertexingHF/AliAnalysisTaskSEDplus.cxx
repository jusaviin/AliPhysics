/**************************************************************************
 * Copyright(c) 1998-2008, ALICE Experiment at CERN, All rights reserved. *
 *                                                                        *
 * Author: The ALICE Off-line Project.                                    *
 * Contributors are mentioned in the code where appropriate.              *
 *                                                                        *
 * Permission to use, copy, modify and distribute this software and its   *
 * documentation strictly for non-commercial purposes is hereby granted   *
 * without fee, provided that the above copyright notice appears in all   *
 * copies and that both the copyright notice and this permission notice   *
 * appear in the supporting documentation. The authors make no claims     *
 * about the suitability of this software for any purpose. It is          *
 * provided "as is" without express or implied warranty.                  *
 **************************************************************************/

/////////////////////////////////////////////////////////////
//
// AliAnalysisTaskSE for the extraction of signal(e.g D+) of heavy flavor
// decay candidates with the MC truth.

/////////////////////////////////////////////////////////////

#include <TClonesArray.h>
#include <TNtuple.h>
#include <TList.h>
#include <TH1F.h>
#include "AliAODEvent.h"
#include "AliAODVertex.h"
#include "AliAODTrack.h"
#include "AliAODMCHeader.h"
#include "AliAODMCParticle.h"
#include "AliAODRecoDecayHF3Prong.h"
#include "AliAnalysisVertexingHF.h"
#include "AliAnalysisTaskSE.h"
#include "AliAnalysisTaskSEDplus.h"

ClassImp(AliAnalysisTaskSEDplus)


//________________________________________________________________________
AliAnalysisTaskSEDplus::AliAnalysisTaskSEDplus():
AliAnalysisTaskSE(),
fOutput(0), 
fNtupleDplus(0),
fNtupleDplusbackg(0),
fHistMass(0),
fHistSignal(0),
fHistBackground(0),
fVHF(0)
{
  // Default constructor

  // Output slot #1 writes into a TList container
  DefineOutput(1,TList::Class());  //My private output
}

//________________________________________________________________________
AliAnalysisTaskSEDplus::AliAnalysisTaskSEDplus(const char *name):
AliAnalysisTaskSE(name),
fOutput(0), 
fNtupleDplus(0),
fNtupleDplusbackg(0),
fHistMass(0),
fHistSignal(0),
fHistBackground(0),
fVHF(0)
{
  // Default constructor

  // Output slot #1 writes into a TList container
  DefineOutput(1,TList::Class());  //My private output
}

//________________________________________________________________________
AliAnalysisTaskSEDplus::~AliAnalysisTaskSEDplus()
{
  // Destructor
  if (fOutput) {
    delete fOutput;
    fOutput = 0;
  }
  if (fVHF) {
    delete fVHF;
    fVHF = 0;
  }
}  

//________________________________________________________________________
void AliAnalysisTaskSEDplus::Init()
{
  // Initialization

  if(fDebug > 1) printf("AnalysisTaskSEDplus::Init() \n");

  gROOT->LoadMacro("ConfigVertexingHF.C");

  fVHF = (AliAnalysisVertexingHF*)gROOT->ProcessLine("ConfigVertexingHF()");  
  fVHF->PrintStatus();

  return;
}

//________________________________________________________________________
void AliAnalysisTaskSEDplus::UserCreateOutputObjects()
{
  // Create the output container
  //
  if(fDebug > 1) printf("AnalysisTaskSEDplus::UserCreateOutputObjects() \n");

  // Several histograms are more conveniently managed in a TList
  fOutput = new TList();
  fOutput->SetOwner();

  fHistMass = new TH1F("fHistMass", "D^{+} invariant mass; M [GeV]; Entries",100,1.765,1.965);
  fHistSignal = new TH1F("fHistSignal", "D^{+} invariant mass - MC; M [GeV]; Entries",100,1.765,1.965);
  fHistBackground =new TH1F("fHistBackground", "Background invariant mass - MC; M [GeV]; Entries",100,1.765,1.965);




  fHistMass->Sumw2();
  fHistSignal->Sumw2();
  fHistBackground->Sumw2();


  //fHistMass->SetMinimum(0);
  //fHistSignal->SetMinimum(0);
  //fHistBackground->SetMinimum(0);
  fOutput->Add(fHistMass);
  fOutput->Add(fHistSignal);
  fOutput->Add(fHistBackground);
  

  fNtupleDplus = new TNtuple("fNtupleDplus","D +","pdg:Px:Py:Pz:Ptpi:Ptpi2:PtK:PtRec:PtTrue:PointingAngle:DecLeng:VxTrue:VxRec:InvMass:sigvert");
  fNtupleDplusbackg = new TNtuple("fNtupleDplusbackg","D + backg","Ptpibkg:Ptpi2bkg:PtKbkg:PtRecbkg:PointingAnglebkg:DLbkg:VxRecbkg:InvMassbkg:sigvertbkg");
  
  fOutput->Add(fNtupleDplus);
  fOutput->Add(fNtupleDplusbackg);

  return;
}

//________________________________________________________________________
void AliAnalysisTaskSEDplus::UserExec(Option_t */*option*/)
{
  // Execute analysis for current event:
  // heavy flavor candidates association to MC truth

  
  
  AliAODEvent *aod = dynamic_cast<AliAODEvent*> (InputEvent());

  // load Dplus->Kpipi candidates                                                   
  TClonesArray *array3Prong =
    (TClonesArray*)aod->GetList()->FindObject("Charm3Prong");
  if(!array3Prong) {
    printf("AliAnalysisTaskSEDplus::UserExec: Charm3Prong branch not found!\n");
    return;
  }

  // AOD primary vertex
    AliAODVertex *vtx1 = (AliAODVertex*)aod->GetPrimaryVertex();
    //    vtx1->Print();

  // load MC particles
  TClonesArray *arrayMC = 
    (TClonesArray*)aod->GetList()->FindObject(AliAODMCParticle::StdBranchName());
  if(!arrayMC) {
    printf("AliAnalysisTaskSEDplus::UserExec: MC particles branch not found!\n");
    return;
  }

  // load MC header
  AliAODMCHeader *mcHeader = 
    (AliAODMCHeader*)aod->GetList()->FindObject(AliAODMCHeader::StdBranchName());
  if(!mcHeader) {
    printf("AliAnalysisTaskSEDplus::UserExec: MC header branch not found!\n");
    return;
  }
    Int_t n3Prong = array3Prong->GetEntriesFast();
    printf("Number of D+->Kpipi: %d\n",n3Prong);


    for (Int_t i3Prong = 0; i3Prong < n3Prong; i3Prong++) {
      AliAODRecoDecayHF3Prong *d = (AliAODRecoDecayHF3Prong*)array3Prong->UncheckedAt(i3Prong);


      Bool_t unsetvtx=kFALSE;
      if(!d->GetOwnPrimaryVtx()){
	d->SetOwnPrimaryVtx(vtx1);
	unsetvtx=kTRUE;
      }
      if(d->SelectDplus(fVHF->GetDplusCuts()))
	{
	  
	  
	  Int_t labDp = d->MatchToMC(411,arrayMC);
	  //  if(labDp>=0) {
	    AliAODMCParticle *partDp = (AliAODMCParticle*)arrayMC->At(labDp);
	    if(labDp>=0) {
	      Int_t pdgDp = TMath::Abs(partDp->GetPdgCode());
	      if(pdgDp==411){

	  fHistSignal->Fill(d->InvMassDplus());
	  fHistMass->Fill(d->InvMassDplus());
	    
	  // Post the data already here
	  PostData(1,fOutput);

	  fNtupleDplus->Fill(pdgDp,partDp->Px()-d->Px(),partDp->Py()-d->Py(),partDp->Pz()-d->Pz(),d->PtProng(0),d->PtProng(2),d->PtProng(1),d->Pt(),partDp->Pt(),d->CosPointingAngle(),d->DecayLength(),partDp->Xv(),d->Xv(),d->InvMassDplus(),d->GetSigmaVert());
	      }
	    }	  
else {     
	    fHistBackground->Fill(d->InvMassDplus());
	    fNtupleDplusbackg->Fill(d->PtProng(0),d->PtProng(2),d->PtProng(1),d->Pt(),d->CosPointingAngle(),d->DecayLength(),d->Xv(),d->InvMassDplus(),d->GetSigmaVert());	    

	  fHistMass->Fill(d->InvMassDplus());
 }
	  
	
	}


	
      if(unsetvtx) d->UnsetOwnPrimaryVtx();

    }
     // end loop on D+->Kpipi


  return;
}

//________________________________________________________________________
void AliAnalysisTaskSEDplus::Terminate(Option_t */*option*/)
{
  // Terminate analysis
  //
  if(fDebug > 1) printf("AnalysisTaskSEDplus: Terminate() \n");

  fOutput = dynamic_cast<TList*> (GetOutputData(1));
  if (!fOutput) {     
    printf("ERROR: fOutput not available\n");
    return;
  }

  fNtupleDplus = dynamic_cast<TNtuple*>(fOutput->FindObject("fNtupleDplus"));
  fHistMass = dynamic_cast<TH1F*>(fOutput->FindObject("fHistMass"));
  fHistSignal = dynamic_cast<TH1F*>(fOutput->FindObject("fHistSignal"));
  fHistBackground = dynamic_cast<TH1F*>(fOutput->FindObject("fHistBackground"));
  fNtupleDplusbackg = dynamic_cast<TNtuple*>(fOutput->FindObject("fNtupleDplusbackg"));
    return;
}

