/*************************************************************************
 * Copyright (C) 1995-2021, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#ifndef ROOT_Browsable_RFieldHolder
#define ROOT_Browsable_RFieldHolder


#include <ROOT/Browsable/RHolder.hxx>

#include <ROOT/RNTuple.hxx>
#include <ROOT/RNTupleDescriptor.hxx>
#include <ROOT/RMiniFile.hxx>

#include "TClass.h"


template<typename T>
class RFieldHolder : public ROOT::Experimental::Browsable::RHolder {
   std::shared_ptr<ROOT::Experimental::RNTupleReader> fNTuple;

   ROOT::Experimental::DescriptorId_t fFieldId;

public:
   RFieldHolder(std::shared_ptr<ROOT::Experimental::RNTupleReader> tuple, ROOT::Experimental::DescriptorId_t id) : fNTuple(tuple), fFieldId(id) {}

   const TClass *GetClass() const override { return TClass::GetClass<ROOT::Experimental::RNTuple>(); }

   /** Returns direct (temporary) object pointer */
   const void *GetObject() const override { return nullptr; }

   auto GetNTuple() const { return fNTuple; }
   auto GetId() const { return fFieldId; }

};



#endif
