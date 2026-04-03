#pragma once
#include <ql/quantlib.hpp>
#include <string>
#include <ql_utils/indexes/ois-swap-index.hpp>
#include <ql_utils/indexes/overnight-compounded-avg.hpp>

namespace QuantLib {
    // EUR OIS swap index
    template <
        typename OVERNIGHTINDEX, // OvernightIndex can be Estr or Eonia
        Frequency FREQ = Frequency::Annual  // cashflow frequency for both legs
    >
    class EurOvernightIndexedSwapIsdaFix: public OvernightIndexedSwapIndexEx<OVERNIGHTINDEX, FREQ> {
    public:
        EurOvernightIndexedSwapIsdaFix(
            const Period& tenor,
            const Handle<YieldTermStructure>& h = {} // index estimating term structure
        ) :
            OvernightIndexedSwapIndexEx<OVERNIGHTINDEX, FREQ>
            (
                tenor,
                2,  // T+2 swap settlement on the fixing calendar
                EURCurrency(),
                h,
                1,   // 1 day payment lag on the payment calendar
                OVERNIGHTINDEX().fixingCalendar(),    // payment calendar: uses overnight index's fixing calendar
                OVERNIGHTINDEX().fixingCalendar()   // fixing calendar: uses overnight index's fixing calendar
            )
        {}
    };

    template <
        typename OVERNIGHTINDEX, // OvernightIndex can be Estr or Eonia
        Frequency FREQ = Frequency::Annual  // frequency/tenor/maturity of the index, usually 1 year
    >
    class EurOvernightCompoundedAverageIndex : public OvernightCompoundedAverageInArrearsIndex<OVERNIGHTINDEX, FREQ> {
    public:
        EurOvernightCompoundedAverageIndex(
            const Handle<YieldTermStructure>& h = {} // index estimating term structure
        ) :OvernightCompoundedAverageInArrearsIndex<OVERNIGHTINDEX, FREQ>(
            2,  // T+2 index fixing
            h
        ) {}
    };

    template <
        typename OVERNIGHTINDEX, // OvernightIndex can be Estr or Eonia
        Frequency FREQ = Frequency::Annual  // cashflow frequency for both legs
    >
    class EurFwdOISVanillaSwapIndex : public FwdOISVanillaSwapIndex<OVERNIGHTINDEX, FREQ> {
    public:
        EurFwdOISVanillaSwapIndex(
            const Period& tenor,
            const Handle<YieldTermStructure>& h = {} // index estimating term structure
        ) :
            FwdOISVanillaSwapIndex<OVERNIGHTINDEX, FREQ>
            (
                tenor,
                2,  // T+2 swap settlement
                h
            )
        {}
    };
}
