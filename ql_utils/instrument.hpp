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
            const QuantLib::Handle<QuantLib::YieldTermStructure>& discountingTermStructure = QuantLib::Handle<QuantLib::YieldTermStructure>()
        ) const = 0;
        virtual QuantLib::Real impliedQuote(
            const QuantLib::Handle<QuantLib::YieldTermStructure>& estimatingTermStructure,
            const QuantLib::Handle<QuantLib::YieldTermStructure>& discountingTermStructure = QuantLib::Handle<QuantLib::YieldTermStructure>()
        ) const = 0;

        static QuantLib::Rate simpleForwardRate(const QuantLib::Date& start, const QuantLib::Date& end, const QuantLib::DayCounter& dayCounter, const QuantLib::Handle<QuantLib::YieldTermStructure>& h) {
            auto t = dayCounter.yearFraction(start, end);
            auto compounding = h->discount(start) / h->discount(end);
            return (compounding - 1.0) / t;
        }
    };

    // Couponed Bond instrument
    // US Treasury Bonds and Notes use this class for bootstrapping
    // BondTraits:
    // typedef SecurityTraits
    // couponFrequency(tenor)
    // accruedDayCounter(tenor)
    // endOfMonth(tenor)
    // scheduleCalendar(tenor)
    // convention(tenor)
    // terminationDateConvention(tenor)
    // parYieldSplineDayCounter(tenor)
    template <typename BondTraits>
    class CouponedBond : public BootstrapInstrument, public ParYieldTermStructInstrument {
    private:
        typename BondTraits::SecurityTraits securityTraits_;
        BondTraits bondTraits_;
        QuantLib::Rate coupon_;
    public:
        CouponedBond(
            const QuantLib::Period& tenor,
            const QuantLib::Date& maturityDate,
            QuantLib::Rate coupon = 0.0
        ) : BootstrapInstrument(BootstrapInstrument::Price, tenor, maturityDate), coupon_(coupon) {}
        const QuantLib::Rate& coupon() const {
            return coupon_;
        }
        QuantLib::Rate& coupon() {
            return coupon_;
        }
        const QuantLib::Date& bondMaturityDate() const {
            return datedDate();
        }
        QuantLib::Date& bondMaturityDate() {
            return datedDate();
        }
        QuantLib::Calendar settlementCalendar() const {
            return securityTraits_.settlementCalendar(tenor());
        }
        QuantLib::Natural settlementDays() const {
            return securityTraits_.settlementDays(tenor());
        }
        QuantLib::Real parNotional() const {
            return securityTraits_.parNotional(tenor());
        }
        QuantLib::Date settlementDate() const {
            auto calendar = settlementCalendar();
            QuantLib::Date today = QuantLib::Settings::instance().evaluationDate();
            auto d = calendar.adjust(today);
            return calendar.advance(d, settlementDays() * QuantLib::Days);
        }
        QuantLib::DayCounter accruedDayCounter() const {
            return bondTraits_.accruedDayCounter(tenor());
        }
        QuantLib::Frequency couponFrequency() const {
            return bondTraits_.couponFrequency(tenor());
        }
        bool endOfMonth() const {
            return bondTraits_.endOfMonth(tenor());
        }
        QuantLib::Calendar scheduleCalendar() const {
            return bondTraits_.scheduleCalendar(tenor());
        }
        QuantLib::BusinessDayConvention convention() const {
            return bondTraits_.convention(tenor());
        }
        QuantLib::BusinessDayConvention terminationDateConvention() const {
            return bondTraits_.terminationDateConvention(tenor());
        }
        QuantLib::DayCounter parYieldSplineDayCounter() const {
            return bondTraits_.parYieldSplineDayCounter(tenor());
        }
        const QuantLib::Real& cleanPrice() const {
            return price();
        }
        QuantLib::Real& cleanPrice() {
            return price();
        }
        QuantLib::Real accruedAmmount() {
            auto bond = makeFixedRateBond();
            return bond->accruedAmount();
        }
        QuantLib::Real dirtyPrice() {
            return cleanPrice() + accruedAmmount();
        }
        QuantLib::Rate yield() const {
            auto bond = makeFixedRateBond();
            return bondYield(bond, cleanPrice());
        }
        QuantLib::Real dv01() const {
            auto bond = makeFixedRateBond();
            auto yield = bondYield(bond, cleanPrice());
            return bondDV01(bond, yield);
        }
        QuantLib::Schedule bondSchedule() const {
            BondSechdulerWithoutIssueDate scheduler(
                settlementDays(),
                settlementCalendar(),
                couponFrequency(),
                endOfMonth(),
                scheduleCalendar(),
                convention(),
                terminationDateConvention()
            );
            return scheduler(bondMaturityDate());
        }
        // implied clean price by the discounting term structure
        QuantLib::Real impliedCleanPrice(
            const QuantLib::Handle<QuantLib::YieldTermStructure>& discountingTermStructure
        ) const {
            auto bond = makeFixedRateBond();
            QuantLib::ext::shared_ptr<QuantLib::PricingEngine> pricingEngine(new QuantLib::DiscountingBondEngine(discountingTermStructure));
            bond->setPricingEngine(pricingEngine);
            return bond->cleanPrice();
        }
        // implied dirty price by the discounting term structure
        QuantLib::Real impliedDirtyPrice(
            const QuantLib::Handle<QuantLib::YieldTermStructure>& discountingTermStructure
        ) const {
            auto bond = makeFixedRateBond();
            QuantLib::ext::shared_ptr<QuantLib::PricingEngine> pricingEngine(new QuantLib::DiscountingBondEngine(discountingTermStructure));
            bond->setPricingEngine(pricingEngine);
            return bond->dirtyPrice();
        }
        // implied yield by the discounting term structure
        QuantLib::Real impliedYield(
            const QuantLib::Handle<QuantLib::YieldTermStructure>& discountingTermStructure
        ) const {
            auto bond = makeFixedRateBond();
            QuantLib::ext::shared_ptr<QuantLib::PricingEngine> pricingEngine(new QuantLib::DiscountingBondEngine(discountingTermStructure));
            bond->setPricingEngine(pricingEngine);
            auto cleanPrice = bond->cleanPrice();
            return bondYield(bond, cleanPrice);
        }
        // implied dv01 by the discounting term structure
        QuantLib::Rate impliedDV01(
            const QuantLib::Handle<QuantLib::YieldTermStructure>& discountingTermStructure
        ) const {
            auto bond = makeFixedRateBond();
            QuantLib::ext::shared_ptr<QuantLib::PricingEngine> pricingEngine(new QuantLib::DiscountingBondEngine(discountingTermStructure));
            bond->setPricingEngine(pricingEngine);
            auto cleanPrice = bond->cleanPrice();
            auto yield = bondYield(bond, cleanPrice);
            return bondDV01(bond, yield);
        }
        QuantLib::Date startDate() const {
            return fixedRateBondHelper()->bond()->settlementDate();
        }
        QuantLib::Date maturityDate() const {
            return fixedRateBondHelper()->bond()->maturityDate();
        }
        QuantLib::ext::shared_ptr<QuantLib::RateHelper> rateHelper(
            const QuantLib::Handle<QuantLib::YieldTermStructure>& discountingTermStructure = QuantLib::Handle<QuantLib::YieldTermStructure>()
        ) const {
            return fixedRateBondHelper();
        };
        QuantLib::Real impliedQuote(
            const QuantLib::Handle<QuantLib::YieldTermStructure>& estimatingTermStructure,
            const QuantLib::Handle<QuantLib::YieldTermStructure>& discountingTermStructure = QuantLib::Handle<QuantLib::YieldTermStructure>()
        ) const {
            return impliedCleanPrice(discountingTermStructure);
        }
        QuantLib::Rate parYield() const {
            return yield();
        }
        QuantLib::Time parTerm() const {
            return parYieldSplineDayCounter().yearFraction(settlementDate(), bondMaturityDate());
        }
    protected:
        static QuantLib::Real solverAccuracy() {
            return 1.0e-16;
        }
        static QuantLib::Real solverQuess() {
            return 0.05;
        }
        static QuantLib::Size solverMaxIterations() {
            return 300;
        }
        // calculate bond's yield from clean price
        QuantLib::Rate bondYield(const std::shared_ptr<QuantLib::FixedRateBond>& bond, QuantLib::Real cleanPrice) const {
            auto yield = bond->yield(
                cleanPrice,
                accruedDayCounter(),
                QuantLib::Compounded,
                couponFrequency(),
                settlementDate(),
                solverAccuracy(),
                solverMaxIterations(),
                solverQuess(),
                QuantLib::Bond::Price::Clean
            );
            return yield;
        }
        QuantLib::Real bondDV01(const std::shared_ptr<QuantLib::FixedRateBond>& bond, QuantLib::Rate yield) const {
            auto dv01 = std::abs(QuantLib::BondFunctions::basisPointValue(
                *bond,
                yield,
                accruedDayCounter(),
                QuantLib::Compounded,
                couponFrequency()
            ) / parNotional());
            return dv01;
        }
        QuantLib::ext::shared_ptr<QuantLib::FixedRateBondHelper> fixedRateBondHelper() const {
            auto schedule = bondSchedule();
            auto targetPrice = cleanPrice();
            auto quote = QuantLib::ext::make_shared<QuantLib::SimpleQuote>(targetPrice);
            return QuantLib::ext::shared_ptr<QuantLib::FixedRateBondHelper>(new QuantLib::FixedRateBondHelper(
                QuantLib::Handle<QuantLib::Quote>(quote),
                settlementDays(),
                parNotional(),
                schedule,
                std::vector<QuantLib::Rate>{coupon()},
                accruedDayCounter(),
                convention(),
                parNotional(),
                QuantLib::Date(),
                QuantLib::Calendar(),
                QuantLib::Period(),
                QuantLib::Calendar(),
                QuantLib::Unadjusted,
                false,
                QuantLib::Bond::Price::Clean
            ));
        }
        std::shared_ptr<QuantLib::FixedRateBond> makeFixedRateBond() const {
            auto schedule = bondSchedule();
            return std::shared_ptr<QuantLib::FixedRateBond>(new QuantLib::FixedRateBond(
                settlementDays(),
                parNotional(),
                schedule,
                std::vector<QuantLib::Rate>{coupon()},
                accruedDayCounter(),
                convention(),
                parNotional()
            ));
        }
    };

    // base class for all par instruments
    // par instrument is a for rate-based (not price-based) bootstrap bacause the price is anchored at 100 (par)
    class ParInstrument : public BootstrapInstrument, public ParYieldTermStructInstrument {
    public:
        ParInstrument(
            const QuantLib::Period& tenor,
            const QuantLib::Date& datedDate = QuantLib::Date()
        ) : BootstrapInstrument(BootstrapInstrument::Rate, tenor, datedDate) {}
    protected:
        QuantLib::ext::shared_ptr<QuantLib::Bond> parBond() const {
            return fixedRateBondHelper()->bond();
        }
    public:
        // interfaces
        /////////////////////////////////////////////////////////////////////////////////////////////////
        virtual QuantLib::ext::shared_ptr<QuantLib::FixedRateBondHelper> fixedRateBondHelper() const = 0;
        virtual QuantLib::Rate impliedParRate(
            const QuantLib::Handle<QuantLib::YieldTermStructure>& discountingTermStructure
        ) const = 0;
        virtual QuantLib::DayCounter parYieldSplineDayCounter() const = 0;
        /////////////////////////////////////////////////////////////////////////////////////////////////

        const QuantLib::Rate& parRate() const {
            return rate();
        }
        QuantLib::Rate& parRate() {
            return rate();
        }
        // for any par instrument, yield is the par rate
        QuantLib::Rate yield() const {
            return parRate();
        }
        QuantLib::Date startDate() const {
            return parBond()->settlementDate();
        }
        QuantLib::Date maturityDate() const {
            return parBond()->maturityDate();
        }
        QuantLib::ext::shared_ptr<QuantLib::RateHelper> rateHelper(
            const QuantLib::Handle<QuantLib::YieldTermStructure>& discountingTermStructure = QuantLib::Handle<QuantLib::YieldTermStructure>()
        ) const {
            return fixedRateBondHelper();
        };
        QuantLib::Real impliedQuote(
            const QuantLib::Handle<QuantLib::YieldTermStructure>& estimatingTermStructure,
            const QuantLib::Handle<QuantLib::YieldTermStructure>& discountingTermStructure = QuantLib::Handle<QuantLib::YieldTermStructure>()
        ) const {
            return impliedParRate(discountingTermStructure);
        }
        QuantLib::Rate parYield() const {
            return parRate();
        }
        QuantLib::Time parTerm() const {
            return parYieldSplineDayCounter().yearFraction(parBond()->settlementDate(), parBond()->maturityDate());
        }
    };

    // for converting spot or fwd par rates back to zero curve (par-to-zero bootstrapping)
    // used during par-shock
    template <
        QuantLib::Frequency COUPON_FREQ = QuantLib::Semiannual,
        QuantLib::Thirty360::Convention THIRTY_360_DC_CONVENTION = QuantLib::Thirty360::BondBasis
    >
    class ParRate : public ParInstrument {
    private:
        using ParYieldHelper = ParYieldHelper<COUPON_FREQ, THIRTY_360_DC_CONVENTION>;
        QuantLib::Date baseReferenceDate_;
        QuantLib::Period forwardStart_;
    public:
        ParRate(
            const QuantLib::Period& tenor,
            const QuantLib::Date& baseReferenceDate,
            const QuantLib::Period& forwardStart = QuantLib::Period(0, QuantLib::Days)
        ) :
            ParInstrument(tenor),
            baseReferenceDate_(baseReferenceDate),
            forwardStart_(forwardStart)
        {}
        QuantLib::Date& baseReferenceDate() {
            return baseReferenceDate_;
        }
        const QuantLib::Date& baseReferenceDate() const {
            return baseReferenceDate_;
        }
        QuantLib::Period& forwardStart() {
            return forwardStart_;
        }
        const QuantLib::Period& forwardStart() const {
            return forwardStart_;
        }
        QuantLib::DayCounter parYieldSplineDayCounter() const {
            return ParYieldHelper::parBondDayCounter();
        }
        QuantLib::ext::shared_ptr<QuantLib::FixedRateBondHelper> fixedRateBondHelper() const {
            QuantLib::ext::shared_ptr<QuantLib::FixedRateBondHelper> helper = ParYieldHelper(tenor())
                .withParYield(parRate())
                .withBaseReferenceDate(baseReferenceDate())
                .withForwardStart(forwardStart());
            return helper;
        }
        QuantLib::Rate impliedParRate(
            const QuantLib::Handle<QuantLib::YieldTermStructure>& discountingTermStructure
        ) const {
            QL_ASSERT(discountingTermStructure->referenceDate() == baseReferenceDate(), "discount curve base reference date (" << discountingTermStructure->referenceDate() << ") is not what's expected (" << baseReferenceDate() << ")");
            return ParYieldHelper::parYield(
                discountingTermStructure.currentLink(),
                tenor(),
                forwardStart()
            );
        }
    };

    // base class for all par bond and bill securities
    // SecurityTraits
    // settlementCalendar(tenor)
    // settlementDays(tenor)
    // parNotional(tenor)
    template <typename SecurityTraits>
    class ParBondSecurity : public ParInstrument {
    public:
        enum ParSecurityType {
            Bill = 0,
            CouponedBond = 1
        };
    private:
        ParSecurityType securityType_;
        SecurityTraits securityTraits_;
    public:
        ParBondSecurity(
            const ParSecurityType& securityType,
            const QuantLib::Period& tenor,
            const QuantLib::Date& bondMaturityDate = QuantLib::Date()
        ) : ParInstrument(tenor, bondMaturityDate), securityType_(securityType) {
            if (bondMaturityDate == QuantLib::Date()) {
                datedDate() = settlementCalendar().advance(settlementDate(), tenor);
            }
        }
        const QuantLib::Date& bondMaturityDate() const {
            return datedDate();
        }
        QuantLib::Date& bondMaturityDate() {
            return datedDate();
        }
        QuantLib::Calendar settlementCalendar() const {
            return securityTraits_.settlementCalendar(tenor());
        }
        QuantLib::Natural settlementDays() const {
            return securityTraits_.settlementDays(tenor());
        }
        QuantLib::Real parNotional() const {
            return securityTraits_.parNotional(tenor());
        }
        QuantLib::Date settlementDate() const {
            auto calendar = settlementCalendar();
            QuantLib::Date today = QuantLib::Settings::instance().evaluationDate();
            auto d = calendar.adjust(today);
            return calendar.advance(d, settlementDays() * QuantLib::Days);
        }
        const ParSecurityType& securityType() const {
            return securityType_;
        }
    };

    // US Treasury Bills use this class for bootstrapping
    // ZeroCouponBillTraits
    // typedef SecurityTraits
    // dayCounter(tenor)
    // bondEquivCouponFrequency(tenor)
    // discountRateDayCounter(tenor)
    // parYieldSplineDayCounter(tenor)
    template <typename ZeroCouponBillTraits>
    class ZeroCouponBill : public ParBondSecurity<typename ZeroCouponBillTraits::SecurityTraits> {
    private:
        using BaseType = ParBondSecurity<typename ZeroCouponBillTraits::SecurityTraits>;
        ZeroCouponBillTraits billTraits_;
    public:
        ZeroCouponBill(
            const QuantLib::Period& tenor,
            const QuantLib::Date& bondMaturityDate
        ) : BaseType(BaseType::Bill, tenor, bondMaturityDate) {}
        QuantLib::DayCounter dayCounter() const {
            return billTraits_.dayCounter(this->tenor());
        }
        QuantLib::Frequency bondEquivCouponFrequency() const {
            return billTraits_.bondEquivCouponFrequency(this->tenor());
        }
        QuantLib::DayCounter discountRateDayCounter() const {
            return billTraits_.discountRateDayCounter(this->tenor());
        }
        QuantLib::Time bondEquivCouponInterval() const {
            return 1.0 / bondEquivCouponFrequency();
        }
        QuantLib::DayCounter parYieldSplineDayCounter() const {
            return billTraits_.parYieldSplineDayCounter(this->tenor());
        }
        bool simpleCompounding() const {
            auto t = dayCounter().yearFraction(this->settlementDate(), this->bondMaturityDate());
            auto firstCouponTime = bondEquivCouponInterval();
            return (t <= firstCouponTime);
        }
        std::pair<QuantLib::Compounding, QuantLib::Frequency> getYieldCompoundingAndFrequency() const {
            auto simpleComp = simpleCompounding();
            auto comp = (simpleComp ? QuantLib::Simple : QuantLib::Compounded);
            auto freq = (simpleComp ? QuantLib::NoFrequency : bondEquivCouponFrequency());
            return std::pair<QuantLib::Compounding, QuantLib::Frequency>(comp, freq);
        }
        // convert between discount factor and yield
        ////////////////////////////////////////////////////////////////////////////////////////////////
        QuantLib::Rate yieldFromDiscountFactor(QuantLib::DiscountFactor df) const {
            auto pr = getYieldCompoundingAndFrequency();
            auto ir = QuantLib::InterestRate::impliedRate(
                1.0 / df,
                dayCounter(),
                pr.first,
                pr.second,
                this->settlementDate(),
                this->bondMaturityDate()
            );
            return ir.rate();
        }
        QuantLib::DiscountFactor discountFactorFromYield(QuantLib::Rate yield) const {
            auto pr = getYieldCompoundingAndFrequency();
            QuantLib::InterestRate ir(yield, dayCounter(), pr.first, pr.second);
            return ir.discountFactor(this->settlementDate(), this->bondMaturityDate());
        }
        ////////////////////////////////////////////////////////////////////////////////////////////////
        // convert between discount rate and discount factor
        ////////////////////////////////////////////////////////////////////////////////////////////////
        QuantLib::DiscountFactor discountFactorFromDiscountRate(QuantLib::Rate discountRate) const {
            auto d1 = this->settlementDate();
            auto d2 = this->bondMaturityDate();
            auto t = this->discountRateDayCounter().yearFraction(d1, d2);
            return 1.0 - discountRate * t;
        }
        QuantLib::Rate discountRateFromDiscountFactor(QuantLib::DiscountFactor discountFactor) const {
            auto d1 = this->settlementDate();
            auto d2 = this->bondMaturityDate();
            auto t = this->discountRateDayCounter().yearFraction(d1, d2);
            return (1.0 - discountFactor)/t;
        }
        ////////////////////////////////////////////////////////////////////////////////////////////////
        // set the yield by yield
        ZeroCouponBill<ZeroCouponBillTraits>& withYield(QuantLib::Rate yield) {
            this->parRate() = yield;
            return *this;
        }
        // set the yield by discount factor
        ZeroCouponBill<ZeroCouponBillTraits>& withDiscountFactor(QuantLib::DiscountFactor discountFactor) {
            return withYield(yieldFromDiscountFactor(discountFactor));
        }
        // set the yield by price
        ZeroCouponBill<ZeroCouponBillTraits>& withBondPrice(QuantLib::Real bondPrice) {
            return withDiscountFactor(bondPrice / this->parNotional());
        }
        // set the yield by discount rate
        ZeroCouponBill<ZeroCouponBillTraits>& withDiscountRate(QuantLib::Rate discountRate) {
            return withDiscountFactor(discountFactorFromDiscountRate(discountRate));
        }
        QuantLib::DiscountFactor discountFactor() const {
            return discountFactorFromYield(this->yield());
        }
        QuantLib::Real bondPrice() const {
            return discountFactor() * this->parNotional();
        }
        QuantLib::Real discountRate() const {
            return discountRateFromDiscountFactor(discountFactor());
        }
        QuantLib::Real dv01() const {
            auto bond = makeZeroCouponBond();
            return bondDV01(bond, this->yield());
        }
        // implied discount factor by the discounting term structure
        QuantLib::DiscountFactor impliedDiscountFactor(
            const QuantLib::Handle<QuantLib::YieldTermStructure>& discountingTermStructure
        ) const {
            auto d1 = this->settlementDate();
            auto d2 = this->bondMaturityDate();
            auto df = discountingTermStructure->discount(d2) / discountingTermStructure->discount(d1);
            return df;
        }
        // implied bond price by the discounting term structure
        QuantLib::DiscountFactor impliedBondPrice(
            const QuantLib::Handle<QuantLib::YieldTermStructure>& discountingTermStructure
        ) const {
            auto df = impliedDiscountFactor(discountingTermStructure);
            return df * this->parNotional();
        }
        // implied discount rate by the discounting term structure
        QuantLib::Rate impliedDiscountRate(
            const QuantLib::Handle<QuantLib::YieldTermStructure>& discountingTermStructure
        ) const {
            auto df = impliedDiscountFactor(discountingTermStructure);
            return discountRateFromDiscountFactor(df);
        }
        // implied yield by the discounting term structure
        QuantLib::Rate impliedYield(
            const QuantLib::Handle<QuantLib::YieldTermStructure>& discountingTermStructure
        ) const {
            auto df = impliedDiscountFactor(discountingTermStructure);
            return yieldFromDiscountFactor(df);
        }
        // implied dv01 by the discounting term structure
        QuantLib::Rate impliedDV01(
            const QuantLib::Handle<QuantLib::YieldTermStructure>& discountingTermStructure
        ) const {
            auto bond = makeZeroCouponBond();
            auto yield = impliedYield(discountingTermStructure);
            return bondDV01(bond, yield);
        }
        // returns a bond equiv. fixed rate helper
        QuantLib::ext::shared_ptr<QuantLib::FixedRateBondHelper> fixedRateBondHelper() const {
            auto targetPrice = discountFactorFromYield(this->yield()) * this->parNotional();
            auto quote = QuantLib::ext::make_shared<QuantLib::SimpleQuote>(targetPrice);
            BondSechdulerWithoutIssueDate scheduler(this->settlementDays(), this->settlementCalendar(), bondEquivCouponFrequency(), false);
            auto bondEquivSchedule = scheduler(this->bondMaturityDate());
            return QuantLib::ext::shared_ptr<QuantLib::FixedRateBondHelper>(new QuantLib::FixedRateBondHelper(
                QuantLib::Handle<QuantLib::Quote>(quote),
                this->settlementDays(),
                this->parNotional(),
                bondEquivSchedule,
                std::vector<QuantLib::Rate>{0.0},   // zero coupon
                this->dayCounter(),
                QuantLib::Unadjusted,
                this->parNotional()
            ));
        }
        QuantLib::Rate impliedParRate(
            const QuantLib::Handle<QuantLib::YieldTermStructure>& discountingTermStructure
        ) const {
            return impliedYield(discountingTermStructure);
        }
    protected:
        std::shared_ptr<QuantLib::ZeroCouponBond> makeZeroCouponBond() const {
            return std::shared_ptr<QuantLib::ZeroCouponBond>(
                new QuantLib::ZeroCouponBond(
                    this->settlementDays(),
                    this->settlementCalendar(),
                    this->parNotional(),
                    this->bondMaturityDate(),
                    QuantLib::Unadjusted,
                    this->parNotional()
                )
            );
        }
        QuantLib::Real bondDV01(const std::shared_ptr<QuantLib::ZeroCouponBond>& bond, QuantLib::Rate yield) const {
            auto pr = getYieldCompoundingAndFrequency();
            auto dv01 = std::abs(QuantLib::BondFunctions::basisPointValue(
                *bond,
                yield,
                dayCounter(),
                pr.first,
                pr.second
            ) / this->parNotional());
            return dv01;
        }
    };

    // Theoretical Par Bond - used for par yield spline bootstrapping
    // BondTraits
    // typedef SecurityTraits
    // couponFrequency(tenor)
    // accruedDayCounter(tenor)
    // endOfMonth(tenor)
    // scheduleCalendar(tenor)
    // convention(tenor)
    // terminationDateConvention(tenor)
    // parYieldSplineDayCounter(tenor)
    template <typename BondTraits>
    class ParBond : public ParBondSecurity<typename BondTraits::SecurityTraits> {
    private:
        using BaseType = ParBondSecurity<typename BondTraits::SecurityTraits>;
        BondTraits bondTraits_;
    public:
        ParBond(
            const QuantLib::Period& tenor,
            const QuantLib::Date& bondMaturityDate = QuantLib::Date()
        ) : BaseType(BaseType::CouponedBond, tenor, bondMaturityDate) {
            if (bondMaturityDate == QuantLib::Date()) {
                this->datedDate() = bondSchedule().endDate();
            }
        }
        QuantLib::DayCounter accruedDayCounter() const {
            return bondTraits_.accruedDayCounter(this->tenor());
        }
        QuantLib::Frequency couponFrequency() const {
            return bondTraits_.couponFrequency(this->tenor());
        }
        bool endOfMonth() const {
            return bondTraits_.endOfMonth(this->tenor());
        }
        QuantLib::Calendar scheduleCalendar() const {
            return bondTraits_.scheduleCalendar(this->tenor());
        }
        QuantLib::BusinessDayConvention convention() const {
            return bondTraits_.convention(this->tenor());
        }
        QuantLib::BusinessDayConvention terminationDateConvention() const {
            return bondTraits_.terminationDateConvention(this->tenor());
        }
        QuantLib::DayCounter parYieldSplineDayCounter() const {
            return bondTraits_.parYieldSplineDayCounter(this->tenor());
        }
        QuantLib::Schedule bondSchedule() const {
            QuantLib::Period couponTenor(couponFrequency());
            auto terminationDate = scheduleCalendar().advance(this->settlementDate(), this->tenor());
            return QuantLib::Schedule(
                this->settlementDate(),
                terminationDate,
                couponTenor,
                scheduleCalendar(),
                convention(),
                terminationDateConvention(),
                QuantLib::DateGeneration::Forward,
                endOfMonth()
            );
        }
        QuantLib::Real dv01() const {
            auto bond = makeFixedRateBond();
            return bondDV01(bond, this->yield());
        }
        QuantLib::Real impliedDV01(const QuantLib::Handle<QuantLib::YieldTermStructure>& discountingTermStructure) const {
            auto bond = makeFixedRateBond();
            auto yield = impliedParRate(discountingTermStructure);
            return bondDV01(bond, yield);
        }
    protected:
        std::shared_ptr<QuantLib::FixedRateBond> makeFixedRateBond(QuantLib::Rate bondCoupon) const {
            auto schedule = bondSchedule();
            return std::shared_ptr<QuantLib::FixedRateBond>(new QuantLib::FixedRateBond(
                    this->settlementDays(),
                    this->parNotional(),
                    schedule,
                    std::vector<QuantLib::Rate>{bondCoupon},
                    accruedDayCounter(),
                    convention(),
                    this->parNotional()
                ));
        }
        QuantLib::Real bondDV01(const std::shared_ptr<QuantLib::FixedRateBond>& bond, QuantLib::Rate yield) const {
            auto dv01 = std::abs(QuantLib::BondFunctions::basisPointValue(
                *bond,
                yield,
                accruedDayCounter(),
                QuantLib::Compounded,
                couponFrequency()
            ) / this->parNotional());
            return dv01;
        }
        static QuantLib::Real solverAccuracy() {
            return 1.0e-16;
        }
        static QuantLib::Real solverQuess() {
            return 0.05;
        }
        static QuantLib::Size solverMaxIterations() {
            return 300;
        }
    public:
        QuantLib::ext::shared_ptr<QuantLib::FixedRateBondHelper> fixedRateBondHelper() const {
            auto schedule = bondSchedule();
            auto targetPrice = this->parNotional();
            auto quote = QuantLib::ext::make_shared<QuantLib::SimpleQuote>(targetPrice);
            return QuantLib::ext::shared_ptr<QuantLib::FixedRateBondHelper>(new QuantLib::FixedRateBondHelper(
                QuantLib::Handle<QuantLib::Quote>(quote),
                this->settlementDays(),
                this->parNotional(),
                schedule,
                std::vector<QuantLib::Rate>{this->parRate()},
                accruedDayCounter(),
                convention(),
                this->parNotional(),
                QuantLib::Date(),
                QuantLib::Calendar(),
                QuantLib::Period(),
                QuantLib::Calendar(),
                QuantLib::Unadjusted,
                false,
                QuantLib::Bond::Price::Clean
            ));
        }
        QuantLib::Rate impliedParRate(
            const QuantLib::Handle<QuantLib::YieldTermStructure>& discountingTermStructure
        ) const {
            QuantLib::Brent solver;
            QuantLib::Real accuracy = solverAccuracy();
            QuantLib::Rate guess = solverQuess();
            QuantLib::Rate step = guess / 10.0;
            solver.setMaxEvaluations(solverMaxIterations());
            QuantLib::ext::shared_ptr<QuantLib::PricingEngine> pricingEngine(new QuantLib::DiscountingBondEngine(discountingTermStructure));
            auto const& me = *this;
            auto parRate = solver.solve([&pricingEngine, &me](const QuantLib::Rate& coupon) -> QuantLib::Real {
                auto bond = me.makeFixedRateBond(coupon);
                bond->setPricingEngine(pricingEngine);
                return bond->cleanPrice() - me.parNotional();
                }, accuracy, guess, step);
            return parRate;
        }
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

    template
    <
        typename SWAP_TRAITS
    >
    class OISSwapIndex:
        public BootstrapInstrument {
    public:
        typedef typename SWAP_TRAITS SwapTraits;
        typedef typename SwapTraits::BaseSwapIndex BaseSwapIndex;
        typedef typename SwapTraits::OvernightIndex OvernightIndex;
    private:
        SwapTraits swapTraits_;
    public:
        OISSwapIndex(
            const QuantLib::Period& tenor
        ) : BootstrapInstrument(BootstrapInstrument::Rate, tenor) {}
    private:
        QuantLib::ext::shared_ptr<QuantLib::OvernightIndexedSwap> createSwap(
            const QuantLib::Handle<QuantLib::YieldTermStructure>& estimatingTermStructure = QuantLib::Handle<QuantLib::YieldTermStructure>()
        ) const {
            auto overnightIndex = swapTraits_.createOvernightIndex(estimatingTermStructure);
            QuantLib::ext::shared_ptr<QuantLib::OvernightIndexedSwap> swap = QuantLib::MakeOIS(tenor(), overnightIndex, 0.0)
                .withSettlementDays(swapTraits_.settlementDays(tenor()))
                .withTelescopicValueDates(swapTraits_.telescopicValueDates(tenor()))
                .withPaymentAdjustment(swapTraits_.paymentAdjustment(tenor()))
                .withAveragingMethod(swapTraits_.averagingMethod(tenor()));
            return swap;
        }
    public:
        QuantLib::Date startDate() const {
            return createSwap()->startDate();
        }
        QuantLib::Date maturityDate() const {
            return createSwap()->maturityDate();
        }
        QuantLib::ext::shared_ptr<QuantLib::RateHelper> rateHelper(
            const QuantLib::Handle<QuantLib::YieldTermStructure>& discountingTermStructure = QuantLib::Handle<QuantLib::YieldTermStructure>()
        ) const {
            auto overnightIndex = swapTraits_.createOvernightIndex();
            QuantLib::ext::shared_ptr<QuantLib::OISRateHelper> helper(
                new QuantLib::OISRateHelper(
                    swapTraits_.settlementDays(tenor()),
                    tenor(),
                    quote(),
                    overnightIndex,
                    discountingTermStructure,   // exogenous discounting curve
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
            const QuantLib::Handle<QuantLib::YieldTermStructure>& discountingTermStructure = QuantLib::Handle<QuantLib::YieldTermStructure>()
        ) const {
            QuantLib::ext::shared_ptr<QuantLib::PricingEngine> swapPricingEngine(new QuantLib::DiscountingSwapEngine(discountingTermStructure));
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
            const QuantLib::Handle<QuantLib::YieldTermStructure>& discountingTermStructure = QuantLib::Handle<QuantLib::YieldTermStructure>()
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
            const QuantLib::Handle<QuantLib::YieldTermStructure>& discountingTermStructure = QuantLib::Handle<QuantLib::YieldTermStructure>()
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
            const QuantLib::Handle<QuantLib::YieldTermStructure>& discountingTermStructure = QuantLib::Handle<QuantLib::YieldTermStructure>()
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
            const QuantLib::Handle<QuantLib::YieldTermStructure>& discountingTermStructure = QuantLib::Handle<QuantLib::YieldTermStructure>()
        ) const {
            auto result = getValueMaturityDates();
            auto const& start = std::get<0>(result);
            auto const& end = std::get<1>(result);
            auto const& dayCounter = std::get<2>(result);
            auto rate = this->simpleForwardRate(start, end, dayCounter, estimatingTermStructure);
            return rate;
        }
    };
    
    // FRA
    // this can be use to bootstrap Libor 3m estimating zero curve given a monthly Libor 3m forward rate curves
    class FRA : public SwapCurveInstrument {
    private:
        typedef std::tuple<QuantLib::Date, QuantLib::Date, QuantLib::DayCounter> CalcDatesResult;
        CalcDatesResult calcDates() const {
            auto iborIndex = this->iborIndex();
            QuantLib::FraRateHelper rateHelper(0., forward(), iborIndex);
            return CalcDatesResult(rateHelper.earliestDate(), rateHelper.pillarDate(), iborIndex->dayCounter());
        }
    public:
        FRA(
            const IborIndexFactory& iborIndexFactory,
            const QuantLib::Period& forward
        ) : SwapCurveInstrument(iborIndexFactory, BootstrapInstrument::Rate, SwapCurveInstrument::FRA, forward, QuantLib::Null<QuantLib::Date>())
        {
            this->datedDate_ = std::get<0>(calcDates());    // datedDate_ stores the earliestDate/startDate
        }
        // for FRA tenor is the forward period
        const QuantLib::Period& forward() const {
            return tenor();
        }
        QuantLib::Date startDate() const {
            return this->datedDate_;
        }
        QuantLib::Date maturityDate() const {
            return std::get<1>(calcDates());
        }
        QuantLib::ext::shared_ptr<QuantLib::RateHelper> rateHelper(
            const QuantLib::Handle<QuantLib::YieldTermStructure>& discountingTermStructure = QuantLib::Handle<QuantLib::YieldTermStructure>()
        ) const {
            auto iborIndex = this->iborIndex();
            QuantLib::ext::shared_ptr<QuantLib::FraRateHelper> helper(
                new QuantLib::FraRateHelper(
                    quote(),
                    forward(),
                    iborIndex
                )
            );
            return helper;
        }
        QuantLib::Real impliedQuote(
            const QuantLib::Handle<QuantLib::YieldTermStructure>& estimatingTermStructure,
            const QuantLib::Handle<QuantLib::YieldTermStructure>& discountingTermStructure = QuantLib::Handle<QuantLib::YieldTermStructure>()
        ) const {
            auto iborIndex = this->iborIndex();
            QuantLib::FraRateHelper rateHelper(0., forward(), iborIndex);
            rateHelper.setTermStructure(estimatingTermStructure.currentLink().get());
            auto rate = rateHelper.impliedQuote();
            return rate;
        }
    };
    
    template<typename SwapTraits>
    class SwapIndex : public SwapCurveInstrument {
    private:
        SwapTraits swapTraits_;
    public:
        SwapIndex(
            const IborIndexFactory& iborIndexFactory,
            const QuantLib::Period& tenor
        ) : SwapCurveInstrument(iborIndexFactory, BootstrapInstrument::Rate, SwapCurveInstrument::Swap, tenor) {}

        // calculate swap index's settlement days and swap start/effective date
        static std::pair<QuantLib::Natural, QuantLib::Date> getSwapIndexStartInfo(
            const QuantLib::Natural swapFixingDays,
            const QuantLib::ext::shared_ptr<QuantLib::IborIndex>& iborIndex,
            const QuantLib::Calendar& swapFixingCalendar,
            const QuantLib::Date& today = QuantLib::Date()
        ) {
            auto d = (today == QuantLib::Date() ? QuantLib::Settings::instance().evaluationDate() : today); // today
            auto settlementDays = swapFixingDays;
            auto swapFixingDate = swapFixingCalendar.adjust(d);
            while (true) {
                auto swapValueDate = swapFixingCalendar.advance(swapFixingDate, settlementDays * QuantLib::Days);
                auto iborIndexFixingDate = iborIndex->fixingDate(swapValueDate);
                if (iborIndexFixingDate >= d) { // ibor index fixing date has to be greater than or equal today for safe curve building because the curve's base reference is going to be today
                    return std::pair<QuantLib::Natural, QuantLib::Date>(settlementDays, swapValueDate); // swapValueDate is the swap start date
                }
                settlementDays++;
            }
        }
    private:
        QuantLib::ext::shared_ptr<QuantLib::VanillaSwap> createSwap(
            const QuantLib::Handle<QuantLib::YieldTermStructure>& estimatingTermStructure = QuantLib::Handle<QuantLib::YieldTermStructure>()
        ) const {
            auto iborIndex = this->iborIndex(estimatingTermStructure);
            QuantLib::Calendar calendar = swapTraits_.fixingCalendar(tenor());
            auto pr = getSwapIndexStartInfo(swapTraits_.settlementDays(tenor()), iborIndex, calendar);
            auto const& settlementDays = pr.first;
            QuantLib::ext::shared_ptr<QuantLib::VanillaSwap> swap = QuantLib::MakeVanillaSwap(this->tenor(), iborIndex, 0.0)
                .withSettlementDays(settlementDays)
                .withFixedLegCalendar(calendar)
                .withFixedLegTenor(swapTraits_.fixedLegTenor(tenor()))
                .withFixedLegConvention(swapTraits_.fixedLegConvention(tenor()))
                .withFixedLegDayCount(swapTraits_.fixedLegDayCount(tenor()))
                .withFixedLegEndOfMonth(swapTraits_.endOfMonth(tenor()))
                .withFloatingLegCalendar(calendar)
                .withFloatingLegEndOfMonth(swapTraits_.endOfMonth(tenor()));
            return swap;
        }
    public:
        QuantLib::Date startDate() const {
            return createSwap()->startDate();
        }
        QuantLib::Date maturityDate() const {
            return createSwap()->maturityDate();
        }
        QuantLib::ext::shared_ptr<QuantLib::RateHelper> rateHelper(
            const QuantLib::Handle<QuantLib::YieldTermStructure>& discountingTermStructure = QuantLib::Handle<QuantLib::YieldTermStructure>()
        ) const {
            auto iborIndex = this->iborIndex();
            QuantLib::Calendar calendar = swapTraits_.fixingCalendar(tenor());
            auto pr = getSwapIndexStartInfo(swapTraits_.settlementDays(tenor()), iborIndex, calendar);
            auto const& settlementDays = pr.first;
            QuantLib::ext::shared_ptr<QuantLib::SwapRateHelper> helper(
                new QuantLib::SwapRateHelper(
                    quote(),
                    tenor(),
                    calendar,
                    swapTraits_.fixedLegFrequency(tenor()),
                    swapTraits_.fixedLegConvention(tenor()),
                    swapTraits_.fixedLegDayCount(tenor()),
                    iborIndex,
                    QuantLib::Handle<QuantLib::Quote>(),    // spread
                    0 * QuantLib::Days,// fwdStart
                    discountingTermStructure,
                    settlementDays,
                    QuantLib::Pillar::LastRelevantDate,
                    QuantLib::Date(),
                    swapTraits_.endOfMonth(tenor())
                )
            );
            return helper;
        }
        QuantLib::Real impliedQuote(
            const QuantLib::Handle<QuantLib::YieldTermStructure>& estimatingTermStructure,
            const QuantLib::Handle<QuantLib::YieldTermStructure>& discountingTermStructure = QuantLib::Handle<QuantLib::YieldTermStructure>()
        ) const {
            QuantLib::ext::shared_ptr<QuantLib::PricingEngine> swapPricingEngine(new QuantLib::DiscountingSwapEngine(discountingTermStructure));
            auto swap = createSwap(estimatingTermStructure);
            swap->setPricingEngine(swapPricingEngine);
            auto swapRate = swap->fairRate();
            return swapRate;
        }
    };

    // for converting simple forward rate curve to zero curve (simple-forward-to-zero bootstrapping)
    // use QuantLib::Thirty360::ISDA as default day count convention for simplified calculation
    template <
        QuantLib::Thirty360::Convention THIRTY_360_DC_CONVENTION = QuantLib::Thirty360::ISDA
    >
    class SimpleForward : public FRA {
    private:
        QuantLib::Natural lengthInMonths_;  // tenor in months
        std::string familyName_;
    public:
        static IborIndexFactory getIborFactory(
            QuantLib::Natural lengthInMonths,
            const std::string& familyName
        ) {
            return [lengthInMonths, familyName](const QuantLib::Handle<QuantLib::YieldTermStructure>& h) {
                return QuantLib::ext::make_shared<QuantLib::IborIndex>(
                    familyName, // family name
                    lengthInMonths * QuantLib::Months,
                    0,  // no fixing day
                    QuantLib::Currency(),
                    QuantLib::NullCalendar(),   // no holiday
                    QuantLib::BusinessDayConvention::Unadjusted,    // no business day adjustment
                    false,   // endOfMonth = false
                    QuantLib::Thirty360(THIRTY_360_DC_CONVENTION), // 30/360 day counting
                    h
                );
            };
        }
        SimpleForward(
            const QuantLib::Period& forward,
            QuantLib::Natural lengthInMonths = 1,    // default to 1-month tenor
            const std::string& familyName = "no-fix"
        ) :
            FRA(getIborFactory(lengthInMonths, familyName), forward),
            lengthInMonths_(lengthInMonths),
            familyName_(familyName)
        {}
        QuantLib::Natural lengthInMonths() const {
            return lengthInMonths_;
        }
        const std::string& familyName() const {
            return familyName_;
        }
        IborIndexFactory getIborFactory() const {
            return getIborFactory(lengthInMonths(), familyName());
        }
    };

    // for converting forward par rate curve to zero curve (forward-par-to-zero bootstrapping)
    // use QuantLib::Thirty360::ISDA as default day count convention for simplified calculation
    template <
        QuantLib::Frequency COUPON_FREQ = QuantLib::Semiannual,
        QuantLib::Thirty360::Convention THIRTY_360_DC_CONVENTION = QuantLib::Thirty360::ISDA
    >
    class ParForward : public ParRate<COUPON_FREQ, THIRTY_360_DC_CONVENTION> {
    public:
        ParForward(
            const QuantLib::Period& tenor,
            const QuantLib::Period& forward
        ) :
            ParRate<COUPON_FREQ, THIRTY_360_DC_CONVENTION>(
                tenor,
                QuantLib::Settings::instance().evaluationDate(),
                forward
            )
        {}
    };
    // for converting par rate curve to zero curve (spot-par-to-zero bootstrapping)
    // use QuantLib::Thirty360::ISDA as default day count convention for simplified calculation
    template <
        QuantLib::Frequency COUPON_FREQ = QuantLib::Semiannual,
        QuantLib::Thirty360::Convention THIRTY_360_DC_CONVENTION = QuantLib::Thirty360::ISDA
    >
    class ParSpot : public ParForward<COUPON_FREQ, THIRTY_360_DC_CONVENTION> {
    public:
        ParSpot(
            const QuantLib::Period& tenor
        ) :
            ParForward<COUPON_FREQ, THIRTY_360_DC_CONVENTION>(tenor, QuantLib::Period(0, QuantLib::Days))
        {}
    };
}
