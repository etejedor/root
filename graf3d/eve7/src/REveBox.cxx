// @(#)root/eve:$Id$
// Author: Matevz Tadel, 2010

/*************************************************************************
 * Copyright (C) 1995-2007, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#include "ROOT/REveBox.hxx"
#include "ROOT/REveProjectionManager.hxx"

using namespace ROOT::Experimental;

/** \class REveBox
\ingroup REve
3D box with arbitrary vertices (cuboid).
Vertices 0-3 specify the "bottom" rectangle in clockwise direction and
vertices 4-7 the "top" rectangle so that 4 is above 0, 5 above 1 and so on.

If vertices are provided some local coordinates the transformation matrix
of the element should also be set (but then the memory usage is increased
by the size of the REveTrans object).

Currently only supports 3D -> 2D projections.
*/

ClassImp(REveBox);

////////////////////////////////////////////////////////////////////////////////
/// Constructor.

REveBox::REveBox(const char* n, const char* t) :
   REveShape(n, t)
{
}

////////////////////////////////////////////////////////////////////////////////
/// Destructor.

REveBox::~REveBox()
{
}

////////////////////////////////////////////////////////////////////////////////
/// Set vertex 'i'.

void REveBox::SetVertex(Int_t i, Float_t x, Float_t y, Float_t z)
{
   fVertices[i][0] = x;
   fVertices[i][1] = y;
   fVertices[i][2] = z;
   ResetBBox();
}

////////////////////////////////////////////////////////////////////////////////
/// Set vertex 'i'.

void REveBox::SetVertex(Int_t i, const Float_t* v)
{
   fVertices[i][0] = v[0];
   fVertices[i][1] = v[1];
   fVertices[i][2] = v[2];
   ResetBBox();
}

////////////////////////////////////////////////////////////////////////////////
/// Set vertices.

void REveBox::SetVertices(const Float_t* vs)
{
   memcpy(fVertices, vs, sizeof(fVertices));
   ResetBBox();
}


////////////////////////////////////////////////////////////////////////////////
/// Compute bounding-box of the data.

void REveBox::ComputeBBox()
{
   REveShape::CheckAndFixBoxOrientationFv(fVertices);

   BBoxInit();
   for (Int_t i=0; i<8; ++i)
   {
      BBoxCheckPoint(fVertices[i]);
   }
}

////////////////////////////////////////////////////////////////////////////////
/// Virtual from REveProjectable, return REveBoxProjected class.

TClass* REveBox::ProjectedClass(const REveProjection*) const
{
   return REveBoxProjected::Class();
}


/** \class REveBoxProjected
\ingroup REve
Projection of REveBox.
*/

ClassImp(REveBoxProjected);

Bool_t REveBoxProjected::fgDebugCornerPoints = kFALSE;

////////////////////////////////////////////////////////////////////////////////
/// Constructor.

REveBoxProjected::REveBoxProjected(const char* n, const char* t) :
   REveShape(n, t),
   fBreakIdx(0)
{
}

////////////////////////////////////////////////////////////////////////////////
/// Destructor.

REveBoxProjected::~REveBoxProjected()
{
}

////////////////////////////////////////////////////////////////////////////////
/// Compute bounding-box, virtual from TAttBBox.

void REveBoxProjected::ComputeBBox()
{
   BBoxInit();
   for (vVector2_i i = fPoints.begin(); i != fPoints.end(); ++i)
   {
      BBoxCheckPoint(i->fX, i->fY, fDepth);
   }
}

////////////////////////////////////////////////////////////////////////////////
/// This is virtual method from base-class REveProjected.

void REveBoxProjected::SetDepthLocal(Float_t d)
{
   SetDepthCommon(d, this, fBBox);
}

////////////////////////////////////////////////////////////////////////////////
/// This is virtual method from base-class REveProjected.

void REveBoxProjected::SetProjection(REveProjectionManager* mng, REveProjectable* model)
{
   REveProjected::SetProjection(mng, model);
   CopyVizParams(dynamic_cast<REveElement*>(model));
}

////////////////////////////////////////////////////////////////////////////////
/// Re-project the box. Projects all points and finds 2D convex-hull.
///
/// The only issue is with making sure that initial conditions for
/// hull-search are reasonable -- that is, there are no overlaps with the
/// first point.

void REveBoxProjected::UpdateProjection()
{
   REveBox *box = dynamic_cast<REveBox*>(fProjectable);

   fDebugPoints.clear();

   // Project points in global CS, remove overlaps.
   vVector2_t pp[2];
   {
      REveProjection *projection = fManager->GetProjection();
      REveTrans      *trans      = box->PtrMainTrans(kFALSE);

      REveVector pbuf;
      for (Int_t i = 0; i < 8; ++i)
      {
         projection->ProjectPointfv(trans, box->GetVertex(i), pbuf, fDepth);
         vVector2_t& ppv = pp[projection->SubSpaceId(pbuf)];

         REveVector2 p(pbuf);
         Bool_t      overlap = kFALSE;
         for (vVector2_i j = ppv.begin(); j != ppv.end(); ++j)
         {
            if (p.SquareDistance(*j) < REveProjection::fgEpsSqr)
            {
               overlap = kTRUE;
               break;
            }
         }
         if (! overlap)
         {
            ppv.push_back(p);
            if (fgDebugCornerPoints)
               fDebugPoints.push_back(p);
         }
      }
   }

   fPoints.clear();
   fBreakIdx = 0;

   if ( ! pp[0].empty())
   {
      FindConvexHull(pp[0], fPoints, this);
   }
   if ( ! pp[1].empty())
   {
      fBreakIdx = fPoints.size();
      FindConvexHull(pp[1], fPoints, this);
   }
}

////////////////////////////////////////////////////////////////////////////////
/// Get state of fgDebugCornerPoints static.

Bool_t REveBoxProjected::GetDebugCornerPoints()
{
   return fgDebugCornerPoints;
}

////////////////////////////////////////////////////////////////////////////////
/// Set state of fgDebugCornerPoints static.
/// When this is true, points will be drawn at the corners of
/// computed convex hull.

void REveBoxProjected::SetDebugCornerPoints(Bool_t d)
{
   fgDebugCornerPoints = d;
}
