/**
 * @file   larcorealg/CoreUtils/enumerate.h
 * @brief  Definition of `util::enumerate()`.
 * @author Gianluca Petrillo (petrillo@slac.stanford.edu)
 * @date   April 14, 2019
 *
 * This is a header-only library.
 */

#ifndef LARCOREALG_COREUTILS_ENUMERATE_H
#define LARCOREALG_COREUTILS_ENUMERATE_H

// LArSoft libraries
#include "larcorealg/CoreUtils/counter.h"
#include "larcorealg/CoreUtils/zip.h"

// C/C++ libraries
#include <cstddef> // std::size_t
#include <utility> // std::forward()

namespace util {

  // -- BEGIN -- Enumerated iterations -----------------------------------------
  /// @name Enumerated iterations
  /// @{

  /**
   * @brief Range-for loop helper tracking the number of iteration.
   * @tparam Lead index of the parameter driving the start and end of the loop
   * @tparam Iterables type of objects to be iterated together
   * @param iterables all iterable objects to be iterated together
   * @return an object suitable for range-for loop
   *
   * In the range-for loop, at each iteration this object yields a `tuple` of
   * values, each of the type returned by dereferencing `begin(iterable)`.
   * For example:
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
   * constexpr std::size_t N = 4;
   * std::array<int, N> twice;
   * std::vector<double> thrice(N + 1);
   *
   * for (auto&& [i, a, b]: util::enumerate(twice, thrice)) {
   *
   *   a = 2 * i;
   *   b = 3.0 * i;
   *
   * } // for
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   * In this example, `N` iterations will be run because that is the size of
   * the first iterable given to `enumerate`. If a different leading iterable
   * is needed, that has to be specified as an argument. The following loop
   * is completely equivalent to the former one:
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
   * for (auto&& [i, b, a]: util::enumerate<1U>(thrice, twice)) {
   *
   *   a = 2 * i;
   *   b = 3.0 * i;
   *
   * } // for
   * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   * (the index is zero-based, so `1U` refers to the second argument).
   *
   */
  template <std::size_t Lead, typename... Iterables>
  auto enumerate(Iterables&&... iterables)
  {
    return zip<Lead + 1>(infinite_counter(), std::forward<Iterables>(iterables)...);
  }

  /// This version of `enumerate` implicitly uses the first iterable as lead.
  template <typename... Iterables>
  auto enumerate(Iterables&&... iterables)
  {
    return enumerate<0U>(std::forward<Iterables>(iterables)...);
  }

  /// @}
  // --- END -- Enumerated iterations ------------------------------------------

} // namespace util

#endif // LARCOREALG_COREUTILS_ENUMERATE_H