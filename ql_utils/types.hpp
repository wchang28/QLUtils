#pragma once

#include <string>
#include <sstream>
#include <ql/quantlib.hpp>
#include <functional>
#include <ostream>
#include <vector>
#include <memory> 

namespace QLUtils {
    template<typename _Elem> using string_t = std::basic_string<_Elem>;
    template<typename _Elem> using ostringstream_t = std::basic_ostringstream<_Elem>;

	enum RateUnit {
		Decimal = 0,
		Percent = 1,
		BasisPoint = 2,
	};

	template <
		typename MATURITY_TYPE,
		typename RATE_TYPE
	>
	struct YieldTSNodes {
		typedef MATURITY_TYPE MaturityType;
		typedef RATE_TYPE RateType;
		typedef std::vector<MaturityType> MaturityVector;
		typedef std::vector<RateType> RateVector;
		MaturityVector maturities;
		RateVector rates;
		YieldTSNodes(size_t n = 0): maturities(n), rates(n) {}
		YieldTSNodes(
			const MaturityVector& maturities,
			const RateVector& rates
		):maturities(maturities), rates(rates) {
			assertValid();
		}
		void resize(size_t n) {
			maturities.resize(n);
			rates.resize(n);
		}
		size_t size() const {
			return maturities.size();
		}
		bool empty() const {
			return maturities.empty();
		}
		void clear() {
			maturities.clear();
			rates.clear();
		}
		void assertValid() const {
			QL_ASSERT(maturities.size() == rates.size(), "the length of maturities (" << maturities.size() << " is different from the length of rates (" << rates.size() << ")");
			QL_ASSERT(!empty(), "term structure is empty");
		}
	};

	template <
		typename TERM_TYPE = QuantLib::Time,
		typename RATE_TYPE = QuantLib::Rate
	>
	struct TermStructureNode {
		typedef TERM_TYPE TermType;
		typedef RATE_TYPE RateType;
		TermType term;
		RateType rate;
		TermStructureNode(
			TermType term = 0,
			RateType rate = 0
		) :term(term), rate(rate)
		{}
	};

    template <
        typename TERM_TYPE = QuantLib::Time,
        typename RATE_TYPE = QuantLib::Rate
    >
    class TermStructureNodes {
    public:
        typedef TERM_TYPE TermType;
        typedef RATE_TYPE RateType;
        typedef TermStructureNode<TermType, RateType> Node;
        typedef std::shared_ptr<Node> pNode;
        typedef std::vector<pNode> Nodes;
        typedef std::shared_ptr<Nodes> pNodes;
        typedef std::vector<TermType> TermVector;
        typedef std::vector<RateType> RateVector;
        typedef std::shared_ptr<TermVector> pTermVector;
        typedef std::shared_ptr<RateVector> pRateVector;
        typedef std::pair<pTermVector, pRateVector> TermRateVectorsPair;
    private:
        pNodes pNodes_;
    public:
        TermStructureNodes(
            const pNodes& pNodes = nullptr
        ) : pNodes_(pNodes)
        {}
        TermStructureNodes(
            const Nodes& nodes
        ) : pNodes_(new Nodes(nodes))
        {}
        const pNodes& pTSNodes() const {
            return pNodes_;
        }
        pNodes& pTSNodes() {
            return pNodes_;
        }
        const Nodes& nodes() const {
            QL_ASSERT(pNodes_ != nullptr, "term structure is null");
            return *pNodes_;
        }
        Nodes& nodes() {
            QL_ASSERT(pNodes_ != nullptr, "term structure is null");
            return *pNodes_;
        }
        const pNode& nodeAt(
            size_t i
        ) const {
            return nodes().at(i);
        }
        pNode& nodeAt(
            size_t i
        ) {
            return nodes().at(i);
        }
        bool empty() const {
            return nodes().empty();
        }
        size_t size() const {
            return nodes().size();
        }
        void resize(size_t n) {
            if (pNodes_ == nullptr) {
                pNodes_.reset(new Nodes(n));
            }
            else {
                pNodes_->resize(n);
            }
        }
        TermStructureNodes& operator *= (double x) {
            auto n = size();
            for (decltype(n) i = 0; i < n; ++i) {   // for each node
                try {
                    auto& pNode = nodeAt(i);
                    QL_ASSERT(pNode != nullptr, "term structure node is null");
                    pNode->rate *= x;
                }
                catch (const std::exception& e) {
                    QL_FAIL("TS node[" << i << "]: " << e.what());
                }
            }
            return *this;
        }
        TermStructureNodes& operator /= (double x) {
            QL_ASSERT(x != 0., "divided by 0");
            return ((*this) *= (1./x));
        }
        void assertValid() const {
            QL_ASSERT(!empty(), "term structure is empty");
        }
        static void assertValidVectorPairs(
            const TermVector& termVector,
            const RateVector& rateVector
        ) {
            QL_ASSERT(!termVector.empty(), "term structure is empty");
            auto n = termVector.size();
            QL_ASSERT(rateVector.size() == n, "rate vector's size (" << rateVector.size() << ") is not what's expected (" << n << ")");
        }
        static void assertValidVectorPairs(
            const TermRateVectorsPair& vpr
        ) {
            QL_ASSERT(vpr.first != nullptr, "term vector is null");
            QL_ASSERT(vpr.second != nullptr, "rate vector is null");
            assertValidVectorPairs(*(vpr.first), *(vpr.second));
        }
    private:
        void transferToVectorsPair(
            TermVector& termVector,
            RateVector& rateVector
        ) const {
            const auto& nodes = this->nodes();
            auto n = size();
            QL_ASSERT(termVector.size() == n, "size of the term vector (" << termVector.size() << ") is not what's expected (" << n << ")");
            QL_ASSERT(rateVector.size() == n, "size of the rate vector (" << rateVector.size() << ") is not what's expected (" << n << ")");
            for (decltype(n) i = 0; i < n; ++i) {   // for each node
                try {
                    const auto& pNode = nodes.at(i);
                    QL_ASSERT(pNode != nullptr, "term structure node is null");
                    termVector[i] = pNode->term;
                    rateVector[i] = pNode->rate;
                }
                catch (const std::exception& e) {
                    QL_FAIL("TS node[" << i << "]: " << e.what());
                }
            }
        }
        TermStructureNodes& transferFromVectorsPair(
            const TermVector& termVector,
            const RateVector& rateVector
        ) {
            assertValidVectorPairs(termVector, rateVector);
            auto n = termVector.size();
            resize(n);
            for (decltype(n) i = 0; i < n; ++i) {
                auto& pNode = nodeAt(i);
                pNode.reset(new Node(termVector[i], rateVector[i]));
            }
            return *this;
        }
    public:
        operator TermRateVectorsPair() const {
            assertValid();
            auto n = size();
            TermRateVectorsPair ret;
            ret.first.reset(new TermVector(n));
            ret.second.reset(new RateVector(n));
            transferToVectorsPair(*(ret.first), *(ret.second));
            return ret;
        }
        operator std::shared_ptr<YieldTSNodes<TermType, RateType>>() const {
            assertValid();
            auto n = size();
            std::shared_ptr<YieldTSNodes<TermType, RateType>> ret(new YieldTSNodes<TermType, RateType>(n));
            transferToVectorsPair(ret->maturities, ret->rates);
            return ret;
        }
        friend TermStructureNodes& operator << (
            TermStructureNodes& dest,
            const TermRateVectorsPair& rhs
        ) {
            TermStructureNodes::assertValidVectorPairs(rhs);
            return dest.transferFromVectorsPair(*(rhs.first), *(rhs.second));
        }
        friend TermStructureNodes& operator << (
            TermStructureNodes& dest,
            const YieldTSNodes<TermType, RateType>& rhs
        ) {
            rhs.assertValid();
            return dest.transferFromVectorsPair(rhs.maturities, rhs.rates);
        }
    };
    template <
        typename TERM_TYPE = QuantLib::Time,
        typename RATE_TYPE = QuantLib::Rate
    >
    using pTermStructureNodes = std::shared_ptr<TermStructureNodes<TERM_TYPE, RATE_TYPE>>;

	typedef std::vector<QuantLib::Real> MonthlyZeroRates;
	typedef std::vector<QuantLib::Real> MonthlyForwardCurve;
	typedef std::vector<QuantLib::Real> MonthlyRates;
	typedef MonthlyRates HistoricalMonthlyRates;

	struct ParYieldTermStructInstrument {
		virtual QuantLib::Time parTerm() const = 0;
		virtual QuantLib::Rate parYield() const = 0;
	};
    using IborIndexFactory = std::function<QuantLib::ext::shared_ptr<QuantLib::IborIndex>(const QuantLib::Handle<QuantLib::YieldTermStructure>&)>;
}