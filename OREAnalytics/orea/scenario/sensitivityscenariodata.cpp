/*
 Copyright (C) 2016 Quaternion Risk Management Ltd
 All rights reserved.

 This file is part of ORE, a free-software/open-source library
 for transparent pricing and risk analysis - http://opensourcerisk.org

 ORE is free software: you can redistribute it and/or modify it
 under the terms of the Modified BSD License.  You should have received a
 copy of the license along with this program.
 The license is also available online at <http://opensourcerisk.org>

 This program is distributed on the basis that it will form a useful
 contribution to risk analytics and model standardisation, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 FITNESS FOR A PARTICULAR PURPOSE. See the license for more details.
*/

#include <orea/scenario/sensitivityscenariodata.hpp>
#include <ored/utilities/xmlutils.hpp>
#include <ored/utilities/log.hpp>

#include <boost/lexical_cast.hpp>

using namespace QuantLib;

namespace ore {
namespace analytics {

  /*
bool SensitivityScenarioData::operator==(const SensitivityScenarioData& rhs) {

    if (irDomain_ != rhs.irDomain_ ||
	//irCurrencies_ != rhs.irCurrencies_ ||
	//irIndices_ != rhs.irIndices_ ||
	irShiftTenors_ != rhs.irShiftTenors_ ||
	irShiftType_ != rhs.irShiftType_ ||
	irShiftSize_ != rhs.irShiftSize_ ||
	fxShiftType_ != rhs.fxShiftType_ ||
	//fxCurrencyPairs_ != rhs.fxCurrencyPairs_ ||
	fxShiftSize_ != rhs.fxShiftSize_ ||
	infDomain_ != rhs.infDomain_ ||
	//infIndices_ != rhs.infIndices_ ||
	infShiftTenors_ != rhs.infShiftTenors_ ||
	infShiftType_ != rhs.infShiftType_ ||
	infShiftSize_ != rhs.infShiftSize_ ||
	crDomain_ != rhs.crDomain_ ||
	//crNames_ != rhs.crNames_ ||
	crShiftTenors_ != rhs.crShiftTenors_ ||
	crShiftType_ != rhs.crShiftType_ ||
	crShiftSize_ != rhs.crShiftSize_ ||
	//swaptionVolCurrencies_ != rhs.swaptionVolCurrencies_ ||
	swaptionVolShiftExpiries_ != rhs.swaptionVolShiftExpiries_ ||
	swaptionVolShiftTerms_ != rhs.swaptionVolShiftTerms_ ||
	swaptionVolShiftStrikes_ != rhs.swaptionVolShiftStrikes_ ||
	swaptionVolShiftType_ != rhs.swaptionVolShiftType_ ||
	swaptionVolShiftSize_ != rhs.swaptionVolShiftSize_) {
      return false;
    } else {
        return true;
    }
}

bool SensitivityScenarioData::operator!=(const SensitivityScenarioData& rhs) { return !(*this == rhs); }
  */
  
void SensitivityScenarioData::fromXML(XMLNode* root) {
    XMLNode* node = XMLUtils::locateNode(root, "SensitivityAnalysis");
    XMLUtils::checkNode(node, "SensitivityAnalysis");

    //discountCurrencies_ = XMLUtils::getChildrenValues(node, "DiscountCurrencies", "Currency", true);
    discountLabel_ = XMLUtils::getChildValue(node, "DiscountLabel", true);
    discountDomain_ = XMLUtils::getChildValue(node, "DiscountDomain", true);
    discountShiftType_ = XMLUtils::getChildValue(node, "DiscountShiftType", true);
    discountShiftSize_ = XMLUtils::getChildValueAsDouble(node, "DiscountShiftSize", true);
    discountShiftTenors_ = XMLUtils::getChildrenValuesAsPeriods(node, "DiscountShiftTenors", true);

    //indexNames_ = XMLUtils::getChildrenValues(node, "IndexNames", "Index", true);
    indexLabel_ = XMLUtils::getChildValue(node, "IndexLabel", true);
    indexDomain_ = XMLUtils::getChildValue(node, "IndexDomain", true);
    indexShiftType_ = XMLUtils::getChildValue(node, "IndexShiftType", true);
    indexShiftSize_ = XMLUtils::getChildValueAsDouble(node, "IndexShiftSize", true);
    indexShiftTenors_ = XMLUtils::getChildrenValuesAsPeriods(node, "IndexShiftTenors", true);

    //fxCurrencyPairs_ = XMLUtils::getChildrenValues(node, "FxCurrencyPairs", "CurrencyPair", true);
    fxLabel_ = XMLUtils::getChildValue(node, "FxLabel", true);
    fxShiftType_ = XMLUtils::getChildValue(node, "FxShiftType", true);
    fxShiftSize_ = XMLUtils::getChildValueAsDouble(node, "FxShiftSize", true);

    //infIndices_ = XMLUtils::getChildrenValues(node, "InflationIndices", "Index", true);
    infLabel_ = XMLUtils::getChildValue(node, "InflationLabel", true);
    infDomain_ = XMLUtils::getChildValue(node, "InflationDomain", true);
    infShiftType_ = XMLUtils::getChildValue(node, "InflationShiftType", true);
    infShiftSize_ = XMLUtils::getChildValueAsDouble(node, "InflationShiftSize", true);
    infShiftTenors_ = XMLUtils::getChildrenValuesAsPeriods(node, "InflationShiftTenors", true);

    //swaptionVolCurrencies_ = XMLUtils::getChildrenValues(node, "SwaptionVolatilityCurrencies", "Currency", true);
    swaptionVolLabel_ = XMLUtils::getChildValue(node, "SwaptionVolatilityLabel", true);
    swaptionVolShiftType_ = XMLUtils::getChildValue(node, "SwaptionVolatilityShiftType");
    swaptionVolShiftSize_ = XMLUtils::getChildValueAsDouble(node, "SwaptionVolatilityShiftSize");
    swaptionVolShiftTerms_ = XMLUtils::getChildrenValuesAsPeriods(node, "SwaptionVolatilityShiftTerms", true);
    swaptionVolShiftExpiries_ = XMLUtils::getChildrenValuesAsPeriods(node, "SwaptionVolatilityShiftExpiries", true);
    swaptionVolShiftStrikes_ = XMLUtils::getChildrenValuesAsDoubles(node, "SwaptionVolatilityShiftStrikes", "Strike", true);

    //crNames_ = XMLUtils::getChildrenValues(node, "CreditNames", "Name", true);
    crLabel_ = XMLUtils::getChildValue(node, "CreditLabel", true);
    crDomain_ = XMLUtils::getChildValue(node, "CreditDomain", true);
    crShiftType_ = XMLUtils::getChildValue(node, "CreditShiftType", true);
    crShiftSize_ = XMLUtils::getChildValueAsDouble(node, "CreditShiftSize", true);
    crShiftTenors_ = XMLUtils::getChildrenValuesAsPeriods(node, "CreditShiftTenors", true);  
}

XMLNode* SensitivityScenarioData::toXML(XMLDocument& doc) {

    XMLNode* node = doc.allocNode("SensitivityAnalysis");

    return node;
}
}
}
