#pragma once
#include <ql/quantlib.hpp>
#include <string>
#include <ql_utils/indexes/ois-swap-index.hpp>
#include <ql_utils/indexes/overnight-compounded-avg.hpp>

namespace QuantLib {
    // USD OIS swap index
    template
	<
		typename OVERNIGHTINDEX // OvernightIndex can be Sofr or FedFunds
	>
    class UsdOvernightIndexedSwapIsdaFix: public OvernightIndexedSwapIndexEx<OVERNIGHTINDEX> {
    public:
        typedef typename OVERNIGHTINDEX OvernightIndex;
    public:
        UsdOvernightIndexedSwapIsdaFix(
            const Period& tenor,
            const Handle<YieldTermStructure>& indexEstimatingTermStructure = Handle<YieldTermStructure>()
        ) :
            OvernightIndexedSwapIndexEx<OVERNIGHTINDEX>
            (
                std::string("UsdOvernightIndexedSwapIsdaFix<<") + OvernightIndex().name() + ">>",
                tenor,
                2,  // T+2 settlement
                USDCurrency(),
                indexEstimatingTermStructure,
                2,   // 2 days payment lag
                UnitedStates(UnitedStates::FederalReserve)    // payment calendar: uses FederalReserve calendar due to backward compatibility with FedFunds OIS
            )
        {}
    };

    template <
        typename OVERNIGHTINDEX // OvernightIndex can be Sofr or FedFunds
    >
    class UsdOvernightCompoundedAverageIndex : public OvernightCompoundedAverageInArrearsIndex<OVERNIGHTINDEX> {
    public:
        UsdOvernightCompoundedAverageIndex(
            const Handle<YieldTermStructure>& indexEstimatingTermStructure = Handle<YieldTermStructure>()
        ) :OvernightCompoundedAverageInArrearsIndex<OVERNIGHTINDEX>(
            2,  // T+2 fixing
            indexEstimatingTermStructure
        ) {}
    };

    template <
        typename OVERNIGHTINDEX // OvernightIndex can be Sofr or FedFunds
    >
    class UsdFwdOISVanillaSwapIndex : public FwdOISVanillaSwapIndex<OVERNIGHTINDEX> {
    public:
        UsdFwdOISVanillaSwapIndex(
            const Period& tenor,
            const Handle<YieldTermStructure>& indexEstimatingTermStructure = Handle<YieldTermStructure>()
        ) :
            FwdOISVanillaSwapIndex<OVERNIGHTINDEX>
            (
                std::string("UsdFwdOISVanillaSwapIndex<<") + OVERNIGHTINDEX().name() + ">>",
                tenor,
                2,  // T+2 settlement
                USDCurrency(),
                indexEstimatingTermStructure
            )
        {}
    };
}
