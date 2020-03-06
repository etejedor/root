/*************************************************************************
 * Copyright (C) 1995-2017, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#ifndef ROOT7_RPalette
#define ROOT7_RPalette

#include <ROOT/RStringView.hxx>
#include <ROOT/RColor.hxx>

#include <utility>
#include <vector>

namespace ROOT {
namespace Experimental {

/** \class RPalette
\ingroup GpadROOT7
\brief A set of colors. `RColor`s can be conveniently generated from this.

  A palette associates a color with an ordinal number: for a normalized palette,
  this number ranges from 0..1. For user-valued palettes, the palette yields a color for
  user-coordinates (for instance histogram content), in an arbitrary range.

  A palette can be a smooth gradients by interpolation of support points, or a set of
  discrete colors.

\author Axel Naumann <axel@cern.ch>
\date 2017-09-26
\warning This is part of the ROOT 7 prototype! It will change without notice. It might trigger earthquakes. Feedback is welcome!

*/

class RPalette {
public:
   /// An ordinal value and its associated color.
   struct OrdinalAndColor {
      double fOrdinal{0.}; ///< The value associated with the color.
      RColor fColor;       ///< The color associated with the value.
      /** CAUTION!!!
       * All constructors are required that std::sort works correctly
       * To be investigated! */
      OrdinalAndColor() = default;
      OrdinalAndColor(double ordinal, const RColor &color) { fOrdinal = ordinal; fColor = color; }
      OrdinalAndColor(const OrdinalAndColor &src)
      {
         fOrdinal = src.fOrdinal;
         fColor = src.fColor;
      }
      OrdinalAndColor &operator=(const OrdinalAndColor &src)
      {
         fOrdinal = src.fOrdinal;
         fColor = src.fColor;
         return *this;
      }
   };

   /// Compare two `OrdinalAndColor`s, for sorting.
   friend bool operator<(const OrdinalAndColor &lhs, const OrdinalAndColor &rhs)
   {
      return lhs.fOrdinal < rhs.fOrdinal;
   }

   /// Compare an `OrdinalAndColor` and an ordinal value.
   friend bool operator<(const OrdinalAndColor &lhs, double rhs) { return lhs.fOrdinal < rhs; }

private:
   /// Palette colors: the color points and their ordinal value.
   std::vector<OrdinalAndColor> fColors;

   /// Whether to interpolate between the colors (in contrast to picking one of fColors).
   bool fInterpolate = true;

   /// Whether the palette's ordinal numbers are normalized.
   bool fNormalized = true;

   RPalette(bool interpolate, bool knownNormalized, const std::vector<OrdinalAndColor> &points);
   RPalette(bool interpolate, const std::vector<RColor> &points);

public:
   /// Tag type used to signal that the palette's colors should not be interpolated.
   struct Discrete_t {
   };

   /// Tag value used to signal that the palette's colors should not be interpolated. Can be passed to the
   /// constructor: `RPalette palette(RPalette::kDiscrete, {{-100., RColor::kWhite}, {100., RColor::kRed}})`
   static constexpr const Discrete_t kDiscrete{};

   RPalette() = default;

   /// Construct a RPalette from a vector of (ordinal|color) pairs as interpolation points.
   /// Palette colors will be these points for the ordinal, and interpolated in between the
   /// ordinal points. The points will be sorted.
   /// The palette is normalized if the lowest ordinal is 0. and the highest ordinal is 1.;
   /// otherwise, the palette is a user-valued palette.
   RPalette(const std::vector<OrdinalAndColor> &interpPoints): RPalette(true, false, interpPoints) {}

   /// Construct a RPalette from a vector of (ordinal|color) pairs. For a given value, the palette returns
   /// the color with an ordinal that is closest to the value. The points will be sorted.
   /// The palette is normalized if the lowest ordinal is 0. and the highest ordinal is 1.;
   /// otherwise, the palette is a user-valued palette.
   RPalette(Discrete_t, const std::vector<OrdinalAndColor> &points): RPalette(false, false, points) {}

   /// Construct a normalized RPalette from a vector of colors as interpolation points. The ordinal associated
   /// with each color is equidistant from 0..1, i.e. for three colors it will be 0., 0.5 and 1, respectively.
   /// Palette colors will be these points for the ordinal associated with the color,
   /// and interpolated in between the ordinal points.
   RPalette(const std::vector<RColor> &interpPoints): RPalette(true, interpPoints) {}

   /// Construct a normalized RPalette from a vector of colors. The ordinal associated
   /// with each color is equidistant from 0..1, i.e. for three colors it will be 0., 0.5 and 1, respectively.
   /// For a given value, the palette returns the color with an ordinal that is closest to the value.
   RPalette(Discrete_t, const std::vector<RColor> &points): RPalette(false, points) {}

   /// Whether the palette is normalized, i.e. covers colors in the ordinal range 0..1.
   bool IsNormalized() const { return fNormalized; }

   /// Whether the palette is discrete, i.e. does no interpolation between colors.
   bool IsDiscrete() const { return !fInterpolate; }

   /// Whether the palette is a smooth gradient generated by interpolating between the color points.
   bool IsGradient() const { return fInterpolate; }

   /// Get the color associated with the ordinal value. The value is expected to be 0..1 for a normalized
   /// palette.
   RColor GetColor(double ordinal);

   ///\{
   ///\name Global Palettes

   /// Register a palette in the set of global palettes, making it available to `GetPalette()`.
   /// This function is not thread safe; any concurrent call to global Palette manipulation must be synchronized!
   static void RegisterPalette(std::string_view name, const RPalette &palette);

   /// Get a global palette by name. Returns an empty palette if no palette with that name is known.
   /// This function is not thread safe; any concurrent call to global Palette manipulation must be synchronized!
   static const RPalette &GetPalette(std::string_view name = "");

   ///\}
};

} // namespace Experimental
} // namespace ROOT

#endif
