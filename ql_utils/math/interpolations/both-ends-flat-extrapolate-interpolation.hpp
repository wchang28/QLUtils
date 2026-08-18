#pragma once

#include <ql/quantlib.hpp>

namespace QuantLib {
    namespace Utils {
        // define the both ends flat extrapolation version of the Interpolator
        template <
            typename Interpolator    // can be BackwardFlat or Linear
        >
        struct BothEndsFlatExtrapolate {
            typedef void FlatExtrapInterpolator;
        };
        // specialization for "BackwardFlat"
        template <>
        struct BothEndsFlatExtrapolate<BackwardFlat> {
            typedef BackwardFlat FlatExtrapInterpolator;    // flat extrapolation version of "BackwardFlat" is itself because "BackwardFlat" extrapolates flatly by default
        };
        // specialization for "Linear"
        template <>
        struct BothEndsFlatExtrapolate<Linear> {
            typedef LinearFlat FlatExtrapInterpolator;  // flat extrapolation version of "Linear" is "LinearFlat"
        };
    }
}
