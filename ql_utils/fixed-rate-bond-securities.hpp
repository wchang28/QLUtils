#pragma once

#include <ql_utils/instrument.hpp>
#include <vector>

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
            Schedule createForwardSchedule(const Date& startDate, const Date& endDate) const {
                return {};
            }
            Schedule createBackwardSchedule(const Date& maturityDate) {
                return {};
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
                paymentCalendar_(bondTraits_.paymentCalendar(tenor))
            {
                auto settleDate = settlementDate();
				if (maturityDate == Date()) {  // maturity date is not given => create a forward bond schedule from the settlement date with the given tenor
                    accrualSchedule_ = createForwardSchedule(settleDate, settleDate + tenor);
                    QL_ASSERT(settleDate == accrualSchedule_.dates().front(), "Settlement date (" << settleDate << ") does not match the first date of the accrual schedule");
					this->datedDate() = accrualSchedule_.dates().back();    // set the maturity date to the last date of the schedule
                }
                else {  // maturity date is given => created a backward bond schedule from the maturity date
                    accrualSchedule_ = createBackwardSchedule(maturityDate);
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
            Frequency couponFrequency() const {
                return bondTraits_.couponFrequency(tenor());
            }
            Date startDate() const {
                return settlementDate();
            }
			// bond maturity date
            Date maturityDate() const {
                return datedDate();
			}
            const Schedule& accrualSchedule() const {
                return accrualSchedule_;
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
            bool priceIsSet() const {
                return this->valueIsSet();
            }
			// get the clean price, which is the price used for bootstrapping, from the price of the bond
            const Real& cleanPrice() const {
                return price();
            }
			// set the clean price, which is the price used for bootstrapping, to the price of the bond
            Real& cleanPrice() {
                return price();
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
            const DayCounter& accrualDayCounter() const {
                return accrualDayCounter_;
			}
            const DayCounter& yieldCalcDayCounter() const {
                return yieldCalcDayCounter_;
            }
            const DayCounter& parYieldSplineDayCounter() const {
                return parYieldSplineDayCounter_;
            }

            Rate ytm() const {
                return 5.0/100.0;   // TODO:
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
            Schedule createForwardSchedule(const Date& startDate, const Date& endDate) const {
                return {};
			}
        public:
            ZeroCouponBill(
                const Period& tenor,
                const Date& maturityDate
            ) : FixedCoupondedBond<typename BillTraits::BondTraits>(tenor, maturityDate, 0.0)
            {
                marketConventionSchedule_ = createForwardSchedule(this->settlementDate(), paymentDate());
            }
            // the sole payment date of the bill
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
				this->cleanPrice() = this->parNotional();   // set the price to par for par coupon bond
            }
        };
    }
}
