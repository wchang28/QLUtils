#pragma once
#include <ql/quantlib.hpp>
#include <string>
#include <ql_utils/indexes/ois-swap-index.hpp>

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
                std::string("GbpOvernightIndexedSwapIsdaFix<<") + OvernightIndex().name() + ">>",
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
    class GbpFwdOISVanillaSwapIndex : public FwdOISVanillaSwapIndex<OVERNIGHTINDEX> {
    public:
        GbpFwdOISVanillaSwapIndex(
            const Period& tenor,
            const Handle<YieldTermStructure>& indexEstimatingTermStructure = Handle<YieldTermStructure>()
        ) :
            FwdOISVanillaSwapIndex<OVERNIGHTINDEX>
            (
                std::string("GbpFwdOISVanillaSwapIndex<<") + OVERNIGHTINDEX().name() + ">>",
                tenor,
                0,  // T+0 settlement
                GBPCurrency(),
                indexEstimatingTermStructure
            )
        {}
    };
}