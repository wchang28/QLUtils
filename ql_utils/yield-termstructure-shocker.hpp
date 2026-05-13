#pragma once

#include <ql/quantlib.hpp>
#include <ql_utils/utilities/iso-date-conv.hpp>
#include <ql_utils/utilities/ramp.hpp>
#include <ios>
#include <iostream>

namespace QuantLib {
    namespace Utils {
        class YieldTermStructureShocker {
        public:
            typedef ext::shared_ptr<YieldTermStructure> YieldTermStructurePtr;
            typedef Ramp<Frequency::Monthly> monthly_ramp;
        public:
            // input
            YieldTermStructurePtr yieldTermStructure;    // input yield term structure to be shocked
        protected:
            virtual void verifyInputs() const {
                QL_REQUIRE(yieldTermStructure != nullptr, "input yield term structure not set");
            }
        public:
            // reference date of the output shocked curve
            // !!! must be matching with the input curve !!!
            Date curveRefDate() const {
                QL_ASSERT(yieldTermStructure != nullptr, "input yield term structure not set");
                return yieldTermStructure->referenceDate();
            }
            virtual void monthlyRampShock(
                const monthly_ramp& monthlyRamp,
                const DayCounter& curveDayCounter = Actual365Fixed()
            ) = 0;
            virtual Rate verifyShock(
                std::ostream& os,
                std::streamsize precision = 16
            ) const = 0;
        };
        template <
            typename OUTPUT_CURVE_TYPE
        >
        class YieldCurveShocker: public YieldTermStructureShocker {
        public:
            typedef OUTPUT_CURVE_TYPE OutputCurveType;
            typedef ext::shared_ptr<OutputCurveType> OutputCurvePtr;
        public:
            // output
            OutputCurvePtr shockedCurve;    // output shocked curve
        protected:
            virtual void resetOutputs() {
                shockedCurve = nullptr;
            }
            virtual void verifyOutputs() const {
                QL_ASSERT(shockedCurve != nullptr, "output shocked curve is null");
                auto curveRefDate = this->curveRefDate();
                QL_ASSERT(shockedCurve->referenceDate() == curveRefDate, "shocked curve's reference date (" << ISODateConv::to_str(shockedCurve->referenceDate()) << ") is not what's expected (" << ISODateConv::to_str(curveRefDate) << ")");
            }
        };
    }
}
