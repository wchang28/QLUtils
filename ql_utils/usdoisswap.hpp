#pragma once
#include <ql/quantlib.hpp>
#include <string>

namespace QuantLib {
    // USD OIS swap index
    template
	<
		typename OVERNIGHTINDEX // OvernightIndex can be Sofr or FedFunds
	>
    class UsdOvernightIndexedSwapIsdaFix: public OvernightIndexedSwapIndex {
    public:
        typedef typename OVERNIGHTINDEX OvernightIndex;
    public:
        UsdOvernightIndexedSwapIsdaFix(
            const Period& tenor,
            const Handle<YieldTermStructure>& indexEstimatingTermStructure = Handle<YieldTermStructure>()
        ) :
            OvernightIndexedSwapIndex
            (
                std::string("UsdOvernightIndexedSwapIsdaFix<<") + OvernightIndex().name() + ">>",
                tenor,
                2,
                USDCurrency(),
                ext::shared_ptr<OvernightIndex>(new OvernightIndex(indexEstimatingTermStructure)),
                false,
                RateAveraging::Compound
            )
        {}
    };
}
