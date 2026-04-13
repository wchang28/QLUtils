#pragma once
#include <ql/quantlib.hpp>
#include <string>
#include <ql_utils/indexes/ois-swap-index.hpp>
#include <ql_utils/indexes/overnight-compounded-avg.hpp>

namespace QuantLib {
    // GBP OIS swap index
    template <
        typename OVERNIGHTINDEX, // OvernightIndex can be Sonia
        Frequency FREQ = Frequency::Annual  // cashflow frequency for both legs
    >
    class GbpOvernightIndexedSwapIsdaFix: public OvernightIndexedSwapIndexEx<OVERNIGHTINDEX, FREQ> {
    public:
        GbpOvernightIndexedSwapIsdaFix(
            const Period& tenor,
            const Handle<YieldTermStructure>& h = {} // index estimating term structure
        ) :
            OvernightIndexedSwapIndexEx<OVERNIGHTINDEX, FREQ>
            (
                tenor,
                0,  // T+0 swap settlement on the fixing calendar
                GBPCurrency(),
                h,
                0,   // 0 day payment lag on the payment calendar
                OVERNIGHTINDEX().fixingCalendar(),    // payment calendar: uses overnight index's fixing calendar
                OVERNIGHTINDEX().fixingCalendar()   // fixing calendar: uses overnight index's fixing calendar
            )
        {}
    };

    template <
        typename OVERNIGHTINDEX, // OvernightIndex can be Sonia
        Frequency FREQ = Frequency::Annual  // frequency/tenor/maturity of the index, usually 1 year
    >
    class GbpOvernightCompoundedAverageIndex : public OvernightCompoundedAverageInArrearsIndex<OVERNIGHTINDEX, FREQ> {
    public:
        GbpOvernightCompoundedAverageIndex(
            const Handle<YieldTermStructure>& h = {} // index estimating term structure
        ) :OvernightCompoundedAverageInArrearsIndex<OVERNIGHTINDEX, FREQ>(
            0,  // T+0 index fixing
            h
        ) {}
    };

    template <
        typename OVERNIGHTINDEX, // OvernightIndex can be Sonia
        Frequency FREQ = Frequency::Annual  // cashflow frequency for both legs
    >
    class GbpFwdOISVanillaSwapIndex : public FwdOISVanillaSwapIndex<OVERNIGHTINDEX, FREQ> {
    public:
		typedef GbpOvernightCompoundedAverageIndex<OVERNIGHTINDEX, FREQ> IborIndexType;
    public:
        GbpFwdOISVanillaSwapIndex(
            const Period& tenor,
            const Handle<YieldTermStructure>& h = {} // index estimating term structure
        ) :
            FwdOISVanillaSwapIndex<OVERNIGHTINDEX, FREQ>
            (
                tenor,
                0,  // T+0 swap settlement
                h
            )
        {}
    };
}
