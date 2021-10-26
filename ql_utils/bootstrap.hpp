#pragma once

#include <ql/quantlib.hpp>
#include <ql_utils/PiecewiseCurveBuilder.hpp>
#include <ql_utils/instrument.hpp>
#include <ql_utils/dateformat.hpp>
#include <memory>
#include <vector>
#include <iostream>
#include <iomanip>
#include <cmath>

namespace QLUtils {
    class Bootstrapper {
    public:
        typedef std::shared_ptr<BootstrapInstrument> pInstrument;
        typedef std::vector<pInstrument> Instruments;
        typedef std::shared_ptr<Instruments> pInstruments;
    protected:
        template <typename ImpliedValueCalculator, typename ActualVsImpliedComparison>
        static QuantLib::Rate verifyImpl(
            const pInstruments& instruments,
            const ImpliedValueCalculator& impliedValueCalculator,
            std::ostream& os,
            std::streamsize precision,
            const ActualVsImpliedComparison& compare
        ) {
            os << std::fixed;
            os << std::setprecision(precision);
            QuantLib::Rate err = 0.0;
            for (auto const& inst : *instruments) { // for each instrument
                if (inst->use()) {
                    auto const& actual = inst->value();
                    auto implied = impliedValueCalculator(inst);
                    auto diff = compare(os, inst, actual, implied);
                    err += std::pow(diff, 2.0);
                }
            }
            return std::sqrt(err);
        }
    };
	
    template <typename I = QuantLib::Linear>
    class ZeroCurvesBootstrap : public Bootstrapper {
    protected:
        std::shared_ptr<PiecewiseCurveBuilder<QuantLib::ZeroYield, I>> curveBuilder_;
    public:
        enum BootstrapMode {
            BothCurvesConcurrently = 0,
            EstimatingCurveOnly = 1
        };

        // input
        pInstruments instruments; // bootstrap instruments
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

        struct DefaultActualVsImpliedComparison {
            QuantLib::Rate operator() (
                std::ostream& os,
                const std::shared_ptr<BootstrapInstrument>& inst,
                const QuantLib::Real& actual,
                const QuantLib::Real& implied
                ) const {
                auto multiplierValue = (inst->valueType() == BootstrapInstrument::Rate ? 100.0 : 1.0);
                auto multiplierDiffBp = (inst->valueType() == BootstrapInstrument::Rate ? 10000.0 : 100.0);
                auto diff = implied - actual;
                os << inst->tenor();
                os << "," << inst->ticker();
                os << "," << "actual=" << actual * multiplierValue;
                os << "," << "implied=" << implied * multiplierValue;
                os << "," << "diff=" << diff * multiplierDiffBp << " bp";
                os << std::endl;
                return (inst->valueType() == BootstrapInstrument::Rate ? diff : diff/100.0);
            }
        };

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
        template<typename ActualVsImpliedComparison = DefaultActualVsImpliedComparison>
        QuantLib::Rate verify(
            std::ostream& os,
            std::streamsize precision = 16,
            const ActualVsImpliedComparison& compare = ActualVsImpliedComparison()
        ) const {
            QuantLib::ext::shared_ptr<QuantLib::YieldTermStructure> discountTS = verificationDiscountTermStructure();
            QL_REQUIRE(discountTS != nullptr, "discount term structure cannot be null");
            QL_REQUIRE(estimatingZeroCurve != nullptr, "estimating zero curve cannot be null");
            checkInstruments();
            QuantLib::Handle<QuantLib::YieldTermStructure> discountCurve(discountTS);
            QuantLib::Handle<QuantLib::YieldTermStructure> estimatingCurve(estimatingZeroCurve);
            return verifyImpl(
                instruments,
                [&discountCurve, &estimatingCurve](const pInstrument& inst) -> QuantLib::Real {
                    return inst->impliedQuote(estimatingCurve, discountCurve);
                },
                os,
                precision,
                compare
            );
        }
    };
}