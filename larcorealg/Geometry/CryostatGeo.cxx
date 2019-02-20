////////////////////////////////////////////////////////////////////////
/// \file larcorealg/Geometry/CryostatGeo.cxx
///
/// \author  brebel@fnal.gov
////////////////////////////////////////////////////////////////////////

// class header
#include "larcorealg/Geometry/CryostatGeo.h"

// LArSoft includes
#include "larcorealg/CoreUtils/SortByPointers.h"

// Framework includes
#include "messagefacility/MessageLogger/MessageLogger.h"
#include "cetlib_except/exception.h"

// ROOT includes
#include "TGeoNode.h"
#include "TGeoBBox.h"
#include "TClass.h"

// C++ standard libraries
#include <sstream> // std::ostringstream
#include <limits> // std::numeric_limits<>
#include <algorithm> // std::sort()
#include <utility> // std::move()


namespace geo{

  //......................................................................
  CryostatGeo::CryostatGeo(
    TGeoNode const& node, geo::TransformationMatrix&& trans,
    TPCList_t&& TPCs,
    OpDetList_t&& OpDets
    )
    : fTrans(std::move(trans))
    , fTPCs(std::move(TPCs))
    , fOpDets(std::move(OpDets))
    , fVolume(nullptr)
  {
    
    // all planes are going to be contained in the volume named volCryostat
    // now get the total volume of the Cryostat
    fVolume = node.GetVolume();
    if(!fVolume)
      throw cet::exception("CryostatGeo") << "cannot find cryostat outline volume\n";
    
    MF_LOG_DEBUG("Geometry") << "cryostat  volume is " << fVolume->GetName();
    
    // set the bounding box
    InitCryoBoundaries();
    
    // Set OpDetName;
    fOpDetGeoName = "volOpDetSensitive";
    
    // sort the OpDets according to xyz position
    // 600 intended to separate dune10kt geometry from others when sorting
    ///\todo: remove the hard-coded 600 in favor of selecting sorting the same way as in ChannelMapAlgs
    /// (LArSoft issue #16812)
  /*  auto sorter = (fOpDets.size() != 600)? opdet_sort: DUNE_opdet_sort;
    util::SortByPointers(fOpDets,
      [&sorter](auto& coll){ std::sort(coll.begin(), coll.end(), sorter); }
      );
    */
  }


  //......................................................................
  // sort the TPCGeo objects, and the PlaneGeo objects inside
  void CryostatGeo::SortSubVolumes(geo::GeoObjectSorter const& sorter)
  {
    //
    // TPCs
    //
    util::SortByPointers
      (fTPCs, [&sorter](auto& coll){ sorter.SortTPCs(coll); });
    
    for (geo::TPCGeo& TPC: fTPCs) { 
      TPC.SortSubVolumes(sorter);
    } // for TPCs
    
    //
    // optical detectors
    //
    util::SortByPointers
      (fOpDets, [&sorter](auto& coll){ sorter.SortOpDets(coll); });
    
    
  } // CryostatGeo::SortSubVolumes()


  //......................................................................
  void CryostatGeo::UpdateAfterSorting(geo::CryostatID cryoid) {
    
    // update the cryostat ID
    fID = cryoid;
    
    // trigger all the TPCs to update as well
    for (unsigned int tpc = 0; tpc < NTPC(); ++tpc)
      fTPCs[tpc].UpdateAfterSorting(geo::TPCID(fID, tpc));
    
  } // CryostatGeo::UpdateAfterSorting()
  
  
  //......................................................................
  const TPCGeo& CryostatGeo::TPC(unsigned int itpc) const
  {
    TPCGeo const* pTPC = TPCPtr(itpc);
    if(!pTPC){
      throw cet::exception("TPCOutOfRange") << "Request for non-existant TPC "
                                            << itpc << "\n";
    }

    return *pTPC;
  }



  //......................................................................
  const OpDetGeo& CryostatGeo::OpDet(unsigned int iopdet) const
  {
    if(iopdet >= fOpDets.size()){
      throw cet::exception("OpDetOutOfRange") << "Request for non-existant OpDet " 
					      << iopdet;
    }

    return fOpDets[iopdet];
  }
  
  
  
  //......................................................................
  // wiggle is 1+a small number to allow for rounding errors on the 
  // passed in world loc relative to the boundaries.
  geo::TPCID::TPCID_t CryostatGeo::FindTPCAtPosition
    (double const worldLoc[3], double wiggle) const
  {
    geo::TPCID tpcid
      = PositionToTPCID(geo::vect::makePointFromCoords(worldLoc), wiggle);
    return tpcid? tpcid.TPC: geo::TPCID::InvalidID;
  } // CryostatGeo::FindTPCAtPosition()

  //......................................................................
  // wiggle is 1+a small number to allow for rounding errors on the 
  // passed in world loc relative to the boundaries.
  geo::TPCID CryostatGeo::PositionToTPCID
    (geo::Point_t const& point, double wiggle) const
  { 
    geo::TPCGeo const* tpc = PositionToTPCptr(point, wiggle);
    return tpc? tpc->ID(): geo::TPCID{};
  }

  //......................................................................
  // wiggle is 1+a small number to allow for rounding errors on the 
  // passed in world loc relative to the boundaries.
  TPCGeo const& CryostatGeo::PositionToTPC
    (geo::Point_t const& point, double wiggle) const
  {
    geo::TPCGeo const* tpc = PositionToTPCptr(point, wiggle);
    if (!tpc) {
      throw cet::exception("CryostatGeo")
        << "Can't find any TPC for position " << point << " within " << ID()
        << "\n";
    }
    return *tpc;
  }
  
  //......................................................................
  geo::TPCGeo const* CryostatGeo::PositionToTPCptr
    (geo::Point_t const& point, double wiggle) const
  {
    for (auto const& tpc: TPCs())
      if (tpc.ContainsPosition(point, wiggle)) return &tpc;
    return nullptr;
  } // CryostatGeo::PositionToTPCptr()
  
  
  //......................................................................
  unsigned int CryostatGeo::MaxPlanes() const {
    unsigned int maxPlanes = 0;
    for (geo::TPCGeo const& TPC: fTPCs) {
      unsigned int maxPlanesInTPC = TPC.Nplanes();
      if (maxPlanesInTPC > maxPlanes) maxPlanes = maxPlanesInTPC;
    } // for
    return maxPlanes;
  } // CryostatGeo::MaxPlanes()
  
  //......................................................................
  unsigned int CryostatGeo::MaxWires() const {
    unsigned int maxWires = 0;
    for (geo::TPCGeo const& TPC: fTPCs) {
      unsigned int maxWiresInTPC = TPC.MaxWires();
      if (maxWiresInTPC > maxWires) maxWires = maxWiresInTPC;
    } // for
    return maxWires;
  } // CryostatGeo::MaxWires()
  
  //......................................................................
  double CryostatGeo::HalfWidth()  const 
  {
    return static_cast<TGeoBBox const*>(fVolume->GetShape())->GetDX();
  }

  //......................................................................
  double CryostatGeo::HalfHeight() const 
  {
    return static_cast<TGeoBBox const*>(fVolume->GetShape())->GetDY();
  }

  //......................................................................
  double CryostatGeo::HalfLength() const
  { 
    return static_cast<TGeoBBox const*>(fVolume->GetShape())->GetDZ();
  }

  //......................................................................
  void CryostatGeo::Boundaries(double* boundaries) const {
    boundaries[0] = MinX();
    boundaries[1] = MaxX();
    boundaries[2] = MinY();
    boundaries[3] = MaxY();
    boundaries[4] = MinZ();
    boundaries[5] = MaxZ();
  } // CryostatGeo::CryostatBoundaries(double*)
  
  
  //......................................................................
  std::string CryostatGeo::CryostatInfo
    (std::string indent /* = "" */, unsigned int verbosity /* = 1 */) const
  {
    std::ostringstream sstr;
    PrintCryostatInfo(sstr, indent, verbosity);
    return sstr.str();
  } // CryostatGeo::CryostatInfo()

  //......................................................................
  // Find the nearest opdet to point in this cryostat

  geo::OpDetGeo const* CryostatGeo::GetClosestOpDetPtr
    (geo::Point_t const& point) const
  {
    unsigned int iOpDet = GetClosestOpDet(point);
    return
      (iOpDet == std::numeric_limits<double>::max())? nullptr: &OpDet(iOpDet);
  }
  
  //......................................................................
  unsigned int CryostatGeo::GetClosestOpDet(geo::Point_t const& point) const {
    unsigned int ClosestDet = std::numeric_limits<unsigned int>::max();
    double ClosestDist = std::numeric_limits<double>::max();
    
    for(unsigned int o = 0U; o < NOpDet(); ++o) {
      double const ThisDist = OpDet(o).DistanceToPoint(point); 
      if(ThisDist < ClosestDist) {
        ClosestDist = ThisDist;
        ClosestDet  = o;
      }
    } // for
    return ClosestDet;
  } // CryostatGeo::GetClosestOpDet(geo::Point_t)
  
  //......................................................................
  unsigned int CryostatGeo::GetClosestOpDet(double const* point) const
    { return GetClosestOpDet(geo::vect::makePointFromCoords(point)); }
  
  //......................................................................
  void CryostatGeo::InitCryoBoundaries() {
    
    // check that this is indeed a box
    if (!dynamic_cast<TGeoBBox*>(Volume()->GetShape())) {
      // at initialisation time we don't know yet our real ID
      throw cet::exception("CryostatGeo") << "Cryostat is not a box! (it is a "
        << Volume()->GetShape()->IsA()->GetName() << ")\n";
    }
    
    // get the half width, height, etc of the cryostat
    const double halflength = HalfLength();
    const double halfwidth  = HalfWidth();
    const double halfheight = HalfHeight();
    
    SetBoundaries(
      toWorldCoords(LocalPoint_t{ -halfwidth, -halfheight, -halflength }),
      toWorldCoords(LocalPoint_t{ +halfwidth, +halfheight, +halflength })
      );
    
  } // CryostatGeo::InitCryoBoundaries()
  
  
  //......................................................................

} // namespace geo
////////////////////////////////////////////////////////////////////////
