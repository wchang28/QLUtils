#pragma once

#include <ql/quantlib.hpp>
#include <ql_utils/types.hpp>

#define INTERP_FROM_CURVE(INTERP_TRAITS, INTERP, CURVE)    {\
        using BaseCurveType = typename INTERP_TRAITS<InterpolationType::INTERP>::BaseCurveType;  \
        auto base_curve = ext::dynamic_pointer_cast<BaseCurveType>(CURVE); \
        if (base_curve != nullptr) {\
            return InterpolationType::INTERP;   \
        }   \
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
        };
        template<>
        struct YieldTermStructureInterpTraits<YieldTermStructureInterpolation::ytsiPiecewiseLinearCont> {
            typedef ZeroYield TraitsType;
            typedef Linear InterpType;
            typedef InterpolatedZeroCurve<InterpType> BaseCurveType;    // = QuantLib::ZeroCurve
        };
        template<>
        struct YieldTermStructureInterpTraits<YieldTermStructureInterpolation::ytsiPiecewiseLinearSimple> {
            typedef SimpleZeroYield TraitsType;
            typedef Linear InterpType;
            typedef InterpolatedSimpleZeroCurve<InterpType> BaseCurveType;
        };
        template<>
        struct YieldTermStructureInterpTraits<YieldTermStructureInterpolation::ytsiStepForwardCont> {
            typedef ForwardRate TraitsType;
            typedef BackwardFlat InterpType;
            typedef InterpolatedForwardCurve<InterpType> BaseCurveType; // = QuantLib::ForwardCurve
        };
        template<>
        struct YieldTermStructureInterpTraits<YieldTermStructureInterpolation::ytsiSmoothForwardCont> {
            typedef ForwardRate TraitsType;
            typedef ConvexMonotone InterpType;
            typedef InterpolatedForwardCurve<InterpType> BaseCurveType;
        };
        template<>
        struct YieldTermStructureInterpTraits<YieldTermStructureInterpolation::ytsiPiecewiseLinearForwardCont> {
            typedef ForwardRate TraitsType;
            typedef Linear InterpType;
            typedef InterpolatedForwardCurve<InterpType> BaseCurveType;
        };
        template<>
        struct YieldTermStructureInterpTraits<YieldTermStructureInterpolation::ytsiLogLinearDiscount> {
            typedef Discount TraitsType;
            typedef LogLinear InterpType;
            typedef InterpolatedDiscountCurve<InterpType> BaseCurveType;    // = QuantLib::DiscountCurve
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
            QL_FAIL("unsupported yield term structure interpolation type");
        }

        template<
            ForwardSpreadInterpolation INTERP
        >
        struct ForwardSpreadInterpTraits {
            typedef void InterpType;
            typedef void SpreadsOnlyCurveType;
            typedef void ForwardSpreadedCurveType;
            typedef SpreadsOnlyCurveType BaseCurveType;
        };
        template<>
        struct ForwardSpreadInterpTraits<ForwardSpreadInterpolation::fsiStep> {
            typedef BackwardFlat InterpType;
            typedef InterpolatedForwardCurve<InterpType> SpreadsOnlyCurveType;
            typedef InterpolatedPiecewiseForwardSpreadedTermStructure<InterpType> ForwardSpreadedCurveType;
            typedef SpreadsOnlyCurveType BaseCurveType;
        };
        template<>
        struct ForwardSpreadInterpTraits<ForwardSpreadInterpolation::fsiLinear> {
            typedef Linear InterpType;
            typedef InterpolatedForwardCurve<InterpType> SpreadsOnlyCurveType;
            typedef InterpolatedPiecewiseForwardSpreadedTermStructure<InterpType> ForwardSpreadedCurveType;
            typedef SpreadsOnlyCurveType BaseCurveType;
        };
        
        inline ForwardSpreadInterpolation get_forward_spread_interp_from_curve(
            const ext::shared_ptr<YieldTermStructure>& spreadsOnlyCurve
        ) {
            using InterpolationType = ForwardSpreadInterpolation;
            INTERP_FROM_CURVE(ForwardSpreadInterpTraits, fsiStep, spreadsOnlyCurve)
            INTERP_FROM_CURVE(ForwardSpreadInterpTraits, fsiLinear, spreadsOnlyCurve)
            QL_FAIL("unsupported forward spread term structure interpolation type");
        }
    }
}

#undef INTERP_FROM_CURVE
