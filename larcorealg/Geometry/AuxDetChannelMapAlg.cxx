////////////////////////////////////////////////////////////////////////
/// \file  ChannelMapAlg.cxx
/// \brief Interface to algorithm class for a specific detector channel mapping
///
/// \author  brebel@fnal.gov
////////////////////////////////////////////////////////////////////////

#include "cetlib_except/exception.h"

#include "larcorealg/Geometry/AuxDetChannelMapAlg.h"
#include "larcorealg/Geometry/AuxDetGeo.h"
#include "larcorealg/Geometry/AuxDetSensitiveGeo.h"

#include <limits.h>

namespace geo {

  //----------------------------------------------------------------------------
  size_t AuxDetChannelMapAlg::NearestAuxDet(const double* point,
                                            std::vector<geo::AuxDetGeo> const& auxDets,
                                            double tolerance) const
  {
    double HalfCenterWidth = 0.;
    double localPoint[3] = {0.};

    for (size_t a = 0; a < auxDets.size(); ++a) {

      auxDets[a].WorldToLocal(point, localPoint);

      HalfCenterWidth = 0.5 * (auxDets[a].HalfWidth1() + auxDets[a].HalfWidth2());

      if (localPoint[2] >= (-auxDets[a].Length() / 2 - tolerance) &&
          localPoint[2] <= (auxDets[a].Length() / 2 + tolerance) &&
          localPoint[1] >= (-auxDets[a].HalfHeight() - tolerance) &&
          localPoint[1] <= (auxDets[a].HalfHeight() + tolerance) &&
          // if AuxDet a is a box, then HalfSmallWidth = HalfWidth
          localPoint[0] >= (-HalfCenterWidth +
                            localPoint[2] * (HalfCenterWidth - auxDets[a].HalfWidth2()) /
                              (0.5 * auxDets[a].Length()) -
                            tolerance) &&
          localPoint[0] <= (HalfCenterWidth -
                            localPoint[2] * (HalfCenterWidth - auxDets[a].HalfWidth2()) /
                              (0.5 * auxDets[a].Length()) +
                            tolerance))
        return a;

    } // for loop over AudDet a

    // throw an exception because we couldn't find the sensitive volume
    throw cet::exception("AuxDetChannelMapAlg") << "Can't find AuxDet for position (" << point[0]
                                                << "," << point[1] << "," << point[2] << ")\n";

    return UINT_MAX;
  }

  //----------------------------------------------------------------------------
  size_t AuxDetChannelMapAlg::NearestSensitiveAuxDet(const double* point,
                                                     std::vector<geo::AuxDetGeo> const& auxDets,
                                                     size_t& ad,
                                                     double tolerance) const
  {
    double HalfCenterWidth = 0.;
    double localPoint[3] = {0.};

    ad = this->NearestAuxDet(point, auxDets, tolerance);

    geo::AuxDetGeo const& adg = auxDets[ad];

    for (size_t a = 0; a < adg.NSensitiveVolume(); ++a) {

      geo::AuxDetSensitiveGeo const& adsg = adg.SensitiveVolume(a);
      adsg.WorldToLocal(point, localPoint);

      HalfCenterWidth = 0.5 * (adsg.HalfWidth1() + adsg.HalfWidth2());

      if (localPoint[2] >= -(adsg.Length() / 2 + tolerance) &&
          localPoint[2] <= adsg.Length() / 2 + tolerance &&
          localPoint[1] >= -(adsg.HalfHeight() + tolerance) &&
          localPoint[1] <= adsg.HalfHeight() + tolerance &&
          // if AuxDet a is a box, then HalfSmallWidth = HalfWidth
          localPoint[0] >=
            -HalfCenterWidth +
              localPoint[2] * (HalfCenterWidth - adsg.HalfWidth2()) / (0.5 * adsg.Length()) -
              tolerance &&
          localPoint[0] <=
            HalfCenterWidth -
              localPoint[2] * (HalfCenterWidth - adsg.HalfWidth2()) / (0.5 * adsg.Length()) +
              tolerance)
        return a;
    } // for loop over AuxDetSensitive a

    // throw an exception because we couldn't find the sensitive volume
    throw cet::exception("Geometry") << "Can't find AuxDetSensitive for position (" << point[0]
                                     << "," << point[1] << "," << point[2] << ")\n";

    return UINT_MAX;
  }

  //----------------------------------------------------------------------------
  size_t AuxDetChannelMapAlg::ChannelToAuxDet(std::vector<geo::AuxDetGeo> const& /* auxDets */,
                                              std::string const& detName,
                                              uint32_t const& /*channel*/) const
  {
    // loop over the map of AuxDet names to Geo object numbers to determine which auxdet
    // we have.  If no name in the map matches the provided string, throw an exception;
    // the list of AuxDetGeo passed as argument is ignored!
    // Note that fADGeoToName must have been updated by a derived class.
    for (auto itr : fADGeoToName)
      if (itr.second.compare(detName) == 0) return itr.first;

    throw cet::exception("Geometry") << "No AuxDetGeo matching name: " << detName;

    return UINT_MAX;
  }

  //----------------------------------------------------------------------------
  // the first member of the pair is the index in the auxDets vector for the AuxDetGeo,
  // the second member is the index in the vector of AuxDetSensitiveGeos for that AuxDetGeo
  std::pair<size_t, size_t> AuxDetChannelMapAlg::ChannelToSensitiveAuxDet(
    std::vector<geo::AuxDetGeo> const& auxDets,
    std::string const& detName,
    uint32_t const& channel) const
  {
    size_t adGeoIdx = this->ChannelToAuxDet(auxDets, detName, channel);

    // look for the index of the sensitive volume for the given channel
    if (fADGeoToChannelAndSV.count(adGeoIdx) > 0) {

      auto itr = fADGeoToChannelAndSV.find(adGeoIdx);

      // get the vector of channels to AuxDetSensitiveGeo index
      if (channel < itr->second.size())
        return std::make_pair(adGeoIdx, itr->second[channel].second);

      throw cet::exception("Geometry")
        << "Given AuxDetSensitive channel, " << channel
        << ", cannot be found in vector associated to AuxDetGeo index: " << adGeoIdx
        << ". Vector has size " << itr->second.size();
    }

    throw cet::exception("Geometry") << "Given AuxDetGeo with index " << adGeoIdx
                                     << " does not correspond to any vector of sensitive volumes";

    return std::make_pair(adGeoIdx, UINT_MAX);
  }

}
