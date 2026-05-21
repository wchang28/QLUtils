#pragma once

#include <ql/quantlib.hpp>

namespace QuantLib {
    namespace Utils {
        template <
            typename Interpolator,
            typename X_Unit,
            typename Y_Unit
        >
        struct StripIntervalEndValueCalculator {
            // returns the value at the end of the strip interval
            Y_Unit operator() (
                Y_Unit start_y, // value at the start of the strip interval
                Real strip_area,    // area of the strip interval
                X_Unit dx   // length/width of the strip interval
            ) const {
                QL_FAIL("not implemented for the interpolator");
            }
        };

        // specialization for BackwardFlat interpolation
        template<
            typename X_Unit,
            typename Y_Unit
        >
        struct StripIntervalEndValueCalculator<BackwardFlat, X_Unit, Y_Unit> {
            Y_Unit operator() (
                Y_Unit,
                Real strip_area,
                X_Unit dx
            ) const {
                Y_Unit end_y = strip_area / dx;	// for BackwardFlat interpolation, the area is a rectangle (strip_area = end_y * dx)
                return end_y;
            }
        };

        // specialization for Linear interpolation
        template<
            typename X_Unit,
            typename Y_Unit
        >
        struct StripIntervalEndValueCalculator<Linear, X_Unit, Y_Unit> {
            Y_Unit operator() (
                Y_Unit start_y,
                Real strip_area,
                X_Unit dx
            ) const {
                Y_Unit end_y = strip_area * 2.0 / dx - start_y;	// for Linear interpolation, the area is a trapezoid (strip_area = (start_y + end_y) * dx / 2)
                return end_y;
            }
        };
    }
}
