/*****************************************************
 * A program to process raw root files from wavedump
 *
 * NB This program is replaced by analyse_cooked
 * but is kept to preserve routines not yet migrated
 * across
 *
 * Author 
 *  gary.smith@ed.ac.uk
 *  01 01 2019
 *
 * Purpose
 *  This program reads a TTree
 *
 * How to build
 *  $ make 
 *
 * How to run
 *
 *  Example One - analyse one raw root file
 *  $ ./executePMTAnalysis ../../RawRootData/Run_11_PMT_90_Loc_0_Test_D.root  
 *
 *  Example Two - analyse two raw root files (two HV steps)
 *  $ ./executePMTAnalysis ../../RawRootData/Run_11_PMT_90_Loc_0_HV_1.root ../../RawRootData/Run_11_PMT_90_Loc_0_HV_2.root 
 *
 *  Example Three - analyse multiple raw root files ( all run numbers )
 *  $ ./executePMTAnalysis ../../RawRootData/Run_*_PMT_90_Loc_0_Test_D.root  
 *
 * Dependencies
 *  root.cern
 *  PMTAnalyser
 *  DataInfo
 *  ShippingData
 *
 * Known issues
 *   Under linux you will likely be required to add 
 *   the shared (.so) file location to your library path 
 *   ie add the path to this directory 
 *
 */ 

#include <iostream>

#include <string>
#include "TFile.h"
#include "TTree.h"
#include "TH1.h"
#include "TString.h"
#include "PMTAnalyser.h"
#include "ShippingData.h"
#include "FileNameParser.h"

using namespace std;

bool IsValidDigitiser(Char_t, Int_t);

Bool_t IsInteger(string); 

Bool_t IsYes(string); 

Bool_t IsValidArgc(int); 
  
int main(Int_t argc, Char_t *argv[]){
  
  // 'V' for VME, 'D' for desktop
  Char_t  digitiser = 'D';
  
  Bool_t investigateTiming   = kFALSE;
  Bool_t investigateRiseFall = kFALSE;
  Bool_t investigatePulses   = kFALSE;
  Bool_t investigateDarkRate = kFALSE;
  Bool_t investigateFFT      = kFALSE;

  Bool_t writeOutput         = kTRUE;

  TFile * outFile = nullptr;  
  TFile * inFile  = nullptr;
  
  TTree          * tree = nullptr;
  PMTAnalyser    * PMT = nullptr;
  //  ShippingData   * shipData = nullptr;
  FileNameParser * testInfo = new FileNameParser(argv[1]);

//    shipData = new ShippingData(130);
//    cout << " DR(130) = " << shipData->GetDR() << endl;
  
  if( !IsValidDigitiser(digitiser,testInfo->GetRun()) ||
      !IsValidArgc(argc) ){
    return -1;
  }

  // signal threshold for timing
  Float_t thresh_mV  = 20.0;

  // Dark Rate
  Int_t   darkRate   = 8000.;
  
  // Testing reading from root file, writing to new file
  TH1F  * hQ   = nullptr;
  
  // Timing peaks
  Float_t peakMeans[argc-1], peakMean = 50;
  for (Int_t i = 0 ; i < (argc-1) ; i++)
    peakMeans[i] = 0.;
  
  // argv should be a path to a file
  // or list of files ( wildcards work )
  for( int iFile = 1 ; iFile < argc ; iFile++){
    
    inFile = new TFile(argv[iFile],"READ");
    
    if (!inFile || !inFile->IsOpen()) {
      inFile = new TFile();
      cerr << " Error, Check File: " << argv[iFile] << endl; 
      return -1;
    }

    // connect to tree in input file
    TString treeName = (TString)testInfo->GetTreeName(argv[iFile]);
    inFile->GetObject(treeName,tree); 
    
    // initalise analysis object using tree 
    PMT = new PMTAnalyser(tree,
			  digitiser);
    
    // Set plot attributes to WM TStyle 
    PMT->SetStyle();
    
    // Limit to subset of entries for quicker testing
    //PMT->SetTestMode(kFALSE);
    //PMT->SetTestMode();
     
    // Testing output 
    //PMT->MakeCalibratedTree();

    //shipData = new ShippingData(testInfo->pmtID(argv[iFile]));

    // FFT study
    // PMT->PlotAccumulatedFFT();
    
    if     (testInfo->run(argv[iFile])==102){
      thresh_mV  = 20.0;
    }
    else if(testInfo->run(argv[iFile])==103){
      thresh_mV  = 2.0;
    }
    
    //------------
    // Timing Study
    
    if(iFile < argc && investigateTiming){
      peakMean = PMT->TimeOfPeak(thresh_mV);
      peakMeans[iFile-1] = peakMean;
      cout << endl;
      cout << " mean of gaussian fit to peak " << 
	peakMeans[iFile-1] << endl;
    }
    
    //------------
    // Rise/Fall Time Study
    
    // number of pulses to fit 
    int nPulses = 50;
    
    // threshold for pulse fitting
    // and dark counts
    thresh_mV = 15;
    
    if     (testInfo->run(argv[iFile])==102){
      peakMean = 292.;
    }
    else if(testInfo->run(argv[iFile])==103){
      peakMean  = 264.;
      thresh_mV = 1.5;
    }
    
    if(investigateRiseFall)
      PMT->RiseFallTime(nPulses,peakMean,
			thresh_mV);

    int event = 0;
    if(!investigatePulses) 
      event = -1;
    
    // plot waveforms and 
    // optionall1y in addition FFT
    
    Bool_t plotFFTs = true;
    string usrInFFT = "n";

    while ( event!= -1 ){
      cout << endl;
      cout << " Which waveform to plot?" << endl;
      cout << " enter event number,    " << endl;
      cout << " enter for next event   " << endl;
      cout << " or -1 to quit          " << endl;
      
       string usrInWav;
       getline(cin, usrInWav);
       
       if (usrInWav.empty())
	 event++;
       else if(IsInteger(usrInWav))
	 event = stoi(usrInWav); 
       else 
	 continue;
       
       if ( event > -1 )
	 PMT->PlotWaveform(event);
       else
	 continue;
       
       if(!plotFFTs)
	 continue;
       
       if(usrInFFT=='n'){
	 cout << endl;
	 cout << " Plot FFT/s?"  << endl;
	 cout << " answer: y/n " << endl;
	 getline(cin, usrInFFT);
       }
       
       if  ( IsYes(usrInFFT) )
	 PMT->PlotFFT(event);
       else
	 plotFFTs = false;
    }
    //------------
     //  Dark Rate
     if( investigateDarkRate && 
	 testInfo->test(argv[iFile])=='D'){
       
       darkRate = PMT->DarkRate(thresh_mV);
       cout << " PMT Test  Dark Rate = " << darkRate          << endl;
     }
     
     //------------
     //  FFT investigation
     // Make Filtered Histograms
     if(investigateFFT){ 
       TCanvas * canvas = PMT->Make_FFT_Canvas();
       
       TString canvasName;
       canvasName.Form("./Plots/FFT_Run_%d_PMT_%d_Test_%c.pdf",
		       testInfo->run(argv[iFile]),
		       testInfo->pmtID(argv[iFile]),
		       testInfo->test(argv[iFile]));
       
       if(canvas){
	 canvas->SaveAs(canvasName);
	 
	 canvasName.Form("./Plots/FFT_Run_%d_PMT_%d_Test_%c.root",
			 testInfo->run(argv[iFile]),
			 testInfo->pmtID(argv[iFile]),
			 testInfo->test(argv[iFile]));
	 
	 canvas->SaveAs(canvasName);
       }
       else{
	 cout << endl;
	 cout << " No canvas produced " << endl;
       }
     }
     
     cout << endl;
     cout << " Analysis Complete " << endl;

     cout << endl;
     cout << " Deleting input file pointer" << endl;
     inFile->Delete();

     if(!writeOutput)
       continue;
     
     TString hName = testInfo->Get_hQ_Fixed_Name(argv[iFile]); 
     hQ = (TH1F*)inFile->Get(hName);
     
     TString outputName = "_out.root";
     
     TString fileID = (TString)testInfo->GetFileID(argv[iFile]);
     
     outputName = fileID + outputName;

     cout << endl;
     cout << " Recreating output file " << endl;
     outFile = new TFile(outputName,"RECREATE");
     
     // Writing to file
     // put file in same directory as objects
     outFile->cd();
     
     cout << endl;
     cout << " Writing histogram " << endl;
     // Write specific histogram
     hQ->Write();
     
     // hQ->SetDirectory(outFile);
     
     // Write objects in directory 
     // outFile->Write();

     cout << endl;
     cout << " Closing Output file " << endl;
     outFile->Close();
     
   }
   
   if(investigateTiming){
     cout << endl;
     for (Int_t i = 0 ; i < (argc-1) ; i++)
       cout << "peakMeans[" << i << "]= " << peakMeans[i] << endl;; 
   }
   
  return 1;
}


bool IsValidDigitiser(Char_t digitiser,
		      Int_t  run){
  
  TString strDigi = "Desktop";

  if( digitiser=='V' && 
      ( run > 39 && run < 70  )){
    
    cerr << " \n Error: Invalid digitiser setting 'V' \n" << endl;
    return false;
    
  }
  else if( digitiser=='D' && 
	   run < 40 ){
    cerr << " \n Error: Invalid digitiser setting 'D' \n" << endl;
    return false;
  }
  else if(digitiser=='D' ||
	  digitiser=='V'){ 
    
    if(digitiser=='V')
      strDigi = "VME";
    
    cout << endl;
    cout << " Using " << strDigi << " digitiser settings " << endl;
    
    return true;
  }
  else
    return false;
  
}

Bool_t IsInteger(string usrInput){
  
  try {
    stoi(usrInput);
  }
  catch (...) {
    cout << "Invalid input. Please try again!\n";
    return kFALSE;
  }
  
  return kTRUE;
}

Bool_t IsYes(string usrInput){
  
  Char_t buff[usrInput.length()+1];
  strcpy(buff,usrInput.c_str());
  
  if( ( usrInput.length() == 1      ) && 
      ( buff[0]=='y' || buff[0]=='Y') )
    return kTRUE;
  else
    return kFALSE;
}

Bool_t IsValidArgc(int argc){

  if (argc >= 2)
    return true;
  else{
    cerr << " Error, argument needed " << endl; 
    return false;
  }
}
