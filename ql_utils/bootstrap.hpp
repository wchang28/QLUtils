#pragma once

#include <ql/quantlib.hpp>
#include <ql_utils/PiecewiseCurveBuilder.hpp>
#include <ql_utils/instrument.hpp>
#include <ql_utils/dateformat.hpp>
#include <ql_utils/types.hpp>
#include <memory>
#include <vector>
#include <iostream>
#include <iomanip>

namespace QLUtils {
    template <typename I = QuantLib::Linear>
    class ZeroCurvesBootstrap : public Verifiable {
    protected:
        std::shared_ptr<PiecewiseCurveBuilder<QuantLib::ZeroYield, I>> curveBuilder_;
    public:
        enum BootstrapMode {
            BothCurvesConcurrently = 0,
            EstimatingCurveOnly = 1
        };

        // input
        std::shared_ptr<std::vector<std::shared_ptr<BootstrapInstrument>>> instruments; // bootstrap instruments
        QuantLib::ext::shared_ptr<QuantLib::YieldTermStructure> discountingTermStructure;   // this can be nullptr

        // output
        QuantLib::ext::shared_ptr<QuantLib::InterpolatedZeroCurve<I>> discountZeroCurve;   // this can be nullptr
        QuantLib::ext::shared_ptr<QuantLib::InterpolatedZeroCurve<I>> estimatingZeroCurve;

        ZeroCurvesBootstrap() {}
        BootstrapMode bootstrapMode() const {
            return (discountingTermStructure != nullptr ? EstimatingCurveOnly : BothCurvesConcurrently);
        }
    protected:
        QuantLib::ext::shared_ptr<QuantLib::YieldTermStructure> verificationDiscountTermStructure() const {
            return (bootstrapMode() == EstimatingCurveOnly ? discountingTermStructure : discountZeroCurve);
        }
        void checkInstruments() const {
            QL_REQUIRE(instruments != nullptr, "instruments is not set");
            QL_REQUIRE(!instruments->empty(), "instruments cannot be empty");
        }
        std::shared_ptr<PiecewiseCurveBuilder<QuantLib::ZeroYield, I>>& curveBuilder() {
            return curveBuilder_;
        }
    public:
        const std::shared_ptr<PiecewiseCurveBuilder<QuantLib::ZeroYield, I>>& curveBuilder() const {
            return curveBuilder_;
        }
        void clearOutputs() {
            curveBuilder() = nullptr;
            discountZeroCurve = nullptr;
            estimatingZeroCurve = nullptr;
        }
        void bootstrap(
            const QuantLib::Date& curveReferenceDate,
            const QuantLib::DayCounter& dayCounter = QuantLib::Actual365Fixed(),
            const I& interp = I()
        ) {
            clearOutputs();
            checkInstruments();
            if (bootstrapMode() == EstimatingCurveOnly) {
                auto expected = discountingTermStructure->referenceDate();
                QL_REQUIRE(curveReferenceDate == expected, "curve ref. date (" << DateFormat<char>::to_yyyymmdd(curveReferenceDate, true)  << ") is not what's expected ("<< expected << ")");
            }
            QuantLib::Handle<QuantLib::YieldTermStructure> discountingCurve(discountingTermStructure);
            curveBuilder().reset(new PiecewiseCurveBuilder<QuantLib::ZeroYield, I>());
            for (auto const& inst : *instruments) { // for each instrument
                if (inst->use()) {
                    this->curveBuilder_->AddHelper(inst->rateHelper(discountingCurve));
                }
            }
            estimatingZeroCurve = curveBuilder()->GetCurve(curveReferenceDate, dayCounter, interp);
            discountZeroCurve = (bootstrapMode() == BothCurvesConcurrently ? estimatingZeroCurve : nullptr);
        }
        void verify(std::ostream& os, std::streamsize precision = 16) const {
            QuantLib::ext::shared_ptr<QuantLib::YieldTermStructure> discountTS = verificationDiscountTermStructure();
            QL_REQUIRE(discountTS != nullptr, "discount term structure cannot be null");
            QL_REQUIRE(estimatingZeroCurve != nullptr, "estimating zero curve cannot be null");
            checkInstruments();
            os << std::fixed;
            os << std::setprecision(precision);
            QuantLib::Handle<QuantLib::YieldTermStructure> discountCurve(discountTS);
            QuantLib::Handle<QuantLib::YieldTermStructure> estimatingCurve(estimatingZeroCurve);
            for (auto const& inst : *instruments) { // for each instrument
                if (inst->use()) {
                    auto const& actual = inst->value();
                    auto valueIsPrice = (inst->valueType() == BootstrapInstrument::Price);
                    auto implied = inst->impliedQuote(estimatingCurve, discountCurve);
                    auto startDate = inst->startDate();
                    auto maturityDate = inst->maturityDate();
                    os << inst->tenor();
                    os << "," << "ticker=" << inst->ticker();
                    os << "," << "start=" << DateFormat<char>::to_yyyymmdd(startDate, true);
                    os << "," << "maturity=" << DateFormat<char>::to_yyyymmdd(maturityDate, true);
                    os << "," << "actual=" << (valueIsPrice ? actual : actual * 100.0);
                    os << "," << "implied=" << (valueIsPrice ? implied : implied * 100.0);
                    os << std::endl;
                }
            }
        }
    };
}