#pragma once

#include <ql/quantlib.hpp>
#include <string>
#include <ql_utils/indexes/overnight-compounded-avg.hpp>

namespace QuantLib {
    // enhanced version of the OvernightIndexedSwapIndex that supports
    // telescopicValueDates()
    // averagingMethod()
    // paymentLag()
    // paymentConvention()
    // paymentFrequency()
    // paymentCalendar()
    template <
        typename OVERNIGHTINDEX // OvernightIndex can be Sofr, FedFunds, Sonia, Estr, Eonia
    >
    class OvernightIndexedSwapIndexEx : public OvernightIndexedSwapIndex {
    protected:
        Natural paymentLag_;
        Calendar paymentCalendar_;
    public:
        OvernightIndexedSwapIndexEx(
            const std::string& familyName,
            const Period& tenor,
            Natural settlementDays,
            const Currency& currency,
            const Handle<YieldTermStructure>& indexEstimatingTermStructure = Handle<YieldTermStructure>(),
            Natural paymentLag = 0,
            const Calendar& paymentCalendar = Calendar()
        ): 
            OvernightIndexedSwapIndex(
                familyName,
                tenor,
                settlementDays,
                currency,
                ext::shared_ptr<OvernightIndex>(new OVERNIGHTINDEX(indexEstimatingTermStructure)),  // overnightIndex
                false,  // telescopicValueDates
                RateAveraging::Compound // averagingMethod
            ),
            paymentLag_(paymentLag),
            paymentCalendar_(paymentCalendar)
        {}
        bool telescopicValueDates() const {
            return telescopicValueDates_;
        }
        RateAveraging::Type averagingMethod() const {
            return averagingMethod_;
        }
        Natural paymentLag() const {
            return paymentLag_;
        }
        BusinessDayConvention paymentConvention() const {
            return BusinessDayConvention::Following;
        }
        QuantLib::Frequency paymentFrequency() const {
            return fixedLegTenor().frequency();
        }
        const Calendar& paymentCalendar() const {
            return paymentCalendar_;
        }
        ext::shared_ptr<OvernightIndexedSwap> underlyingSwap(
            const Date& fixingDate
        ) const {
            QL_REQUIRE(fixingDate != Date(), "null fixing date");

            // caching mechanism
            if (lastFixingDate_ != fixingDate) {
                Rate fixedRate = 0.0;
                lastSwap_ = MakeOIS(tenor_, overnightIndex_, fixedRate)
                    .withEffectiveDate(valueDate(fixingDate))
                    .withFixedLegDayCount(dayCounter_)
                    .withTelescopicValueDates(telescopicValueDates_)
                    .withAveragingMethod(averagingMethod_)
                    .withPaymentLag(paymentLag_)
                    .withPaymentAdjustment(paymentConvention())
                    .withPaymentFrequency(paymentFrequency())
                    .withPaymentCalendar(paymentCalendar_)
                    ;
                lastFixingDate_ = fixingDate;
            }
            return lastSwap_;
        }
    };

    template <
        typename OVERNIGHTINDEX // OvernightIndex can be Sofr, FedFunds, Sonia, Estr, Eonia
    >
    class FwdOISVanillaSwapIndex : public SwapIndex {
    public:
        typedef typename OVERNIGHTINDEX OvernightIndex;
    public:
        FwdOISVanillaSwapIndex(
            const std::string& familyName,
            const Period& tenor,
            Natural settlementDays,
            const Currency currency,
            const Handle<YieldTermStructure>& indexEstimatingTermStructure = Handle<YieldTermStructure>()
        ) :
            SwapIndex(
                familyName, // familyName
                tenor,  // tenor
                settlementDays, // settlementDays
                currency,	// currency
                OvernightIndex().fixingCalendar(),	// swap fixingCalendar =  overnight index's fixing calendar
                1 * Years, // fixedLegTenor, fixed leg tenor is also one year
                ModifiedFollowing, // fixedLegConvention, = ModifiedFollowing since all OIS swap's fixed leg convention is ModifiedFollowing
                OvernightIndex().dayCounter(), // fixedLegDaycounter = overnight index's day counter
                ext::shared_ptr<IborIndex>(
                    new OvernightCompoundedAverageInArrearsIndex<OvernightIndex>(
                        settlementDays, // fixingDays of the index matches with the settlementDays of the swap
                        indexEstimatingTermStructure
                    )
                )  // iborIndex 
            )
        {}
    };
}
