#pragma once
#include <ql/quantlib.hpp>
#include <string>

namespace QuantLib {
	// USD OIS swap index
	template <typename OvernightIndex>	// OvernightIndex can be FedFunds or Sofr
	class UsdOvernightIndexedSwapIsdaFix : public
	#ifdef QL_OVERNIGHT_INDEXED_SWAP_INDEX_MISSING_IMPL
		QuantLib::OvernightIndexedSwapIndexEx
	#else
		QuantLib::OvernightIndexedSwapIndex
	#endif
			{
	public:
		UsdOvernightIndexedSwapIsdaFix(
			const QuantLib::Period& tenor,
			const QuantLib::Handle<QuantLib::YieldTermStructure>& indexEstimateTermStructure = QuantLib::Handle<QuantLib::YieldTermStructure>()
		) :
	#ifdef QL_OVERNIGHT_INDEXED_SWAP_INDEX_MISSING_IMPL
			OvernightIndexedSwapIndexEx
	#else
			OvernightIndexedSwapIndex
	#endif
			(
				std::string("UsdOvernightIndexedSwapIsdaFix<<")+ OvernightIndex().name() + ">>",
				tenor,
				2,
				QuantLib::USDCurrency(),
				QuantLib::ext::shared_ptr<OvernightIndex>(new OvernightIndex(indexEstimateTermStructure)),
				true,	// could be false ???
				QuantLib::RateAveraging::Compound
			) {}
	};
}
