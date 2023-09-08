#pragma once
#include <ql/quantlib.hpp>
#include <string>
#include <ql_utils/ois-swap-index.hpp>

namespace QuantLib {
    // GBP OIS swap index
    template <
        typename OVERNIGHTINDEX // OvernightIndex can be Sonia
    >
    class GbpOvernightIndexedSwapIsdaFix: public OvernightIndexedSwapIndexEx {
public:
    typedef typename OVERNIGHTINDEX OvernightIndex;
public:
    GbpOvernightIndexedSwapIsdaFix(
        const Period& tenor,
        const Handle<YieldTermStructure>& indexEstimatingTermStructure = Handle<YieldTermStructure>()
    ) :
        OvernightIndexedSwapIndexEx
        (
            std::string("GbpOvernightIndexedSwapIsdaFix<<") + OvernightIndex().name() + ">>",
            tenor,
            0,  // T+0 settlement
            GBPCurrency(),
            ext::shared_ptr<OvernightIndex>(new OvernightIndex(indexEstimatingTermStructure)),
            false,
            RateAveraging::Compound,
            0,   // 0 day payment lag
            BusinessDayConvention::Following,    // payment adjustment convention
            UnitedKingdom(UnitedKingdom::Exchange)    // payment calendar
        ) {}
    };
}
