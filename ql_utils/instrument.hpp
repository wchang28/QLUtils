#pragma once

#include <string>
#include <ql/quantlib.hpp>
#include <ql_utils/types.hpp>
#include <ql_utils/bondschedulerwoissuedt.hpp>

namespace QLUtils {
    class BootstrapInstrument {
    public:
        enum ValueType {
            Rate = 0,
            Price = 1,
        };
    protected:
        std::string ticker_;
        QuantLib::Period tenor_;
        QuantLib::Date datedDate_;
        ValueType valueType_;
        QuantLib::Real value_;
        bool use_;
    public:
        BootstrapInstrument(
            const ValueType& valueType,
            const QuantLib::Period& tenor = QuantLib::Period(),
            const QuantLib::Date& datedDate = QuantLib::Date()
        ) :
            valueType_(valueType),
            tenor_(tenor),
            datedDate_(datedDate),
            value_(QuantLib::Null < QuantLib::Real >()),
            use_(true) {}
        const std::string& ticker() const {
            return ticker_;
        }
        std::string& ticker() {
            return ticker_;
        }
        const QuantLib::Period& tenor() const {
            return tenor_;
        }
        QuantLib::Period& tenor() {
            return tenor_;
        }
        const QuantLib::Date& datedDate() const {
            return datedDate_;
        }
        QuantLib::Date& datedDate() {
            return datedDate_;
        }
        const ValueType& valueType() const {
            return valueType_;
        }
        ValueType& valueType() {
            return valueType_;
        }
        const QuantLib::Real& value() const {
            return value_;
        }
        QuantLib::Real& value() {
            return value_;
        }
        const bool& use() const {
            return use_;
        }
        bool& use() {
            return use_;
        }
        const QuantLib::Rate& rate() const {
            return value_;
        }
        QuantLib::Rate& rate() {
            return value_;
        }
        const QuantLib::Real& price() const {
            return value_;
        }
        QuantLib::Real& price() {
            return value_;
        }
        QuantLib::Handle<QuantLib::Quote> quote() const {
            QuantLib::Handle<QuantLib::Quote> q(QuantLib::ext::shared_ptr<QuantLib::Quote>(new QuantLib::SimpleQuote(value())));
            return q;
        }
        virtual QuantLib::Date startDate() const = 0;
        virtual QuantLib::Date maturityDate() const = 0;
        virtual QuantLib::ext::shared_ptr<QuantLib::RateHelper> rateHelper(
            const QuantLib::Handle<QuantLib::YieldTermStructure>& discounteringTermStructure = QuantLib::Handle<QuantLib::YieldTermStructure>()
        ) const = 0;
        virtual QuantLib::Real impliedQuote(
            const QuantLib::Handle<QuantLib::YieldTermStructure>& estimatingTermStructure,
            const QuantLib::Handle<QuantLib::YieldTermStructure>& discounteringTermStructure = QuantLib::Handle<QuantLib::YieldTermStructure>()
        ) const = 0;

        static QuantLib::Rate simpleForwardRate(const QuantLib::Date& start, const QuantLib::Date& end, const QuantLib::DayCounter& dayCounter, const QuantLib::Handle<QuantLib::YieldTermStructure>& h) {
            auto t = dayCounter.yearFraction(start, end);
            auto compounding = h->discount(start) / h->discount(end);
            return (compounding - 1.0) / t;
        }
    };

    class ParInstrument : public BootstrapInstrument {
    public:
        ParInstrument(
            const QuantLib::Period& tenor
        ) : BootstrapInstrument(BootstrapInstrument::Rate, tenor) {}
    protected:
        virtual QuantLib::ext::shared_ptr<QuantLib::FixedRateBondHelper> fixRateBondHelper() const = 0;
        virtual QuantLib::Rate impliedParRate(
            const QuantLib::Handle<QuantLib::YieldTermStructure>& discounteringTermStructure
        ) const = 0;
        QuantLib::ext::shared_ptr<QuantLib::Bond> parBond() const {
            return fixRateBondHelper()->bond();
        }
    public:
        const QuantLib::Rate& parRate() const {
            return rate();
        }
        QuantLib::Rate& parRate() {
            return rate();
        }
        QuantLib::Date startDate() const {
            return parBond()->settlementDate();
        }
        QuantLib::Date maturityDate() const {
            return parBond()->maturityDate();
        }
        QuantLib::ext::shared_ptr<QuantLib::RateHelper> rateHelper(
            const QuantLib::Handle<QuantLib::YieldTermStructure>& discounteringTermStructure = QuantLib::Handle<QuantLib::YieldTermStructure>()
        ) const {
            return fixRateBondHelper();
        };
        QuantLib::Real impliedQuote(
            const QuantLib::Handle<QuantLib::YieldTermStructure>& estimatingTermStructure,
            const QuantLib::Handle<QuantLib::YieldTermStructure>& discounteringTermStructure = QuantLib::Handle<QuantLib::YieldTermStructure>()
        ) const {
            return impliedParRate(discounteringTermStructure);
        }
    };

    template <QuantLib::Frequency COUPON_FREQ = QuantLib::Semiannual>
    class ParRate : public ParInstrument {
    private:
        QuantLib::Date baseReferenceDate_;
    public:
        ParRate(
            const QuantLib::Period& tenor,
            const QuantLib::Date& baseReferenceDate
        ) : ParInstrument(tenor), baseReferenceDate_(baseReferenceDate) {}
    public:
        QuantLib::Date& baseReferenceDate() {
            return baseReferenceDate_;
        }
        const QuantLib::Date& baseReferenceDate() const {
            return baseReferenceDate_;
        }
    protected:
        QuantLib::ext::shared_ptr<QuantLib::FixedRateBondHelper> fixRateBondHelper() const {
            QuantLib::ext::shared_ptr<QuantLib::FixedRateBondHelper> helper = ParYieldHelper<COUPON_FREQ>(tenor())
                .withParYield(parRate())
                .withBaseReferenceDate(baseReferenceDate());
            return helper;
        }
        QuantLib::Rate impliedParRate(
            const QuantLib::Handle<QuantLib::YieldTermStructure>& discounteringTermStructure
        ) const {
            QL_ASSERT(discounteringTermStructure->referenceDate() == baseReferenceDate(), "discount curve base reference date (" << discounteringTermStructure->referenceDate() << ") is not what's expected (" << baseReferenceDate() << ")");
            return ParYieldHelper<COUPON_FREQ>::parYield(discounteringTermStructure.currentLink(), tenor());
        };
    };

    // BondTraits
    // settlementCalendar(tenor)
    // settlementDays(tenor)
    // isZeroCoupon(tenor)
    // bondCouponFrequency(tenor)
    // dayCounter(tenor)
    // bondEndOfMonth(tenoor)
    // bondScheduleCalendar(tenor)
    // bondConvention(tenor)
    // bondTerminationDateConvention(tenor)
    template <typename BondTraits>
    class ParBond : public ParInstrument {
    private:
        QuantLib::Date bondMaturityDate_;
        BondTraits bondTraits;
    public:
        ParBond(
            const QuantLib::Period& tenor,
            const QuantLib::Date& bondMaturityDate
        ) : ParInstrument(tenor), bondMaturityDate_(bondMaturityDate) {}
    public:
        const QuantLib::Date& bondMaturityDate() const {
            return bondMaturityDate_;
        }
        QuantLib::Calendar settlementCalendar() const {
            return bondTraits.settlementCalendar(tenor());
        }
        QuantLib::Natural settlementDays() const {
            return bondTraits.settlementDays(tenor());
        }
        QuantLib::Date settlementDate() const {
            auto calendar = settlementCalendar();
            QuantLib::Date today = QuantLib::Settings::instance().evaluationDate();
            auto d = calendar.adjust(today);
            return calendar.advance(d, settlementDays() * QuantLib::Days);
        }
        bool isZeroCoupon() const {
            return bondTraits.isZeroCoupon(tenor());
        }
        QuantLib::Frequency bondCouponFrequency() const {
            return bondTraits.bondCouponFrequency(tenor());
        }
        QuantLib::Time couponInterval() const {
            return 1.0 / bondCouponFrequency();
        }
        QuantLib::DayCounter dayCounter() const {
            return bondTraits.dayCounter(tenor());
        }
        bool bondEndOfMonth() const {
            return bondTraits.bondEndOfMonth(tenor());
        }
        QuantLib::Calendar bondScheduleCalendar() const {
            return bondTraits.bondScheduleCalendar(tenor());
        }
        QuantLib::BusinessDayConvention bondConvention() const {
            return bondTraits.bondConvention(tenor());
        }
        QuantLib::BusinessDayConvention bondTerminationDateConvention() const {
            return bondTraits.bondTerminationDateConvention(tenor());
        }
        QuantLib::Schedule bondSchedule() {
            BondSechdulerWithoutIssueDate scheduler(settlementDays(), settlementCalendar(), bondCouponFrequency(), bondEndOfMonth(), bondScheduleCalendar(), bondConvention(), bondTerminationDateConvention());
            return scheduler(bondMaturityDate());
        }
        QuantLib::Rate bey(QuantLib::Real price, QuantLib::Rate coupon = 0.0) {
            if (isZeroCoupon()) {
                auto notional = 100.0;
                auto df = price / notional;
                auto d1 = settlementDate();
                auto d2 = bondMaturityDate();
                auto t = dayCounter().yearFraction(d1, d2);
                auto firstCouponTime = couponInterval();
                auto simpleCompounding = (t <= firstCouponTime);
                auto comp = (simpleCompounding ? QuantLib::Simple : QuantLib::Compounded);
                auto freq = (simpleCompounding ? QuantLib::NoFrequency : bondCouponFrequency());

                auto compond = 1.0 / df;
                auto ir = QuantLib::InterestRate::impliedRate(compond, dayCounter, comp, freq, d1, d2);
                return ir.rate();
            }
            else {
                // TODO:
                return 0.0;
            }
        }
    protected:
        QuantLib::ext::shared_ptr<QuantLib::FixedRateBondHelper> fixRateBondHelper() const {
            auto schedule = bondSchedule();
            auto notional = 100.0;
            std::vector<QuantLib::Rate> coupons;
            QuantLib::Real targetPrice = QuantLib::Null<QuantLib::Real>();    // target price
            if (isZeroCoupon()) {
                auto d1 = settlementDate();
                auto d2 = bondMaturityDate();
                auto t = dayCounter().yearFraction(d1, d2);
                auto firstCouponTime = couponInterval();
                auto simpleCompounding = (t <= firstCouponTime);
                auto comp = (simpleCompounding ? QuantLib::Simple : QuantLib::Compounded);
                auto freq = (simpleCompounding ? QuantLib::NoFrequency : bondCouponFrequency());

                QuantLib::InterestRate ir(parRate(), dayCounter, comp, freq);
                auto df = ir.discountFactor(d1, d2);
                targetPrice = notional * df;
                coupons.resize(1, 0.0);
            }
            else {
                targetPrice = notional;
                coupons.resize(1, parRate());
            }
            auto quote = QuantLib::ext::make_shared<QuantLib::SimpleQuote>(targetPrice);    // make target price the quote
            return QuantLib::ext::shared_ptr<QuantLib::FixedRateBondHelper>(new QuantLib::FixedRateBondHelper(
                QuantLib::Handle<QuantLib::Quote>(quote),
                settlementDays(),
                notional,
                schedule,
                coupons,
                dayCounter(),
                bondConvention(),
                notional
            ));
        }
        QuantLib::Rate impliedParRate(
            const QuantLib::Handle<QuantLib::YieldTermStructure>& discounteringTermStructure
        ) const {
            auto notional = 100.0;
            if (isZeroCoupon()) {
                auto d1 = settlementDate();
                auto d2 = bondMaturityDate();
                auto df = discounteringTermStructure->discount(d2)/ discounteringTermStructure->discount(d1);
                auto price = df * notional;
                return bey(price);
            }
            else {
                auto schedule = bondSchedule();
                QuantLib::Brent solver;
                QuantLib::Real accuracy = 1.0e-12;
                QuantLib::Rate guess = 0.05;
                QuantLib::Rate step = 0.01;
                auto parRate = solver.solve([&discounteringTermStructure, &schedule, &notional](const QuantLib::Rate& coupon) -> QuantLib::Real {
                    QuantLib::FixedRateBond bond(settlementDays(), notional, schedule, std::vector<QuantLib::Rate>{coupon}, bondConvention(), notional);
                    bond.setPricingEngine(new QuantLib::DiscountingBondEngine(discounteringTermStructure));
                    return bond.cleanPrice() - notional;
                }, accuracy, guess, step);
                return parRate;
            }
        };
    };

    class IborIndexInstrument : public BootstrapInstrument {
    protected:
        IborIndexFactory iborIndexFactory_;
    public:
        IborIndexInstrument(
            const IborIndexFactory& iborIndexFactory,
            const BootstrapInstrument::ValueType& valueType,
            const QuantLib::Period& tenor = QuantLib::Period(),
            const QuantLib::Date& datedDate = QuantLib::Date()
        ) : BootstrapInstrument(valueType, tenor, datedDate), iborIndexFactory_(iborIndexFactory) {}
        const IborIndexFactory& iborIndexFactory() const {
            return iborIndexFactory_;
        }
        IborIndexFactory& iborIndexFactory() {
            return iborIndexFactory_;
        }
        QuantLib::ext::shared_ptr<QuantLib::IborIndex> iborIndex(const QuantLib::Handle<QuantLib::YieldTermStructure>& h = QuantLib::Handle<QuantLib::YieldTermStructure>()) const {
            return iborIndexFactory()(h);
        }
    };

    template<typename SwapTraits>
    class OISSwapIndex: public IborIndexInstrument {
    private:
        SwapTraits swapTraits_;
    public:
        OISSwapIndex(
            const IborIndexFactory& iborIndexFactory,
            const QuantLib::Period& tenor
        ) : IborIndexInstrument(iborIndexFactory, BootstrapInstrument::Rate, tenor) {}
    private:
        static QuantLib::ext::shared_ptr<QuantLib::OvernightIndex> castToOvernightIndex(
            const QuantLib::ext::shared_ptr<QuantLib::IborIndex>& iborIndex
        ) {
            return QuantLib::ext::dynamic_pointer_cast<QuantLib::OvernightIndex>(iborIndex);
        }
        QuantLib::ext::shared_ptr<QuantLib::OvernightIndexedSwap> createSwap(
            const QuantLib::Handle<QuantLib::YieldTermStructure>& estimatingTermStructure = QuantLib::Handle<QuantLib::YieldTermStructure>()
        ) const {
            auto overnightIndex = castToOvernightIndex(this->iborIndex(estimatingTermStructure));
            QuantLib::ext::shared_ptr<QuantLib::OvernightIndexedSwap> swap = QuantLib::MakeOIS(tenor(), overnightIndex, 0.0)
                .withSettlementDays(swapTraits_.settlementDays(tenor()))
                .withTelescopicValueDates(swapTraits_.telescopicValueDates(tenor()))
                .withPaymentAdjustment(swapTraits_.paymentAdjustment(tenor()))
                .withAveragingMethod(swapTraits_.averagingMethod(tenor()));
            return swap;
        }
        QuantLib::Date startDate() const {
            return createSwap()->startDate();
        }
        QuantLib::Date maturityDate() const {
            return createSwap()->maturityDate();
        }
        QuantLib::ext::shared_ptr<QuantLib::RateHelper> rateHelper(
            const QuantLib::Handle<QuantLib::YieldTermStructure>& discounteringTermStructure = QuantLib::Handle<QuantLib::YieldTermStructure>()
        ) const {
            auto overnightIndex = castToOvernightIndex(this->iborIndex());
            QuantLib::ext::shared_ptr<QuantLib::OISRateHelper> helper(
                new QuantLib::OISRateHelper(
                    swapTraits_.settlementDays(tenor()),
                    tenor(),
                    quote(),
                    overnightIndex,
                    discounteringTermStructure,   // exogenous discounting curve
                    swapTraits_.telescopicValueDates(tenor()),
                    0,
                    swapTraits_.paymentAdjustment(tenor()),
                    QuantLib::Annual,
                    QuantLib::Calendar(),
                    0 * QuantLib::Days,
                    0.0,
                    QuantLib::Pillar::LastRelevantDate,
                    QuantLib::Date(),
                    swapTraits_.averagingMethod(tenor())
                )
            );
            return helper;
        }
        QuantLib::Real impliedQuote(
            const QuantLib::Handle<QuantLib::YieldTermStructure>& estimatingTermStructure,
            const QuantLib::Handle<QuantLib::YieldTermStructure>& discounteringTermStructure = QuantLib::Handle<QuantLib::YieldTermStructure>()
        ) const {
            QuantLib::ext::shared_ptr<QuantLib::PricingEngine> swapPricingEngine(new QuantLib::DiscountingSwapEngine(discounteringTermStructure));
            auto swap = createSwap(estimatingTermStructure);
            swap->setPricingEngine(swapPricingEngine);
            auto swapRate = swap->fairRate();
            return swapRate;
        }
    };

    class SwapCurveInstrument : public IborIndexInstrument {
    public:
        enum InstType {
            Deposit = 0,
            Future = 1,
            FRA = 2,
            Swap = 3,
        };
    protected:
        InstType instType_;
    public:
        SwapCurveInstrument(
            const IborIndexFactory& iborIndexFactory,
            const BootstrapInstrument::ValueType& valueType,
            const InstType& instType,
            const QuantLib::Period& tenor = QuantLib::Period(),
            const QuantLib::Date& datedDate = QuantLib::Date()
        ) : IborIndexInstrument(iborIndexFactory, valueType, tenor, datedDate), instType_(instType)
        {}
        const InstType& instType() const {
            return instType_;
        }
        InstType& instType() {
            return instType_;
        }
    };

    class IMMFuture : public SwapCurveInstrument {
    protected:
        QuantLib::Natural immOrdinal_;
        std::string immTicker_;
        QuantLib::Rate convexityAdj_;
    public:
        static void ensureIMMDate(const QuantLib::Date& immDate) {
            QL_REQUIRE(QuantLib::IMM::isIMMdate(immDate, true), "specified date " << immDate << " is not a main cycle IMM date");
        }
        static void ensureIMMDateNotExpired(
            const QuantLib::Date& immDate,
            const QuantLib::Date& refDate
        ) {
            QL_REQUIRE(immDate >= refDate, "IMM date " << immDate << " already expired");
        }
        static QuantLib::Date IMMMainCycleStartDateForOrdinal(
            QuantLib::Natural iMMOrdinal,
            const QuantLib::Date& today = QuantLib::Date()
        ) {
            QL_REQUIRE(iMMOrdinal > 0, "IMM ordinal must be an integer greater than 0");
            QuantLib::Date d = (today == QuantLib::Date() ? QuantLib::Settings::instance().evaluationDate() : today);
            d += 2;
            if (!QuantLib::IMM::isIMMdate(d, true)) {
                d = QuantLib::IMM::nextDate(d, true);
            }
            // d is now the first IMM date after today
            QuantLib::Natural immFound = 1;
            while (immFound < iMMOrdinal) {
                d = QuantLib::IMM::nextDate(d, true);
                immFound++;
            }
            return d;
        }
        static QuantLib::Natural IMMMainCycleOrdinalForStartDate(
            const QuantLib::Date& immDate,
            const QuantLib::Date& today = QuantLib::Date()
        ) {
            ensureIMMDate(immDate);
            QuantLib::Date d = (today == QuantLib::Date() ? QuantLib::Settings::instance().evaluationDate() : today);
            d += 2;
            if (!QuantLib::IMM::isIMMdate(d, true)) {
                d = QuantLib::IMM::nextDate(d, true);
            }
            // d is now the first IMM date after today
            ensureIMMDateNotExpired(immDate, d);
            QuantLib::Natural ordinal = 1;
            while (d < immDate) {
                d = QuantLib::IMM::nextDate(d, true);
                ordinal++;
            }
            return ordinal;
        }
        static QuantLib::Period calculateTenor(
            const QuantLib::Date& immDate,
            const QuantLib::Date& today = QuantLib::Date()
        ) {
            ensureIMMDate(immDate);
            QuantLib::Date d = (today == QuantLib::Date() ? QuantLib::Settings::instance().evaluationDate() : today);
            d += 2;
            ensureIMMDateNotExpired(immDate, d);
            auto days = immDate - d;
            return days * QuantLib::Days;
        }
        IMMFuture(const IborIndexFactory& iborIndexFactory, QuantLib::Natural immOrdinal) :
            SwapCurveInstrument(iborIndexFactory, BootstrapInstrument::Price, SwapCurveInstrument::Future, QuantLib::Period(), QuantLib::Null < QuantLib::Date >()),
            immOrdinal_(immOrdinal), convexityAdj_(0.0) {
            this->datedDate_ = IMMMainCycleStartDateForOrdinal(immOrdinal);
            this->tenor_ = calculateTenor(this->datedDate_);
            this->immTicker_ = QuantLib::IMM::code(this->datedDate_);
        }
        IMMFuture(const IborIndexFactory& iborIndexFactory, const QuantLib::Date& immDate) :
            SwapCurveInstrument(iborIndexFactory, BootstrapInstrument::Price, SwapCurveInstrument::Future, calculateTenor(immDate), immDate),
            immOrdinal_(IMMMainCycleOrdinalForStartDate(immDate)),
            immTicker_(QuantLib::IMM::code(immDate)),
            convexityAdj_(0.0){}
        const QuantLib::Date& immDate() const {
            return this->datedDate_;
        }
        const QuantLib::Natural& immOrdinal() const {
            return immOrdinal_;
        }
        const std::string& immTicker() const {
            return immTicker_;
        }
        const QuantLib::Rate& convexityAdj() const {
            return convexityAdj_;
        }
        QuantLib::Rate& convexityAdj() {
            return convexityAdj_;
        }
        QuantLib::Date immEndDate() const {
            return QuantLib::IMM::nextDate(immDate(), true);
        }
        QuantLib::Date startDate() const {
            return immDate();
        }
        QuantLib::Date maturityDate() const {
            return immEndDate();
        }
        QuantLib::ext::shared_ptr<QuantLib::RateHelper> rateHelper(
            const QuantLib::Handle<QuantLib::YieldTermStructure>& discounteringTermStructure = QuantLib::Handle<QuantLib::YieldTermStructure>()
        ) const {
            auto iborIndex = this->iborIndex();
            QuantLib::Handle<QuantLib::Quote> convAdjQuote(QuantLib::ext::shared_ptr<QuantLib::Quote>(new QuantLib::SimpleQuote(convexityAdj())));
            QuantLib::ext::shared_ptr<QuantLib::FuturesRateHelper> helper(
                new QuantLib::FuturesRateHelper(
                    quote(),
                    immDate(),
                    QuantLib::Date(),
                    iborIndex->dayCounter(),
                    convAdjQuote
                )
            );
            return helper;
        }
        QuantLib::Real impliedQuote(
            const QuantLib::Handle<QuantLib::YieldTermStructure>& estimatingTermStructure,
            const QuantLib::Handle<QuantLib::YieldTermStructure>& discounteringTermStructure = QuantLib::Handle<QuantLib::YieldTermStructure>()
        ) const {
            auto iborIndex = this->iborIndex();
            auto forwardRate = this->simpleForwardRate(immDate(), immEndDate(), iborIndex->dayCounter(), estimatingTermStructure);
            auto const& convAdj = convexityAdj();
            auto futureRate = forwardRate + convAdj;
            return 100.0 * (1.0 - futureRate);
        }
    };

    class CashDepositIndex : public SwapCurveInstrument {
    public:
        CashDepositIndex(
            const IborIndexFactory& iborIndexFactory,
            const QuantLib::Period& tenor
        ) : SwapCurveInstrument(iborIndexFactory, BootstrapInstrument::Rate, SwapCurveInstrument::Deposit, tenor) {}
    private:
        std::tuple<QuantLib::Date, QuantLib::Date, QuantLib::DayCounter> getValueMaturityDates() const {
            auto iborIndex = this->iborIndex();
            auto fixingDate = iborIndex->fixingCalendar().adjust(QuantLib::Settings::instance().evaluationDate());
            auto valueDate = iborIndex->valueDate(fixingDate);
            auto maturityDate = iborIndex->maturityDate(valueDate);
            return std::tuple<QuantLib::Date, QuantLib::Date, QuantLib::DayCounter>(valueDate, maturityDate, iborIndex->dayCounter());
        }
    public:
        QuantLib::Date startDate() const {
            return std::get<0>(getValueMaturityDates());
        }
        QuantLib::Date maturityDate() const {
            return std::get<1>(getValueMaturityDates());
        }
        QuantLib::ext::shared_ptr<QuantLib::RateHelper> rateHelper(
            const QuantLib::Handle<QuantLib::YieldTermStructure>& discounteringTermStructure = QuantLib::Handle<QuantLib::YieldTermStructure>()
        ) const {
            auto iborIndex = this->iborIndex();
            QuantLib::ext::shared_ptr<QuantLib::DepositRateHelper> helper(
                new QuantLib::DepositRateHelper(
                    quote(),
                    iborIndex
                )
            );
            return helper;
        }
        QuantLib::Real impliedQuote(
            const QuantLib::Handle<QuantLib::YieldTermStructure>& estimatingTermStructure,
            const QuantLib::Handle<QuantLib::YieldTermStructure>& discounteringTermStructure = QuantLib::Handle<QuantLib::YieldTermStructure>()
        ) const {
            auto result = getValueMaturityDates();
            auto const& start = std::get<0>(result);
            auto const& end = std::get<1>(result);
            auto const& dayCounter = std::get<2>(result);
            auto rate = this->simpleForwardRate(start, end, dayCounter, estimatingTermStructure);
            return rate;
        }
    };
    /* TODO:
    class ForwardRateAgreement : public SwapCurveInstrument<_Elem> {
    public:
        ForwardRateAgreement(const IborIndexFactory& iborIndexFactory, const QuantLib::Period& tenor) : SwapCurveInstrument(iborIndexFactory, BootstrapInstrument::Rate, SwapCurveInstrument::FRA, tenor) {}
        QuantLib::Date startDate() const {
            return immDate();
        }
        QuantLib::Date maturityDate() const {
            return immEndDate();
        }
    };
    */

    template<typename SwapTraits>
    class SwapIndex : public SwapCurveInstrument {
    private:
        SwapTraits swapTraits_;
    public:
        SwapIndex(
            const IborIndexFactory& iborIndexFactory,
            const QuantLib::Period& tenor
        ) : SwapCurveInstrument(iborIndexFactory, BootstrapInstrument::Rate, SwapCurveInstrument::Swap, tenor) {}
    private:
        QuantLib::ext::shared_ptr<QuantLib::VanillaSwap> createSwap(
            const QuantLib::Handle<QuantLib::YieldTermStructure>& estimatingTermStructure = QuantLib::Handle<QuantLib::YieldTermStructure>()
        ) const {
            auto iborIndex = this->iborIndex(estimatingTermStructure);
            QuantLib::ext::shared_ptr<QuantLib::VanillaSwap> swap = QuantLib::MakeVanillaSwap(this->tenor(), iborIndex, 0.0)
                .withSettlementDays(swapTraits_.settlementDays(tenor()))
                .withFixedLegCalendar(swapTraits_.fixingCalendar(tenor()))
                .withFixedLegTenor(swapTraits_.fixedLegTenor(tenor()))
                .withFixedLegConvention(swapTraits_.fixedLegConvention(tenor()))
                .withFixedLegDayCount(swapTraits_.fixedLegDayCount(tenor()))
                .withFixedLegEndOfMonth(swapTraits_.endOfMonth(tenor()))
                .withFloatingLegCalendar(swapTraits_.fixingCalendar(tenor()))
                .withFloatingLegEndOfMonth(swapTraits_.endOfMonth(tenor()));
            return swap;
        }
        QuantLib::Date startDate() const {
            return createSwap()->startDate();
        }
        QuantLib::Date maturityDate() const {
            return createSwap()->maturityDate();
        }
        QuantLib::ext::shared_ptr<QuantLib::RateHelper> rateHelper(
            const QuantLib::Handle<QuantLib::YieldTermStructure>& discounteringTermStructure = QuantLib::Handle<QuantLib::YieldTermStructure>()
        ) const {
            auto iborIndex = this->iborIndex();
            QuantLib::ext::shared_ptr<QuantLib::SwapRateHelper> helper(
                new QuantLib::SwapRateHelper(
                    quote(),
                    tenor(),
                    swapTraits_.fixingCalendar(tenor()),
                    swapTraits_.fixedLegFrequency(tenor()),
                    swapTraits_.fixedLegConvention(tenor()),
                    swapTraits_.fixedLegDayCount(tenor()),
                    iborIndex,
                    QuantLib::Handle<QuantLib::Quote>(),
                    0 * QuantLib::Days,
                    discounteringTermStructure,
                    swapTraits_.settlementDays(tenor()),
                    QuantLib::Pillar::LastRelevantDate,
                    QuantLib::Date(),
                    swapTraits_.endOfMonth(tenor())
                )
            );
            return helper;
        }
        QuantLib::Real impliedQuote(const QuantLib::Handle<QuantLib::YieldTermStructure>& estimatingTermStructure, const QuantLib::Handle<QuantLib::YieldTermStructure>& discounteringTermStructure = QuantLib::Handle<QuantLib::YieldTermStructure>()) const {
            QuantLib::ext::shared_ptr<QuantLib::PricingEngine> swapPricingEngine(new QuantLib::DiscountingSwapEngine(discounteringTermStructure));
            auto swap = createSwap(estimatingTermStructure);
            swap->setPricingEngine(swapPricingEngine);
            auto swapRate = swap->fairRate();
            return swapRate;
        }
    };
}
