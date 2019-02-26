/// \file RTreeEntry.cxx
/// \ingroup Forest ROOT7
/// \author Jakob Blomer <jblomer@cern.ch>
/// \date 2018-10-15
/// \warning This is part of the ROOT 7 prototype! It will change without notice. It might trigger earthquakes. Feedback
/// is welcome!

/*************************************************************************
 * Copyright (C) 1995-2015, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#include <ROOT/RTreeEntry.hxx>
#include <ROOT/RTreeValue.hxx>

ROOT::Experimental::RTreeEntry::~RTreeEntry()
{
   for (auto idx : fManagedValues) {
      fTreeValues[idx].GetField()->DestroyValue(fTreeValues[idx]);
   }
}

void ROOT::Experimental::RTreeEntry::AddValue(const Detail::RTreeValueBase& value)
{
   fManagedValues.emplace_back(fTreeValues.size());
   fTreeValues.push_back(value);
}

void ROOT::Experimental::RTreeEntry::CaptureValue(const Detail::RTreeValueBase& value)
{
   fTreeValues.push_back(value);
}
