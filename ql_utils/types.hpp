#pragma once

#include <string>
#include <sstream>
#include <ql/quantlib.hpp>
#include <functional>
#include <ostream>
#include <vector>

namespace QLUtils {
    template<typename _Elem> using string_t = std::basic_string<_Elem>;
    template<typename _Elem> using ostringstream_t = std::basic_ostringstream<_Elem>;

	enum RateUnit {
		Decimal = 0,
		Percent = 1,
		BasisPoint = 2,
	};

	template <typename MATURITY_TYPE, typename RATE_TYPE>
	struct YieldTSNodes {
		typedef MATURITY_TYPE MaturityType;
		typedef RATE_TYPE RateType;
		std::vector<MaturityType> maturities;
		std::vector<RateType> rates;
		void resize(size_t n) {
			maturities.resize(n);
			rates.resize(n);
		}
		size_t size() const {
			return maturities.size();
		}
		void clear() {
			maturities.clear();
			rates.clear();
		}
	};

	struct ParYieldTermStructInstrument {
		virtual QuantLib::Time parTerm() const = 0;
		virtual QuantLib::Rate parYield() const = 0;
	};
    using IborIndexFactory = std::function<QuantLib::ext::shared_ptr<QuantLib::IborIndex>(const QuantLib::Handle<QuantLib::YieldTermStructure>&)>;
}