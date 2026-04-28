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
    
    class IYieldCurvesBootstrap : public Bootstrapper {
    public:
        enum BootstrapMode {
            BothCurvesConcurrently = 0,
            EstimatingCurveOnly = 1
        };
        typedef QuantLib::ext::shared_ptr<QuantLib::YieldTermStructure> YieldTermStructurePtr;
        typedef QuantLib::Handle<QuantLib::YieldTermStructure> YieldTermStructureHandle;
    public:
        // input for the yield curve bootstrap
        pInstruments instruments; // bootstrap instruments
        YieldTermStructurePtr exogenousDiscountTermStructure;   // exogenous discount term structure. this can be nullptr (mode==BothCurvesConcurrently)
    public:
        BootstrapMode bootstrapMode() const {
            return (exogenousDiscountTermStructure != nullptr ? EstimatingCurveOnly : BothCurvesConcurrently);
        }
    public:
        virtual void piecewiseBootstrap(
            const QuantLib::Date& curveReferenceDate,
            const QuantLib::DayCounter& dayCounter = QuantLib::Actual365Fixed()
        ) = 0;
        virtual YieldTermStructurePtr discounTermStructure() const = 0;
        virtual YieldTermStructurePtr estimatingTermStructure() const = 0;
        virtual QuantLib::Rate verifyBootstrap(
            std::ostream& os,
            std::streamsize precision = 16
        ) const = 0;
    };

    template <
        typename Traits = QuantLib::ZeroYield,   // QuantLib::ZeroYield, QuantLib::Discount, QuantLib::ForwardRate, or QuantLib::SimpleZeroYield
        typename Interpolator = QuantLib::Linear
    >
    class YieldCurvesBootstrap : public IYieldCurvesBootstrap {
    public:
        typedef PiecewiseCurveBuilder<Traits, Interpolator> PiecewiseCurveBuilderType;
        typedef QuantLib::PiecewiseYieldCurve<Traits, Interpolator> PiecewiseCurveType;
        typedef typename Traits::template curve<Interpolator>::type BaseCurveType;	// InterpolatedZeroCurve<Interpolator>, InterpolatedDiscountCurve<Interpolator>, InterpolatedForwardCurve<Interpolator>, or InterpolatedSimpleZeroCurve<Interpolator>
    protected:
        std::shared_ptr<PiecewiseCurveBuilderType> curveBuilder_;
    public:
        // output
        QuantLib::ext::shared_ptr<BaseCurveType> discountCurve;   // this can be nullptr (mode==EstimatingCurveOnly)
        QuantLib::ext::shared_ptr<BaseCurveType> estimatingCurve;
    public:
        YieldCurvesBootstrap() {}
        YieldTermStructurePtr discounTermStructure() const override {
            return (bootstrapMode() == EstimatingCurveOnly ? exogenousDiscountTermStructure : discountCurve); 
        }
        YieldTermStructurePtr estimatingTermStructure() const override {
            return estimatingCurve; 
        }
    protected:
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
                using DtFormat = DateFormat<char>;
                auto startDate = inst->startDate();
                auto endDate = inst->maturityDate();
                auto diff = implied - actual;
                os << inst->tenor();
                os << "," << inst->ticker();
                os << "," << "[" << DtFormat::to_yyyymmdd(startDate, true) << "," << DtFormat::to_yyyymmdd(endDate, true) << ")";
                os << "," << "actual=" << actual * inst->valueMultiplier();
                os << "," << "implied=" << implied * inst->valueMultiplier();
                os << "," << "diff=" << diff * inst->basisPointDiffMultiplier() << " bp";
                os << std::endl;
                return diff * inst->absoluteDiffMultiplier();
            }
        };
        void bootstrap(
            const QuantLib::Date& curveReferenceDate,
            const QuantLib::DayCounter& dayCounter = QuantLib::Actual365Fixed(),
            const Interpolator& interp = Interpolator()
        ) {
            using DtFormat = DateFormat<char>;
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
        QuantLib::Rate verify(
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
                instruments,
                [&hDiscountTS, &hEstimatingTS](const pInstrument& inst) -> QuantLib::Real {
                    return inst->impliedQuote(hEstimatingTS, hDiscountTS);
                },
                os,
                precision,
                compare
            );
        }
        
        void piecewiseBootstrap(
            const QuantLib::Date& curveReferenceDate,
            const QuantLib::DayCounter& dayCounter
        ) override {
            this->bootstrap(curveReferenceDate, dayCounter);
        }
        
        QuantLib::Rate verifyBootstrap(
            std::ostream& os,
            std::streamsize precision
        ) const override {
            return this->verify<DefaultActualVsImpliedComparison>(os, precision);
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

namespace QuantLib {
    namespace Utils {
        using YieldCurvesBootstrapPtr = std::shared_ptr<QLUtils::IYieldCurvesBootstrap>;
        inline YieldCurvesBootstrapPtr make_yield_curve_bootstrapper(YieldTermStructureInterpolation interpolation) {
            switch(interpolation) {
            case YieldTermStructureInterpolation::ytsiPiecewiseLinearCont:
                return YieldCurvesBootstrapPtr(new QLUtils::YieldCurvesBootstrap<QuantLib::ZeroYield, QuantLib::Linear>());
            case YieldTermStructureInterpolation::ytsiPiecewiseLinearSimple:
                return YieldCurvesBootstrapPtr(new QLUtils::YieldCurvesBootstrap<QuantLib::SimpleZeroYield, QuantLib::Linear>());
            case YieldTermStructureInterpolation::ytsiStepForwardCont:
                return YieldCurvesBootstrapPtr(new QLUtils::YieldCurvesBootstrap<QuantLib::ForwardRate, QuantLib::BackwardFlat>());
            case YieldTermStructureInterpolation::ytsiSmoothForwardCont:
                return YieldCurvesBootstrapPtr(new QLUtils::YieldCurvesBootstrap<QuantLib::ForwardRate, QuantLib::ConvexMonotone>());
            default:
                QL_FAIL("unsupported term structure interpolation: " << interpolation);
            }
        }
    }
}
