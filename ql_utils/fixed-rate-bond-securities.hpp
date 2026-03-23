#pragma once

#include <ql_utils/instrument.hpp>
#include <vector>
#include <memory>
#include <cmath>
#include <algorithm> // Required for std::reverse

namespace QuantLib {
    namespace Utils {
        template <
            typename BondTraits
        >
        class FixedCoupondedBond :
            public QLUtils::BootstrapInstrument,
            public QLUtils::IParYieldSplineNode {
        protected:
			BondTraits bondTraits_;
            Rate coupon_;
			Schedule accrualSchedule_;
            Calendar settlementCalendar_;
            Calendar accrualScheduleCalendar_;
            DayCounter accrualDayCounter_;
            DayCounter yieldCalcDayCounter_;
            Calendar paymentCalendar_;
            DayCounter parYieldSplineDayCounter_;
        public:
            struct AccruedPeriod {
                Date accrualStartDate;
                Date accrualEndDate;
                Date paymentDate;
                AccruedPeriod(Date accrualStart = {}, Date accrualEnd = {}, Date paymentDate = {})
                    : accrualStartDate(accrualStart), accrualEndDate(accrualEnd), paymentDate(paymentDate)
                {}
			};
			typedef std::vector<AccruedPeriod> AccruedPeriods;
        protected:
            // set the maturity date
            Date& maturityDate() {
                return datedDate();
            }
            ext::shared_ptr<FixedRateBondHelper> makeFixedRateBondHelper() const {
                auto targetPrice = cleanPrice();
                auto priceType = Bond::Price::Clean;
                auto quote = ext::make_shared<SimpleQuote>(targetPrice);
                return ext::shared_ptr<FixedRateBondHelper>(new FixedRateBondHelper(
                    Handle<Quote>(quote),               // price
                    settlementDays(),                   // settlementDays
                    parNotional(),                      // faceAmount
                    accrualSchedule(),                  // schedule
                    std::vector<Rate>{coupon()},        // coupons
                    accrualDayCounter(),                // accrualDayCounter
                    paymentConvention(),                // paymentConv
                    parNotional(),                      // redemption
                    Date(),                             // issueDate
                    paymentCalendar(),                  // paymentCalendar
                    Period(),                           // exCouponPeriod
                    Calendar(),                         // exCouponCalendar
                    BusinessDayConvention::Unadjusted,  // exCouponConvention
                    priceType                           // priceType
                ));
            }
            std::shared_ptr<FixedRateBond> makeFixedRateBond(
                Rate coupon = Null<Rate>()
            ) const {
                if (coupon == Null<Rate>()) {
                    coupon = this->coupon();
                }
                return std::shared_ptr<FixedRateBond>(new FixedRateBond(
                    settlementDays(),               // settlementDays
                    parNotional(),                  // faceAmount
                    accrualSchedule(),              // schedule
                    std::vector<Rate>{coupon},      // coupons
                    accrualDayCounter(),            // accrualDayCounter
                    paymentConvention(),            // paymentConvention
                    parNotional(),                  // redemption
                    Date(),                         // issueDate
                    paymentCalendar()               // paymentCalendar
                ));
            }
            // is if p1 is a multiple of p2 ?
            static std::pair<bool, Integer> isMultiple(const Period& p1, const Period& p2) {
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
        private:
            // create a forward schedule of n coupon periods starting from the startDate
            Schedule createForwardSchedule(
                const Date& effectiveDate, Natural numCoupons
            ) const {
                auto tenor = this->couponTenor();
                auto p = tenor * (numCoupons + 5);
                auto terminationDate = effectiveDate + p;
                auto calendar = this->accrualScheduleCalendar();
                auto convention = this->accrualConvention();
                auto terminationDateConvention = BusinessDayConvention::Unadjusted;
                auto rule = DateGeneration::Rule::Forward;
                auto endOfMonth = this->accrualEndOfMonth();
                Schedule schedule(
                    effectiveDate,  // effectiveDate
                    terminationDate,    // terminationDate
                    tenor,    // tenor
                    calendar,   // calendar
                    convention,                     // convention
                    terminationDateConvention,  // terminationDateConvention
                    rule,   // rule
                    endOfMonth  // endOfMonth
                );
                QL_ASSERT(schedule.dates().size() >= numCoupons + 1, "The number of dates in the schedule (" << schedule.dates().size() << ") is less than minimum exprected (" << (numCoupons + 1) << ")");
                std::vector<Date> dates(schedule.dates().begin(), schedule.dates().begin() + (numCoupons + 1));
                Schedule sched(
                    dates,
                    calendar,
                    convention,
                    terminationDateConvention,
                    tenor,
                    rule,
                    endOfMonth
                );
                return sched;
            }
            Schedule createBackwardSchedule(
                const Date& startDate,
                const Date& terminationDate
            ) {
                auto tenor = this->couponTenor();
                auto p = tenor * 5;
                auto effectiveDate = startDate - p;
                auto calendar = this->accrualScheduleCalendar();
                auto convention = this->accrualConvention();
                auto terminationDateConvention = BusinessDayConvention::Unadjusted;
                auto rule = DateGeneration::Rule::Backward;
                auto endOfMonth = this->accrualEndOfMonth();
                Schedule schedule(
                    effectiveDate,  // effectiveDate
                    terminationDate,    // terminationDate
                    tenor,    // tenor
                    calendar,   // calendar
                    convention,                     // convention
                    terminationDateConvention,  // terminationDateConvention
                    rule,   // rule
                    endOfMonth  // endOfMonth
                );
                std::vector<Date> dates;
                for (auto it = schedule.dates().rbegin(); it != schedule.dates().rend(); ++it) {
                    dates.push_back(*it);
                    if (*it <= startDate) {
                        break;
                    }
                }
                std::reverse(dates.begin(), dates.end());
                Schedule sched(
                    dates,
                    calendar,
                    convention,
                    terminationDateConvention,
                    tenor,
                    rule,
                    endOfMonth
                );
                return sched;
            }
        public:
            FixedCoupondedBond(
                const Period& tenor,    // bond tenor
                const Date& maturityDate = Date(),
                Rate coupon = 0.0
			) :
                QLUtils::BootstrapInstrument(QLUtils::BootstrapInstrument::vtPrice, tenor, maturityDate),
                coupon_(coupon),
				settlementCalendar_(bondTraits_.settlementCalendar(tenor)),
                accrualScheduleCalendar_(bondTraits_.accrualScheduleCalendar(tenor)),
                paymentCalendar_(bondTraits_.paymentCalendar(tenor))
            {
                QL_REQUIRE(tenor.length() > 0, "The length of the tenor (" << tenor.length() << ") must be greater than 0");
                auto settleDate = this->settlementDate();
				if (maturityDate == Date()) {  // maturity date is not given
                    auto [multipile, n] = isMultiple(tenor, this->couponTenor());
                    if (multipile) {    // tenor is a multiple of coupon period => create a forward bond schedule of n coupon periods starting fro the settlement date
                        QL_ASSERT(n > 0, "the number of coupon accrual periods (" << n << ") must be greater than 0");
                        accrualSchedule_ = createForwardSchedule(settleDate, (Natural)n);
                        QL_ASSERT(accrualSchedule_.dates().size() - 1 == n, "The number of coupon accrual periods (" << (accrualSchedule_.dates().size() - 1) << ") is not what's expected (" << n << ")");
                        QL_ASSERT(settleDate == accrualSchedule_.dates().front(), "Settlement date (" << settleDate << ") does not match the start date (" << accrualSchedule_.dates().front() << ") of the first coupon accrual period");
                        this->maturityDate() = accrualSchedule_.dates().back();    // set the maturity date to the last date of the schedule
                    }
                    else {  // tenor is not a mutiple of the coupon period
                        this->maturityDate() = settleDate + tenor;    // set the maturity date to be the settle date plus the tenor
                        accrualSchedule_ = createBackwardSchedule(settleDate, this->maturityDate());    // create a schedule goting backward
                        QL_ASSERT(this->maturityDate() == accrualSchedule_.dates().back(), "The end date of the last coupon accrual period (" << accrualSchedule_.dates().back() << ") is not what's expected (" << this->maturityDate() << ")");
                        QL_ASSERT(accrualSchedule_.dates().front() <= settleDate, "The start date of the first coupon accrual period (" << accrualSchedule_.dates().front() << ") is greater than the settlement date (" << settleDate << ")");
                    }
                }
                else {  // maturity date is given => created a backward bond schedule from the maturity date
                    accrualSchedule_ = createBackwardSchedule(settleDate, maturityDate);
                    QL_ASSERT(this->maturityDate() == accrualSchedule_.dates().back(), "The end date of the last coupon accrual period (" << accrualSchedule_.dates().back() << ") is not what's expected (" << this->maturityDate() << ")");
                    QL_ASSERT(accrualSchedule_.dates().front() <= settleDate, "The start date of the first coupon accrual period (" << accrualSchedule_.dates().front() << ") is greater than the settlement date (" << settleDate << ")");
                }
				accrualDayCounter_ = bondTraits_.accrualDayCounter(tenor, accrualSchedule_);
				yieldCalcDayCounter_ = bondTraits_.yieldCalcDayCounter(tenor, accrualSchedule_);
				parYieldSplineDayCounter_ = bondTraits_.parYieldSplineDayCounter(tenor, accrualSchedule_);
            }
            const Calendar& settlementCalendar() const {
                return settlementCalendar_;
			}
            Natural settlementDays() const{
				return bondTraits_.settlementDays(tenor());
            }
            Date settlementDate() const {
                auto today = Settings::instance().evaluationDate();
				auto d = settlementCalendar().advance(today, settlementDays(), Days, Following);
                return d;
            }
            Real parNotional() const {
                return bondTraits_.parNotional(tenor());
			}
            const Rate& coupon() const {
                return coupon_;
			}
            Rate& coupon() {
				return coupon_;
			}
            bool isZeroCoupon() const {
                return (coupon_ == 0.);
            }
            Frequency couponFrequency() const {
                return bondTraits_.couponFrequency(tenor());
            }
            Period couponTenor() const {
                return Period(couponFrequency());
            }
            Date startDate() const {
                return settlementDate();
            }
			// bond maturity date
            Date maturityDate() const {
                return datedDate();
			}
            const Calendar& accrualScheduleCalendar() const {
                return accrualScheduleCalendar_;
            }
            BusinessDayConvention accrualConvention() const {
                return bondTraits_.accrualConvention(tenor());
            }
            bool accrualEndOfMonth() const {
                return bondTraits_.accrualEndOfMonth(tenor());
            }
            const Schedule& accrualSchedule() const {
                return accrualSchedule_;
            }
            const DayCounter& accrualDayCounter() const {
                return accrualDayCounter_;
            }
            const Calendar& paymentCalendar() const {
                return paymentCalendar_;
			}
            BusinessDayConvention paymentConvention() const {
				return bondTraits_.paymentConvention(tenor());
			}
            Date lastPaymentDate() const {
                auto lastAccrualEndDate = accrualSchedule_.dates().back();
				auto d = paymentCalendar().adjust(lastAccrualEndDate, paymentConvention());
				return d;
            }
            bool cleanPriceIsSet() const {
                return this->valueIsSet();
            }
            void checkCleanPriceIsSet() const {
                QL_REQUIRE(cleanPriceIsSet(), "clean price is not set for the bond");
            }
			// get the clean price, which is the price used for bootstrapping, from the price of the bond
            const Real& cleanPrice() const {
                return price();
            }
			// set the clean price, which is the price used for bootstrapping, to the price of the bond
            Real& cleanPrice() {
                return price();
            }
            Real accruedAmmount() const {
                auto bond = makeFixedRateBond();
                return bond->accruedAmount();
            }
            Real dirtyPrice() const {
                return cleanPrice() + accruedAmmount();
            }
            operator AccruedPeriods() const {
                AccruedPeriods accruedPeriods;
                auto scheduleDates = accrualSchedule_.dates();
                for (Size i = 1; i < scheduleDates.size(); ++i) {
                    auto accrualStartDate = scheduleDates[i - 1];
                    auto accrualEndDate = scheduleDates[i];
                    auto paymentDate = paymentCalendar().adjust(accrualEndDate, paymentConvention());
                    accruedPeriods.push_back(AccruedPeriod{accrualStartDate, accrualEndDate, paymentDate});
                }
                return accruedPeriods;
			}
            const DayCounter& yieldCalcDayCounter() const {
                return yieldCalcDayCounter_;
            }
            const DayCounter& parYieldSplineDayCounter() const {
                return parYieldSplineDayCounter_;
            }
        protected:
            // solver setting functions
            /////////////////////////////////////////////////////////////////
            static Real solverAccuracy() {
                return 1.0e-16;
            }
            // guess for yield/coupon
            static Real solverRateQuess() {
                return 0.05;    // guess for yield/coupon = 5%
            }
            static Real solverRateStep() {
                return 0.0010; // 10 bp step
            }
            static Size solverMaxIterations() {
                return 300;
            }
            /////////////////////////////////////////////////////////////////
        protected:
            // calculate bond's yield to maturity given bond price
            Rate bondYield(
                const FixedRateBond& bond,
                const Bond::Price& price
            ) const {
                auto yield = bond.yield(
                    price,
                    yieldCalcDayCounter(),
                    Compounding::Compounded,
                    couponFrequency(),
                    settlementDate(),
                    solverAccuracy(),
                    solverMaxIterations(),
                    solverRateQuess()
                );
                return yield;
            }
            // calculate bond's price given yield to maturity
            Real bondPrice(
                const FixedRateBond& bond,
                Rate yield,
                Bond::Price::Type priceType
            ) const {
                return (
                    priceType == Bond::Price::Type::Clean ?
                    bond.cleanPrice(
                        yield,
                        yieldCalcDayCounter(),
                        Compounding::Compounded,
                        couponFrequency(),
                        settlementDate()
                    ) :
                    bond.dirtyPrice(
                        yield,
                        yieldCalcDayCounter(),
                        Compounding::Compounded,
                        couponFrequency(),
                        settlementDate()
                    )
                );
            }
            // calculate bond's DV01 given yield to maturity
            Real bondDV01(
                const FixedRateBond& bond,
                Rate yield
            ) const {
                auto dv01 = std::abs(BondFunctions::basisPointValue(
                    bond,
                    yield,
                    yieldCalcDayCounter(),
                    Compounding::Compounded,
                    couponFrequency(),
                    settlementDate()
                ) / parNotional());
                return dv01;
            }
        public:
            // given bond's quoted clean price, what is it's yield to maturity
            Rate ytm() const {
                checkCleanPriceIsSet();
                auto bond = makeFixedRateBond();
                Bond::Price price(cleanPrice(), Bond::Price::Type::Clean);
                return bondYield(*bond, price);
			}
            // given bond's quoted clean price, what is it's DV01
            Real dv01() const {
                checkCleanPriceIsSet();
                auto bond = makeFixedRateBond();
                Bond::Price price(cleanPrice(), Bond::Price::Type::Clean);
                auto ytm = bondYield(*bond, price);
                auto dv01 = bondDV01(*bond, ytm);
                return dv01;
            }
        public:
            // implied by discount term structure functions
            /////////////////////////////////////////////////////////////////////////////////////////////
            Real impliedPrice(
                const Handle<YieldTermStructure>& discountingTermStructure,
                Bond::Price::Type priceType
            ) const {
                auto bond = makeFixedRateBond();
                ext::shared_ptr<PricingEngine> pricingEngine(new DiscountingBondEngine(discountingTermStructure));
                bond->setPricingEngine(pricingEngine);
                return (priceType == Bond::Price::Type::Clean ? bond->cleanPrice() : bond->dirtyPrice());
            }
            Real impliedDirtyPrice(
                const Handle<YieldTermStructure>& discountingTermStructure
            ) const {
                return impliedPrice(discountingTermStructure, Bond::Price::Type::Dirty);
            }
            Real impliedCleanPrice(
                const Handle<YieldTermStructure>& discountingTermStructure
            ) const {
                return impliedPrice(discountingTermStructure, Bond::Price::Type::Clean);
            }
            Rate impliedYTM(
                const Handle<YieldTermStructure>& discountingTermStructure
            ) const {
                auto bond = makeFixedRateBond();
                ext::shared_ptr<PricingEngine> pricingEngine(new DiscountingBondEngine(discountingTermStructure));
                bond->setPricingEngine(pricingEngine);
                auto dirtyPrice = bond->dirtyPrice();
                Bond::Price price(dirtyPrice, Bond::Price::Type::Dirty);
                auto ytm = bondYield(*bond, price);
                return ytm;
            }
            Real impliedDV01(
                const Handle<YieldTermStructure>& discountingTermStructure
            ) const {
                auto bond = makeFixedRateBond();
                ext::shared_ptr<PricingEngine> pricingEngine(new DiscountingBondEngine(discountingTermStructure));
                bond->setPricingEngine(pricingEngine);
                auto dirtyPrice = bond->dirtyPrice();
                Bond::Price price(dirtyPrice, Bond::Price::Type::Dirty);
                auto ytm = bondYield(*bond, price);
                auto dv01 = bondDV01(*bond, ytm);
                return dv01;
            }
            // given a discount term structure, what coupon would give the bond a clean price of 100
            // this function can be used to calculate CMT rate if the bond is a CMT bond
            Rate impliedParCoupon(
                const Handle<YieldTermStructure>& discountingTermStructure
            ) const {
                const auto& me = *this;
                Real targetCleanPrice = 100.;
                auto f = [&me, &discountingTermStructure, &targetCleanPrice](Rate coupon) -> Real {
                    auto bond = me.makeFixedRateBond(coupon);
                    ext::shared_ptr<PricingEngine> pricingEngine(new DiscountingBondEngine(discountingTermStructure));
                    bond->setPricingEngine(pricingEngine);
                    auto cleanPrice = bond->cleanPrice();
                    return cleanPrice - targetCleanPrice;
                };
                Brent solver;
                solver.setMaxEvaluations(solverMaxIterations());
                Rate parCoupon = solver.solve(f, solverAccuracy(), solverRateQuess(), solverRateStep());
                return parCoupon;
            }
            /////////////////////////////////////////////////////////////////////////////////////////////
            ext::shared_ptr<RateHelper> rateHelper(
                const Handle<YieldTermStructure>& discountingTermStructure = Handle<YieldTermStructure>()
            ) const {
                return makeFixedRateBondHelper();
            }
            // quote is clean proce of the bond
            Real impliedQuote(
                const Handle<YieldTermStructure>& estimatingTermStructure,
                const Handle<YieldTermStructure>& discountingTermStructure = Handle<YieldTermStructure>()
            ) const {
                return impliedCleanPrice(discountingTermStructure);
            }

            Time parTerm() const {
				return parYieldSplineDayCounter().yearFraction(settlementDate(), maturityDate());
            }
            Rate parYield() const {
                return ytm();
            }
        };

        template <
            typename BillTraits
        >
        class ZeroCouponBill : public FixedCoupondedBond<typename BillTraits::BondTraits> {
        protected:
            BillTraits billTraits_;
			Schedule marketConventionSchedule_; // forward schedule (from the settle date) of the tenor length, used for calculating market convention yield
        private:
            Schedule createForwardSchedule(
                const Date& effectiveDate,
                const Date& endDate
            ) const {
                auto tenor = this->couponTenor();
                auto p = tenor * 5;
                auto terminationDate = endDate + p;
                auto calendar = this->accrualScheduleCalendar();
                auto convention = this->accrualConvention();
                auto terminationDateConvention = BusinessDayConvention::Unadjusted;
                auto rule = DateGeneration::Rule::Forward;
                auto endOfMonth = this->accrualEndOfMonth();
                Schedule schedule(
                    effectiveDate,              // effectiveDate
                    terminationDate,            // terminationDate
                    tenor,                      // tenor
                    calendar,                   // calendar
                    convention,                 // convention
                    terminationDateConvention,  // terminationDateConvention
                    rule,                       // rule
                    endOfMonth                  // endOfMonth
                );
                std::vector<Date> dates;
                for (auto it = schedule.dates().begin(); it != schedule.dates().end(); ++it) {
                    dates.push_back(*it);
                    if (*it >= endDate) {
                        break;
                    }
                }
                Schedule sched(
                    dates,
                    calendar,
                    convention,
                    terminationDateConvention,
                    tenor,
                    rule,
                    endOfMonth
                );
                return sched;
			}
        public:
            ZeroCouponBill(
                const Period& tenor,
                const Date& maturityDate
            ) : FixedCoupondedBond<typename BillTraits::BondTraits>(tenor, maturityDate, 0.0)
            {
                auto settleDate = this->settlementDate();
                auto paymentDate = this->paymentDate();
                marketConventionSchedule_ = createForwardSchedule(settleDate, paymentDate);
                QL_ASSERT(settleDate == marketConventionSchedule_.dates().front(), "Settlement date (" << settleDate << ") does not match the start date (" << marketConventionSchedule_.dates().front() << ") of the first coupon accrual period");
                QL_ASSERT(marketConventionSchedule_.dates().back() >= paymentDate, "The end date (" << marketConventionSchedule_.dates().back() << ") of the last coupon accrual period is less than the payment date (" << paymentDate << ")");
            }
            // date of the sole payment (principal/face amount) for the bill
            Date paymentDate() const {
                return this->lastPaymentDate();
            }
            const Schedule& marketConventionSchedule() const {
                return marketConventionSchedule_;
			}
        };

        template <
            typename BondTraits
        >
        class CMTBond : public FixedCoupondedBond<BondTraits> {
        public:
            CMTBond(
                const Period& tenor,
                Rate coupon = 0.0
            ) :FixedCoupondedBond<BondTraits>(tenor, Date(), coupon) {
				this->cleanPrice() = this->parNotional();   // default the clean price of the bond to par by assuming the coupon variable is a par coupon
            }
        };
    }
}
