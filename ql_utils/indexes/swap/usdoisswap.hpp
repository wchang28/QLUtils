#pragma once
#include <ql/quantlib.hpp>
#include <string>
#include <ql_utils/indexes/ois-swap-index.hpp>
#include <ql_utils/indexes/overnight-compounded-avg.hpp>

namespace QuantLib {
    // USD OIS swap index
    template
    <
        typename OVERNIGHTINDEX, // OvernightIndex can be Sofr or FedFunds
        Frequency FREQ = Frequency::Annual  // cashflow frequency for both legs
    >
    class UsdOvernightIndexedSwapIsdaFix: public OvernightIndexedSwapIndexEx<OVERNIGHTINDEX, FREQ> {
    public:
        UsdOvernightIndexedSwapIsdaFix(
            const Period& tenor,
            const Handle<YieldTermStructure>& indexEstimatingTermStructure = {}
        ) :
            OvernightIndexedSwapIndexEx<OVERNIGHTINDEX, FREQ>
            (
                tenor,
                2,  // T+2 swap settlement
                USDCurrency(),
                indexEstimatingTermStructure,
                2,   // 2 days payment lag
                UnitedStates(UnitedStates::FederalReserve)    // payment calendar: uses FederalReserve calendar due to backward compatibility with FedFunds OIS
            )
        {}
    };

    template <
        typename OVERNIGHTINDEX, // OvernightIndex can be Sofr or FedFunds
        Frequency FREQ = Frequency::Annual  // frequency/tenor/maturity of the index, usually 1 year
    >
    class UsdOvernightCompoundedAverageIndex : public OvernightCompoundedAverageInArrearsIndex<OVERNIGHTINDEX, FREQ> {
    public:
        UsdOvernightCompoundedAverageIndex(
            const Handle<YieldTermStructure>& indexEstimatingTermStructure = {}
        ) :OvernightCompoundedAverageInArrearsIndex<OVERNIGHTINDEX, FREQ>(
            2,  // T+2 index fixing
            indexEstimatingTermStructure
        ) {}
    };

    template <
        typename OVERNIGHTINDEX, // OvernightIndex can be Sofr or FedFunds
        Frequency FREQ = Frequency::Annual  // cashflow frequency for both legs
    >
    class UsdFwdOISVanillaSwapIndex : public FwdOISVanillaSwapIndex<OVERNIGHTINDEX, FREQ> {
    public:
        UsdFwdOISVanillaSwapIndex(
            const Period& tenor,
            const Handle<YieldTermStructure>& indexEstimatingTermStructure = {}
        ) :
            FwdOISVanillaSwapIndex<OVERNIGHTINDEX, FREQ>
            (
                tenor,
                2,  // T+2 swap settlement
                indexEstimatingTermStructure
            )
        {}
    };
}
