#include <memory>

#include <TRandom.h>
#include <TTree.h>
#include <TList.h>
#include <TH1.h>
#include <TH3.h>
#include <TFile.h>
#include <TString.h>
#include <TLorentzVector.h>

#include "AliLog.h"
#include "AliVEvent.h"
#include "AliESDEvent.h"
#include "AliMCEvent.h"
#include "AliMCParticle.h"
#include "AliAnalysisManager.h"
#include "AliESDInputHandler.h"
#include "AliTriggerIR.h"
#include "AliESDVertex.h"
#include "AliTriggerIR.h"

#include "AliAnalysisTaskDG.h"
#include "AliRawEventHeaderBase.h"
#include "AliESDVZERO.h"
#include "AliESDAD.h"
#include "AliESDtrackCuts.h"

#include "AliITSsegmentationSPD.h"

ClassImp(AliAnalysisTaskDG);
ClassImp(AliAnalysisTaskDG::TreeData);
ClassImp(AliAnalysisTaskDG::TrackData);

void AliAnalysisTaskDG::EventInfo::Fill(const AliESDEvent* esdEvent) {
  const AliESDHeader *esdHeader = esdEvent->GetHeader();
  if (NULL == esdHeader) // this is already dealt with in UserExec
    return;

  fClassMask       = esdHeader->GetTriggerMask();
  fClassMaskNext50 = esdHeader->GetTriggerMaskNext50();

  fRunNumber       = esdEvent->GetRunNumber();

  fL0Inputs        = esdHeader->GetL0TriggerInputs();
  fL1Inputs        = esdHeader->GetL1TriggerInputs();
  fL2Inputs        = esdHeader->GetL2TriggerInputs();

  fBCID            = esdHeader->GetBunchCrossNumber();
  fOrbitID         = esdHeader->GetOrbitNumber();
  fPeriod          = esdHeader->GetPeriodNumber();
  fTimeStamp       = esdHeader->GetTimeStamp();
}

void AliAnalysisTaskDG::ADV0::FillInvalid() {
  fTime[0] = fTime[1] = -10240.0f;
  fBB[0] = fBG[0] = fBB[1] = fBG[1] = -1;
  for (Int_t bc=0; bc<21; ++bc)
    fPFBBA[bc] = fPFBBC[bc] = fPFBGA[bc] = fPFBGC[bc] = 0;
}

void AliAnalysisTaskDG::ADV0::FillAD(const AliESDEvent *esdEvent, AliTriggerAnalysis &trigAna) {
  fDecisionOnline[0]  = trigAna.ADTrigger(esdEvent, AliTriggerAnalysis::kCSide, kFALSE);
  fDecisionOnline[1]  = trigAna.ADTrigger(esdEvent, AliTriggerAnalysis::kASide, kFALSE);

  fDecisionOffline[0] = trigAna.ADTrigger(esdEvent, AliTriggerAnalysis::kCSide, kTRUE);
  fDecisionOffline[1] = trigAna.ADTrigger(esdEvent, AliTriggerAnalysis::kASide, kTRUE);

  const AliESDAD *esdAD = esdEvent->GetADData();
  if (NULL == esdAD) {
    FillInvalid();
    return;
  }
  fTime[0] = esdAD->GetADCTime();
  fTime[1] = esdAD->GetADATime();

  fBB[0] = fBB[1] = fBG[0] = fBG[1] = 0;
  for (Int_t ch=0; ch<4; ++ch) {
    fBB[0] += (esdAD->GetBBFlag(ch  ) && esdAD->GetBBFlag(ch+ 4));
    fBB[1] += (esdAD->GetBBFlag(ch+8) && esdAD->GetBBFlag(ch+12));
    fBG[0] += (esdAD->GetBGFlag(ch  ) && esdAD->GetBGFlag(ch+ 4));
    fBG[1] += (esdAD->GetBGFlag(ch+8) && esdAD->GetBGFlag(ch+12));
  }

  for (Int_t bc=0; bc<21; ++bc) {
    fPFBBA[bc] = fPFBBC[bc] = fPFBGA[bc] = fPFBGC[bc] = 0;
    for (Int_t ch=0; ch<4; ++ch) {
      fPFBBC[bc] += (esdAD->GetPFBBFlag(ch, bc) && esdAD->GetPFBBFlag(ch+4, bc));
      fPFBGC[bc] += (esdAD->GetPFBGFlag(ch, bc) && esdAD->GetPFBGFlag(ch+4, bc));

      fPFBBA[bc] += (esdAD->GetPFBBFlag(ch+8, bc) && esdAD->GetPFBBFlag(ch+12, bc));
      fPFBGA[bc] += (esdAD->GetPFBGFlag(ch+8, bc) && esdAD->GetPFBGFlag(ch+12, bc));
    }
  }
}
void AliAnalysisTaskDG::ADV0::FillV0(const AliESDEvent *esdEvent, AliTriggerAnalysis &trigAna) {
  fDecisionOnline[0]  = trigAna.V0Trigger(esdEvent, AliTriggerAnalysis::kCSide, kFALSE);
  fDecisionOnline[1]  = trigAna.V0Trigger(esdEvent, AliTriggerAnalysis::kASide, kFALSE);

  fDecisionOffline[0] = trigAna.V0Trigger(esdEvent, AliTriggerAnalysis::kCSide, kTRUE);
  fDecisionOffline[1] = trigAna.V0Trigger(esdEvent, AliTriggerAnalysis::kASide, kTRUE);

  const AliESDVZERO *esdV0 = esdEvent->GetVZEROData();
  if (NULL == esdV0) {
    FillInvalid();
    return;
  }

  fTime[0] = esdV0->GetV0CTime();
  fTime[1] = esdV0->GetV0ATime();

  fBB[0] = fBB[1] = fBG[0] = fBG[1] = 0;
  for (Int_t ch=0; ch<64; ++ch) {
    fBB[ch/32] += esdV0->GetBBFlag(ch);
    fBG[ch/32] += esdV0->GetBGFlag(ch);
  }

  for (Int_t bc=0; bc<21; ++bc) {
    fPFBBA[bc] = fPFBBC[bc] = fPFBGA[bc] = fPFBGC[bc] = 0;
    for (Int_t ch=0; ch<32; ++ch) {
      fPFBBC[bc] += esdV0->GetPFBBFlag(ch,    bc);
      fPFBGC[bc] += esdV0->GetPFBGFlag(ch,    bc);
      fPFBBA[bc] += esdV0->GetPFBBFlag(ch+32, bc);
      fPFBGA[bc] += esdV0->GetPFBGFlag(ch+32, bc);
    }
  }
}

void AliAnalysisTaskDG::FMD::Fill(const AliESDEvent *esdEvent, AliTriggerAnalysis &trigAna) {
  fA = trigAna.FMDTrigger(esdEvent, AliTriggerAnalysis::kASide);
  fC = trigAna.FMDTrigger(esdEvent, AliTriggerAnalysis::kCSide);
}

void AliAnalysisTaskDG::FindChipKeys(AliESDtrack *tr, Short_t chipKeys[2], Int_t status[2]) {
  chipKeys[0] = chipKeys[1] = -1;
  status[0]   = status[1]   = -1;
  if (!tr)
    return;

  Int_t   idet=0;
  Float_t xloc=0, zloc=0;
  const AliITSsegmentationSPD seg;
  for (Int_t layer=0; layer<2; ++layer) {
    chipKeys[layer] = -1;
    status[layer]   = -1;
    const Int_t module = tr->GetITSModuleIndex(layer);
    if (module < 0)
      continue;

    tr->GetITSModuleIndexInfo(layer, idet, status[layer], xloc, zloc);
    if (status[layer] == 0)
      continue;

    Int_t off = seg.GetChipFromLocal(xloc, zloc);
    if (off < 0)
      continue;

    off = (layer==0 ? 4-off : off);
    AliDebugClassF(5, "layer=%d module=%10d idet=%3d xloc=%5.1f zloc=%5.1f off=%d status=%d chipKey=%4d",
		   layer, module, idet, xloc, zloc, off, status[layer], 5*idet + off);
    chipKeys[layer] = 5*idet + off;
  }
}

void AliAnalysisTaskDG::TrackData::Fill(AliESDtrack *tr, AliPIDResponse *pidResponse=NULL) {
  if (NULL == tr || NULL == pidResponse) {
    AliError(Form("tr=%p pidResponse=%p", tr, pidResponse));
    return;
  }
  fSign = tr->GetSign();
  fPx   = tr->Px();
  fPy   = tr->Py();
  fPz   = tr->Pz();
  fITSsignal = tr->GetITSsignal();
  fTPCsignal = tr->GetTPCsignal();
  fTOFsignal = tr->GetTOFsignal();

  fPIDStatus[0] = pidResponse->CheckPIDStatus(AliPIDResponse::kITS, tr);
  fPIDStatus[1] = pidResponse->CheckPIDStatus(AliPIDResponse::kTPC, tr);
  fPIDStatus[2] = pidResponse->CheckPIDStatus(AliPIDResponse::kTOF, tr);

  for (Int_t i=0; i<AliPID::kSPECIES; ++i) {
    const AliPID::EParticleType particleType(static_cast<const AliPID::EParticleType>(i));
    fNumSigmaITS[i] = pidResponse->NumberOfSigmasITS(tr, particleType);
    fNumSigmaTPC[i] = pidResponse->NumberOfSigmasTPC(tr, particleType);
    fNumSigmaTOF[i] = pidResponse->NumberOfSigmasTOF(tr, particleType);
    AliDebugF(5, "%d %f %f %f", i, fNumSigmaITS[i], fNumSigmaTPC[i], fNumSigmaTOF[i]);
  }

  AliAnalysisTaskDG::FindChipKeys(tr, fChipKey, fStatus);
}

void AliAnalysisTaskDG::SetMaxTracksSave(Int_t m) {
  fTrackData.Expand(m);
  fMaxTracksSave = m;
}

AliAnalysisTaskDG::AliAnalysisTaskDG(const char *name)
  : AliAnalysisTaskSE(name)
  , fIsMC(kFALSE)
  , fTreeBranchNames("")
  , fTrackCutType("TPCOnly")
  , fTriggerSelection("")
  , fTriggerSelectionSPD("")
  , fMaxTracksSave(4)
  , fTriggerAnalysis()
  , fAnalysisUtils()
  , fList(NULL)
  , fTE(NULL)
  , fIR1InteractionMap()
  , fIR2InteractionMap()
  , fFastOrMap()
  , fFiredChipMap()
  , fVertexSPD()
  , fVertexTPC()
  , fVertexTracks()
  , fTOFHeader()
  , fTriggerIRs("AliTriggerIR", 3)
  , fTrackData("AliAnalysisTaskDG::TrackData", fMaxTracksSave)
  , fMCTracks("TLorentzVector", 2)
  , fTrackCuts(NULL)
{
  for (Int_t i=0; i<kNHist;++i) {
    fHist[i] = NULL;
  }
  DefineOutput(1, TList::Class());
  DefineOutput(2, TTree::Class());
}

 AliAnalysisTaskDG::~AliAnalysisTaskDG()
{
  const AliAnalysisManager *man = AliAnalysisManager::GetAnalysisManager();
  if (NULL != man && man->GetAnalysisType() == AliAnalysisManager::kProofAnalysis)
    return;

  if (NULL != fList)
    delete fList;
  fList = NULL;

  if (NULL != fTE)
    delete fTE;
  fTE = NULL;

  if (NULL != fTrackCuts)
    delete fTrackCuts;
  fTrackCuts = NULL;

}

void AliAnalysisTaskDG::SetBranches(TTree* t) {
  t->Branch("AliAnalysisTaskDG::TreeData",            &fTreeData);

  if (fTreeBranchNames.Contains("IRMap")) {
    t->Branch("IR1InteractionMap", &fIR1InteractionMap, 32000, 0);
    t->Branch("IR2InteractionMap", &fIR2InteractionMap, 32000, 0);
  }
  if (fTreeBranchNames.Contains("SPDMaps")) {
    t->Branch("FastOrMap",         &fFastOrMap,    32000, 0);
    t->Branch("FiredChipMap",      &fFiredChipMap, 32000, 0);
  }
  if (fTreeBranchNames.Contains("VertexSPD")) {
    t->Branch("VertexSPD",         &fVertexSPD, 32000, 0);
  }
  if (fTreeBranchNames.Contains("VertexTPC")) {
    t->Branch("VertexTPC",         &fVertexTPC, 32000, 0);
  }
  if (fTreeBranchNames.Contains("VertexTracks")) {
    t->Branch("VertexTracks",      &fVertexTracks, 32000, 0);
  }
  if (fTreeBranchNames.Contains("TOFHeader")) {
    t->Branch("TOFHeader",         &fTOFHeader, 32000, 0);
  }
  if (fTreeBranchNames.Contains("TriggerIR")) {
    t->Branch("TriggerIRs",        &fTriggerIRs, 32000, 0);
  }

  if (fTreeBranchNames.Contains("Tracks")) {
    t->Branch("AliAnalysisTaskDG::TrackData", &fTrackData);
  }

  if (fIsMC) {
    t->Branch("TLorentzVector", &fMCTracks, 32000, 0);
  }

}

void AliAnalysisTaskDG::UserCreateOutputObjects()
{
  if (fTrackCutType == "ITSPureSA") {
    fTrackCuts  = AliESDtrackCuts::GetStandardITSPureSATrackCuts2010(kTRUE, kFALSE);
    fTrackCuts->SetClusterRequirementITS(AliESDtrackCuts::kSPD,
					 AliESDtrackCuts::kBoth);
  }

  if (fTrackCutType == "TPCOnly")
    fTrackCuts = AliESDtrackCuts::GetStandardTPCOnlyTrackCuts();

  if (fTrackCutType == "ITSTPC2011")
    fTrackCuts  = AliESDtrackCuts::GetStandardITSTPCTrackCuts2011(kTRUE, 1);

  if (fTrackCutType == "ITSTPC2011_SPDboth") {
    fTrackCuts  = AliESDtrackCuts::GetStandardITSTPCTrackCuts2011(kTRUE, 1);
    fTrackCuts->SetClusterRequirementITS(AliESDtrackCuts::kSPD,
					 AliESDtrackCuts::kBoth);
  }

  if (NULL == fTrackCuts) {
    AliFatal(Form("NULL == fTrackCuts (%s)", fTrackCutType.Data()));
  }

  fList = new TList;
  fList->SetOwner(kTRUE);
  fList->SetName(GetListName());
  fHist[kHistTrig] = new TH1D("HTrig", ";trigger class index", 102, -1.5, 100.5);
  fHist[kHistTrig]->SetStats(0);
  fList->Add(fHist[kHistTrig]);

  fHist[kHistSPDFiredTrk] = new TH3D("HSPDFiredTrk", fTriggerSelectionSPD+";chip key;BCmod4;mult",
				     1200, -0.5, 1199.5, 4, -0.5, 3.5, 10, -0.5, 9.5);
  fHist[kHistSPDFiredTrk]->SetStats(0);
  fList->Add(fHist[kHistSPDFiredTrk]);

  fHist[kHistSPDFOTrk] = new TH3D("HSPDFOTrk", fTriggerSelectionSPD+";chip key;BCmod4;mult",
				  1200, -0.5, 1199.5, 4, -0.5, 3.5, 10, -0.5, 9.5);
  fHist[kHistSPDFOTrk]->SetStats(0);
  fList->Add(fHist[kHistSPDFOTrk]);

  fHist[kHistSPDFiredTrkVsMult] = new TH3D("HSPDFiredTrkVsMult", fTriggerSelectionSPD+";chip key;BCmod4;log_{10}(number of tracklets)",
					   1200, -0.5, 1199.5, 4, -0.5, 3.5, 25, 0.0, 5.0);
  fHist[kHistSPDFiredTrkVsMult]->SetStats(0);
  fList->Add(fHist[kHistSPDFiredTrkVsMult]);

  fHist[kHistSPDFOTrkVsMult] = new TH3D("HSPDFOTrkVsMult", fTriggerSelectionSPD+";chip key;BCmod4;log_{10}(number of tracklets)",
					1200, -0.5, 1199.5, 4, -0.5, 3.5, 25, 0.0, 5.0);
  fHist[kHistSPDFOTrkVsMult]->SetStats(0);
  fList->Add(fHist[kHistSPDFOTrkVsMult]);

  fHist[kHistSPDFiredVsMult] = new TH3D("HSPDFiredVsMult", fTriggerSelectionSPD+";chip key;BCmod4;log_{10}(number of tracklets)",
					1200, -0.5, 1199.5, 4, -0.5, 3.5, 25, 0.0, 5.0);
  fHist[kHistSPDFiredVsMult]->SetStats(0);
  fList->Add(fHist[kHistSPDFiredVsMult]);

  fHist[kHistSPDFOVsMult] = new TH3D("HSPDFOVsMult", fTriggerSelectionSPD+";chip key;BCmod4;log_{10}(number of tracklets)",
				     1200, -0.5, 1199.5, 4, -0.5, 3.5, 25, 0.0, 5.0);
  fHist[kHistSPDFOVsMult]->SetStats(0);
  fList->Add(fHist[kHistSPDFOVsMult]);

  PostData(1, fList);

  TDirectory *owd = gDirectory;
  TFile *fSave = OpenFile(1);
  fTE = new TTree(GetTreeName(), "");
  SetBranches(fTE);
  PostData(2, fTE);
  owd->cd();
}

void AliAnalysisTaskDG::NotifyRun()
{
  AliInfoF("run %d", fCurrentRunNumber);
}

class TClonesArrayGuard {
public:
  TClonesArrayGuard(TClonesArray &a)
    : fA(a) {}
  ~TClonesArrayGuard() {
    fA.Delete();
  }
private:
  TClonesArrayGuard(const TClonesArrayGuard&);
  TClonesArrayGuard& operator=(const TClonesArrayGuard&);
  TClonesArray& fA;
} ;

void AliAnalysisTaskDG::FillTH3(Int_t idx, Double_t x, Double_t y, Double_t z, Double_t w) {
  if (idx < 0 || idx >= kNHist)
    AliFatalF("idx=%d", idx);
  TH3 *h = dynamic_cast<TH3*>(fHist[idx]);
  if (!h)
    AliFatal("h==NULL");
  h->Fill(x, y, z, w);
}
void AliAnalysisTaskDG::UserExec(Option_t *)
{
  AliVEvent *event = InputEvent();
  if (NULL == event) {
    AliFatal("NULL == event");
    return;
  }

  // ESD Event
  AliESDEvent* esdEvent = dynamic_cast<AliESDEvent*>(InputEvent());
  if (NULL == esdEvent) {
    AliFatal("NULL == esdEvent");
    return;
  }

  // input handler
  const AliAnalysisManager* man(AliAnalysisManager::GetAnalysisManager());
  if (NULL == man) {
    AliFatal("NULL == man");
    return;
  }

  AliESDInputHandler* inputHandler(dynamic_cast<AliESDInputHandler*>(man->GetInputEventHandler()));
  if (NULL == inputHandler) {
    AliFatal("NULL == inputHandler");
    return;
  }

  AliPIDResponse* pidResponse = inputHandler->GetPIDResponse();
  if (NULL == pidResponse) {
    AliFatal("NULL == pidResponse");
    return;
  }

  const AliESDHeader *esdHeader = esdEvent->GetHeader();
  if (NULL == esdHeader) {
    AliFatal("NULL == esdHeader");
    return;
  }

  if (kFALSE == fIsMC && esdHeader->GetEventType() != AliRawEventHeaderBase::kPhysicsEvent)
    return;

  const AliMultiplicity *mult = esdEvent->GetMultiplicity();
  if (NULL == mult) {
    AliFatal("NULL == mult");
    return;
  }

  fHist[kHistTrig]->Fill(-1); // # analyzed events in underflow bin

  for (Int_t i=0; i<50; ++i) {
    const ULong64_t mask(1ULL<<i);
    if ((esdHeader->GetTriggerMask() & mask) == mask)
      fHist[kHistTrig]->Fill(i);
    if ((esdHeader->GetTriggerMaskNext50() & mask) == mask)
      fHist[kHistTrig]->Fill(50+i);
  }

  Bool_t selectedForSPD = (fTriggerSelectionSPD == "");
  if (!selectedForSPD) { // trigger selection for SPD efficiency studies
    std::unique_ptr<const TObjArray> split(fTriggerSelectionSPD.Tokenize("|"));
    for (Int_t i=0, n=split->GetEntries() && !selectedForSPD; i<n; ++i) {
      const TString tcName(split->At(i)->GetName());
      selectedForSPD = esdEvent->GetFiredTriggerClasses().Contains(tcName);
    }
  }
  AliInfoF("selectedForSPD = %d", selectedForSPD);
  if (selectedForSPD) { // PF protection
    const AliESDAD    *esdAD = esdEvent->GetADData();
    const AliESDVZERO *esdV0 = esdEvent->GetVZEROData();
    Int_t nBB=0;
    for (Int_t bc=3; bc<=17 && !nBB; ++bc) {
      if (bc == 10)
	continue;
      for (Int_t ch=0; ch<4; ++ch) {
	nBB += (esdAD->GetPFBBFlag(ch,   bc) && esdAD->GetPFBBFlag(ch+ 4, bc));
	nBB += (esdAD->GetPFBBFlag(ch+8, bc) && esdAD->GetPFBBFlag(ch+12, bc));
      }
      for (Int_t ch=0; ch<64; ++ch)
	nBB += esdV0->GetPFBBFlag(ch, bc);
    }
    if (!nBB) {
      Int_t matched[1200] = { 0 };
      for (Int_t l=0; l<1200; ++l)
	matched[l] = 0;
      std::unique_ptr<AliESDtrackCuts> tc(AliESDtrackCuts::GetStandardITSPureSATrackCuts2010(kTRUE, kFALSE));
      std::unique_ptr<const TObjArray> oa(tc->GetAcceptedTracks(esdEvent));
      for (Int_t i=0, n=oa->GetEntries(); i<n; ++i) {
	AliESDtrack *tr = dynamic_cast<AliESDtrack*>(oa->At(i));
	Short_t chipKeys[2] = { -1, -1};
	Int_t   status[2]   = { -1, -1};
	AliAnalysisTaskDG::FindChipKeys(tr, chipKeys, status);
	for (Int_t layer=0; layer<2; ++layer) {
	  if (chipKeys[layer] >= 0 && chipKeys[layer]<1200 && status[layer] == 1)
	    matched[chipKeys[layer]] += 1;
	}
      }
      const Int_t    bcMod4         = (esdHeader->GetBunchCrossNumber() % 4);
      const Double_t log10Tracklets = (mult->GetNumberOfTracklets() > 0
				       ? TMath::Log10(mult->GetNumberOfTracklets())
				       : -1.0);
      for (Int_t chipKey=0; chipKey<1200; ++chipKey) {
	if (mult->TestFiredChipMap(chipKey)) {
	  FillTH3(kHistSPDFiredTrk,       chipKey, bcMod4, matched[chipKey]);
	  FillTH3(kHistSPDFiredTrkVsMult, chipKey, bcMod4, log10Tracklets, (matched[chipKey]>0));
	  FillTH3(kHistSPDFiredVsMult,    chipKey, bcMod4, log10Tracklets);
	}
	if (mult->TestFastOrFiredChips(chipKey)) {
	  FillTH3(kHistSPDFOTrk,       chipKey, bcMod4, matched[chipKey]);
	  FillTH3(kHistSPDFOTrkVsMult, chipKey, bcMod4, log10Tracklets, (matched[chipKey]>0));
	  FillTH3(kHistSPDFOVsMult,    chipKey, bcMod4, log10Tracklets);
	}
      }
    }
  }

  PostData(1, fList);

  Bool_t cutNotV0    = kFALSE;
  Bool_t useOnly2Trk = kFALSE;

  Bool_t selected = (fTriggerSelection == "");

  if (!selected) {
    // fTriggerSelection can be "CLASS1|CLASS2&NotV0|CLASS2&Only2Trk"
    std::unique_ptr<const TObjArray> split(fTriggerSelection.Tokenize("|"));

    Int_t sumCutNotV0(0);
    Int_t sumUseOnly2Trk(0);

    Int_t counter=0;
    for (Int_t i=0, n=split->GetEntries(); i<n; ++i) {
      TString tcName(split->At(i)->GetName());
      std::unique_ptr<const TObjArray> s(tcName.Tokenize("&"));
      if (esdEvent->GetFiredTriggerClasses().Contains(s->At(0)->GetName())) {
	sumCutNotV0    += (s->GetEntries() == 2 && TString(s->At(1)->GetName()) == "NotV0");
	sumUseOnly2Trk += (s->GetEntries() == 2 && TString(s->At(1)->GetName()) == "Only2Trk");
	++counter;
      }
    }

    selected    = (counter != 0);
    cutNotV0    = (counter == sumCutNotV0);
    useOnly2Trk = (counter == sumUseOnly2Trk);
  }

  AliInfoF("selected: %d %d %d %s ", selected, cutNotV0, useOnly2Trk, esdEvent->GetFiredTriggerClasses().Data());
  if (!selected)
    return;

  fTreeData.fEventInfo.Fill(esdEvent);

  fTreeData.fIsIncompleteDAQ          = esdEvent->IsIncompleteDAQ();
  fTreeData.fIsSPDClusterVsTrackletBG = fAnalysisUtils.IsSPDClusterVsTrackletBG(esdEvent);
  fTreeData.fIskMB                    = (inputHandler->IsEventSelected() & AliVEvent::kMB);
  AliInfoF("inputHandler->IsEventSelected() = %d", inputHandler->IsEventSelected());
  fTreeData.fV0Info.FillV0(esdEvent, fTriggerAnalysis);
  fTreeData.fADInfo.FillAD(esdEvent, fTriggerAnalysis);

  if (cutNotV0 &&
      (fTreeData.fV0Info.fDecisionOnline[0] != 0 ||
       fTreeData.fV0Info.fDecisionOnline[1] != 0))
    return;

  fVertexSPD    = *(esdEvent->GetPrimaryVertexSPD());
  fVertexTPC    = *(esdEvent->GetPrimaryVertexTPC());
  fVertexTracks = *(esdEvent->GetPrimaryVertexTracks());

  fTOFHeader    = *(esdEvent->GetTOFHeader());

  TClonesArrayGuard guardTriggerIR(fTriggerIRs);
  // store trigger IR for up to +-1 orbits around the event
  for (Int_t i=0,j=0,n=esdHeader->GetTriggerIREntries(); i<n; ++i) {
    const AliTriggerIR *ir = esdHeader->GetTriggerIR(i);
    if (!ir || TMath::Abs(Int_t(ir->GetOrbit()&0xFFFF) - Int_t(fTreeData.fEventInfo.fOrbitID)) > 1)
      continue;
    new(fTriggerIRs[j++]) AliTriggerIR(*ir);
  }

  fFastOrMap    = mult->GetFastOrFiredChips();
  fFiredChipMap = mult->GetFiredChipMap();

  fIR1InteractionMap = esdHeader->GetIRInt1InteractionMap();
  fIR2InteractionMap = esdHeader->GetIRInt2InteractionMap();

  for (Int_t i=0; i<4; ++i)
    fTreeData.fEventInfo.fnTrklet[i] = 0;
  for (Int_t i=0, n=mult->GetNumberOfTracklets(); i<n; ++i) {
    const Double_t eta = -TMath::Log(TMath::Tan(0.5*mult->GetTheta(i)));
    fTreeData.fEventInfo.fnTrklet[0] += 1;           // all tracklets
    fTreeData.fEventInfo.fnTrklet[1] += (eta <  -0.9);
    fTreeData.fEventInfo.fnTrklet[2] += (eta >= -0.9 && eta <= +0.9);
    fTreeData.fEventInfo.fnTrklet[3] += (eta >  +0.9);
  }

  std::unique_ptr<const TObjArray> oa(fTrackCuts->GetAcceptedTracks(esdEvent));
  fTreeData.fEventInfo.fnTrk = oa->GetEntries();
  fTreeData.fEventInfo.fCharge = 0;

  if (useOnly2Trk && fTreeData.fEventInfo.fnTrk != 2)
    return;

  for (Int_t i=0, n=oa->GetEntries(); i<n; ++i)
    fTreeData.fEventInfo.fCharge += Int_t(dynamic_cast<AliESDtrack*>(oa->At(i))->GetSign());

  TClonesArrayGuard guardTrackData(fTrackData);
  if (oa->GetEntries() <= fMaxTracksSave)  {
    for (Int_t i=0, n=TMath::Min(oa->GetEntries(), fMaxTracksSave); i<n; ++i)
      new(fTrackData[i]) TrackData(dynamic_cast<AliESDtrack*>(oa->At(i)), pidResponse);
  }

  TClonesArrayGuard guardMCTracks(fMCTracks);
  if (fIsMC) {
    AliMCEvent *mcEvent = MCEvent();
    if (NULL == mcEvent)
      AliFatal("NULL ==mcEvent");

    Int_t counter = 0;
    for(Int_t i=0, n=mcEvent->GetNumberOfTracks(); i<n && counter<2; ++i) {
      AliMCParticle *p = dynamic_cast<AliMCParticle*>(mcEvent->GetTrack(i));
      if (NULL == p) continue;
      new(fMCTracks[counter]) TLorentzVector;
      TLorentzVector *v = dynamic_cast<TLorentzVector*>(fMCTracks.At(counter));
      p->Particle()->Momentum(*v);
      ++counter;
    }
  } // fIsMC

  fTE->Fill();
  PostData(2, fTE);
}

void AliAnalysisTaskDG::Terminate(Option_t*)
{
  fList  = dynamic_cast<TList*>(GetOutputData(1));
  if (NULL == fList)
    Error("Terminate","fList is not available");

  fTE  = dynamic_cast<TTree*>(GetOutputData(2));
  if (NULL == fTE)
    Error("Terminate","fTE is not available");

}
