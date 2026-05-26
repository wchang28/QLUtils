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
#include <cmath>

namespace QuantLib {
    namespace Utils {
        class Bootstrapper {
        public:
            typedef QLUtils::BootstrapInstrument Instrument;
            typedef std::shared_ptr<Instrument> pInstrument;
            typedef std::vector<pInstrument> Instruments;
            typedef std::shared_ptr<Instruments> pInstruments;
            typedef ext::shared_ptr<YieldTermStructure> YieldTermStructurePtr;
            typedef Handle<YieldTermStructure> YieldTermStructureHandle;
            struct DefaultActualVsImpliedComparison {
                Rate operator() (
                    std::ostream& os,
                    const Instrument& inst,
                    const Real& actual,
                    const Real& implied
                ) const {
                    using DtFormat = QLUtils::DateFormat<char>;
                    auto startDate = inst.startDate();
                    auto endDate = inst.maturityDate();
                    auto diff = implied - actual;
                    os << inst.tenor();
                    os << "," << inst.ticker();
                    os << "," << "[" << DtFormat::to_yyyymmdd(startDate, true) << "," << DtFormat::to_yyyymmdd(endDate, true) << ")";
                    os << "," << "actual=" << actual * inst.valueMultiplier();
                    os << "," << "implied=" << implied * inst.valueMultiplier();
                    os << "," << "diff=" << diff * inst.basisPointDiffMultiplier() << " bp";
                    os << std::endl;
                    return diff * inst.absoluteDiffMultiplier();
                }
            };
        protected:
            template <
                typename ImpliedValueCalculator,
                typename ActualVsImpliedComparison
            >
            static Rate verifyImpl(
                const Instruments& instruments,
                const ImpliedValueCalculator& impliedValueCalculator,
                std::ostream& os,
                std::streamsize precision,
                const ActualVsImpliedComparison& compare
            ) {
                os << std::fixed;
                os << std::setprecision(precision);
                Rate err = 0.0;
                for (const auto& pInst : instruments) { // for each instrument
                    if (pInst != nullptr && pInst->use()) {
                        const auto& actual = pInst->value();
                        auto implied = impliedValueCalculator(pInst);
                        auto diff = compare(os, *pInst, actual, implied);
                        err += std::pow(diff, 2.0);
                    }
                }
                return std::sqrt(err);
            }
        };
        
        class IYieldCurvesBootstrap : public Bootstrapper {
        public:
            enum BootstrapMode {
                BothCurvesConcurrently = 0,
                EstimatingCurveOnly = 1
            };
        public:
            // input for the yield curve bootstrap
            pInstruments instruments; // bootstrap instruments
            YieldTermStructurePtr exogenousDiscountTermStructure;   // exogenous discount term structure. this can be nullptr (mode==BothCurvesConcurrently)
        public:
            BootstrapMode bootstrapMode() const {
                return (exogenousDiscountTermStructure != nullptr ? EstimatingCurveOnly : BothCurvesConcurrently);
            }
        protected:
            void checkInstruments() const {
                QL_REQUIRE(instruments != nullptr, "instruments is not set");
                QL_REQUIRE(!instruments->empty(), "instruments cannot be empty");
            }
        public:
            virtual void piecewiseBootstrap(
                const Date& curveReferenceDate,
                const DayCounter& dayCounter = Actual365Fixed()
            ) = 0;
            virtual YieldTermStructurePtr discounTermStructure() const = 0;
            virtual YieldTermStructurePtr estimatingTermStructure() const = 0;
            virtual Rate verifyBootstrap(
                std::ostream& os,
                std::streamsize precision = 16
            ) const = 0;
        };

        template <
            typename Traits = ZeroYield,   // ZeroYield, Discount, ForwardRate, or SimpleZeroYield
            typename Interpolator = Linear
        >
        class YieldCurvesBootstrap : public IYieldCurvesBootstrap {
        public:
            typedef QLUtils::PiecewiseCurveBuilder<Traits, Interpolator> PiecewiseCurveBuilderType;
            typedef PiecewiseYieldCurve<Traits, Interpolator> PiecewiseCurveType;
            typedef typename Traits::template curve<Interpolator>::type BaseCurveType;	// InterpolatedZeroCurve<Interpolator>, InterpolatedDiscountCurve<Interpolator>, InterpolatedForwardCurve<Interpolator>, or InterpolatedSimpleZeroCurve<Interpolator>
        protected:
            std::shared_ptr<PiecewiseCurveBuilderType> curveBuilder_;
        public:
            // output
            ext::shared_ptr<BaseCurveType> discountCurve;   // this can be nullptr (mode==EstimatingCurveOnly)
            ext::shared_ptr<BaseCurveType> estimatingCurve;
        public:
            YieldCurvesBootstrap() {}
            YieldTermStructurePtr discounTermStructure() const override {
                return (bootstrapMode() == EstimatingCurveOnly ? exogenousDiscountTermStructure : discountCurve); 
            }
            YieldTermStructurePtr estimatingTermStructure() const override {
                return estimatingCurve; 
            }
        protected:
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
            void bootstrap(
                const Date& curveReferenceDate,
                const DayCounter& dayCounter = Actual365Fixed(),
                const Interpolator& interp = Interpolator()
            ) {
                using DtFormat = QLUtils::DateFormat<char>;
                clearOutputs();
                checkInstruments();
                if (bootstrapMode() == EstimatingCurveOnly) {
                    auto expected = exogenousDiscountTermStructure->referenceDate();
                    QL_REQUIRE(curveReferenceDate == expected, "to be bootstrapped estimating curve ref. date (" << DtFormat::to_yyyymmdd(curveReferenceDate, true) << ") is not what's expected (discount curve ref. date=" << DtFormat::to_yyyymmdd(expected, true) << ")");
                }
                YieldTermStructureHandle hExogenousDiscountTS(exogenousDiscountTermStructure);
                curveBuilder().reset(new PiecewiseCurveBuilderType()); // create the curve builder
                for (const auto& inst : *instruments) { // for each instrument
                    if (inst->use()) {
                        this->curveBuilder_->AddHelper(inst->rateHelper(hExogenousDiscountTS));
                    }
                }
                estimatingCurve = curveBuilder()->GetCurve(curveReferenceDate, dayCounter, interp);
                discountCurve = (bootstrapMode() == BothCurvesConcurrently ? estimatingCurve : nullptr);
            }
            template<
                typename ActualVsImpliedComparison = DefaultActualVsImpliedComparison
            >
            Rate verify(
                std::ostream& os,
                std::streamsize precision = 16,
                const ActualVsImpliedComparison& compare = ActualVsImpliedComparison()
            ) const {
                auto discountTS = this->discounTermStructure();
                QL_REQUIRE(discountTS != nullptr, "discount term structure cannot be null");
                auto estimatingTS = this->estimatingTermStructure();
                QL_REQUIRE(estimatingTS != nullptr, "forward estimating term structure cannot be null");
                checkInstruments();
                YieldTermStructureHandle hDiscountTS(discountTS);
                YieldTermStructureHandle hEstimatingTS(estimatingTS);
                return verifyImpl(
                    *instruments,
                    [&hDiscountTS, &hEstimatingTS](const pInstrument& pInst) -> Real {
                        return pInst->impliedQuote(hEstimatingTS, hDiscountTS);
                    },
                    os,
                    precision,
                    compare
                );
            }
            
            void piecewiseBootstrap(
                const Date& curveReferenceDate,
                const DayCounter& dayCounter
            ) override {
                this->bootstrap(curveReferenceDate, dayCounter);
            }
            
            Rate verifyBootstrap(
                std::ostream& os,
                std::streamsize precision
            ) const override {
                return this->verify<DefaultActualVsImpliedComparison>(os, precision);
            }
        };

        template <typename Interpolator>
        using ZeroCurvesBootstrap = YieldCurvesBootstrap<ZeroYield, Interpolator>;

        template <typename Interpolator>
        using DiscountCurvesBootstrap = YieldCurvesBootstrap<Discount, Interpolator>;

        template <typename Interpolator>
        using ForwardCurvesBootstrap = YieldCurvesBootstrap<ForwardRate, Interpolator>;

        template <typename Interpolator>
        using SimpleZeroCurvesBootstrap = YieldCurvesBootstrap<SimpleZeroYield, Interpolator>;

        using YieldCurvesBootstrapPtr = std::shared_ptr<IYieldCurvesBootstrap>;
        inline YieldCurvesBootstrapPtr make_yield_curve_bootstrapper(YieldTermStructureInterpolation interpolation) {
            switch(interpolation) {
            case YieldTermStructureInterpolation::ytsiPiecewiseLinearCont:
                return YieldCurvesBootstrapPtr(new YieldCurvesBootstrap<ZeroYield, Linear>());
            case YieldTermStructureInterpolation::ytsiPiecewiseLinearSimple:
                return YieldCurvesBootstrapPtr(new YieldCurvesBootstrap<SimpleZeroYield, Linear>());
            case YieldTermStructureInterpolation::ytsiStepForwardCont:
                return YieldCurvesBootstrapPtr(new YieldCurvesBootstrap<ForwardRate, BackwardFlat>());
            case YieldTermStructureInterpolation::ytsiSmoothForwardCont:
                return YieldCurvesBootstrapPtr(new YieldCurvesBootstrap<ForwardRate, ConvexMonotone>());
            case YieldTermStructureInterpolation::ytsiPiecewiseLinearForwardCont:
                return YieldCurvesBootstrapPtr(new YieldCurvesBootstrap<ForwardRate, Linear>());
            default:
                QL_FAIL("unsupported yield term structure interpolation for bootstrapping: " << interpolation);
            }
        }
    }
}
