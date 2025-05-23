// Copyright 2019-2020 CERN and copyright holders of ALICE O2.
// See https://alice-o2.web.cern.ch/copyright for details of the copyright holders.
// All rights not expressly granted are reserved.
//
// This software is distributed under the terms of the GNU General Public
// License v3 (GPL Version 3), copied verbatim in the file "COPYING".
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

R__ADD_INCLUDE_PATH($ALICE_ROOT)
R__ADD_INCLUDE_PATH($ALICE_PHYSICS)

#include <iostream>

#include "ANALYSIS/macros/train/AddESDHandler.C"                  // NOLINT
#include "ANALYSIS/macros/train/AddAODHandler.C"                  // NOLINT
#include "ANALYSIS/macros/train/AddMCHandler.C"                   // NOLINT
#include "OADB/COMMON/MULTIPLICITY/macros/AddTaskMultSelection.C" // NOLINT
#include "OADB/macros/AddTaskPhysicsSelection.C"                  // NOLINT
#include "ANALYSIS/macros/AddTaskPIDResponse.C"                   // NOLINT
#include "RUN3/AddTaskAO2Dconverter.C"                            // NOLINT
#include "PWGGA/GammaConv/macros/AddTask_V0Reader.C"              // NOLINT

#include "utilitiesAli.h"

Long64_t convertAO2D(TString listoffiles, bool isMC = 1, bool useAliEvCuts = false, bool isESD = 1, int nmaxevents = -1)
{
  const char* anatype = isESD ? "ESD" : "AOD";
  if (isMC) {
    std::cout << "I AM DOING MC" << std::endl;
  }

  //  TGrid::Connect("alien:");

  // Create the chain based on xml collection or txt file
  // The entries in the txt file can be local paths or alien paths
  TChain* chain = CreateLocalChain(listoffiles.Data(), anatype, -1);
  if (!chain)
    return -1;
  chain->SetNotify(0x0);
  ULong64_t nentries = chain->GetEntries();
  if (nmaxevents != -1)
    nentries = nmaxevents;
  std::cout << nentries << " entries in the chain." << endl;
  std::cout << nentries << " converted" << endl;
  AliAnalysisManager* mgr = new AliAnalysisManager("AOD converter");
  if (isESD) {
    AddESDHandler();
  } else {
    AddAODHandler();
  }

  AddTaskMultSelection();
  AddTaskPhysicsSelection(isMC);
  AddTaskPIDResponse();

  // V0Reader related stuff
  Int_t isHeavyIon = 0;
  TString periodNameV0Reader = "LHC17p";
  TString cutnumberEvent = "00000003";
  if (isHeavyIon == 1)
    cutnumberEvent = "10000003";
  else if (isHeavyIon == 2)
    cutnumberEvent = "80000003";
  else if (isHeavyIon == 29) // UPC
    cutnumberEvent = "100c0003";
  TString conversionPhotonCutnumber = "00000008400000001100000000";
  AddTask_V0Reader(periodNameV0Reader, kFALSE, 0, kTRUE, isHeavyIon, cutnumberEvent + "_" + conversionPhotonCutnumber, conversionPhotonCutnumber);

  if (isMC && isESD)
    AliMCEventHandler* handlerMC = AddMCHandler();
  AliAnalysisTaskAO2Dconverter* converter = AddTaskAO2Dconverter("");
  converter->SetTruncation(true);
  converter->SetCompression(501);
  converter->SetMaxBytes(250000000);
  converter->SetEMCALAmplitudeThreshold(0.075);
  converter->SetConversionCut(conversionPhotonCutnumber);
  converter->SetDeltaAODBranchName(Form("GammaConv_%s_%s_gamma", cutnumberEvent.Data(), conversionPhotonCutnumber.Data()));
  // converter->SelectCollisionCandidates(AliVEvent::kAny);
  if (useAliEvCuts)
    converter->SetUseEventCuts(kTRUE);
  if (isMC)
    converter->SetMCMode();
  if (!mgr->InitAnalysis())
    return -1;
  // PH   mgr->SetBit(AliAnalysisManager::kTrueNotify);
  // mgr->SetRunFromPath(244918);
  mgr->PrintStatus();

  mgr->SetDebugLevel(1);
  return mgr->StartAnalysis("localfile", chain, nentries, 0);
}
