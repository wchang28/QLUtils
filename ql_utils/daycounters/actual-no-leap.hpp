#pragma once

#include <ql/quantlib.hpp>
#include <string>
#include <sstream>

namespace QuantLib {
	template <
		Size DAYS_PER_YEAR = 365
	>
	class ActualNoLeap : public DayCounter {
	private:
		class Impl final : public DayCounter::Impl {
		public:
			std::string name() const {
				std::ostringstream oss;
				oss << "Actual(No Leap)/" << DAYS_PER_YEAR;
				return oss.str();
			}
			Date::serial_type dayCount(
				const Date& d1,
				const Date& d2
			) const {
				Actual365Fixed dcNL(Actual365Fixed::NoLeap);
				return dcNL.dayCount(d1, d2);
			}
			Time yearFraction(
				const Date& d1,
				const Date& d2,
				const Date&,
				const Date&
			) const {
				return ((Time)this->dayCount(d1, d2))/((Time)DAYS_PER_YEAR);
			}
		};
	public:
		explicit ActualNoLeap()
			: DayCounter(
				ext::shared_ptr<DayCounter::Impl>(new ActualNoLeap::Impl())
			)
		{}
	};
}