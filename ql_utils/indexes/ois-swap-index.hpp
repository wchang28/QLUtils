#pragma once

#include <ql/quantlib.hpp>
#include <string>
#include <sstream>
#include <tuple>
#include <ql_utils/fixing-date-adjustment.hpp>
#include <ql_utils/indexes/overnight-compounded-avg.hpp>

namespace QuantLib {
    // enhanced version of the OvernightIndexedSwapIndex that supports
    // cashflow frequency for both legs and custom fixing calendar
    // telescopicValueDates()
    // averagingMethod()
    // paymentLag()
    // paymentConvention()
    // paymentFrequency()
    // paymentCalendar()
    template <
        Frequency FREQ = Frequency::Annual  // cashflow frequency for both legs
    >
    class OvernightIndexedSwapIndexEnhanced : public OvernightIndexedSwapIndex {
    protected:
        Calendar customFixingCalendar_; // a fixing calendar that is different from the default (OvernightIndex's fixingCalendar)
        Natural paymentLag_;
        Calendar paymentCalendar_;
    private:
        static std::string makeFamilyName(
            const Currency& currency,
            const ext::shared_ptr<OvernightIndex>& overnightIndex
        ) {
            std::ostringstream oss;
            oss << currency.code();
            oss << "OvernightIndexedSwapIndex<<";
            oss << overnightIndex->name();
            oss << ",";
            oss << Period(FREQ);
            oss << ">>";
            return oss.str();
        }
    public:
        typedef Date FixingDate;
        typedef Date StartDate;
        typedef Date MaturityDate;
    public:
        OvernightIndexedSwapIndexEnhanced(
            const Period& tenor,
            Natural settlementDays,
            const Currency& currency,
            const ext::shared_ptr<OvernightIndex>& overnightIndex,
            Natural paymentLag = 0,
            const Calendar& paymentCalendar = Calendar(),
            const Calendar& fixingCalendar = Calendar()
        ) :
            OvernightIndexedSwapIndex(
                makeFamilyName(currency, overnightIndex),   // familyName
                tenor,                                      // swap tenor
                settlementDays,
                currency,
                overnightIndex,
                false,                                      // telescopicValueDates
                RateAveraging::Compound                     // averagingMethod
            ),
            paymentLag_(paymentLag),
            paymentCalendar_(paymentCalendar),
            customFixingCalendar_(fixingCalendar)
        {
            this->fixedLegTenor_ = Period(FREQ);    // set the cashflow tenor for both legs
        }
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
        QuantLib::Period paymentTenor() const {
            return fixedLegTenor();
        }
        const Calendar& paymentCalendar() const {
            return paymentCalendar_;
        }
        // fixing calendar for both legs (fixed leg and overnight leg)
        Calendar fixingCalendar() const override {
            return (customFixingCalendar_ == Calendar() ? InterestRateIndex::fixingCalendar() : customFixingCalendar_);
        }
        // end of month flags for both legs
        bool endOfMonth() const {
            return true;
        }
        BusinessDayConvention convention() const {
            return fixedLegConvention();
        }
        // is if p1 is a multiple of p2 ?
        static std::pair<bool, Integer> isMultiple(
            const Period& p1,
            const Period& p2
        ) {
            // Basic check: if p2 is longer than p1, p1 can't be a multiple of p2 (unless 0)
            // Note: QuantLib throws if comparison is undecidable (e.g. 1M vs 30D)
            try {
                if (p1 < p2) return { false, Null<Integer>()};
            }
            catch (...) {
                // Handle undecidable cases or return false/error
                return { false, Null<Integer>() };
            }

            // Example logic for matching units:
            if (p1.units() == p2.units()) {
                auto multiple = (p1.length() % p2.length() == 0);
                return { multiple, (multiple ? p1.length()/ p2.length() : Null<Integer>())};
            }

            // Convert Years to Months for comparison
            if ((p1.units() == Years || p1.units() == Months) &&
                (p2.units() == Years || p2.units() == Months)) {
                int p1_months = (p1.units() == Years) ? p1.length() * 12 : p1.length();
                int p2_months = (p2.units() == Years) ? p2.length() * 12 : p2.length();
                auto multiple = (p1_months % p2_months == 0);
                return { multiple, (multiple ? p1_months/ p2_months : Null<Integer>())};
            }

            // Convert Weeks to Days for comparison
            if ((p1.units() == Weeks || p1.units() == Days) &&
                (p2.units() == Weeks || p2.units() == Days)) {
                int p1_days = (p1.units() == Weeks) ? p1.length() * 7 : p1.length();
                int p2_days = (p2.units() == Weeks) ? p2.length() * 7 : p2.length();
                auto multiple = (p1_days % p2_days == 0);
                return { multiple, (multiple ? p1_days/ p2_days : Null<Integer>())};
            }

            return { false, Null<Integer>() }; // Units are incompatible (e.g., Months vs Days)
        }
        DateGeneration::Rule rule() const {
            auto swapTenor = tenor();
            auto couponTenor = paymentTenor();
            auto [multipile, n] = isMultiple(swapTenor, couponTenor);
            return (multipile ? DateGeneration::Rule::Forward : DateGeneration::Rule::Backward);
        }
        // make the correct adjustment to the swap fixing date so the it is a valid working date on the fixing calendar
        Date fixingDateAdj(
            Date d = Date()
        ) const {
            if (d == Date()) {
                d = Settings::instance().evaluationDate();
            }
            QuantLib::Utils::FixingDateAdjustment fixingAdj(this->fixingDays(), fixingCalendar());
            return fixingAdj.adjust(d);
        }
        Date maturityDate(
            const Date& valueDate
        ) const override {
            Date d = this->fixingDate(valueDate);
            Date fixingDate = fixingDateAdj(d);
            return underlyingSwap(fixingDate)->maturityDate();
        }
        // import dates (fixingDate, startDate, maturityDate) for the swap given a reference date
        std::tuple<FixingDate, StartDate, MaturityDate> getImportantDates(
            Date refDate = Date()
        ) const {
            Date fixingDate = fixingDateAdj(refDate);
            Date startDate = this->valueDate(fixingDate);
            Date maturityDate = underlyingSwap(fixingDate)->maturityDate();
            return std::tuple<FixingDate, StartDate, MaturityDate> {
                fixingDate,
                startDate,
                maturityDate
            };
        }
        ext::shared_ptr<OvernightIndexedSwap> makeSwap(
            const Date& fixingDate,
            Swap::Type type = Swap::Type::Payer,
            Rate fixedRate = Null<Rate>()
        ) const {
            QL_REQUIRE(fixingDate != Date(), "null fixing date");
            auto startDate = this->valueDate(fixingDate);
            ext::shared_ptr<OvernightIndexedSwap> swap = MakeOIS(tenor(), overnightIndex(), fixedRate)
                .withType(type)
                .withEffectiveDate(startDate)
                .withTelescopicValueDates(telescopicValueDates())
                .withAveragingMethod(averagingMethod())
                .withPaymentLag(paymentLag())
                .withPaymentAdjustment(paymentConvention())
                .withPaymentFrequency(paymentFrequency())
                .withPaymentCalendar(paymentCalendar())
                .withFixedLegCalendar(fixingCalendar())
                .withOvernightLegCalendar(fixingCalendar())
                .withRule(rule())
                ;
            return swap;
        }
        ext::shared_ptr<OvernightIndexedSwap> underlyingSwap(
            const Date& fixingDate
        ) const {
            QL_REQUIRE(fixingDate != Date(), "null fixing date");
            // caching mechanism
            if (lastFixingDate_ != fixingDate) {
                Rate fixedRate = 0.0;
                lastSwap_ = makeSwap(fixingDate, Swap::Type::Payer, fixedRate);
                lastFixingDate_ = fixingDate;
            }
            return lastSwap_;
        }
    };

    template <
        typename OVERNIGHTINDEX, // OvernightIndex can be Sofr, FedFunds, Sonia, Estr, Eonia
        Frequency FREQ = Frequency::Annual  // cashflow frequency for both legs
    >
    class OvernightIndexedSwapIndexEx : public OvernightIndexedSwapIndexEnhanced<FREQ> {
    public:
        typedef OVERNIGHTINDEX OvernightIndex;
    public:
        OvernightIndexedSwapIndexEx(
            const Period& tenor,
            Natural settlementDays,
            const Currency& currency,
            const Handle<YieldTermStructure>& indexEstimatingTermStructure = {},
            Natural paymentLag = 0,
            const Calendar& paymentCalendar = Calendar(),
            const Calendar& fixingCalendar = Calendar()
        ):
            OvernightIndexedSwapIndexEnhanced<FREQ>(
                tenor,
                settlementDays,
                currency,
                ext::shared_ptr<OvernightIndex>(new OVERNIGHTINDEX(indexEstimatingTermStructure)),  // overnightIndex
                paymentLag,
                paymentCalendar,
                fixingCalendar
            )
        {}
    };

    // enhanced version of SwapIndex (Vanilla swap) thats support end of month for both legs
    // this is needed because OIS-based vanilla swaps (such as term sofr swaps) have endOfMonth=true for both legs
    // this class also ensure fixed leg calendar and floating leg calendar are the same
    class SwapIndexEx : public SwapIndex {
    protected:
        bool endOfMonth_;   // end of month flag for both legs
    public:
        typedef Date FixingDate;
        typedef Date StartDate;
        typedef Date MaturityDate;
    public:
        SwapIndexEx(
            const std::string& familyName,
            const Period& tenor,
            Natural settlementDays,
            const Currency& currency,
            const Calendar& fixingCalendar, // fixing calendar for both legs
            const Period& fixedLegTenor,
            BusinessDayConvention fixedLegConvention,
            const DayCounter& fixedLegDayCounter,
            ext::shared_ptr<IborIndex> iborIndex,
            bool endOfMonth = false // for vanilla swap, the default end of month is false
        ) : SwapIndex(
                familyName,
                tenor,
                settlementDays,
                currency,
                fixingCalendar,
                fixedLegTenor,
                fixedLegConvention,
                fixedLegDayCounter,
                iborIndex
            ),
            endOfMonth_(endOfMonth)
        {}
        SwapIndexEx(
            const std::string& familyName,
            const Period& tenor,
            Natural settlementDays,
            const Currency& currency,
            const Calendar& fixingCalendar,  // fixing calendar for both legs
            const Period& fixedLegTenor,
            BusinessDayConvention fixedLegConvention,
            const DayCounter& fixedLegDayCounter,
            ext::shared_ptr<IborIndex> iborIndex,
            Handle<YieldTermStructure> discountingTermStructure,
            bool endOfMonth = false // for vanilla swap, the default end of month is false
        ): SwapIndex(
                familyName,
                tenor,
                settlementDays,
                currency,
                fixingCalendar,
                fixedLegTenor,
                fixedLegConvention,
                fixedLegDayCounter,
                iborIndex,
                discountingTermStructure
            ),
            endOfMonth_(endOfMonth)
        {}
        bool endOfMonth() const {
            return endOfMonth_;
        }
        ext::shared_ptr<VanillaSwap> makeSwap(
            const Date& fixingDate,
            Swap::Type type = Swap::Type::Payer,
            Rate fixedRate = Null<Rate>()
        ) const {
            QL_REQUIRE(fixingDate != Date(), "null fixing date");
            auto startDate = this->valueDate(fixingDate);
            ext::shared_ptr<VanillaSwap> swap;
            if (exogenousDiscount())
                swap = MakeVanillaSwap(tenor(), iborIndex(), fixedRate)
                .withType(type)
                .withEffectiveDate(startDate)
                .withFixedLegCalendar(fixingCalendar())
                .withFixedLegDayCount(dayCounter()) // dayCounter() returns the fixed leg day counter
                .withFixedLegTenor(fixedLegTenor())
                .withFixedLegConvention(fixedLegConvention())
                .withFixedLegTerminationDateConvention(fixedLegConvention())
                .withFixedLegEndOfMonth(endOfMonth())
                .withFloatingLegCalendar(fixingCalendar())
                .withFloatingLegEndOfMonth(endOfMonth())
                .withDiscountingTermStructure(discountingTermStructure())
                ;
            else
                swap = MakeVanillaSwap(tenor(), iborIndex(), fixedRate)
                .withType(type)
                .withEffectiveDate(startDate)
                .withFixedLegCalendar(fixingCalendar())
                .withFixedLegDayCount(dayCounter()) // dayCounter() returns the fixed leg day counter
                .withFixedLegTenor(fixedLegTenor())
                .withFixedLegConvention(fixedLegConvention())
                .withFixedLegTerminationDateConvention(fixedLegConvention())
                .withFixedLegEndOfMonth(endOfMonth())
                .withFloatingLegCalendar(fixingCalendar())
                .withFloatingLegEndOfMonth(endOfMonth())
                ;
            return swap;
        }
        ext::shared_ptr<VanillaSwap> underlyingSwap(
            const Date& fixingDate
        ) const {
            QL_REQUIRE(fixingDate != Date(), "null fixing date");
            // caching mechanism
            if (lastFixingDate_ != fixingDate) {
                Rate fixedRate = 0.0;
                lastSwap_ = makeSwap(fixingDate, Swap::Type::Payer, fixedRate);
                lastFixingDate_ = fixingDate;
            }
            return lastSwap_;
        }
        // make the correct adjustment to the swap fixing date so the it is a valid working date on the fixing calendar
        Date fixingDateAdj(
            Date d = Date()
        ) const {
            if (d == Date()) {
                d = Settings::instance().evaluationDate();
            }
            QuantLib::Utils::FixingDateAdjustment fixingAdj(fixingDays(), fixingCalendar());
            return fixingAdj.adjust(d);
        }
        Date maturityDate(
            const Date& valueDate
        ) const override {
            Date d = this->fixingDate(valueDate);
            Date fixingDate = fixingDateAdj(d);
            return underlyingSwap(fixingDate)->maturityDate();
        }
        // import dates (fixingDate, startDate, maturityDate) for the swap given a reference date
        std::tuple<FixingDate, StartDate, MaturityDate> getImportantDates(
            Date refDate = Date()
        ) const {
            Date fixingDate = fixingDateAdj(refDate);
            Date startDate = this->valueDate(fixingDate);
            Date maturityDate = underlyingSwap(fixingDate)->maturityDate();
            return std::tuple<FixingDate, StartDate, MaturityDate> {
                fixingDate,
                startDate,
                maturityDate
            };
        }
    };

    template <
        typename OVERNIGHTINDEX, // OvernightIndex can be Sofr, FedFunds, Sonia, Estr, Eonia
        Frequency FREQ = Frequency::Annual  // cashflow frequency for both legs
    >
    class FwdOISVanillaSwapIndex : public SwapIndexEx {
    public:
        typedef OVERNIGHTINDEX OvernightIndex;
        typedef OvernightCompoundedAverageInArrearsIndex<OvernightIndex, FREQ> IndexType;
    public:
        // both legs' cashflow frequency
        static Frequency legsFrequency() {
            return FREQ;
        }
        // both legs' cashflow tenor
        static Period legsTenor() {
            return Period(legsFrequency());
        }
    private:
        static std::string makeFamilyName(
            const Currency& currency,
            Natural fixingDays  // index fixing days
        ) {
            IndexType index(fixingDays);
            std::ostringstream oss;
            oss << currency.code();
            oss << "FwdOISVanillaSwapIndex";
            oss << "(" << legsFrequency() << ")";
            oss << "<<" << index.name() << ">>";
            return oss.str();
        }
    public:
        FwdOISVanillaSwapIndex(
            const Period& tenor,        // tenor of the swap
            Natural settlementDays,     // number of days required to settle the swap
            const Handle<YieldTermStructure>& h = {} // index estimating term structure
        ) :
            SwapIndexEx(
                makeFamilyName(OvernightIndex().currency(), settlementDays), // familyName
                tenor,  // tenor
                settlementDays, // settlementDays
                OvernightIndex().currency(),	// currency = overnight index's currency
                OvernightIndex().fixingCalendar(),	// swap fixingCalendar =  overnight index's fixing calendar
                legsTenor(), // fixedLegTenor, fixed leg tenor is usually one year
                ModifiedFollowing, // fixedLegConvention, = ModifiedFollowing since all OIS swap's fixed leg convention is ModifiedFollowing
                OvernightIndex().dayCounter(), // fixedLegDaycounter = overnight index's day counter
                ext::shared_ptr<IborIndex>(
                    new IndexType(
                        settlementDays, // fixingDays of the index matches with the settlementDays of the swap
                        h
                    )
                ),  // iborIndex
                true   // endOfMonth, OIS-based vanilla/overnight swaps usually have endOfMonth=true for both legs
            )
        {}
    };
}
