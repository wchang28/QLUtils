#pragma once

#include <ql/quantlib.hpp>
#include <ql_utils/types.hpp>
#include <ql_utils/bootstrap.hpp>

#define INTERP_FROM_CURVE(INTERP_TRAITS, INTERP, CURVE)    {\
        using BaseCurveType = typename INTERP_TRAITS<InterpolationType::INTERP>::BaseCurveType;  \
        auto base_curve = ext::dynamic_pointer_cast<BaseCurveType>(CURVE); \
        if (base_curve != nullptr) {\
            return InterpolationType::INTERP;   \
        }   \
    }

#define HANDLE_YIELD_TERM_STRUCT_INTERP_BOOTSTRAPPER(INTERP) case YieldTermStructureInterpolation::INTERP: {\
        using BootstrapperType = typename YieldTermStructureInterpTraits<YieldTermStructureInterpolation::INTERP>::BootstrapperType;   \
        return YieldCurvesBootstrapPtr(new BootstrapperType()); \
    }

namespace QuantLib {
    namespace Utils {
        template<
            YieldTermStructureInterpolation INTERP
        >
        struct YieldTermStructureInterpTraits {
            typedef void TraitsType;
            typedef void InterpType;
            typedef void BaseCurveType;
            typedef void BootstrapperType;
            static QLUtils::RateUnit valueUnit() {return QLUtils::RateUnit::Percent;}
        };
        template<>
        struct YieldTermStructureInterpTraits<YieldTermStructureInterpolation::ytsiPiecewiseLinearCont> {
            typedef ZeroYield TraitsType;
            typedef Linear InterpType;
            typedef typename TraitsType::template curve<InterpType>::type BaseCurveType;    // = QuantLib::ZeroCurve
            typedef YieldCurvesBootstrap<TraitsType, InterpType> BootstrapperType;
            static QLUtils::RateUnit valueUnit() {return QLUtils::RateUnit::Percent;}
        };
        template<>
        struct YieldTermStructureInterpTraits<YieldTermStructureInterpolation::ytsiPiecewiseLinearSimple> {
            typedef SimpleZeroYield TraitsType;
            typedef Linear InterpType;
            typedef typename TraitsType::template curve<InterpType>::type BaseCurveType;
            typedef YieldCurvesBootstrap<TraitsType, InterpType> BootstrapperType;
            static QLUtils::RateUnit valueUnit() {return QLUtils::RateUnit::Percent;}
        };
        template<>
        struct YieldTermStructureInterpTraits<YieldTermStructureInterpolation::ytsiStepForwardCont> {
            typedef ForwardRate TraitsType;
            typedef BackwardFlat InterpType;
            typedef typename TraitsType::template curve<InterpType>::type BaseCurveType; // = QuantLib::ForwardCurve
            typedef YieldCurvesBootstrap<TraitsType, InterpType> BootstrapperType;
            static QLUtils::RateUnit valueUnit() {return QLUtils::RateUnit::Percent;}
        };
        template<>
        struct YieldTermStructureInterpTraits<YieldTermStructureInterpolation::ytsiSmoothForwardCont> {
            typedef ForwardRate TraitsType;
            typedef ConvexMonotone InterpType;
            typedef typename TraitsType::template curve<InterpType>::type BaseCurveType;
            typedef YieldCurvesBootstrap<TraitsType, InterpType> BootstrapperType;
            static QLUtils::RateUnit valueUnit() {return QLUtils::RateUnit::Percent;}
        };
        template<>
        struct YieldTermStructureInterpTraits<YieldTermStructureInterpolation::ytsiPiecewiseLinearForwardCont> {
            typedef ForwardRate TraitsType;
            typedef Linear InterpType;
            typedef typename TraitsType::template curve<InterpType>::type BaseCurveType;
            typedef YieldCurvesBootstrap<TraitsType, InterpType> BootstrapperType;
            static QLUtils::RateUnit valueUnit() {return QLUtils::RateUnit::Percent;}
        };
        template<>
        struct YieldTermStructureInterpTraits<YieldTermStructureInterpolation::ytsiLogLinearDiscount> {
            typedef Discount TraitsType;
            typedef LogLinear InterpType;
            typedef typename TraitsType::template curve<InterpType>::type BaseCurveType;    // = QuantLib::DiscountCurve
            typedef YieldCurvesBootstrap<TraitsType, InterpType> BootstrapperType;
            static QLUtils::RateUnit valueUnit() {return QLUtils::RateUnit::Decimal;}
        };

        inline YieldTermStructureInterpolation get_yield_term_structure_interp_from_curve(
            const ext::shared_ptr<YieldTermStructure>& curve
        ) {
            using InterpolationType = YieldTermStructureInterpolation;
            INTERP_FROM_CURVE(YieldTermStructureInterpTraits, ytsiPiecewiseLinearCont, curve)
            INTERP_FROM_CURVE(YieldTermStructureInterpTraits, ytsiPiecewiseLinearSimple, curve)
            INTERP_FROM_CURVE(YieldTermStructureInterpTraits, ytsiStepForwardCont, curve)
            INTERP_FROM_CURVE(YieldTermStructureInterpTraits, ytsiSmoothForwardCont, curve)
            INTERP_FROM_CURVE(YieldTermStructureInterpTraits, ytsiPiecewiseLinearForwardCont, curve)
            INTERP_FROM_CURVE(YieldTermStructureInterpTraits, ytsiLogLinearDiscount, curve)
            QL_FAIL("unknown/unsupported yield term structure interpolation type");
        }
        
        inline YieldCurvesBootstrapPtr make_yield_curve_bootstrapper(
            YieldTermStructureInterpolation interpolation
        ) {
            switch(interpolation) {
            HANDLE_YIELD_TERM_STRUCT_INTERP_BOOTSTRAPPER(ytsiPiecewiseLinearCont)
            HANDLE_YIELD_TERM_STRUCT_INTERP_BOOTSTRAPPER(ytsiPiecewiseLinearSimple)
            HANDLE_YIELD_TERM_STRUCT_INTERP_BOOTSTRAPPER(ytsiStepForwardCont)
            HANDLE_YIELD_TERM_STRUCT_INTERP_BOOTSTRAPPER(ytsiSmoothForwardCont)
            HANDLE_YIELD_TERM_STRUCT_INTERP_BOOTSTRAPPER(ytsiPiecewiseLinearForwardCont)
            HANDLE_YIELD_TERM_STRUCT_INTERP_BOOTSTRAPPER(ytsiLogLinearDiscount)
            default:
                QL_FAIL("unknown/unsupported yield term structure interpolation type for bootstrapping: " << interpolation);
            }
        }

        template<
            ForwardSpreadInterpolation INTERP
        >
        struct ForwardSpreadInterpTraits {
            typedef void InterpType;
            typedef void SpreadsOnlyCurveType;
            typedef void ForwardSpreadedCurveType;
            typedef SpreadsOnlyCurveType BaseCurveType;
            static QLUtils::RateUnit valueUnit() {return QLUtils::RateUnit::BasisPoint;}
        };
        template<>
        struct ForwardSpreadInterpTraits<ForwardSpreadInterpolation::fsiStep> {
            typedef BackwardFlat InterpType;
            typedef InterpolatedForwardCurve<InterpType> SpreadsOnlyCurveType;  // = QuantLib::ForwardCurve
            typedef InterpolatedPiecewiseForwardSpreadedTermStructure<InterpType> ForwardSpreadedCurveType;
            typedef SpreadsOnlyCurveType BaseCurveType;
            static QLUtils::RateUnit valueUnit() {return QLUtils::RateUnit::BasisPoint;}
        };
        template<>
        struct ForwardSpreadInterpTraits<ForwardSpreadInterpolation::fsiLinear> {
            typedef Linear InterpType;
            typedef InterpolatedForwardCurve<InterpType> SpreadsOnlyCurveType;
            typedef InterpolatedPiecewiseForwardSpreadedTermStructure<InterpType> ForwardSpreadedCurveType;
            typedef SpreadsOnlyCurveType BaseCurveType;
            static QLUtils::RateUnit valueUnit() {return QLUtils::RateUnit::BasisPoint;}
        };
        
        inline ForwardSpreadInterpolation get_forward_spread_interp_from_curve(
            const ext::shared_ptr<YieldTermStructure>& spreadsOnlyCurve
        ) {
            using InterpolationType = ForwardSpreadInterpolation;
            INTERP_FROM_CURVE(ForwardSpreadInterpTraits, fsiStep, spreadsOnlyCurve)
            INTERP_FROM_CURVE(ForwardSpreadInterpTraits, fsiLinear, spreadsOnlyCurve)
            QL_FAIL("unknown/unsupported forward spread term structure interpolation type");
        }
    }
}

#undef INTERP_FROM_CURVE
#undef HANDLE_YIELD_TERM_STRUCT_INTERP_BOOTSTRAPPER
