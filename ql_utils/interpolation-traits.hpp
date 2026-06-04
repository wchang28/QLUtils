#pragma once

#include <ql/quantlib.hpp>
#include <ql_utils/types.hpp>

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
            typedef InterpolatedZeroCurve<InterpType> BaseCurveType;
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
            typedef InterpolatedForwardCurve<InterpType> BaseCurveType;
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
    }
}
