#pragma once

#include <string>
#include <sstream>
#include <ql/quantlib.hpp>
#include <functional>
#include <ostream>

namespace QLUtils {
    template<typename _Elem> using string_t = std::basic_string<_Elem>;
    template<typename _Elem> using ostringstream_t = std::basic_ostringstream<_Elem>;

	enum RateUnit {
		Decimal = 0,
		Percent = 1,
		BasisPoint = 2,
	};

	struct ParYieldTermStructInstrument {
		virtual QuantLib::Time parTerm() const = 0;
		virtual QuantLib::Rate parYield() const = 0;
	};
    using IborIndexFactory = std::function<QuantLib::ext::shared_ptr<QuantLib::IborIndex>(const QuantLib::Handle<QuantLib::YieldTermStructure>&)>;
}