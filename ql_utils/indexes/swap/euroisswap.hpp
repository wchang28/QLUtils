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
            const Handle<YieldTermStructure>& indexEstimatingTermStructure = {}
        ) :
            OvernightIndexedSwapIndexEx<OVERNIGHTINDEX, FREQ>
            (
                tenor,
                2,  // T+2 swap settlement
                EURCurrency(),
                indexEstimatingTermStructure,
                1,   // 1 day payment lag
                OVERNIGHTINDEX().fixingCalendar()    // payment calendar: uses overnight index's fixing calendar
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
            const Handle<YieldTermStructure>& indexEstimatingTermStructure = {}
        ) :OvernightCompoundedAverageInArrearsIndex<OVERNIGHTINDEX, FREQ>(
            2,  // T+2 index fixing
            indexEstimatingTermStructure
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
            const Handle<YieldTermStructure>& indexEstimatingTermStructure = {}
        ) :
            FwdOISVanillaSwapIndex<OVERNIGHTINDEX, FREQ>
            (
                tenor,
                2,  // T+2 swap settlement
                indexEstimatingTermStructure
            )
        {}
    };
}
