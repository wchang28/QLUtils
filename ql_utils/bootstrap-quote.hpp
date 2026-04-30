#pragma once

#include <string>
#include <ql/quantlib.hpp>
#include <vector>

namespace QuantLib {
    namespace Utils {
        struct BootstrapQuote {
            std::string ticker;
            bool use;   // include for bootstrapping flag
            Real value;   // quoted value
            BootstrapQuote() :
                use(true),
                value(Null<Real>())
            {}
            bool valueSet() const {
                return (value != Null<Real>());
            }
            template <typename Q>
            static bool hasUse(
                const std::vector<Q>& quotes
            ) {
                for (const auto& quote : quotes) {
                    if (quote.use) {
                        return true;
                    }
                }
                return false;
            }
        };
        
        enum GovernmentSecurityType {
            gstBill = 0,	// zero-coupon government security
            gstBond = 1,	// coupon-bearing government security
        };
        
        enum BondQuoteValueType {
            bqvtCleanPrice = 0, // $100 notional clean price
            bqvtDirtyPrice = 1, // $100 notional dirty price
            bqvtPrice = 2,	// $100 notional generic price. for bill (zero coupon), clean price=dirty price. for bond, it is the same as clean price.
            bqvtYTM = 3,    // yield to maturity (% unit)
            bqvtMarketConventionYield = 4,  // market convention yield (% unit)
            bqvtDiscountRate = 5, // discount rate for bill (% unit)
        };
        
        struct GovernmentSecurityQuote: public BootstrapQuote {
            GovernmentSecurityType type;
            Period tenor;
            Rate coupon;
            Date settleDate;
            Date maturityDate;
            BondQuoteValueType valueType;
            GovernmentSecurityQuote() :
                type(GovernmentSecurityType::gstBill),
                coupon(0.0),
                valueType(BondQuoteValueType::bqvtCleanPrice)
            {}
            static bool hasUse(
                const std::vector<GovernmentSecurityQuote>& quotes
            ) {
                return BootstrapQuote::hasUse<GovernmentSecurityQuote>(quotes);
            }
            static Date minSettleDate(
                const std::vector<GovernmentSecurityQuote>& quotes
            ) {
                Date minSettleDate = Date();
                for (const auto& quote : quotes) {
                    if (quote.use) {
                        if (minSettleDate == Date() || quote.settleDate < minSettleDate) {
                            minSettleDate = quote.settleDate;
                        }
                    }
                }
                return minSettleDate;
            }
        };

        struct OISSwapQuote : public BootstrapQuote {
            enum QuoteType {
                osqtDeposit = 0,
                osqtSwap = 1
			};
            QuoteType quoteType;
            Period tenor;
            OISSwapQuote() :
                quoteType(QuoteType::osqtSwap)
            {}
            static bool hasUse(
                const std::vector<OISSwapQuote>& quotes
            ) {
                return BootstrapQuote::hasUse<OISSwapQuote>(quotes);
            }
        };
    }
}
