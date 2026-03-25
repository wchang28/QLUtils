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
        class FixedCoupondBond :
            public QLUtils::BootstrapInstrument,
            public QLUtils::IParYieldSplineNode {
        protected:
			BondTraits bondTraits_;                 // bond traits
			Rate coupon_;                           // bond's fixed coupon rate
			Date settlementDate_;                   // settlement date for the bond
			Calendar settlementCalendar_;           // calendar for the settlement date
			Schedule accrualSchedule_;              // accrual schedule for the bond
			Calendar accrualScheduleCalendar_;      // calendar for the accrual schedule
			DayCounter accrualDayCounter_;          // day counter for the accrual schedule
			Schedule yieldCalcSchedule_;            // schedule for yield calculation (e.g. par yield calculation)
			DayCounter yieldCalcDayCounter_;        // day counter for yield calculation (e.g. par yield calculation)
			Calendar paymentCalendar_;              // calendar for the payment date
            DayCounter parYieldSplineDayCounter_;
        protected:
            // set the maturity date
            void setMaturityDate(
                const Date& maturityDate
            ) {
                this->datedDate() = maturityDate;
            }
			// given today's date/valuation date, find the number of settlement days that would yield the expected settlement date for this bond
            Natural getMatchingSettlementDays() const {
				auto targetSettlementDate = settlementDate();
                auto today = Settings::instance().evaluationDate();
                QL_ASSERT(today != Date(), "evaluation date/today not set");
                QL_REQUIRE(today <= targetSettlementDate, "evaluation date/today is after the target settlement date");
                auto n = settlementDays();
				auto d = settlementCalendar().advance(today, n, Days);
                if (d < targetSettlementDate) {
                    while (d < targetSettlementDate) {
                        n++;
                        d = settlementCalendar().advance(today, n, Days);
					}
                }
                else if (d > targetSettlementDate) {
                    while (d > targetSettlementDate) {
                        n--;
                        d = settlementCalendar().advance(today, n, Days);
                    }
                }
                QL_ASSERT(d == targetSettlementDate, "settlement calendar advance logic did not yield the expected settlement date (" << targetSettlementDate << ")");
                QL_ASSERT(n > 0, "n (" << n << ") is not positive");
                QL_ASSERT(settlementCalendar().advance(today, n, Days) == targetSettlementDate, "settlement calendar advance logic did not yield the expected settlement date(" << targetSettlementDate << ")");
                return n;
			}
        public:
            ext::shared_ptr<FixedRateBondHelper> makeFixedRateBondHelper() const {
				auto targetPrice = cleanPrice();    // bootstrap target price is the clean price
                auto priceType = Bond::Price::Clean;
                auto quote = ext::make_shared<SimpleQuote>(targetPrice);
				auto settlementDays = getMatchingSettlementDays();
                ext::shared_ptr<FixedRateBondHelper> helper(new FixedRateBondHelper(
                    Handle<Quote>(quote),               // price
                    settlementDays,                     // settlementDays
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
				auto bond = helper->bond();
                QL_ASSERT(bond != nullptr, "bond helper's bond is null");
                auto defaultSettleDate = bond->settlementDate();
                QL_ASSERT(defaultSettleDate == settlementDate(), "bond's default settlement logic's settlement date (" << defaultSettleDate << ") is not what's expected (" << settlementDate() << ")");
                auto bondMaturityDate = bond->maturityDate();
                QL_ASSERT(bondMaturityDate == maturityDate(), "bond's maturity date (" << bondMaturityDate << ") is not what's expected (" << maturityDate() << ")");
                auto lastPaymentDate = this->lastPaymentDate();
                auto pillarDate = helper->pillarDate();
                QL_ASSERT(pillarDate == lastPaymentDate, "bond helper's pillar date (" << pillarDate << ") is not what's expected (" << lastPaymentDate << ")");
				return helper;
            }
			// create a FixedRateBond with the same parameters as this instrument, except for the coupon which can be overridden
            ext::shared_ptr<FixedRateBond> makeFixedRateBond(
                Rate coupon = Null<Rate>()
            ) const {
                if (coupon == Null<Rate>()) {
                    coupon = this->coupon();
                }
                auto settlementDays = getMatchingSettlementDays();
                ext::shared_ptr<FixedRateBond> bond(new FixedRateBond(
                    settlementDays,                 // settlementDays
                    parNotional(),                  // faceAmount
                    accrualSchedule(),              // schedule
                    std::vector<Rate>{coupon},      // coupons
                    accrualDayCounter(),            // accrualDayCounter
                    paymentConvention(),            // paymentConvention
                    parNotional(),                  // redemption
                    Date(),                         // issueDate
                    paymentCalendar()               // paymentCalendar
                ));
                auto defaultSettleDate = bond->settlementDate();
                QL_ASSERT(defaultSettleDate == settlementDate(), "bond's default settlement logic's settlement date (" << defaultSettleDate << ") is not what's expected (" << settlementDate() << ")");
                auto bondMaturityDate = bond->maturityDate();
                QL_ASSERT(bondMaturityDate == maturityDate(), "bond's maturity date (" << bondMaturityDate << ") is not what's expected (" << maturityDate() << ")");
                return bond;
            }
            Leg makeLeg(
                Rate coupon = Null<Rate>()
            ) const {
                if (coupon == Null<Rate>()) {
                    coupon = this->coupon();
                }
                Leg leg;
                auto scheduleDates = accrualSchedule_.dates();
                Date lastPaymentDate = Date();
                for (Size i = 1; i < scheduleDates.size(); ++i) {
                    auto accrualStartDate = scheduleDates[i - 1];
                    auto accrualEndDate = scheduleDates[i];
                    auto paymentDate = paymentCalendar().adjust(accrualEndDate, paymentConvention());
					lastPaymentDate = paymentDate;
                    ext::shared_ptr<FixedRateCoupon> pCoupon(new FixedRateCoupon(
                        paymentDate,            // paymentDate
                        parNotional(),          // nominal
                        coupon,                 // rate
                        accrualDayCounter(),    // dayCounter
                        accrualStartDate,       // accrualStartDate
                        accrualEndDate          // accrualEndDate
                    ));
                    leg.push_back(pCoupon);
                }
                ext::shared_ptr<Redemption> pRedemption(new Redemption(
                    parNotional(),      // amount
                    lastPaymentDate     // date
                ));
                leg.push_back(pRedemption);
                return leg;
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
        private:
            // create a forward schedule of n coupon periods starting from the startDate
            Schedule createForwardSchedule(
                const Date& effectiveDate,
                Natural numCoupons
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
                    effectiveDate,              // effectiveDate
                    terminationDate,            // terminationDate
                    tenor,                      // tenor
                    calendar,                   // calendar
                    convention,                 // convention
                    terminationDateConvention,  // terminationDateConvention
                    rule,                       // rule
                    endOfMonth                  // endOfMonth
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
            ) const {
				QL_REQUIRE(startDate < terminationDate, "start date (" << startDate << ") must be before the termination date (" << terminationDate << ")");
                auto tenor = this->couponTenor();
                auto p = tenor * 5;
                auto effectiveDate = startDate - p;
                auto calendar = this->accrualScheduleCalendar();
                auto convention = this->accrualConvention();
                auto terminationDateConvention = BusinessDayConvention::Unadjusted;
                auto rule = DateGeneration::Rule::Backward;
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
            // create a schedule for yield calculation by extending the accrual schedule to include the last payment date if necessary
            // this is needed because yield calculation is also dependent on the payment dates
            Schedule makeYieldCalcSchedule() const {
				auto lastPaymentDate = this->lastPaymentDate();
				auto maturityDate = this->maturityDate();
                if (lastPaymentDate > maturityDate) {
                    // extends the schedule to include the last payment date
                    std::vector<Date> dates = accrualSchedule_.dates();
                    auto d = maturityDate;
                    while (d < lastPaymentDate) {
                        d = accrualScheduleCalendar().advance(d, couponTenor(), accrualConvention(), accrualEndOfMonth());
                        dates.push_back(d);
					}
                    QL_ASSERT(dates.back() >= lastPaymentDate, "The last date in the yield calculation schedule (" << dates.back() << ") does not match the last payment date (" << lastPaymentDate << ")");
                    Schedule schedule(
                        dates,
						accrualSchedule_.calendar(),    // calendar
						accrualSchedule_.businessDayConvention(),   // convention
						accrualSchedule_.terminationDateBusinessDayConvention(),    // terminationDateConvention
                        accrualSchedule_.tenor(),          // tenor
                        accrualSchedule_.rule(),    // rule
						accrualSchedule_.endOfMonth()  // endOfMonth
					);
                    return schedule;
                } else {    // lastPaymentDate <= maturityDate
					// the accrual schedule already covers all payment dates, so it can be used for yield calculation
					return accrualSchedule_;
				}
            }
        public:
            FixedCoupondBond(
                Period tenor,                       // bond's claimed original tenor
				Date maturityDate = Date(),         // bond's maturity date, if not given, it will be calculated based on the settlement date and the tenor
				Rate coupon = 0.,                   // bond's fixed coupon rate
                Date settlementDate = Date()        // bond's settlement date (for calculating accrued interest, prices, and yield to maturity)
			) :
                QLUtils::BootstrapInstrument(QLUtils::BootstrapInstrument::vtPrice, tenor, maturityDate),
                coupon_(coupon),
                settlementDate_(settlementDate),
				settlementCalendar_(bondTraits_.settlementCalendar(tenor)),
                accrualScheduleCalendar_(bondTraits_.accrualScheduleCalendar(tenor)),
                paymentCalendar_(bondTraits_.paymentCalendar(tenor))
            {
                QL_REQUIRE(tenor.length() > 0, "The length of the bond tenor (" << tenor.length() << ") must be greater than 0");
				if (settlementDate_ == Date()) {    // bond settlement date is not given => calculate the settlement date based on the evaluation date and settlement days
					auto today = Settings::instance().evaluationDate();
                    QL_ASSERT(today != Date(), "evaluation date/today not set");
                    settlementDate_ = settlementCalendar().advance(today, settlementDays(), Days);
				}
                auto settleDate = this->settlementDate();
                QL_ASSERT(settleDate != Date(), "settlement date not set");
				if (maturityDate == Date()) {  // maturity date is not given (CMT bond)
                    auto [multipile, n] = isMultiple(tenor, this->couponTenor());
                    if (multipile) {    // tenor is a multiple of coupon period => create a forward bond schedule of n coupon periods starting fro the settlement date
                        QL_ASSERT(n != Null<Integer>(), "the number of coupon accrual periods is null");
                        QL_ASSERT(n > 0, "the number of coupon accrual periods (" << n << ") must be greater than 0");
                        accrualSchedule_ = createForwardSchedule(settleDate, (Natural)n);
						auto nCouponPeriods = accrualSchedule_.dates().size() - 1;
                        QL_ASSERT(nCouponPeriods == Size(n), "The number of coupon accrual periods (" << nCouponPeriods << ") is not what's expected (" << n << ")");
                        QL_ASSERT(settleDate == accrualSchedule_.dates().front(), "Settlement date (" << settleDate << ") does not match the start date (" << accrualSchedule_.dates().front() << ") of the first coupon accrual period");
						maturityDate = accrualSchedule_.dates().back();
                    }
                    else {  // tenor is not a mutiple of the coupon period
                        maturityDate = accrualScheduleCalendar().advance(settleDate, tenor, accrualConvention(), accrualEndOfMonth());
                        accrualSchedule_ = createBackwardSchedule(settleDate, maturityDate);    // create a schedule going backward from the maturity date
                    }
                }
                else {  // maturity date is given
                    accrualSchedule_ = createBackwardSchedule(settleDate, maturityDate);    // create a schedule going backward from the maturity date
                }
                QL_ASSERT(maturityDate != Date(), "maturity date cannot be determined");
                if (this->maturityDate() == Date()) {
                    setMaturityDate(maturityDate);
                }
                QL_ASSERT(this->maturityDate() != Date(), "maturity date not set");
                QL_ASSERT(settleDate < this->maturityDate(), "settlement date (" << settleDate << ") is not before maturity date (" << this->maturityDate() << ")");
                auto nCouponPeriods = accrualSchedule_.dates().size() - 1;
                QL_ASSERT(nCouponPeriods > 0, "the number of coupon accrual periods (" << nCouponPeriods << ") must be greater than 0");
                QL_ASSERT(accrualSchedule_.dates().front() <= settleDate, "The start date of the first coupon accrual period (" << accrualSchedule_.dates().front() << ") is greater than the settlement date (" << settleDate << ")");
                QL_ASSERT(this->maturityDate() == accrualSchedule_.dates().back(), "The end date of the last coupon accrual period (" << accrualSchedule_.dates().back() << ") is not what's expected (" << this->maturityDate() << ")");
				accrualDayCounter_ = bondTraits_.accrualDayCounter(tenor, accrualSchedule_);
                yieldCalcSchedule_ = makeYieldCalcSchedule();
				yieldCalcDayCounter_ = bondTraits_.yieldCalcDayCounter(tenor, yieldCalcSchedule_);
				parYieldSplineDayCounter_ = bondTraits_.parYieldSplineDayCounter(tenor, accrualSchedule_);
            }
            const Calendar& settlementCalendar() const {
                return settlementCalendar_;
			}
            // T+x settlement
            Natural settlementDays() const {
				return bondTraits_.settlementDays(tenor());
            }
            // settlement date for the bond
            Date settlementDate() const {
                return settlementDate_;
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
            Size numCouponPeriods() const {
                return accrualSchedule_.dates().size() - 1;
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
            Real accruedAmount() const {
                auto bond = makeFixedRateBond();
                return bond->accruedAmount(settlementDate());
            }
            operator Leg() const {
                return makeLeg(coupon());
			}
            const Schedule& yieldCalcSchedule() const {
                return yieldCalcSchedule_;
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
            // given bond's quoted clean price, what is it's dirty price
            Real dirtyPrice() const {
                checkCleanPriceIsSet();
                return cleanPrice() + accruedAmount();
            }
        public:
            // implied by discount term structure functions (impliedXXX())
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
            // given a discount term structure, what coupon would give the bond a dirty price of 100
            Rate impliedFairCoupon(
                const Handle<YieldTermStructure>& discountingTermStructure
            ) const {
                const auto& me = *this;
                Real targetDirtyPrice = 100.;
                auto f = [&me, &discountingTermStructure, &targetDirtyPrice](Rate coupon) -> Real {
                    auto bond = me.makeFixedRateBond(coupon);
                    ext::shared_ptr<PricingEngine> pricingEngine(new DiscountingBondEngine(discountingTermStructure));
                    bond->setPricingEngine(pricingEngine);
                    auto dirtyPrice = bond->dirtyPrice();
                    return dirtyPrice - targetDirtyPrice;
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
            // quote is clean price of the bond
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
        class ZeroCouponBill : public FixedCoupondBond<typename BillTraits::BondTraits> {
        protected:
            BillTraits billTraits_;
			Schedule marketConventionYieldCalcSchedule_; // forward schedule (from the settle date) of the tenor length, used for calculating market convention yield
			DayCounter marketConventionYieldCalcDayCounter_;    // day count for calculating market convention yield
            DayCounter discountRateDayCounter_;
        private:
            Schedule createForwardSchedule(
                const Date& effectiveDate,
                const Date& endDate
            ) const {
				QL_REQUIRE(effectiveDate < endDate, "effective date (" << effectiveDate << ") must be before the end date (" << endDate << ")");
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
                Period tenor,
                Date maturityDate,
                Date settlementDate = Date()    // bond settlement date
            ) : FixedCoupondBond<typename BillTraits::BondTraits>(tenor, maturityDate, 0., settlementDate),
				discountRateDayCounter_(billTraits_.discountRateDayCounter(tenor))
            {
                auto settleDate = this->settlementDate();
                auto paymentDate = this->paymentDate();
                marketConventionYieldCalcSchedule_ = createForwardSchedule(settleDate, paymentDate); // create a forward schedule from the settlement date passing the payment date with the same tenor as the bond's coupon frequency, which will be used for calculating market convention yield
				auto nCouponPeriods = marketConventionYieldCalcSchedule_.dates().size() - 1;
                QL_ASSERT(nCouponPeriods > 0, "the number of coupon accrual periods (" << nCouponPeriods << ") must be greater than 0");
                QL_ASSERT(settleDate == marketConventionYieldCalcSchedule_.dates().front(), "Settlement date (" << settleDate << ") does not match the start date (" << marketConventionYieldCalcSchedule_.dates().front() << ") of the first coupon accrual period");
                QL_ASSERT(marketConventionYieldCalcSchedule_.dates().back() >= paymentDate, "The end date (" << marketConventionYieldCalcSchedule_.dates().back() << ") of the last coupon accrual period is less than the payment date (" << paymentDate << ")");
                QL_ASSERT(this->accruedAmount() == 0., "The accrued amount (" << this->accruedAmount() << ") must be 0 for a zero-coupon bill");
				marketConventionYieldCalcDayCounter_ = billTraits_.marketConventionYieldCalcDayCounter(tenor, marketConventionYieldCalcSchedule_);
            }
            // date of the sole payment (principal/face amount) for the bill
            Date paymentDate() const {
                return this->lastPaymentDate();
            }
            const Schedule& marketConventionYieldCalcSchedule() const {
                return marketConventionYieldCalcSchedule_;
			}
            const DayCounter& marketConventionYieldCalcDayCounter() const {
                return marketConventionYieldCalcDayCounter_;
			}
            const DayCounter& discountRateDayCounter() const {
                return discountRateDayCounter_;
            }
            // conversion between discount/compounding factor and market convention yield
            ////////////////////////////////////////////////////////////////////////////////////////////////
            Real compoundingFactorFromMarketConventionYield(
                Rate yield
            ) const {
                const auto& schedule = marketConventionYieldCalcSchedule();
                auto n = schedule.size() - 1;
                Real compoundingFactor = 1.;
                for (decltype(n) i = 0; i < n; ++i) {
                    const auto& start = schedule.date(i);
                    const auto& end = schedule.date(i + 1);
                    auto d = std::min(end, paymentDate());
                    auto t = marketConventionYieldCalcDayCounter().yearFraction(start, d);
                    compoundingFactor *= (1. + yield * t);
                }
                return compoundingFactor;
            }
            Rate marketConventionYieldFromDiscountFactor(
                DiscountFactor df
            ) const {
				QL_ASSERT(df > 0., "Discount factor (" << df << ") must be greater than 0");
                auto compoundingFactor = 1. / df;
                const auto& schedule = marketConventionYieldCalcSchedule();
				auto n = schedule.size() - 1;   // number of coupon accural periods in the market convention schedule
                auto T = marketConventionYieldCalcDayCounter().yearFraction(this->settlementDate(), paymentDate());
                if (n == 1) {   // 1 coupon accrual period
                    // 1 / df = 1 + yield * T, and solve for yield
                    // => yield = (1 / df - 1) / T
                    // => yield = (compoundingFactor - 1) / T
                    return (compoundingFactor - 1.) / T;
                }
                else if (n == 2) {   // 2 coupon accrual periods
                    // 1 / df = (1 + yield * t1) * (1 + yield * (T-t1)), and solve for yield
                    // => t1 * (T-t1) * yield * yield + T * yield + (1 - 1 / df) = 0
                    // => t1 * (T-t1) * yield * yield + T * yield + (1 - compoundingFactor) = 0
                    auto t1 = marketConventionYieldCalcDayCounter().yearFraction(schedule.date(0), schedule.date(1));
                    auto t2 = T - t1;
                    auto a = t1 * t2;
                    auto b = T;
                    auto c = 1. - compoundingFactor;
                    return (-b + std::sqrt(b * b - 4. * a * c)) / (2. * a);
                }
				else {  // more than 2 coupon accrual periods => use numerical solver to solve for yield
                    const auto& me = *this;
                    auto targetCompoundingFactor = compoundingFactor;
                    auto f = [&me, &targetCompoundingFactor](const Rate& yield) -> Real {
                        auto compoundingFactor = me.compoundingFactorFromMarketConventionYield(yield);
                        return compoundingFactor - targetCompoundingFactor;
                    };
                    auto guess = (compoundingFactor - 1.) / T; // use simple yield as the initial guess
                    Brent solver;
                    solver.setMaxEvaluations(this->solverMaxIterations());
                    Rate yield = solver.solve(f, this->solverAccuracy(), guess, this->solverRateStep());
                    return yield;
                }
            }
            DiscountFactor discountFactorFromMarketConventionYield(
                Rate yield
            ) const {
				auto compoundingFactor = compoundingFactorFromMarketConventionYield(yield);
                return 1. / compoundingFactor;
            }
            ////////////////////////////////////////////////////////////////////////////////////////////////
            // conversion between discount rate and discount factor
            ////////////////////////////////////////////////////////////////////////////////////////////////
            DiscountFactor discountFactorFromDiscountRate(
                Rate discountRate
            ) const {
                auto d1 = this->settlementDate();
                auto d2 = paymentDate();
                auto t = discountRateDayCounter().yearFraction(d1, d2);
                return 1. - discountRate * t;
            }
            Rate discountRateFromDiscountFactor(
                DiscountFactor discountFactor
            ) const {
                auto d1 = this->settlementDate();
                auto d2 = paymentDate();
                auto t = discountRateDayCounter().yearFraction(d1, d2);
                return (1. - discountFactor) / t;
            }
            ////////////////////////////////////////////////////////////////////////////////////////////////
            // given bond's quoted clean price, what is it's discount factor, discount rate, and market convention yield
            /////////////////////////////////////////////
            Real bondPrice() const {
                this->checkCleanPriceIsSet();
                auto dirtyPrice = this->dirtyPrice();
                return dirtyPrice;
            }
			// discount factor from settlement date to payment date
            DiscountFactor discountFactor() const {
                DiscountFactor df = bondPrice() / this->parNotional();
                return df;
            }
            Rate discountRate() const {
                auto df = discountFactor();
                return discountRateFromDiscountFactor(df);
            }
            Rate marketConventionYield() const {
                auto df = discountFactor();
                return marketConventionYieldFromDiscountFactor(df);
            }
            ////////////////////////////////////////////////////////////////////////////////////////////////
			// withXXX() methods to set the clean price of the bill based on different inputs (discount factor, discount rate, market convention yield)
            ////////////////////////////////////////////////////////////////////////////////////////////////
            ZeroCouponBill& withBondPrice(
                Real price
            ) {
                this->cleanPrice() = price;
                return *this;
            }
            ZeroCouponBill& withDiscountFactor(
                DiscountFactor discountFactor
            ) {
				auto dirtyPrice = discountFactor * this->parNotional();
				auto cleanPrice = dirtyPrice;   // zero-coupon bill has no accrued interest, so the clean price is the same as the dirty price
				this->cleanPrice() = cleanPrice;
                return *this;
            }
            ZeroCouponBill& withDiscountRate(
                Rate discountRate
            ) {
				return withDiscountFactor(discountFactorFromDiscountRate(discountRate));
            }
            ZeroCouponBill& withMarketConventionYield(
                Rate yield
            ) {
                return withDiscountFactor(discountFactorFromMarketConventionYield(yield));
            }
            ////////////////////////////////////////////////////////////////////////////////////////////////
			// impliedXXX() functions to calculate the implied discount factor, discount rate, and market convention yield given a discount term structure
            ////////////////////////////////////////////////////////////////////////////////////////////////
            DiscountFactor impliedDiscountFactor(
                const Handle<YieldTermStructure>& discountingTermStructure
            ) const {
                auto dirtyPrice = this->impliedPrice(discountingTermStructure, Bond::Price::Type::Dirty);
                DiscountFactor df = dirtyPrice / this->parNotional();
                return df;
            }
            Rate impliedDiscountRate(
                const Handle<YieldTermStructure>& discountingTermStructure
            ) const {
                auto df = impliedDiscountFactor(discountingTermStructure);
				return discountRateFromDiscountFactor(df);
            }
            Rate impliedMarketConventionYield(
                const Handle<YieldTermStructure>& discountingTermStructure
            ) const {
				auto df = impliedDiscountFactor(discountingTermStructure);
                return marketConventionYieldFromDiscountFactor(df);
			}
            ////////////////////////////////////////////////////////////////////////////////////////////////
        };

        template <
            typename BondTraits
        >
        class ConstantMaturityBond : public FixedCoupondBond<BondTraits> {
        public:
            ConstantMaturityBond(
                Period tenor,
                Rate coupon = 0.
            ) :FixedCoupondBond<BondTraits>(tenor, Date(), coupon, Date()) {
				this->cleanPrice() = this->parNotional();   // default the clean price of the bond to par by assuming the coupon variable is a par coupon
            }
        };
    }
}
