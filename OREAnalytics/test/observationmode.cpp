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

#include <test/observationmode.hpp>
#include <test/testmarket.hpp>
#include <ored/utilities/osutils.hpp>
#include <ored/utilities/log.hpp>
#include <orea/cube/npvcube.hpp>
#include <orea/cube/inmemorycube.hpp>
#include <ored/model/lgmdata.hpp>
#include <orea/scenario/scenariosimmarketparameters.hpp>
#include <ored/model/crossassetmodelbuilder.hpp>
#include <orea/scenario/scenariosimmarket.hpp>
#include <orea/scenario/simplescenariofactory.hpp>
#include <orea/scenario/crossassetmodelscenariogenerator.hpp>
#include <qle/methods/multipathgeneratorbase.hpp>
#include <ql/time/date.hpp>
#include <ql/math/randomnumbers/mt19937uniformrng.hpp>
#include <ql/time/calendars/target.hpp>
#include <ql/time/daycounters/actualactual.hpp>
#include <orea/engine/all.hpp>
#include <ored/portfolio/swap.hpp>
#include <ored/portfolio/builders/swap.hpp>
#include <ored/portfolio/portfolio.hpp>
#include <orea/engine/observationmode.hpp>
#include <boost/timer.hpp>

using namespace std;
using namespace QuantLib;
using namespace QuantExt;
using namespace boost::unit_test_framework;
using namespace ore;
using namespace ore::data;
using namespace ore::analytics;

namespace testsuite {

boost::shared_ptr<data::Conventions> conventions() {
    boost::shared_ptr<data::Conventions> conventions(new data::Conventions());

    boost::shared_ptr<data::Convention> swapIndexConv(
        new data::SwapIndexConvention("EUR-CMS-2Y", "EUR-6M-SWAP-CONVENTIONS"));
    conventions->add(swapIndexConv);

    boost::shared_ptr<data::Convention> swapConv(
        new data::IRSwapConvention("EUR-6M-SWAP-CONVENTIONS", "TARGET", "Annual", "MF", "30/360", "EUR-EURIBOR-6M"));
    conventions->add(swapConv);

    return conventions;
}

boost::shared_ptr<Portfolio> buildPortfolio(boost::shared_ptr<EngineFactory>& factory) {

    boost::shared_ptr<Portfolio> portfolio(new Portfolio());

    string ccy = "EUR";
    string index = "EUR-EURIBOR-6M";
    string floatFreq = "6M";
    Real fixedRate = 0.02;
    string fixFreq = "1Y";
    Size term = 10;
    bool isPayer = true;

    Date today = Settings::instance().evaluationDate();
    Calendar cal = TARGET();
    string calStr = "TARGET";
    string conv = "MF";
    string rule = "Forward";
    Size days = 2;
    string fixDC = "30/360";
    string floatDC = "ACT/360";

    vector<double> notional(1, 1000000);
    vector<double> spread(1, 0);

    Date startDate = cal.adjust(today+1*Months);
    Date endDate = cal.adjust(startDate + term * Years);

    // date 2 string
    ostringstream oss;
    oss << io::iso_date(startDate);
    string start(oss.str());
    oss.str("");
    oss.clear();
    oss << io::iso_date(endDate);
    string end(oss.str());
    
    // envelope
    Envelope env("CP");
    
    // Schedules
    ScheduleData floatSchedule(ScheduleRules(start, end, floatFreq, calStr, conv, conv, rule));
    ScheduleData fixedSchedule(ScheduleRules(start, end, fixFreq, calStr, conv, conv, rule));
    
    // fixed Leg - with dummy rate
    FixedLegData fixedLegData(vector<double>(1, fixedRate));
    LegData fixedLeg(isPayer, ccy, fixedLegData, fixedSchedule, fixDC, notional);

    // float Leg
    vector<double> spreads(1, 0);
    FloatingLegData floatingLegData(index, days, true, spread);
    LegData floatingLeg(!isPayer, ccy, floatingLegData, floatSchedule, floatDC, notional);
    
    boost::shared_ptr<Trade> swap(new data::Swap(env, floatingLeg, fixedLeg));
    
    swap->id() = "SWAP";
    
    portfolio->add(swap);
    
    portfolio->build(factory);
    
    return portfolio;
}
  
  void simulation(string dateGridString, bool checkFixings) {
    SavedSettings backup;

    // Log::instance().registerLogger(boost::make_shared<StderrLogger>());
    // Log::instance().switchOn();

    Date today = Date(14, April, 2016); // Settings::instance().evaluationDate();
    Settings::instance().evaluationDate() = today;

    //string dateGridStr = "80,3M"; // 20 years
    boost::shared_ptr<DateGrid> dg = boost::make_shared<DateGrid>(dateGridString);
    Size samples = 100;

    BOOST_TEST_MESSAGE("Date Grid : " << dateGridString);

    // build model
    string baseCcy = "EUR";
    vector<string> ccys;
    ccys.push_back(baseCcy);
    ccys.push_back("GBP");
    ccys.push_back("CHF");
    ccys.push_back("USD");
    ccys.push_back("JPY");

    // Init market
    boost::shared_ptr<Market> initMarket = boost::make_shared<TestMarket>(today);

    // build scenario sim market parameters
    boost::shared_ptr<analytics::ScenarioSimMarketParameters> parameters(new analytics::ScenarioSimMarketParameters());
    parameters->baseCcy() = "EUR";
    parameters->ccys() = {"EUR", "GBP", "USD", "CHF", "JPY"};
    parameters->yieldCurveTenors() = {1 * Months, 6 * Months, 1 * Years, 2 * Years, 5 * Years, 10 * Years, 20 * Years};
    parameters->indices() = {"EUR-EURIBOR-6M", "USD-LIBOR-3M", "GBP-LIBOR-6M", "CHF-LIBOR-6M", "JPY-LIBOR-6M"};
    parameters->interpolation() = "LogLinear";
    parameters->extrapolate() = true;

    parameters->swapVolTerms() = {6 * Months, 1 * Years};
    parameters->swapVolExpiries() = {1 * Years, 2 * Years};
    parameters->swapVolCcys() = ccys;
    parameters->swapVolDecayMode() = "ForwardVariance";

    parameters->fxVolExpiries() = {1 * Months, 3 * Months, 6 * Months, 2 * Years, 3 * Years, 4 * Years, 5 * Years};
    parameters->fxVolDecayMode() = "ConstantVariance";
    parameters->simulateFXVols() = false;

    parameters->fxVolCcyPairs() = {"EURUSD", "EURGBP", "EURCHF", "EURJPY"};

    parameters->fxCcyPairs() = {"EURUSD", "EURGBP", "EURCHF", "EURJPY"};

    parameters->additionalScenarioDataIndices() = {"EUR-EURIBOR-6M", "USD-LIBOR-3M", "GBP-LIBOR-6M", "CHF-LIBOR-6M", "JPY-LIBOR-6M"};
    parameters->additionalScenarioDataCcys() = {"EUR", "GBP", "USD", "CHF", "JPY"};

    // Config

    // Build IR configurations
    CalibrationType calibrationType = CalibrationType::Bootstrap;
    LgmData::ReversionType revType = LgmData::ReversionType::HullWhite;
    LgmData::VolatilityType volType = LgmData::VolatilityType::Hagan;
    vector<string> swaptionExpiries = {"1Y", "2Y", "3Y", "5Y", "7Y", "10Y", "15Y", "20Y", "30Y"};
    vector<string> swaptionTerms = {"5Y", "5Y", "5Y", "5Y", "5Y", "5Y", "5Y", "5Y", "5Y"};
    vector<string> swaptionStrikes(swaptionExpiries.size(), "ATM");
    vector<Time> hTimes = {};
    vector<Time> aTimes = {};
    vector<Real> hValues = {};
    vector<Real> aValues = {};

    std::vector<boost::shared_ptr<LgmData>> irConfigs;

    hValues = {0.02};
    aValues = {0.008};
    irConfigs.push_back(boost::make_shared<LgmData>(
        "EUR", calibrationType, revType, volType, false, ParamType::Constant, hTimes, hValues, true,
        ParamType::Piecewise, aTimes, aValues, 0.0, 1.0, swaptionExpiries, swaptionTerms, swaptionStrikes));

    hValues = {0.03};
    aValues = {0.009};
    irConfigs.push_back(boost::make_shared<LgmData>(
        "USD", calibrationType, revType, volType, false, ParamType::Constant, hTimes, hValues, true,
        ParamType::Piecewise, aTimes, aValues, 0.0, 1.0, swaptionExpiries, swaptionTerms, swaptionStrikes));

    hValues = {0.04};
    aValues = {0.01};
    irConfigs.push_back(boost::make_shared<LgmData>(
        "GBP", calibrationType, revType, volType, false, ParamType::Constant, hTimes, hValues, true,
        ParamType::Piecewise, aTimes, aValues, 0.0, 1.0, swaptionExpiries, swaptionTerms, swaptionStrikes));

    hValues = {0.04};
    aValues = {0.01};
    irConfigs.push_back(boost::make_shared<LgmData>(
        "CHF", calibrationType, revType, volType, false, ParamType::Constant, hTimes, hValues, true,
        ParamType::Piecewise, aTimes, aValues, 0.0, 1.0, swaptionExpiries, swaptionTerms, swaptionStrikes));

    hValues = {0.04};
    aValues = {0.01};
    irConfigs.push_back(boost::make_shared<LgmData>(
        "JPY", calibrationType, revType, volType, false, ParamType::Constant, hTimes, hValues, true,
        ParamType::Piecewise, aTimes, aValues, 0.0, 1.0, swaptionExpiries, swaptionTerms, swaptionStrikes));

    // Compile FX configurations
    vector<string> optionExpiries = {"1Y", "2Y", "3Y", "5Y", "7Y", "10Y"};
    vector<string> optionStrikes(optionExpiries.size(), "ATMF");
    vector<Time> sigmaTimes = {};
    vector<Real> sigmaValues = {};

    std::vector<boost::shared_ptr<FxBsData>> fxConfigs;
    sigmaValues = {0.15};
    fxConfigs.push_back(boost::make_shared<FxBsData>("USD", "EUR", calibrationType, true, ParamType::Piecewise,
                                                     sigmaTimes, sigmaValues, optionExpiries, optionStrikes));

    sigmaValues = {0.20};
    fxConfigs.push_back(boost::make_shared<FxBsData>("GBP", "EUR", calibrationType, true, ParamType::Piecewise,
                                                     sigmaTimes, sigmaValues, optionExpiries, optionStrikes));

    sigmaValues = {0.20};
    fxConfigs.push_back(boost::make_shared<FxBsData>("CHF", "EUR", calibrationType, true, ParamType::Piecewise,
                                                     sigmaTimes, sigmaValues, optionExpiries, optionStrikes));

    sigmaValues = {0.20};
    fxConfigs.push_back(boost::make_shared<FxBsData>("JPY", "EUR", calibrationType, true, ParamType::Piecewise,
                                                     sigmaTimes, sigmaValues, optionExpiries, optionStrikes));

    std::map<std::pair<std::string, std::string>, Real> corr;
    corr[std::make_pair("IR:EUR", "IR:USD")] = 0.6;

    boost::shared_ptr<CrossAssetModelData> config(boost::make_shared<CrossAssetModelData>(irConfigs, fxConfigs, corr));

    // Model Builder & Model
    // model builder
    boost::shared_ptr<CrossAssetModelBuilder> modelBuilder(new CrossAssetModelBuilder(initMarket));
    boost::shared_ptr<QuantExt::CrossAssetModel> model = modelBuilder->build(config);
    modelBuilder = NULL;

    // Path generator
    Size seed = 5;
    bool antithetic = false;
    boost::shared_ptr<QuantExt::MultiPathGeneratorBase> pathGen =
        boost::make_shared<MultiPathGeneratorMersenneTwister>(model->stateProcess(), dg->timeGrid(), seed, antithetic);

    // build scenario generator
    boost::shared_ptr<ScenarioFactory> scenarioFactory(new SimpleScenarioFactory);
    boost::shared_ptr<ScenarioGenerator> scenarioGenerator = boost::make_shared<CrossAssetModelScenarioGenerator>(
        model, pathGen, scenarioFactory, parameters, today, dg, initMarket);

    // build scenario sim market
    Conventions conv = *conventions();
    boost::shared_ptr<analytics::ScenarioSimMarket> simMarket =
        boost::make_shared<analytics::ScenarioSimMarket>(scenarioGenerator, initMarket, parameters, conv);

    // Build Porfolio
    boost::shared_ptr<EngineData> data = boost::make_shared<EngineData>();
    data->model("Swap") = "DiscountedCashflows";
    data->engine("Swap") = "DiscountingSwapEngine";
    boost::shared_ptr<EngineFactory> factory = boost::make_shared<EngineFactory>(data, simMarket);
    factory->registerBuilder(boost::make_shared<SwapEngineBuilder>());

    boost::shared_ptr<Portfolio> portfolio = buildPortfolio(factory);

    // Storage for selected scenario data (index fixings, FX rates, ..)
    if (checkFixings) {
      boost::shared_ptr<InMemoryAggregationScenarioData> inMemoryScenarioData = boost::make_shared<InMemoryAggregationScenarioData>(dg->size(), samples);
	// Set AggregationScenarioData
	simMarket->aggregationScenarioData() = inMemoryScenarioData;
    }

    // Now calculate exposure
    ValuationEngine valEngine(today, dg, simMarket);

    // Calculate Cube
    boost::timer t;
    boost::shared_ptr<NPVCube> cube =
        boost::make_shared<DoublePrecisionInMemoryCube>(today, portfolio->ids(), dg->dates(), samples);
    vector<boost::shared_ptr<ValuationCalculator>> calculators;
    calculators.push_back(boost::make_shared<NPVCalculator>(baseCcy));
    valEngine.buildCube(portfolio, cube, calculators);
    double elapsed = t.elapsed();

    BOOST_TEST_MESSAGE("Cube generated in " << elapsed << " seconds");

    map<string,vector<Real>> referenceFixings;
    // First 10 EUR-EURIBOR-6M fixings at dateIndex 5, date grid 11,1Y 
    referenceFixings["11,1Y"] = {
        0.00745427, 
	0.028119, 
	0.0343574, 
	0.0335416, 
	0.0324554, 
	0.0305116, 
	0.00901458, 
	0.016573, 
	0.0194405, 
	0.0113262, 
	0.0238971 };

    // First 10 EUR-EURIBOR-6M fixings at dateIndex 5, date grid 10,1Y 
    referenceFixings["10,1Y"] = {
        0.00745427, 
	0.0296431, 
	0.0338739, 
	0.012485, 
	0.0135247, 
	0.0148336, 
	0.018856, 
	0.0276796, 
	0.0349766, 
	0.0105696, 
	0.0103713 } ;
      
    if (simMarket->aggregationScenarioData()) {
        QL_REQUIRE(dateGridString == "10,1Y" || dateGridString == "11,1Y",
		   "date grid string " << dateGridString << " unexpected");
	// Reference scenario data:
	Size dateIndex = 5;
	Size maxSample = 10;
	string qualifier = "EUR-EURIBOR-6M";
	Real tolerance = 1.0e-6;
	for (Size sampleIndex = 0; sampleIndex <= maxSample; ++sampleIndex) {
	    Real fix = simMarket->aggregationScenarioData()->get(dateIndex, sampleIndex, AggregationScenarioDataType::IndexFixing, qualifier);
	    Real ref = referenceFixings[dateGridString][sampleIndex];
	    if (fabs(fix - ref) > tolerance)
	        BOOST_FAIL("Stored fixing differs from reference value, found " << fix << ", expected " << ref);
	}
    }
}

void ObservationModeTest::testDisableShort() {
    ObservationMode::instance().setMode(ObservationMode::Mode::Disable);

    BOOST_TEST_MESSAGE("Testing Observation Mode Disable, Short Grid, No Fixing Checks");
    simulation("10,1Y", false);

    BOOST_TEST_MESSAGE("Testing Observation Mode Disable, Short Grid, With Fixing Checks");
    simulation("10,1Y", true);
}

void ObservationModeTest::testDisableLong() {
    ObservationMode::instance().setMode(ObservationMode::Mode::Disable);

    BOOST_TEST_MESSAGE("Testing Observation Mode Disable, Long Grid, No Fixing Checks");
    simulation("11,1Y", false);

    BOOST_TEST_MESSAGE("Testing Observation Mode Disable, Long Grid, With Fixing Checks");
    simulation("11,1Y", true);
}

  void ObservationModeTest::testNone() {
    ObservationMode::instance().setMode(ObservationMode::Mode::None);

    BOOST_TEST_MESSAGE("Testing Observation Mode None, Short Grid, No Fixing Checks");
    simulation("10,1Y", false);

    BOOST_TEST_MESSAGE("Testing Observation Mode None, Short Grid, With Fixing Checks");
    simulation("10,1Y", true);

    BOOST_TEST_MESSAGE("Testing Observation Mode None, Long Grid, No Fixing Checks");
    simulation("11,1Y", false);

    BOOST_TEST_MESSAGE("Testing Observation Mode None, Long Grid, With Fixing Checks");
    simulation("11,1Y", true);

}
  
void ObservationModeTest::testUnregister() {
    ObservationMode::instance().setMode(ObservationMode::Mode::Unregister);

    BOOST_TEST_MESSAGE("Testing Observation Mode Unregister, Long Grid, No Fixing Checks");
    simulation("11,1Y", false);

    BOOST_TEST_MESSAGE("Testing Observation Mode Unregister, Long Grid, With Fixing Checks");
    simulation("11,1Y", true);

    BOOST_TEST_MESSAGE("Testing Observation Mode Unregister, Short Grid, No Fixing Checks");
    simulation("10,1Y", false);

    BOOST_TEST_MESSAGE("Testing Observation Mode Unregister, Short Grid, With Fixing Checks");
    simulation("10,1Y", true);
}

void ObservationModeTest::testDefer() {
    ObservationMode::instance().setMode(ObservationMode::Mode::Defer);

    BOOST_TEST_MESSAGE("Testing Observation Mode Defer, Long Grid, No Fixing Checks");
    simulation("11,1Y", false);

    BOOST_TEST_MESSAGE("Testing Observation Mode Defer, Long Grid, With Fixing Checks");
    simulation("11,1Y", true);

    BOOST_TEST_MESSAGE("Testing Observation Mode Defer, Short Grid, No Fixing Checks");
    simulation("10,1Y", false);

    BOOST_TEST_MESSAGE("Testing Observation Mode Defer, Short Grid, With Fixing Checks");
    simulation("10,1Y", true);
}

test_suite* ObservationModeTest::suite() {
    // Uncomment the below to get detailed output TODO: custom logger that uses BOOST_MESSAGE
    /*
    boost::shared_ptr<ore::data::FileLogger> logger
        = boost::make_shared<ore::data::FileLogger>("swapperformace_test.log");
    ore::data::Log::instance().removeAllLoggers();
    ore::data::Log::instance().registerLogger(logger);
    ore::data::Log::instance().switchOn();
    ore::data::Log::instance().setMask(255);
    */

    test_suite* suite = BOOST_TEST_SUITE("ObservationModeTest");
    // Set the Observation mode here
    suite->add(BOOST_TEST_CASE(&ObservationModeTest::testNone));
    suite->add(BOOST_TEST_CASE(&ObservationModeTest::testUnregister));
    suite->add(BOOST_TEST_CASE(&ObservationModeTest::testDefer));
    suite->add(BOOST_TEST_CASE(&ObservationModeTest::testDisableShort));
    suite->add(BOOST_TEST_CASE(&ObservationModeTest::testDisableLong));
    return suite;
}
}
