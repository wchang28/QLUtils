#pragma once

#include <string>
#include <ql/quantlib.hpp>

namespace QuantLib {
    namespace Utils {
        struct BootstrapQuote {
            std::string ticker;
            bool use;   // include for bootstrapping flag
            Real value;   // quoted value
            BootstrapQuote() :
                use(true),
                value(0.0)
            {}
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
        };
    }
}
