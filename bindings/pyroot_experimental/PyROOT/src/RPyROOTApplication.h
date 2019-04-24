// Author: Enric Tejedor CERN  04/2019
// Original PyROOT code by Wim Lavrijsen, LBL

/*************************************************************************
 * Copyright (C) 1995-2019, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#ifndef ROOT_PyROOTApplication
#define ROOT_PyROOTApplication

// ROOT
#include "TApplication.h"

namespace PyROOT {

// clang-format off
/**
\class PyROOT::RPyROOTApplication
\brief Interactive application for Python.

 The RPyROOTApplication sets up the nuts and bolts for interactive ROOT use
 from Python, closely following TRint. Note that not everything is done here,
 some bits (such as e.g. the use of exception hook for shell escapes) are more
 easily done in Python.
*/
// clang-format on

class RPyROOTApplication : public TApplication {
public:
   static PyObject *InitApplication();

   RPyROOTApplication(const char *acn, int *argc, char **argv);
   virtual ~RPyROOTApplication() {}

private:
   static bool CreateApplication();
   static void InitROOTGlobals();
   static void InitROOTMessageCallback();
};

} // namespace PyROOT

#endif
