#pragma once
#include <ql/quantlib.hpp>
#include <string>
#include <ql_utils/ois-swap-index.hpp>

namespace QuantLib {
    // EUR OIS swap index
    template <
        typename OVERNIGHTINDEX // OvernightIndex can be Estr or Eonia
    >
    class EurOvernightIndexedSwapIsdaFix: public OvernightIndexedSwapIndexEx {
public:
    typedef typename OVERNIGHTINDEX OvernightIndex;
public:
    EurOvernightIndexedSwapIsdaFix(
        const Period& tenor,
        const Handle<YieldTermStructure>& indexEstimatingTermStructure = Handle<YieldTermStructure>()
    ) :
        OvernightIndexedSwapIndexEx
        (
            std::string("EurOvernightIndexedSwapIsdaFix<<") + OvernightIndex().name() + ">>",
            tenor,
            2,  // T+2 settlement
            EURCurrency(),
            ext::shared_ptr<OvernightIndex>(new OvernightIndex(indexEstimatingTermStructure)),
            false,
            RateAveraging::Compound,
            1,   // 1 day payment lag
            BusinessDayConvention::Following,    // payment adjustment convention
            TARGET()    // payment calendar
        ) {}
    };
}
