#pragma once
#include <ql/quantlib.hpp>
#include <string>
#include <ql_utils/ois-swap-index.hpp>

namespace QuantLib {
    // USD OIS swap index
    template
	<
		typename OVERNIGHTINDEX // OvernightIndex can be Sofr or FedFunds
	>
    class UsdOvernightIndexedSwapIsdaFix: public OvernightIndexedSwapIndexEx {
    public:
        typedef typename OVERNIGHTINDEX OvernightIndex;
    public:
        UsdOvernightIndexedSwapIsdaFix(
            const Period& tenor,
            const Handle<YieldTermStructure>& indexEstimatingTermStructure = Handle<YieldTermStructure>()
        ) :
            OvernightIndexedSwapIndexEx
            (
                std::string("UsdOvernightIndexedSwapIsdaFix<<") + OvernightIndex().name() + ">>",
                tenor,
                2,  // T+2 settle
                USDCurrency(),
                ext::shared_ptr<OvernightIndex>(new OvernightIndex(indexEstimatingTermStructure)),
                false,
                RateAveraging::Compound,
                2,   // 2 days payment lag
                BusinessDayConvention::Following    // payment adjustment convention
            )
        {}
    };
}
