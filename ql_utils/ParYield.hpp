#pragma once

#include <ql/quantlib.hpp>

namespace QLUtils {
    // functor that determines whether the given bond tenor is a par couponed bond based on a cutoff months
    template <template<typename> typename COMP = std::greater, QuantLib::Natural COUPON_CUTOFF_MONTHS = 12>
    struct ParBondTenorCouponedWithCutoffMonths {
        // returns true if the given tenor is couponed, false otherwise
        bool operator() (const QuantLib::Period& tenor) const {
            COMP<QuantLib::Period> compare;
            QuantLib::Period tenorCutoff = COUPON_CUTOFF_MONTHS * QuantLib::Months;
            return compare(tenor, tenorCutoff);
        }
    };

    // calculate theoretical (spot or forward) bond schedule and settlement date
    template <QuantLib::Frequency COUPON_FREQ = QuantLib::Semiannual>
    class TheoreticalBondScheduler {
    protected:
        QuantLib::Period timeToMaturity_;    // bond time to maturity from settlement date
        QuantLib::Period forwardSettlePeriod_;  // forward settle period
        QuantLib::Date baseReferenceDate_;  // base reference date

        QuantLib::Date settlementDate_; // bond settlement date
        QuantLib::Natural settlementDays_;  // bond # of days to settle from today
        QuantLib::Date maturityDate_; // bond maturity date
        QuantLib::Date startDate_;  // bond starting date
        bool settleOnCouponPayment_;
        QuantLib::Schedule schedule_;   // calculated bond schedule
    public:
        static QuantLib::Frequency frequency() {
            return COUPON_FREQ;
        }
        // returns number of months between coupon payments
        static QuantLib::Natural couponTenorMonths() {
            return 12 / (QuantLib::Natural)frequency();
        }
    protected:
        void initialize() {
            QuantLib::Date today = QuantLib::Settings::instance().evaluationDate();
            auto baseReferenceDate = (baseReferenceDate_ == QuantLib::Date() ? today : baseReferenceDate_);
            QL_REQUIRE(baseReferenceDate >= today, "base reference date must be greater or equal to today");
            settlementDate_ = baseReferenceDate + forwardSettlePeriod_;
            settlementDays_ = settlementDate_ - today;
            maturityDate_ = settlementDate_ + timeToMaturity_;
            // calculate theoretical start date of the bond
            auto couponMonths = couponTenorMonths();
            auto d = maturityDate_; // starting with the maturity and project backward
            while (d > settlementDate_) {
                d -= couponMonths * QuantLib::Months;
            }
            // d <= settlementDate_ at this point
            settleOnCouponPayment_ = (d == settlementDate_);
            startDate_ = d;
            schedule_ = QuantLib::Schedule(startDate_, maturityDate_, QuantLib::Period(frequency()), QuantLib::NullCalendar(), QuantLib::Unadjusted, QuantLib::Unadjusted, QuantLib::DateGeneration::Backward, false);
        }
    public:
        TheoreticalBondScheduler(
            const QuantLib::Period& timeToMaturity,    // bond time to maturity from settlement date
            const QuantLib::Period& forwardSettlePeriod = QuantLib::Period(0, QuantLib::Days),   // forward settle period
            const QuantLib::Date& baseReferenceDate = QuantLib::Date()    // base reference date
        ) :
            timeToMaturity_(timeToMaturity),
            forwardSettlePeriod_(forwardSettlePeriod),
            baseReferenceDate_(baseReferenceDate),
            settlementDate_(QuantLib::Date()),
            settlementDays_(QuantLib::Null<QuantLib::Natural>()),
            maturityDate_(QuantLib::Date()),
            startDate_(QuantLib::Date()),
            settleOnCouponPayment_(QuantLib::Null<bool>())
        {
            QL_REQUIRE(timeToMaturity.length() > 0, "bond's time to maturity must be positive in time");
            QL_REQUIRE(forwardSettlePeriod.length() >= 0, "forward settle cannot be negative in time");
            initialize();
        }
        const QuantLib::Period& timeToMaturity() const {
            return timeToMaturity_;
        }
        const QuantLib::Period& forwardSettlePeriod() const {
            return forwardSettlePeriod_;
        }
        const QuantLib::Date& baseReferenceDate() const {
            return baseReferenceDate_;
        }
        const QuantLib::Date& settlementDate() const {
            return settlementDate_;
        }
        const QuantLib::Natural& settlementDays() const {
            return settlementDays_;
        }
        const QuantLib::Date& maturityDate() const {
            return maturityDate_;
        }
        const QuantLib::Date& startDate() const {   // theoretical start date of the bond
            return startDate_;
        }
        const bool& settleOnCouponPayment() const {
            return settleOnCouponPayment_;
        }
        const QuantLib::Schedule& schedule() const {
            return schedule_;
        }
    };

    template <QuantLib::Frequency COUPON_FREQ = QuantLib::Semiannual, typename TENOR_COUPONED = ParBondTenorCouponedWithCutoffMonths<>>
    class ParYieldHelper {
    protected:
        typedef TheoreticalBondScheduler<COUPON_FREQ> ParBondScheduler;
        QuantLib::Period tenor_;
        QuantLib::Rate parYield_;
        QuantLib::Date baseReferenceDate_;  // base reference date
    public:
        ParYieldHelper(const QuantLib::Period& tenor) :
            tenor_(tenor),
            parYield_(QuantLib::Null<QuantLib::Rate>()),
            baseReferenceDate_(QuantLib::Date())
        {}
        const QuantLib::Period& tenor() const { return tenor_; }
        
        ParYieldHelper& withParYield(const QuantLib::Rate& parYield) {
            parYield_ = parYield;
            return *this;
        }
        ParYieldHelper& withBaseReferenceDate(const QuantLib::Date& baseReferenceDate) {
            baseReferenceDate_ = baseReferenceDate;
            return *this;
        }
        static bool tenorIsCouponed(const QuantLib::Period& tenor) {
            TENOR_COUPONED tenorCouponed;
            return tenorCouponed(tenor);
        }
        static QuantLib::Frequency frequency() {
            return ParBondScheduler::frequency();
        }
        // returns number of months between coupon payments
        static QuantLib::Natural couponTenorMonths() {
            return ParBondScheduler::couponTenorMonths();
        }
        // returns day counter for the par bond calculation
        static QuantLib::DayCounter parBondDayCounter() {
            return QuantLib::Thirty360(QuantLib::Thirty360::BondBasis);
        }
    public:
        // create spot FixedRateBondHelper for discount curve bootstraping (par yield => zero curve)
        operator QuantLib::ext::shared_ptr<QuantLib::FixedRateBondHelper>() const {
            QL_REQUIRE(parYield_ != QuantLib::Null<QuantLib::Rate>(), "par yield not set");
            ParBondScheduler parBondSched(tenor_, 0 * QuantLib::Days, baseReferenceDate_); // for theoretical bond settle on the spot
            auto const& schedule = parBondSched.schedule();
            auto const& settlementDate = parBondSched.settlementDate();
            auto const& maturityDate = parBondSched.maturityDate();
            auto const& settlementDays = parBondSched.settlementDays();
            QuantLib::DayCounter dc = parBondDayCounter();
            QuantLib::Real notional = 100.0;
            QuantLib::Rate coupon = QuantLib::Null<QuantLib::Rate>();
            QuantLib::Real targetPrice = QuantLib::Null<QuantLib::Real>();    // target price
            QuantLib::Bond::Price::Type targetPriceType = QuantLib::Bond::Price::Clean;     // target for the clean price. for zero coupon bond, dirty price = clean price because accrued interest is 0 (coupon = 0)
            if (tenorIsCouponed(tenor_)) {   // couponed bond
                coupon = parYield_;  // for par bond, coupon is par yield
                targetPrice = notional;  // target the clean price at par
            }
            else {  // zero coupon bond
                coupon = 0.0;  // zero coupon bond
                // df = 1.0/pow(1 + yield/freq, yearFraction * freq), price = df * notional
                QuantLib::InterestRate ir(parYield_, dc, QuantLib::Compounding::Compounded, frequency());
                auto discountFactor = ir.discountFactor(settlementDate, maturityDate);
                targetPrice = discountFactor * notional; // zero coupon bond is sold at a discount, so this is the target price
            }
            auto quote = QuantLib::ext::make_shared<QuantLib::SimpleQuote>(targetPrice);    // make target price the quote
            return QuantLib::ext::shared_ptr<QuantLib::FixedRateBondHelper>(new QuantLib::FixedRateBondHelper(
                QuantLib::Handle<QuantLib::Quote>(quote),
                settlementDays,
                notional,
                schedule,
                std::vector<QuantLib::Rate>(1, coupon),
                dc,
                QuantLib::Unadjusted,
                notional,
                QuantLib::Date(),
                QuantLib::Calendar(),
                QuantLib::Period(),
                QuantLib::Calendar(),
                QuantLib::Unadjusted,
                false,
                targetPriceType
            ));
        }
        QuantLib::ext::shared_ptr<QuantLib::Bond> parBond() {
            auto helper = this->operator boost::shared_ptr<QuantLib::FixedRateBondHelper>();
            return helper->bond();
        }
        // calculate par yield for the given the discounting term structure (zero curve => par yield)
        static QuantLib::Rate parYield(const QuantLib::ext::shared_ptr<QuantLib::YieldTermStructure>& discountTermStructure, const QuantLib::Period& tenor, const QuantLib::Period& forwardTerm = QuantLib::Period(0, QuantLib::Days)) {
            QL_REQUIRE(discountTermStructure != nullptr, "discount term structure not set");
            ParBondScheduler parBondSched(tenor, forwardTerm, discountTermStructure->referenceDate());
            auto const& schedule = parBondSched.schedule();
            auto const& settlementDate = parBondSched.settlementDate();
            auto const& maturityDate = parBondSched.maturityDate();
            auto const& settlementDays = parBondSched.settlementDays();
            QuantLib::DayCounter dc = parBondDayCounter();
            if (tenorIsCouponed(tenor)) { // couponed bond
                auto const& settleOnCouponPayment = parBondSched.settleOnCouponPayment();
                if (settleOnCouponPayment) {   // bond's time to maturity is an integer multiple of coupon tenor
                    // par yield can be solved analytically
                    QuantLib::Real annuity = 0.0;
                    QuantLib::DiscountFactor dfLast = 1.0;
                    auto dfStart = discountTermStructure->discount(settlementDate);
                    for (auto it = schedule.begin() + 1; it != schedule.end(); ++it) {
                        auto const& cfDate = *it;
                        dfLast = discountTermStructure->discount(cfDate)/ dfStart;
                        annuity += dfLast;
                    }
                    QuantLib::Rate parYield = (1.0 - dfLast) / annuity * (double)frequency();
                    return parYield;
                }
                else {
                    // need a solver to solve for par yield
                    QuantLib::RelinkableHandle<QuantLib::YieldTermStructure> discountCurve;
                    QuantLib::ext::shared_ptr<QuantLib::PricingEngine> pricingEngine(new QuantLib::DiscountingBondEngine(discountCurve));
                    discountCurve.linkTo(discountTermStructure);
                    auto accrualDayCounter = dc; 
                    QuantLib::Solver1D<QuantLib::Bisection> solver;
                    QuantLib::Real accuracy = 1.0e-18;
                    QuantLib::Real guess = discountTermStructure->zeroRate(maturityDate, dc, QuantLib::Compounding::Compounded, frequency());
                    solver.setMaxEvaluations(1000);
                    QuantLib::Rate parYield = solver.solve([&pricingEngine, &settlementDays, &schedule, &accrualDayCounter](const QuantLib::Rate& coupon) -> QuantLib::Real {
                        // par coupon solver functor for fixed rate couponed bond
                        // given a settlement days, fixed rate bond schedule, accrual day counter, and a bond pricing engine, can we find it's par coupon?
                        QuantLib::Real notional = 100.0;
                        QuantLib::FixedRateBond bond(settlementDays, notional, schedule, std::vector<QuantLib::Rate>(1, coupon), accrualDayCounter, QuantLib::Unadjusted, notional);
                        bond.setPricingEngine(pricingEngine);
                        auto targetCleanPrice = notional;    // target clean price at par
                        return bond.cleanPrice() - targetCleanPrice;
                    }, accuracy, guess, guess / 10.0);
                    return parYield;
                }
            }
            else {  // zero coupon bond => no need to use solver
                auto dfStart = discountTermStructure->discount(settlementDate);
                auto dfEnd = discountTermStructure->discount(maturityDate);
                auto compound = dfStart / dfEnd;
                auto ir = QuantLib::InterestRate::impliedRate(compound, dc, QuantLib::Compounding::Compounded, frequency(), settlementDate, maturityDate);
                auto parYield = ir.rate();
                return parYield;
            }
        }
    };
}