#pragma once

#include <ql/quantlib.hpp>

namespace QLUtils {
	template <
		typename IndexType
	>
	struct IHistoricalIndexDatabase {
		// historical index grabbing operator
		virtual QuantLib::Rate operator() (
			const IndexType&,
			const QuantLib::Date&
		) const = 0;
	};
}