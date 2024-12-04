#pragma once
#include <ql/quantlib.hpp>
#include <string>
#include <ql_utils/indexes/ois-swap-index.hpp>
#include <ql_utils/indexes/overnight-compounded-avg.hpp>

namespace QuantLib {
    // EUR OIS swap index
    template <
        typename OVERNIGHTINDEX // OvernightIndex can be Estr or Eonia
    >
    class EurOvernightIndexedSwapIsdaFix: public OvernightIndexedSwapIndexEx<OVERNIGHTINDEX> {
    public:
        typedef OVERNIGHTINDEX OvernightIndex;
    public:
        EurOvernightIndexedSwapIsdaFix(
            const Period& tenor,
            const Handle<YieldTermStructure>& indexEstimatingTermStructure = Handle<YieldTermStructure>()
        ) :
            OvernightIndexedSwapIndexEx<OVERNIGHTINDEX>
            (
                tenor,
                2,  // T+2 settlement
                EURCurrency(),
                indexEstimatingTermStructure,
                1,   // 1 day payment lag
                OvernightIndex().fixingCalendar()    // payment calendar: uses overnight index's fixing calendar
            )
        {}
    };

    template <
        typename OVERNIGHTINDEX // OvernightIndex can be Estr or Eonia
    >
    class EurOvernightCompoundedAverageIndex : public OvernightCompoundedAverageInArrearsIndex<OVERNIGHTINDEX> {
    public:
        EurOvernightCompoundedAverageIndex(
            const Handle<YieldTermStructure>& indexEstimatingTermStructure = Handle<YieldTermStructure>()
        ) :OvernightCompoundedAverageInArrearsIndex<OVERNIGHTINDEX>(
            2,  // T+2 fixing
            indexEstimatingTermStructure
        ) {}
    };

    template <
        typename OVERNIGHTINDEX // OvernightIndex can be Estr or Eonia
    >
    class EurFwdOISVanillaSwapIndex : public FwdOISVanillaSwapIndex<OVERNIGHTINDEX> {
    public:
        EurFwdOISVanillaSwapIndex(
            const Period& tenor,
            const Handle<YieldTermStructure>& indexEstimatingTermStructure = Handle<YieldTermStructure>()
        ) :
            FwdOISVanillaSwapIndex<OVERNIGHTINDEX>
            (
                tenor,
                2,  // T+2 settlement
                EURCurrency(),
                indexEstimatingTermStructure
            )
        {}
    };
}
