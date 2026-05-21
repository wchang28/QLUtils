#pragma once

#include <ql/quantlib.hpp>

namespace QuantLib {
    namespace Utils {
        template<
            typename Interp	// "Internal" Interpolation Traits
        >
        class BothEndsFlatExtrapolateInterpolation : public Extrapolator {
        protected:
            Interpolation interp_;	// the internal interpolation object, which is expected to not allow extrapolation
        protected:
            void checkRange(Real x, bool extrapolate) const {
                QL_REQUIRE(extrapolate || allowsExtrapolation() ||
                    isInRange(x),
                    "interpolation range is ["
                    << xMin() << ", " << xMax()
                    << "]: extrapolation at " << x << " not allowed");
            }
        public:
            template <
                typename I1,
                typename I2
            >
            BothEndsFlatExtrapolateInterpolation(
                const I1& xBegin,
                const I1& xEnd,
                const I2& yBegin
            ) : interp_(Interp{}.interpolate(xBegin, xEnd, yBegin))
            {
                QL_ASSERT(!interp_.allowsExtrapolation(), "extrapolation is not allowed for the internal interpolator");
                update();
            }
            bool empty() const { return interp_->empty(); }
            Real operator()(Real x, bool allowExtrapolation = false) const {
                checkRange(x, allowExtrapolation);
                if (x < xMin()) {
                    return interp_(xMin(), false);
                } else if (x > xMax()) {
                    return interp_(xMax(), false);
                }
                else {
                    return interp_(x, false);
                }
            }
            Real primitive(Real x, bool allowExtrapolation = false) const {
                checkRange(x, allowExtrapolation);
                if (x < xMin()) {
                    auto valueMin = interp_(xMin(), false);
                    return valueMin * (x - xMin());
                } else if (x > xMax()) {
                    auto valueMax = interp_(xMax(), false);
                    auto primitiveMax = interp_.primitive(xMax(), false);
                    return  primitiveMax + valueMax * (x - xMax());
                }
                else {
                    return interp_.primitive(x, false);
                }
            }
            Real derivative(Real x, bool allowExtrapolation = false) const {
                checkRange(x, allowExtrapolation);
                return (isInRange(x) ? interp_.derivative(x, false) : 0.0);
            }
            Real secondDerivative(Real x, bool allowExtrapolation = false) const {
                checkRange(x, allowExtrapolation);
                return (isInRange(x) ? interp_.secondDerivative(x, false) : 0.0);
            }
            Real xMin() const {
                return interp_.xMin();
            }
            Real xMax() const {
                return interp_.xMax();
            }
            bool isInRange(Real x) const {
                return interp_.isInRange(x);
            }
            void update() {
                interp_.update();
            }
        };        
    }
}
