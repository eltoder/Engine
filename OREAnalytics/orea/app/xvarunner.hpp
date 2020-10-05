/*
 Copyright (C) 2020 Quaternion Risk Management Ltd
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

/*! \file orea/app/xvarunner.hpp
    \brief A class to run the xva analysis
    \ingroup app
*/
#pragma once

#include <orea/aggregation/postprocess.hpp>
#include <orea/engine/valuationcalculator.hpp>
#include <orea/scenario/scenariogeneratordata.hpp>
#include <orea/scenario/scenariosimmarketparameters.hpp>
#include <ored/configuration/curveconfigurations.hpp>
#include <ored/model/crossassetmodeldata.hpp>

namespace ore {
namespace analytics {

class XvaRunner {
public:
    virtual ~XvaRunner() {}

    XvaRunner(QuantLib::Date asof, const std::string& baseCurrency,
              const boost::shared_ptr<ore::data::Portfolio>& portfolio,
              const boost::shared_ptr<ore::data::NettingSetManager>& netting,
              const boost::shared_ptr<ore::data::EngineData>& engineData,
              const boost::shared_ptr<ore::data::CurveConfigurations>& curveConfigs,
              const boost::shared_ptr<ore::data::Conventions>& conventions,
              const boost::shared_ptr<ore::data::TodaysMarketParameters>& todaysMarketParams,
              const boost::shared_ptr<ScenarioSimMarketParameters>& simMarketData,
              const boost::shared_ptr<ScenarioGeneratorData>& scenarioGeneratorData,
              const boost::shared_ptr<ore::data::CrossAssetModelData>& crossAssetModelData,
              std::vector<boost::shared_ptr<ore::data::LegBuilder>> extraLegBuilders = {},
              std::vector<boost::shared_ptr<ore::data::EngineBuilder>> extraEngineBuilders = {},
              const boost::shared_ptr<ReferenceDataManager>& referenceData = nullptr, QuantLib::Real dimQuantile = 0.99,
              QuantLib::Size dimHorizonCalendarDays = 14, map<string, bool> analytics = {},
              string calculationType = "Symmetric", string dvaName = "", string fvaBorrowingCurve = "",
              string fvaLendingCurve = "", bool fullInitialCollateralisation = true, bool storeFlows = false);

    void runXva(const boost::shared_ptr<ore::data::Market>& market, bool continueOnErr = true);

    const boost::shared_ptr<PostProcess>& postProcess() { return postProcess_; }

protected:
    virtual boost::shared_ptr<NPVCube>
    getNettingSetCube(std::vector<boost::shared_ptr<ValuationCalculator>>& calculators) {
        return nullptr;
    };
    virtual boost::shared_ptr<DynamicInitialMarginCalculator>
    getDimCalculator(const boost::shared_ptr<NPVCube>& cube,
                     const boost::shared_ptr<CubeInterpretation>& cubeInterpreter,
                     const boost::shared_ptr<AggregationScenarioData>& scenarioData,
                     const boost::shared_ptr<QuantExt::CrossAssetModel>& model = nullptr,
                     const boost::shared_ptr<NPVCube>& nettingCube = nullptr);

    QuantLib::Date asof_;
    std::string baseCurrency_;
    boost::shared_ptr<ore::data::Portfolio> portfolio_;
    boost::shared_ptr<ore::data::NettingSetManager> netting_;
    boost::shared_ptr<ore::data::EngineData> engineData_;
    boost::shared_ptr<ore::data::CurveConfigurations> curveConfigs_;
    boost::shared_ptr<ore::data::Conventions> conventions_;
    boost::shared_ptr<ore::data::TodaysMarketParameters> todaysMarketParams_;
    boost::shared_ptr<ScenarioSimMarketParameters> simMarketData_;
    boost::shared_ptr<ScenarioGeneratorData> scenarioGeneratorData_;
    boost::shared_ptr<ore::data::CrossAssetModelData> crossAssetModelData_;
    std::vector<boost::shared_ptr<ore::data::LegBuilder>> extraLegBuilders_;
    std::vector<boost::shared_ptr<ore::data::EngineBuilder>> extraEngineBuilders_;
    boost::shared_ptr<ReferenceDataManager> referenceData_;
    QuantLib::Real dimQuantile_;
    QuantLib::Size dimHorizonCalendarDays_;
    map<string, bool> analytics_;
    string calculationType_;
    string dvaName_;
    string fvaBorrowingCurve_;
    string fvaLendingCurve_;
    bool fullInitialCollateralisation_;
    bool storeFlows_;
    boost::shared_ptr<PostProcess> postProcess_;
};

} // namespace analytics
} // namespace ore