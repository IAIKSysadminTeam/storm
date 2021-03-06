#include "storm/modelchecker/csl/SparseMarkovAutomatonCslModelChecker.h"

#include "storm/modelchecker/csl/helper/SparseMarkovAutomatonCslHelper.h"

#include "storm/modelchecker/multiobjective/multiObjectiveModelChecking.h"

#include "storm/models/sparse/StandardRewardModel.h"

#include "storm/utility/FilteredRewardModel.h"
#include "storm/utility/macros.h"

#include "storm/settings/SettingsManager.h"
#include "storm/settings/modules/GeneralSettings.h"

#include "storm/modelchecker/results/ExplicitQualitativeCheckResult.h"
#include "storm/modelchecker/results/ExplicitQuantitativeCheckResult.h"

#include "storm/logic/FragmentSpecification.h"

#include "storm/exceptions/InvalidPropertyException.h"
#include "storm/exceptions/NotImplementedException.h"

namespace storm {
    namespace modelchecker {
        template<typename SparseMarkovAutomatonModelType>
        SparseMarkovAutomatonCslModelChecker<SparseMarkovAutomatonModelType>::SparseMarkovAutomatonCslModelChecker(SparseMarkovAutomatonModelType const& model) : SparsePropositionalModelChecker<SparseMarkovAutomatonModelType>(model) {
            // Intentionally left empty.
        }
        
        template<typename SparseMarkovAutomatonModelType>
        bool SparseMarkovAutomatonCslModelChecker<SparseMarkovAutomatonModelType>::canHandle(CheckTask<storm::logic::Formula, ValueType> const& checkTask) const {
            return SparseMarkovAutomatonCslModelChecker<SparseMarkovAutomatonModelType>::canHandleImplementation<ValueType>(checkTask);
        }
        
        template <typename SparseMarkovAutomatonModelType>
        template<typename CValueType, typename std::enable_if<storm::NumberTraits<CValueType>::SupportsExponential, int>::type>
        bool SparseMarkovAutomatonCslModelChecker<SparseMarkovAutomatonModelType>::canHandleImplementation(CheckTask<storm::logic::Formula, CValueType> const& checkTask) const {
            storm::logic::Formula const& formula = checkTask.getFormula();
            if(formula.isInFragment(storm::logic::csl().setGloballyFormulasAllowed(false).setNextFormulasAllowed(false).setRewardOperatorsAllowed(true).setReachabilityRewardFormulasAllowed(true).setTotalRewardFormulasAllowed(true).setTimeAllowed(true).setLongRunAverageProbabilitiesAllowed(true).setLongRunAverageRewardFormulasAllowed(true).setRewardAccumulationAllowed(true))) {
                return true;
            } else {
                // Check whether we consider a multi-objective formula
                // For multi-objective model checking, each initial state  requires an individual scheduler (in contrast to single objective model checking). Let's exclude multiple initial states.
                if (this->getModel().getInitialStates().getNumberOfSetBits() > 1) return false;
                if (!checkTask.isOnlyInitialStatesRelevantSet()) return false;
                return formula.isInFragment(storm::logic::multiObjective().setTimeAllowed(true).setTimeBoundedUntilFormulasAllowed(true));
            }
        }
        
        template <typename SparseMarkovAutomatonModelType>
        template<typename CValueType, typename std::enable_if<!storm::NumberTraits<CValueType>::SupportsExponential, int>::type>
        bool SparseMarkovAutomatonCslModelChecker<SparseMarkovAutomatonModelType>::canHandleImplementation(CheckTask<storm::logic::Formula, CValueType> const& checkTask) const {
            storm::logic::Formula const& formula = checkTask.getFormula();
            if(formula.isInFragment(storm::logic::prctl().setGloballyFormulasAllowed(false).setNextFormulasAllowed(false).setRewardOperatorsAllowed(true).setReachabilityRewardFormulasAllowed(true).setTotalRewardFormulasAllowed(true).setTimeAllowed(true).setLongRunAverageProbabilitiesAllowed(true).setLongRunAverageRewardFormulasAllowed(true).setTimeBoundedUntilFormulasAllowed(false).setCumulativeRewardFormulasAllowed(false).setInstantaneousFormulasAllowed(false))) {
                return true;
            } else {
                // Check whether we consider a multi-objective formula
                // For multi-objective model checking, each initial state  requires an individual scheduler (in contrast to single objective model checking). Let's exclude multiple initial states.
                if (this->getModel().getInitialStates().getNumberOfSetBits() > 1) return false;
                if (!checkTask.isOnlyInitialStatesRelevantSet()) return false;
                return formula.isInFragment(storm::logic::multiObjective().setTimeAllowed(true).setTimeBoundedUntilFormulasAllowed(false).setCumulativeRewardFormulasAllowed(false).setInstantaneousFormulasAllowed(false));
            }
        }
        
        template<typename SparseMarkovAutomatonModelType>
        std::unique_ptr<CheckResult> SparseMarkovAutomatonCslModelChecker<SparseMarkovAutomatonModelType>::computeBoundedUntilProbabilities(Environment const& env, CheckTask<storm::logic::BoundedUntilFormula, ValueType> const& checkTask) {
            storm::logic::BoundedUntilFormula const& pathFormula = checkTask.getFormula();
            STORM_LOG_THROW(checkTask.isOptimizationDirectionSet(), storm::exceptions::InvalidPropertyException, "Formula needs to specify whether minimal or maximal values are to be computed on nondeterministic model.");
            STORM_LOG_THROW(pathFormula.getLeftSubformula().isTrueFormula(), storm::exceptions::NotImplementedException, "Only bounded properties of the form 'true U[t1, t2] phi' are currently supported.");
            STORM_LOG_THROW(this->getModel().isClosed(), storm::exceptions::InvalidPropertyException, "Unable to compute time-bounded reachability probabilities in non-closed Markov automaton.");
            std::unique_ptr<CheckResult> rightResultPointer = this->check(env, pathFormula.getRightSubformula());
            ExplicitQualitativeCheckResult const& rightResult = rightResultPointer->asExplicitQualitativeCheckResult();

            STORM_LOG_THROW(pathFormula.getTimeBoundReference().isTimeBound(), storm::exceptions::NotImplementedException, "Currently step-bounded and reward-bounded properties on MAs are not supported.");
            double lowerBound = 0;
            double upperBound = 0;
            if (pathFormula.hasLowerBound()) {
                lowerBound = pathFormula.getLowerBound<double>();
            }
            if (pathFormula.hasUpperBound()) {
                upperBound = pathFormula.getNonStrictUpperBound<double>();
            } else {
                upperBound = storm::utility::infinity<double>();
            }

            std::vector<ValueType> result = storm::modelchecker::helper::SparseMarkovAutomatonCslHelper::computeBoundedUntilProbabilities(env, checkTask.getOptimizationDirection(), this->getModel().getTransitionMatrix(), this->getModel().getExitRates(), this->getModel().getMarkovianStates(), rightResult.getTruthValuesVector(), std::make_pair(lowerBound, upperBound));
            return std::unique_ptr<CheckResult>(new ExplicitQuantitativeCheckResult<ValueType>(std::move(result)));
        }
                
        template<typename SparseMarkovAutomatonModelType>
        std::unique_ptr<CheckResult> SparseMarkovAutomatonCslModelChecker<SparseMarkovAutomatonModelType>::computeUntilProbabilities(Environment const& env, CheckTask<storm::logic::UntilFormula, ValueType> const& checkTask) {
            storm::logic::UntilFormula const& pathFormula = checkTask.getFormula();
            STORM_LOG_THROW(checkTask.isOptimizationDirectionSet(), storm::exceptions::InvalidPropertyException, "Formula needs to specify whether minimal or maximal values are to be computed on nondeterministic model.");
            std::unique_ptr<CheckResult> leftResultPointer = this->check(env, pathFormula.getLeftSubformula());
            std::unique_ptr<CheckResult> rightResultPointer = this->check(env, pathFormula.getRightSubformula());
            ExplicitQualitativeCheckResult& leftResult = leftResultPointer->asExplicitQualitativeCheckResult();
            ExplicitQualitativeCheckResult& rightResult = rightResultPointer->asExplicitQualitativeCheckResult();
            std::vector<ValueType> result = storm::modelchecker::helper::SparseMarkovAutomatonCslHelper::computeUntilProbabilities(env, checkTask.getOptimizationDirection(), this->getModel().getTransitionMatrix(), this->getModel().getBackwardTransitions(), leftResult.getTruthValuesVector(), rightResult.getTruthValuesVector(), checkTask.isQualitativeSet());
            return std::unique_ptr<CheckResult>(new ExplicitQuantitativeCheckResult<ValueType>(std::move(result)));
        }
                
        template<typename SparseMarkovAutomatonModelType>
        std::unique_ptr<CheckResult> SparseMarkovAutomatonCslModelChecker<SparseMarkovAutomatonModelType>::computeReachabilityRewards(Environment const& env, storm::logic::RewardMeasureType, CheckTask<storm::logic::EventuallyFormula, ValueType> const& checkTask) {
            storm::logic::EventuallyFormula const& eventuallyFormula = checkTask.getFormula();
            STORM_LOG_THROW(checkTask.isOptimizationDirectionSet(), storm::exceptions::InvalidPropertyException, "Formula needs to specify whether minimal or maximal values are to be computed on nondeterministic model.");
            STORM_LOG_THROW(this->getModel().isClosed(), storm::exceptions::InvalidPropertyException, "Unable to compute reachability rewards in non-closed Markov automaton.");
            std::unique_ptr<CheckResult> subResultPointer = this->check(env, eventuallyFormula.getSubformula());
            ExplicitQualitativeCheckResult const& subResult = subResultPointer->asExplicitQualitativeCheckResult();
            
            auto rewardModel = storm::utility::createFilteredRewardModel(this->getModel(), checkTask);
            std::vector<ValueType> result = storm::modelchecker::helper::SparseMarkovAutomatonCslHelper::computeReachabilityRewards(env, checkTask.getOptimizationDirection(), this->getModel().getTransitionMatrix(), this->getModel().getBackwardTransitions(), this->getModel().getExitRates(), this->getModel().getMarkovianStates(), rewardModel.get(), subResult.getTruthValuesVector());

            return std::unique_ptr<CheckResult>(new ExplicitQuantitativeCheckResult<ValueType>(std::move(result)));
        }
        
        template<typename SparseMarkovAutomatonModelType>
        std::unique_ptr<CheckResult> SparseMarkovAutomatonCslModelChecker<SparseMarkovAutomatonModelType>::computeTotalRewards(Environment const& env, storm::logic::RewardMeasureType, CheckTask<storm::logic::TotalRewardFormula, ValueType> const& checkTask) {
            STORM_LOG_THROW(checkTask.isOptimizationDirectionSet(), storm::exceptions::InvalidPropertyException, "Formula needs to specify whether minimal or maximal values are to be computed on nondeterministic model.");
            STORM_LOG_THROW(this->getModel().isClosed(), storm::exceptions::InvalidPropertyException, "Unable to compute reachability rewards in non-closed Markov automaton.");
            auto rewardModel = storm::utility::createFilteredRewardModel(this->getModel(), checkTask);
            std::vector<ValueType> result = storm::modelchecker::helper::SparseMarkovAutomatonCslHelper::computeTotalRewards(env, checkTask.getOptimizationDirection(), this->getModel().getTransitionMatrix(), this->getModel().getBackwardTransitions(), this->getModel().getExitRates(), this->getModel().getMarkovianStates(), rewardModel.get());

            return std::unique_ptr<CheckResult>(new ExplicitQuantitativeCheckResult<ValueType>(std::move(result)));
        }
        
        template<typename SparseMarkovAutomatonModelType>
        std::unique_ptr<CheckResult> SparseMarkovAutomatonCslModelChecker<SparseMarkovAutomatonModelType>::computeLongRunAverageProbabilities(Environment const& env, CheckTask<storm::logic::StateFormula, ValueType> const& checkTask) {
            storm::logic::StateFormula const& stateFormula = checkTask.getFormula();
            STORM_LOG_THROW(checkTask.isOptimizationDirectionSet(), storm::exceptions::InvalidPropertyException, "Formula needs to specify whether minimal or maximal values are to be computed on nondeterministic model.");
            STORM_LOG_THROW(this->getModel().isClosed(), storm::exceptions::InvalidPropertyException, "Unable to compute long-run average in non-closed Markov automaton.");
            std::unique_ptr<CheckResult> subResultPointer = this->check(env, stateFormula);
            ExplicitQualitativeCheckResult const& subResult = subResultPointer->asExplicitQualitativeCheckResult();

            std::vector<ValueType> result = storm::modelchecker::helper::SparseMarkovAutomatonCslHelper::computeLongRunAverageProbabilities(env, checkTask.getOptimizationDirection(), this->getModel().getTransitionMatrix(), this->getModel().getBackwardTransitions(), this->getModel().getExitRates(), this->getModel().getMarkovianStates(), subResult.getTruthValuesVector());
            return std::unique_ptr<CheckResult>(new ExplicitQuantitativeCheckResult<ValueType>(std::move(result)));
        }
        
        template<typename SparseMarkovAutomatonModelType>
        std::unique_ptr<CheckResult> SparseMarkovAutomatonCslModelChecker<SparseMarkovAutomatonModelType>::computeLongRunAverageRewards(Environment const& env, storm::logic::RewardMeasureType rewardMeasureType, CheckTask<storm::logic::LongRunAverageRewardFormula, ValueType> const& checkTask) {
            STORM_LOG_THROW(checkTask.isOptimizationDirectionSet(), storm::exceptions::InvalidPropertyException, "Formula needs to specify whether minimal or maximal values are to be computed on nondeterministic model.");
            STORM_LOG_THROW(this->getModel().isClosed(), storm::exceptions::InvalidPropertyException, "Unable to compute long run average rewards in non-closed Markov automaton.");
            auto rewardModel = storm::utility::createFilteredRewardModel(this->getModel(), checkTask);
            std::vector<ValueType> result = storm::modelchecker::helper::SparseMarkovAutomatonCslHelper::computeLongRunAverageRewards<ValueType, RewardModelType>(env, checkTask.getOptimizationDirection(), this->getModel().getTransitionMatrix(), this->getModel().getBackwardTransitions(), this->getModel().getExitRates(), this->getModel().getMarkovianStates(), rewardModel.get());
            return std::unique_ptr<CheckResult>(new ExplicitQuantitativeCheckResult<ValueType>(std::move(result)));
        }
        
        template<typename SparseMarkovAutomatonModelType>
        std::unique_ptr<CheckResult> SparseMarkovAutomatonCslModelChecker<SparseMarkovAutomatonModelType>::computeReachabilityTimes(Environment const& env, storm::logic::RewardMeasureType, CheckTask<storm::logic::EventuallyFormula, ValueType> const& checkTask) {
            storm::logic::EventuallyFormula const& eventuallyFormula = checkTask.getFormula();
            STORM_LOG_THROW(checkTask.isOptimizationDirectionSet(), storm::exceptions::InvalidPropertyException, "Formula needs to specify whether minimal or maximal values are to be computed on nondeterministic model.");
            STORM_LOG_THROW(this->getModel().isClosed(), storm::exceptions::InvalidPropertyException, "Unable to compute expected times in non-closed Markov automaton.");
            std::unique_ptr<CheckResult> subResultPointer = this->check(env, eventuallyFormula.getSubformula());
            ExplicitQualitativeCheckResult& subResult = subResultPointer->asExplicitQualitativeCheckResult();

            std::vector<ValueType> result = storm::modelchecker::helper::SparseMarkovAutomatonCslHelper::computeReachabilityTimes(env, checkTask.getOptimizationDirection(), this->getModel().getTransitionMatrix(), this->getModel().getBackwardTransitions(), this->getModel().getExitRates(), this->getModel().getMarkovianStates(), subResult.getTruthValuesVector());
            return std::unique_ptr<CheckResult>(new ExplicitQuantitativeCheckResult<ValueType>(std::move(result)));
        }
        
        template<typename SparseMarkovAutomatonModelType>
        std::unique_ptr<CheckResult> SparseMarkovAutomatonCslModelChecker<SparseMarkovAutomatonModelType>::checkMultiObjectiveFormula(Environment const& env, CheckTask<storm::logic::MultiObjectiveFormula, ValueType> const& checkTask) {
            return multiobjective::performMultiObjectiveModelChecking(env, this->getModel(), checkTask.getFormula());
        }
        
        template class SparseMarkovAutomatonCslModelChecker<storm::models::sparse::MarkovAutomaton<double>>;
        template class SparseMarkovAutomatonCslModelChecker<storm::models::sparse::MarkovAutomaton<storm::RationalNumber>>;
    }
}
