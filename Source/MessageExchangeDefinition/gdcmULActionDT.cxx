/*
This file contains the implementation for the classes for the DT Actions,
Data Transfer Related Actions (Table 9-8 of ps 3.8-2009).

Since each class is essentially a placeholder for a function pointer, I'm breaking with having
each class have its own file for the sake of brevity of the number of files.
*/

#include "gdcmULActionDT.h"
#include "gdcmARTIMTimer.h"
#include "gdcmPDataTFPDU.h"

using namespace gdcm::network;

//Send P-DATA-TF PDU
EStateID ULActionDT1::PerformAction(ULEvent& inEvent, ULConnection& inConnection){

  PDataTFPDU thePDU;//for now, use Matheiu's default values
  gdcm::network::PresentationDataValue pdv;
  thePDU.AddPresentationDataValue( pdv );
  thePDU.Write(*inConnection.GetProtocol());
  inConnection.GetProtocol()->flush();

  return eSta6TransferReady;
}

//Send P-DATA indication primitive
EStateID ULActionDT2::PerformAction(ULEvent& inEvent, ULConnection& inConnection){

  return eSta6TransferReady;
}
