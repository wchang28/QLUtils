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
        template <
            typename ImpliedValueCalculator,
            typename ActualVsImpliedComparison
        >
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
            for (const auto& inst : *instruments) { // for each instrument
                if (inst->use()) {
                    const auto& actual = inst->value();
                    auto implied = impliedValueCalculator(inst);
                    auto diff = compare(os, inst, actual, implied);
                    err += std::pow(diff, 2.0);
                }
            }
            return std::sqrt(err);
        }
    };

    template <
        typename Traits = QuantLib::ZeroYield,   // QuantLib::ZeroYield, QuantLib::Discount, QuantLib::ForwardRate, or QuantLib::SimpleZeroYield
        typename Interpolator = QuantLib::Linear
    >
    class YieldCurvesBootstrap : public Bootstrapper {
    public:
        typedef PiecewiseCurveBuilder<Traits, Interpolator> PiecewiseCurveBuilderType;
        typedef QuantLib::PiecewiseYieldCurve<Traits, Interpolator> PiecewiseCurveType;
        typedef typename Traits::template curve<Interpolator>::type BaseCurveType;	// InterpolatedZeroCurve<Interpolator>, InterpolatedDiscountCurve<Interpolator>, InterpolatedForwardCurve<Interpolator>, or InterpolatedSimpleZeroCurve<Interpolator>
    protected:
        std::shared_ptr<PiecewiseCurveBuilderType> curveBuilder_;
    public:
        enum BootstrapMode {
            BothCurvesConcurrently = 0,
            EstimatingCurveOnly = 1
        };

        // input
        pInstruments instruments; // bootstrap instruments
        QuantLib::ext::shared_ptr<QuantLib::YieldTermStructure> discountingTermStructure;   // this can be nullptr (mode==BothCurvesConcurrently)

        // output
        QuantLib::ext::shared_ptr<BaseCurveType> discountCurve;   // this can be nullptr (mode==EstimatingCurveOnly)
        QuantLib::ext::shared_ptr<BaseCurveType> estimatingCurve;

        YieldCurvesBootstrap() {}
        BootstrapMode bootstrapMode() const {
            return (discountingTermStructure != nullptr ? EstimatingCurveOnly : BothCurvesConcurrently);
        }
    protected:
        QuantLib::ext::shared_ptr<QuantLib::YieldTermStructure> verificationDiscountTermStructure() const {
            return (bootstrapMode() == EstimatingCurveOnly ? discountingTermStructure : discountCurve);
        }
        void checkInstruments() const {
            QL_REQUIRE(instruments != nullptr, "instruments is not set");
            QL_REQUIRE(!instruments->empty(), "instruments cannot be empty");
        }
        std::shared_ptr<PiecewiseCurveBuilderType>& curveBuilder() {
            return curveBuilder_;
        }
    public:
        const std::shared_ptr<PiecewiseCurveBuilderType>& curveBuilder() const {
            return curveBuilder_;
        }
        void clearOutputs() {
            curveBuilder() = nullptr;
            discountCurve = nullptr;
            estimatingCurve = nullptr;
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
            const Interpolator& interp = Interpolator()
        ) {
            clearOutputs();
            checkInstruments();
            if (bootstrapMode() == EstimatingCurveOnly) {
                auto expected = discountingTermStructure->referenceDate();
                QL_REQUIRE(curveReferenceDate == expected, "curve ref. date (" << DateFormat<char>::to_yyyymmdd(curveReferenceDate, true)  << ") is not what's expected ("<< expected << ")");
            }
            QuantLib::Handle<QuantLib::YieldTermStructure> discountingCurve(discountingTermStructure);
            curveBuilder().reset(new PiecewiseCurveBuilderType()); // create the curve builder
            for (const auto& inst : *instruments) { // for each instrument
                if (inst->use()) {
                    this->curveBuilder_->AddHelper(inst->rateHelper(discountingCurve));
                }
            }
            estimatingCurve = curveBuilder()->GetCurve(curveReferenceDate, dayCounter, interp);
            discountCurve = (bootstrapMode() == BothCurvesConcurrently ? estimatingCurve : nullptr);
        }
        template<
            typename ActualVsImpliedComparison = DefaultActualVsImpliedComparison
        >
        QuantLib::Rate verify(
            std::ostream& os,
            std::streamsize precision = 16,
            const ActualVsImpliedComparison& compare = ActualVsImpliedComparison()
        ) const {
            auto discountTS = verificationDiscountTermStructure();
            QL_REQUIRE(discountTS != nullptr, "discount term structure cannot be null");
            QL_REQUIRE(estimatingCurve != nullptr, "forward estimating curve cannot be null");
            checkInstruments();
            QuantLib::Handle<QuantLib::YieldTermStructure> hDiscountCurve(discountTS);
            QuantLib::Handle<QuantLib::YieldTermStructure> hEstimatingCurve(estimatingCurve);
            return verifyImpl(
                instruments,
                [&hDiscountCurve, &hEstimatingCurve](const pInstrument& inst) -> QuantLib::Real {
                    return inst->impliedQuote(hEstimatingCurve, hDiscountCurve);
                },
                os,
                precision,
                compare
            );
        }
    };

    template <typename Interpolator>
    using ZeroCurvesBootstrap = YieldCurvesBootstrap<QuantLib::ZeroYield, Interpolator>;

    template <typename Interpolator>
    using DiscountCurvesBootstrap = YieldCurvesBootstrap<QuantLib::Discount, Interpolator>;

    template <typename Interpolator>
    using ForwardCurvesBootstrap = YieldCurvesBootstrap<QuantLib::ForwardRate, Interpolator>;

    template <typename Interpolator>
    using SimpleZeroCurvesBootstrap = YieldCurvesBootstrap<QuantLib::SimpleZeroYield, Interpolator>;
}
