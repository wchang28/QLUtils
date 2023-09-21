#pragma once
#include <ql/quantlib.hpp>
#include <string>
#include <ql_utils/indexes/ois-swap-index.hpp>

namespace QuantLib {
    // EUR OIS swap index
    template <
        typename OVERNIGHTINDEX // OvernightIndex can be Estr or Eonia
    >
    class EurOvernightIndexedSwapIsdaFix: public OvernightIndexedSwapIndexEx<OVERNIGHTINDEX> {
    public:
        typedef typename OVERNIGHTINDEX OvernightIndex;
    public:
        EurOvernightIndexedSwapIsdaFix(
            const Period& tenor,
            const Handle<YieldTermStructure>& indexEstimatingTermStructure = Handle<YieldTermStructure>()
        ) :
            OvernightIndexedSwapIndexEx<OVERNIGHTINDEX>
            (
                std::string("EurOvernightIndexedSwapIsdaFix<<") + OvernightIndex().name() + ">>",
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
    class EurFwdOISVanillaSwapIndex : public FwdOISVanillaSwapIndex<OVERNIGHTINDEX> {
    public:
        EurFwdOISVanillaSwapIndex(
            const Period& tenor,
            const Handle<YieldTermStructure>& indexEstimatingTermStructure = Handle<YieldTermStructure>()
        ) :
            FwdOISVanillaSwapIndex<OVERNIGHTINDEX>
            (
                std::string("EurForwardOISVanillaSwapIndex<<") + OVERNIGHTINDEX().name() + ">>",
                tenor,
                2,  // T+2 settlement
                EURCurrency(),
                indexEstimatingTermStructure
            )
        {}
    };
}
