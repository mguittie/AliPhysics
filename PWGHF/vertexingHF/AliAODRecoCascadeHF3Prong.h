#ifndef ALIAODRECOCASCADEHF3PRONG_H
#define ALIAODRECOCASCADEHF3PRONG_H
/* Copyright(c) 1998-2008, ALICE Experiment at CERN, All rights reserved. *
 * See cxx source for full Copyright notice                               */

/* $Id$ */ 

//***********************************************************
// Class AliAODRecoCascadeHF3Prong
// base class for AOD reconstructed heavy-flavour cascade 3Prong decays
// (Xic+->pi Xi pi, ...)
// The convention is: prong 0 is bachelor, prong 1 is cascade
// prong 2 is bachelor
//
// Author: Y.S. Watanabe, wyosuke@cns.s.u-tokyo.ac.jp
//***********************************************************

#include <TRef.h>
#include <TRefArray.h>
#include <TClonesArray.h>
#include <TClass.h>
#include "AliAODVertex.h"
#include "AliAODcascade.h"
#include "AliAODv0.h"
#include "AliAODRecoDecayHF3Prong.h"

class AliAODRecoCascadeHF3Prong : public AliAODRecoDecayHF3Prong {

 public:

  AliAODRecoCascadeHF3Prong();
  AliAODRecoCascadeHF3Prong(AliAODVertex *vtx2, Short_t charge,
			    Double_t *px, Double_t *py, Double_t *pz,
			    Double_t *d0, Double_t *d0err, 
			    Double_t *dca, Double_t sigvert,
			    Double_t dist12,Double_t dist23);
  virtual ~AliAODRecoCascadeHF3Prong();

  AliAODRecoCascadeHF3Prong(const AliAODRecoCascadeHF3Prong& source);
  AliAODRecoCascadeHF3Prong& operator=(const AliAODRecoCascadeHF3Prong& source);

  AliAODTrack* GetBachelor1() const {return (AliAODTrack*)GetDaughter(0);}
  AliAODTrack* GetBachelor2() const {return (AliAODTrack*)GetDaughter(2);}
  AliAODcascade* GetCascade() const {
    if ( ! ((AliAODRecoDecay*)GetDaughter(1))->IsA()->InheritsFrom("AliAODcascade") ){
      AliWarning("Object is not of type cascade");
      return 0;
    }
    return (AliAODcascade*)GetDaughter(1);
  }

  AliAODTrack* GetCascadePositiveTrack() const { return  (AliAODTrack*)GetCascade()->GetDaughter(0);  }
  AliAODTrack* GetCascadeNegativeTrack() const { return  (AliAODTrack*)GetCascade()->GetDaughter(1);  }
  AliAODTrack* GetCascadeBachelorTrack() const { return  (AliAODTrack*)GetCascade()->GetDecayVertexXi()->GetDaughter(0);  }

  // Xic invariant mass
  Double_t InvMassPiXiPi() const {
    UInt_t pdg[3]={211,3312,211}; return InvMass(3,pdg);
  }

  //  Int_t MatchToMC(Int_t pdgabs,Int_t pdgabs3prong,
  //                  Int_t *pdgDg,Int_t *pdgDg3prong,
  //                  TClonesArray *mcArray, Bool_t isV0=kFALSE) const;

  Double_t CascDcaXiDaughters() const;
  Double_t CascDcaV0Daughters() const;
  Double_t CascDecayLength() const;
  Double_t CascDecayLengthV0() const;
  Double_t CascCosPointingAngle() const;
  Double_t CascCosPointingAngleV0() const;
  Double_t CascDcaV0ToPrimVertex() const;
  Double_t CascDcaPosToPrimVertex() const;
  Double_t CascDcaNegToPrimVertex() const;
  Double_t CascDcaBachToPrimVertex() const;
  Double_t CascMassXi() const;
  Double_t CascMassLambda() const;
  Double_t CascMassAntiLambda() const;

  Double_t XicCosPointingAngle() const;

 protected:

  ClassDef(AliAODRecoCascadeHF3Prong, 1); // heavy-flavour cascade 3prong class
};

#endif
