#pragma once

#include <ql/quantlib.hpp>
#include <ql_utils/utilities/iso-date-conv.hpp>
#include <ql_utils/utilities/ramp.hpp>
#include <ios>
#include <iostream>
#include <memory>

namespace QuantLib {
    namespace Utils {
        // base class for all yield curve shockers
        class YieldTermStructureShocker {
        public:
            typedef ext::shared_ptr<YieldTermStructure> YieldTermStructurePtr;
            typedef Ramp<Frequency::Monthly> monthly_ramp;
        public:
            // input
            YieldTermStructurePtr yieldTermStructure;    // input yield term structure to be shocked
        public:
            // reference date of the output shocked curve
            // !!! must be matching with the input curve !!!
            Date curveRefDate() const {
                QL_ASSERT(yieldTermStructure != nullptr, "input yield term structure not set");
                return yieldTermStructure->referenceDate();
            }
        public:
            // pure virtual interface
            ///////////////////////////////////////////////////////////////////////////////
            virtual YieldTermStructurePtr outputShockedTermStructure() const = 0;
            virtual void monthlyRampShock(
                const monthly_ramp& monthlyRamp,
                const DayCounter& curveDayCounter = Actual365Fixed()
            ) = 0;
            virtual Rate verifyShock(
                std::ostream& os,
                std::streamsize precision = 16
            ) const = 0;
            ///////////////////////////////////////////////////////////////////////////////
        protected:
            // protected overridable interface
            virtual void verifyInputs() const {
                QL_REQUIRE(yieldTermStructure != nullptr, "input yield term structure not set");
            }
            virtual void resetOutputs() {}
            virtual void verifyOutputs() const {
                auto shockedTS = outputShockedTermStructure();
                QL_ASSERT(shockedTS != nullptr, "output shocked yield term structure is null");
                auto curveRefDate = this->curveRefDate();
                QL_ASSERT(shockedTS->referenceDate() == curveRefDate, "shocked curve's reference date (" << ISODateConv::to_str(shockedTS->referenceDate()) << ") is not what's expected (" << ISODateConv::to_str(curveRefDate) << ")");
            }
        };
        typedef std::shared_ptr<YieldTermStructureShocker> YieldTermStructureShockerPtr;

        // base class for all yield curve shockers of the output curve type
        template <
            typename OUTPUT_CURVE_TYPE
        >
        class YieldCurveShocker: public YieldTermStructureShocker {
        public:
            typedef OUTPUT_CURVE_TYPE OutputCurveType;
            typedef ext::shared_ptr<OutputCurveType> OutputCurvePtr;
            typedef typename YieldTermStructureShocker::YieldTermStructurePtr YieldTermStructurePtr;
        public:
            // output
            OutputCurvePtr shockedCurve;    // output shocked curve
        protected:
            void resetOutputs() override {
                YieldTermStructureShocker::resetOutputs();
                shockedCurve = nullptr;
            }
        public:
            YieldTermStructurePtr outputShockedTermStructure() const override {
                return shockedCurve;
            }
        };
    }
}
