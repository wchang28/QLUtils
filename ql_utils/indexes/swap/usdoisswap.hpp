#pragma once
#include <ql/quantlib.hpp>
#include <string>
#include <ql_utils/indexes/ois-swap-index.hpp>
#include <ql_utils/indexes/overnight-compounded-avg.hpp>

namespace QuantLib {
    template <
        typename OVERNIGHTINDEX // OvernightIndex can be Sofr or FedFunds
    >
    struct UsdOISFixingCalendarAdaptor {
        Calendar operator() () const {}
    };
    // FedFunds OIS swap's fixing calendar for both legs is UnitedStates(UnitedStates::FederalReserve)
    template<>
    inline Calendar UsdOISFixingCalendarAdaptor<FedFunds>::operator() () const {
        return UnitedStates(UnitedStates::FederalReserve);
    }
    // Sofr OIS swap's fixing calendar is for both legs UnitedStates(UnitedStates::GovernmentBond)
    template<>
    inline Calendar UsdOISFixingCalendarAdaptor<Sofr>::operator() () const {
        return UnitedStates(UnitedStates::GovernmentBond);
    }
    // USD OIS swap index
    template
    <
        typename OVERNIGHTINDEX, // OvernightIndex can be Sofr or FedFunds
        Frequency FREQ = Frequency::Annual  // cashflow frequency for both legs
    >
    class UsdOvernightIndexedSwapIsdaFix: public OvernightIndexedSwapIndexEx<OVERNIGHTINDEX, FREQ> {
    private:
        UsdOISFixingCalendarAdaptor<OVERNIGHTINDEX> fixingCalendarAdaptor_;
    public:
        UsdOvernightIndexedSwapIsdaFix(
            const Period& tenor,
            const Handle<YieldTermStructure>& indexEstimatingTermStructure = {}
        ) :
            OvernightIndexedSwapIndexEx<OVERNIGHTINDEX, FREQ>
            (
                tenor,
                2,  // T+2 swap settlement on the fixing calendar
                USDCurrency(),
                indexEstimatingTermStructure,
                2,   // 2 days payment lag on the payment calendar
                UnitedStates(UnitedStates::FederalReserve),    // payment calendar: uses FederalReserve calendar due to backward compatibility with FedFunds OIS
                fixingCalendarAdaptor_()  // fixing calendar
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
