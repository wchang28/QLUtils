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
            const Handle<YieldTermStructure>& indexEstimatingTermStructure = {}
        ) :
            OvernightIndexedSwapIndexEx<OVERNIGHTINDEX, FREQ>
            (
                tenor,
                0,  // T+0 swap settlement
                GBPCurrency(),
                indexEstimatingTermStructure,
                0,   // 0 day payment lag
                OVERNIGHTINDEX().fixingCalendar()    // payment calendar: uses overnight index's fixing calendar
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
            const Handle<YieldTermStructure>& indexEstimatingTermStructure = {}
        ) :OvernightCompoundedAverageInArrearsIndex<OVERNIGHTINDEX, FREQ>(
            0,  // T+0 index fixing
            indexEstimatingTermStructure
        ) {}
    };

    template <
        typename OVERNIGHTINDEX, // OvernightIndex can be Sonia
        Frequency FREQ = Frequency::Annual  // cashflow frequency for both legs
    >
    class GbpFwdOISVanillaSwapIndex : public FwdOISVanillaSwapIndex<OVERNIGHTINDEX, FREQ> {
    public:
        GbpFwdOISVanillaSwapIndex(
            const Period& tenor,
            const Handle<YieldTermStructure>& indexEstimatingTermStructure = {}
        ) :
            FwdOISVanillaSwapIndex<OVERNIGHTINDEX, FREQ>
            (
                tenor,
                0,  // T+0 swap settlement
                indexEstimatingTermStructure
            )
        {}
    };
}
