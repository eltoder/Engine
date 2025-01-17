/*
 Copyright (C) 2017 Quaternion Risk Management Ltd
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

/*! \file mcmultilegbaseengine.hpp
    \brief base MC engine for multileg (option) instruments
*/

#pragma once

#include <qle/indexes/fxindex.hpp>
#include <qle/instruments/multilegoption.hpp>
#include <qle/methods/multipathgeneratorbase.hpp>
#include <qle/models/crossassetmodel.hpp>
#include <qle/models/lgmvectorised.hpp>
#include <qle/pricingengines/amccalculator.hpp>

#include <ql/indexes/interestrateindex.hpp>
#include <ql/instruments/swaption.hpp>
#include <ql/methods/montecarlo/lsmbasissystem.hpp>

namespace QuantExt {

class McMultiLegBaseEngine {

protected:
    /*! The npv is computed in the model's base currency, discounting curves are taken from the model. simulationDates
        are additional simulation dates. The cross asset model here must be consistent with the multi path that is the
        input to AmcCalculator::simulatePath().

        Current limitations:
        - the parameter minimalObsDate is ignored, the corresponding optimization is not implemented yet
        - polynomType is ignored, Mononimal is used always
        - prixingSamples are ignored, the npv from the training phase is used alway
    */
    McMultiLegBaseEngine(
        const Handle<CrossAssetModel>& model, const SequenceType calibrationPathGenerator,
        const SequenceType pricingPathGenerator, const Size calibrationSamples, const Size pricingSamples,
        const Size calibrationSeed, const Size pricingSeed, const Size polynomOrder,
        const LsmBasisSystem::PolynomialType polynomType, const SobolBrownianGenerator::Ordering ordering,
        const SobolRsg::DirectionIntegers directionIntegers,
        const std::vector<Handle<YieldTermStructure>>& discountCurves = std::vector<Handle<YieldTermStructure>>(),
        const std::vector<Date>& simulationDates = std::vector<Date>(),
        const std::vector<Size>& externalModelIndices = std::vector<Size>(), const bool minimalObsDate = true);

    // run calibration and pricing (called from derived engines)
    void calculate() const;

    // return AmcCalculator instance (called from derived engines, calculate must be called before)
    boost::shared_ptr<AmcCalculator> amcCalculator() const;

    // input data from the derived pricing engines, to be set in these engines
    mutable std::vector<Leg> leg_;
    mutable std::vector<Currency> currency_;
    mutable std::vector<Real> payer_;
    mutable boost::shared_ptr<Exercise> exercise_; // may be empty, if underlying is the actual trade
    mutable Settlement::Type optionSettlement_ = Settlement::Physical;

    // data members
    Handle<CrossAssetModel> model_;
    SequenceType calibrationPathGenerator_, pricingPathGenerator_;
    Size calibrationSamples_, pricingSamples_, calibrationSeed_, pricingSeed_;
    Size polynomOrder_;
    LsmBasisSystem::PolynomialType polynomType_;
    SobolBrownianGenerator::Ordering ordering_;
    SobolRsg::DirectionIntegers directionIntegers_;
    std::vector<Handle<YieldTermStructure>> discountCurves_;
    std::vector<Date> simulationDates_;
    std::vector<Size> externalModelIndices_;
    bool minimalObsDate_;

    // the generated amc calculator
    mutable boost::shared_ptr<AmcCalculator> amcCalculator_;

    // results, these are read from derived engines
    mutable Real resultUnderlyingNpv_, resultValue_;

private:
    // data structure storing info needed to generate the amount for a cashflow
    struct CashflowInfo {
        Size legNo = Null<Size>(), cfNo = Null<Size>();
        Real payTime = Null<Real>();
        Real exIntoCriterionTime = Null<Real>();
        Size payCcyIndex = Null<Size>();
        Real payer = 1.0;
        std::vector<Real> simulationTimes;
        std::vector<std::vector<Size>> modelIndices;
        std::function<RandomVariable(const Size n, const std::vector<std::vector<const RandomVariable*>>&)>
            amountCalculator;
    };

    // convert a date to a time w.r.t. the valuation date
    Real time(const Date& d) const;

    // create the info for a given flow
    CashflowInfo createCashflowInfo(boost::shared_ptr<CashFlow> flow, const Currency& payCcy, Real payer, Size legNo,
                                    Size cfNo) const;

    // get the index of a time in the given simulation times set
    Size timeIndex(const Time t, const std::set<Real>& simulationTimes) const;

    // compute a cashflow path value (in model base ccy)
    RandomVariable cashflowPathValue(const CashflowInfo& cf, const std::vector<std::vector<RandomVariable>>& pathValues,
                                     const std::set<Real>& simulationTimes) const;

    // valuation date
    mutable Date today_;

    // lgm vectorised instances for each ccy
    mutable std::vector<LgmVectorised> lgmVectorised_;
};

class MultiLegBaseAmcCalculator : public AmcCalculator {
public:
    MultiLegBaseAmcCalculator(
        const std::vector<Size>& externalModelIndices, const Settlement::Type settlement,
        const std::set<Real>& exerciseXvaTimes, const std::set<Real>& exerciseTimes, const std::set<Real>& xvaTimes,
        const std::vector<Array>& coeffsUndDirty, const std::vector<Array>& coeffsUndExInto,
        const std::vector<Array>& coeffsContinuationValue, const std::vector<Array>& coeffsOption,
        const std::vector<std::function<RandomVariable(const std::vector<const RandomVariable*>&)>>& basisFns,
        const Real resultValue, const Array& initialState, const Currency& baseCurrency);

    Currency npvCurrency() override { return baseCurrency_; }
    std::vector<QuantExt::RandomVariable> simulatePath(const std::vector<QuantLib::Real>& pathTimes,
                                                       std::vector<std::vector<QuantExt::RandomVariable>>& paths,
                                                       const std::vector<bool>& isRelevantTime,
                                                       const bool stickyCloseOutRun) override;

private:
    std::vector<Size> externalModelIndices_;
    Settlement::Type settlement_;
    std::set<Real> exerciseXvaTimes_;
    std::set<Real> exerciseTimes_;
    std::set<Real> xvaTimes_;
    std::vector<Array> coeffsUndDirty_;
    std::vector<Array> coeffsUndExInto_;
    std::vector<Array> coeffsContinuationValue_;
    std::vector<Array> coeffsOption_;
    std::vector<std::function<RandomVariable(const std::vector<const RandomVariable*>&)>> basisFns_;
    Real resultValue_;
    Array initialState_;
    Currency baseCurrency_;

    std::vector<Filter> exercised_;
};

} // namespace QuantExt
