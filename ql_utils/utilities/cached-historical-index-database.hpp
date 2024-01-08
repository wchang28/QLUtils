#pragma once

#include <ql/quantlib.hpp>
#include <ql_utils/utilities/historical-index-database.hpp>
#include <map>
#include <memory>

namespace QLUtils {
	template <
		typename IndexType
	>
	class CachedHistoricalIndexDatabase :
		public IHistoricalIndexDatabase<IndexType> {
	protected:
		typedef std::map<typename QuantLib::Date::serial_type, QuantLib::Rate> HistoricalRateLookup;
		typedef std::shared_ptr<HistoricalRateLookup> pHistoricalRateLookup;
	protected:
		mutable std::map<IndexType, pHistoricalRateLookup> cached_;
	protected:
		// get all historical rate for this index
		virtual pHistoricalRateLookup getRateLookupFromSource(
			const IndexType&
		) const = 0;
	public:
		// IHistoricalIndexDatabase interface
		QuantLib::Rate operator() (
			const IndexType& index,
			const QuantLib::Date& fixingDate
		) const {
			auto& hist = cached_[index];
			if (hist == nullptr || hist->empty()) {
				hist = getRateLookupFromSource(index);
			}
			QL_ASSERT(hist != nullptr, "error loading index " << index << " form source");
			auto p = hist->find(fixingDate.serialNumber());
			if (p != hist->end()) {
				return p->second;
			}
			else {
				QL_FAIL("unable to find " << fixingDate << "'s rate for index " << index);
			}
		}
	};
}