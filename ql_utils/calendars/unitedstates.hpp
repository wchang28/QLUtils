#pragma once

#include <ql/quantlib.hpp>

namespace QLUtils {
	// extension to the United States calendar
	class UnitedStatesEx : public QuantLib::Calendar {
	private:
		class ConstantMaturityTreasuryImpl : public QuantLib::Calendar::WesternImpl {
		private:
			// Martin Luther King's birthday (third Monday in January)
			template<QuantLib::Natural StartYear = 1983>
			static bool isMartinLutherKing(QuantLib::Day d, QuantLib::Month m, QuantLib::Year y, QuantLib::Weekday w) {
				return (d >= 15 && d <= 21) && w == QuantLib::Monday && m == QuantLib::January && y >= StartYear;
			}
			// Lincoln's Birthday February 12 (1985 is last year it was celebrated)
			static bool isLincolnBirthday(QuantLib::Day d, QuantLib::Month m, QuantLib::Year y, QuantLib::Weekday w) {
				return (d == 12 || (d == 13 && w == QuantLib::Monday)) && m == QuantLib::February && y <= 1985;
			}
			static bool isWashingtonBirthday(QuantLib::Day d, QuantLib::Month m, QuantLib::Year y, QuantLib::Weekday w) {
				if (y >= 1971) {
					// third Monday in February
					return (d >= 15 && d <= 21) && w == QuantLib::Monday && m == QuantLib::February;
				}
				else {
					// February 22nd, possily adjusted
					return (d == 22 || (d == 23 && w == QuantLib::Monday)
						|| (d == 21 && w == QuantLib::Friday)) && m == QuantLib::February;
				}
			}
			static bool isMemorialDay(QuantLib::Day d, QuantLib::Month m, QuantLib::Year y, QuantLib::Weekday w) {
				if (y >= 1971) {
					// last Monday in May
					return d >= 25 && w == QuantLib::Monday && m == QuantLib::May;
				}
				else {
					// May 30th, possibly adjusted
					return (d == 30 || (d == 31 && w == QuantLib::Monday)
						|| (d == 29 && w == QuantLib::Friday)) && m == QuantLib::May;
				}
			}
			static bool isMemorialDayNo1970(QuantLib::Day d, QuantLib::Month m, QuantLib::Year y, QuantLib::Weekday w) {
				if (y >= 1971) {
					// last Monday in May
					return d >= 25 && w == QuantLib::Monday && m == QuantLib::May;
				}
				else {
					// May 30th, possibly adjusted
					return (d == 30 || (d == 31 && w == QuantLib::Monday)
						|| (d == 29 && w == QuantLib::Friday)) && m == QuantLib::May && y != 1970;
				}
			}
			static bool isLaborDay(QuantLib::Day d, QuantLib::Month m, QuantLib::Year y, QuantLib::Weekday w) {
				// first Monday in September
				return d <= 7 && w == QuantLib::Monday && m == QuantLib::September;
			}
			static bool isColumbusDay(QuantLib::Day d, QuantLib::Month m, QuantLib::Year y, QuantLib::Weekday w) {
				if (y >= 1971) {
					// second Monday in October
					return (d >= 8 && d <= 14) && w == QuantLib::Monday && m == QuantLib::October;
				}
				else {
					// October 12, adjusted, but no Saturday to Friday
					return (d == 12 || (d == 13 && w == QuantLib::Monday)) && m == QuantLib::October;
				}
			}
			static bool isVeteransDay(QuantLib::Day d, QuantLib::Month m, QuantLib::Year y, QuantLib::Weekday w) {
				if (y <= 1970 || y >= 1978) {
					// November 11th, adjusted
					return (d == 11 || (d == 12 && w == QuantLib::Monday) ||
						(d == 10 && w == QuantLib::Friday)) && m == QuantLib::November;
				}
				else {
					// fourth Monday in October
					return (d >= 22 && d <= 28) && w == QuantLib::Monday && m == QuantLib::October;
				}
			}
			static bool isVeteransDayNoSaturday(QuantLib::Day d, QuantLib::Month m, QuantLib::Year y, QuantLib::Weekday w) {
				if (y <= 1970 || y >= 1978) {
					// November 11th, adjusted, but no Saturday to Friday
					return (d == 11 || (d == 12 && w == QuantLib::Monday)) && m == QuantLib::November;
				}
				else {
					// fourth Monday in October
					return (d >= 22 && d <= 28) && w == QuantLib::Monday && m == QuantLib::October;
				}
			}
			static bool isVeteransDayNoSaturday2(QuantLib::Day d, QuantLib::Month m, QuantLib::Year y, QuantLib::Weekday w) {
				if (y <= 1970 || y >= 1974) {
					// November 11th, adjusted, but no Saturday to Friday
					return (d == 11 || (d == 12 && w == QuantLib::Monday)) && m == QuantLib::November;
				}
				else {	// for 1971, 1972, 1973 only
					// fourth Monday in October
					return (d >= 22 && d <= 28) && w == QuantLib::Monday && m == QuantLib::October;
				}
			}
			// Election day prior to 1985
			static bool isElectionDay(QuantLib::Day d, QuantLib::Month m, QuantLib::Year y, QuantLib::Weekday w) {
				if (y <= 1984) {
					if (y == 1966 || y == 1977 || y == 1983) {
						// second Tuesday of November
						return (d >= 8 && d <= 14) && w == QuantLib::Tuesday && m == QuantLib::November;
					}
					else {
						// first Tuesday of November
						return d <= 7 && w == QuantLib::Tuesday && m == QuantLib::November;
					}
				}
				else {
					return false;
				}
			}
			static bool isJuneteenth(QuantLib::Day d, QuantLib::Month m, QuantLib::Year y, QuantLib::Weekday w) {
				// declared in 2021, but only observed by exchanges since 2022
				return (d == 19 || (d == 20 && w == QuantLib::Monday) || (d == 18 && w == QuantLib::Friday))
					&& m == QuantLib::June && y >= 2022;
			}
		public:
			std::string name() const override { return "US Constant Maturity Treasury"; }
			bool isBusinessDay(const QuantLib::Date& date) const override {
				auto w = date.weekday();
				auto d = date.dayOfMonth();
				auto dd = date.dayOfYear();
				auto m = date.month();
				auto y = date.year();
				auto em = easterMonday(y);

				// Special busyness days
				if (// President Day not observed in 1969
					(y == 1969 && m == QuantLib::February && d == 21)
					) return true;

				if (isWeekend(w)
					// New Year's Day (possibly moved to Monday if on Sunday)
					|| ((d == 1 || (d == 2 && w == QuantLib::Monday)) && m == QuantLib::January)
					// Martin Luther King's birthday (third Monday in January)
					|| isMartinLutherKing<1985>(d, m, y, w)
					// Lincoln's Birthday
					|| isLincolnBirthday(d, m, y, w)
					// Washington's birthday (third Monday in February)
					|| isWashingtonBirthday(d, m, y, w)
					// Good Friday
					|| (dd == em - 3 && (y < 1995 || !(m == QuantLib::April && d <= 7)))
					// Memorial Day (last Monday in May)
					//|| isMemorialDay(d, m, y, w)
					|| isMemorialDayNo1970(d, m, y, w)
					// Juneteenth (Monday if Sunday or Friday if Saturday)
					|| isJuneteenth(d, m, y, w)
					// Independence Day (Monday if Sunday or Friday if Saturday)
					|| ((d == 4 || (d == 5 && w == QuantLib::Monday) ||
						(d == 3 && w == QuantLib::Friday)) && m == QuantLib::July)
					// Labor Day (first Monday in September)
					|| isLaborDay(d, m, y, w)
					// Columbus Day (second Monday in October)
					|| isColumbusDay(d, m, y, w)
					// Veteran's Day (Monday if Sunday)
					//|| isVeteransDayNoSaturday(d, m, y, w)
					|| isVeteransDayNoSaturday2(d, m, y, w)
					// Election Day
					|| isElectionDay(d, m, y, w)
					// Thanksgiving Day (fourth Thursday in November)
					|| ((d >= 22 && d <= 28) && w == QuantLib::Thursday && m == QuantLib::November)
					// Christmas (Monday if Sunday or Friday if Saturday)
					|| ((d == 25 || (d == 26 && w == QuantLib::Monday) ||
						(d == 24 && w == QuantLib::Friday)) && m == QuantLib::December))
					return false;

				// Special closings
				if (// President Bush's Funeral
					(y == 2018 && m == QuantLib::December && d == 5)
					// Hurricane Sandy
					|| (y == 2012 && m == QuantLib::October && (d == 30))
					// President Reagan's funeral
					|| (y == 2004 && m == QuantLib::June && d == 11)
					// September 11 Attack
					|| (y == 2001 && m == QuantLib::September && (d == 11 || d == 12))
					// President Nixon's funeral
					|| (y == 1994 && m == QuantLib::April && d == 27)
					// Hurricane Gloria
					|| (y == 1985 && m == QuantLib::September && d == 27)
					// Extended Memorial Day
					|| (y == 1978 && m == QuantLib::May && d == 30)
					// Extended Memorial Day
					|| (y == 1979 && m == QuantLib::May && d == 30)
					// 1977 Blackout
					|| (y == 1977 && m == QuantLib::July && d == 14)
					// Christmas Eve 1973 is a day off for some reason
					|| (y == 1973 && m == QuantLib::December && d == 24)
					// National Day of Participation for the lunar exploration.
					|| (y == 1969 && m == QuantLib::July && d == 21)
					// Funeral of former President Eisenhower.
					|| (y == 1969 && m == QuantLib::March && d == 31)
					// Day of mourning for Martin Luther King Jr.
					|| (y == 1968 && m == QuantLib::April && d == 9)
					// Funeral of President Kennedy
					|| (y == 1963 && m == QuantLib::November && d == 25)
					) return false;

				return true;
			}
		};
	public:
		// ConstantMaturityTreasury
		//	1. matches CMT holidays released by H.15. Dated alll the way back to 1/2/1962
		//	2. should be very close to the calendar UnitedStates::GovernmentBond going forward with the exception of Good Friday handling
		enum Market {
			ConstantMaturityTreasury,
		};
		explicit UnitedStatesEx(Market market) {
			static QuantLib::ext::shared_ptr<QuantLib::Calendar::Impl> constantMaturityTreasuryImpl(new UnitedStatesEx::ConstantMaturityTreasuryImpl());
			switch (market) {
			case ConstantMaturityTreasury:
				impl_ = constantMaturityTreasuryImpl;
				break;
			default:
				QL_FAIL("unknown market");
			}
		}
	};
}