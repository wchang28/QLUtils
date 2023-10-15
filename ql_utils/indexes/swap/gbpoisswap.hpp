#pragma once
#include <ql/quantlib.hpp>
#include <string>
#include <ql_utils/indexes/ois-swap-index.hpp>
#include <ql_utils/indexes/overnight-compounded-avg.hpp>

namespace QuantLib {
    // GBP OIS swap index
    template <
        typename OVERNIGHTINDEX // OvernightIndex can be Sonia
    >
    class GbpOvernightIndexedSwapIsdaFix: public OvernightIndexedSwapIndexEx<OVERNIGHTINDEX> {
    public:
        typedef typename OVERNIGHTINDEX OvernightIndex;
    public:
        GbpOvernightIndexedSwapIsdaFix(
            const Period& tenor,
            const Handle<YieldTermStructure>& indexEstimatingTermStructure = Handle<YieldTermStructure>()
        ) :
            OvernightIndexedSwapIndexEx<OVERNIGHTINDEX>
            (
                tenor,
                0,  // T+0 settlement
                GBPCurrency(),
                indexEstimatingTermStructure,
                0,   // 0 day payment lag
                OvernightIndex().fixingCalendar()    // payment calendar: uses overnight index's fixing calendar
            )
        {}
    };

    template <
        typename OVERNIGHTINDEX // OvernightIndex can be Sonia
    >
    class GbpOvernightCompoundedAverageIndex : public OvernightCompoundedAverageInArrearsIndex<OVERNIGHTINDEX> {
    public:
        GbpOvernightCompoundedAverageIndex(
            const Handle<YieldTermStructure>& indexEstimatingTermStructure = Handle<YieldTermStructure>()
        ) :OvernightCompoundedAverageInArrearsIndex<OVERNIGHTINDEX>(
            0,  // T+0 fixing
            indexEstimatingTermStructure
        ) {}
    };

    template <
        typename OVERNIGHTINDEX // OvernightIndex can be Sonia
    >
    class GbpFwdOISVanillaSwapIndex : public FwdOISVanillaSwapIndex<OVERNIGHTINDEX> {
    public:
        GbpFwdOISVanillaSwapIndex(
            const Period& tenor,
            const Handle<YieldTermStructure>& indexEstimatingTermStructure = Handle<YieldTermStructure>()
        ) :
            FwdOISVanillaSwapIndex<OVERNIGHTINDEX>
            (
                tenor,
                0,  // T+0 settlement
                GBPCurrency(),
                indexEstimatingTermStructure
            )
        {}
    };
}
