#ifndef STORM_MODELCHECKER_SPARSE_DTMC_PRCTL_MODELCHECKER_HELPER_H_
#define STORM_MODELCHECKER_SPARSE_DTMC_PRCTL_MODELCHECKER_HELPER_H_

#include <vector>

#include <boost/optional.hpp>

#include "storm/models/sparse/StandardRewardModel.h"
#include "storm/modelchecker/hints/ModelCheckerHint.h"

#include "storm/storage/SparseMatrix.h"
#include "storm/storage/BitVector.h"

#include "storm/solver/LinearEquationSolver.h"
#include "storm/solver/SolveGoal.h"

namespace storm {
    
    class Environment;
    
    namespace modelchecker {
        class CheckResult;
        
        namespace helper {
            
            template <typename ValueType, typename RewardModelType = storm::models::sparse::StandardRewardModel<ValueType>>
            class SparseDtmcPrctlHelper {
            public:
                static std::vector<ValueType> computeBoundedUntilProbabilities(Environment const& env, storm::solver::SolveGoal<ValueType>&& goal, storm::storage::SparseMatrix<ValueType> const& transitionMatrix, storm::storage::SparseMatrix<ValueType> const& backwardTransitions, storm::storage::BitVector const& phiStates, storm::storage::BitVector const& psiStates, uint_fast64_t stepBound, storm::solver::LinearEquationSolverFactory<ValueType> const& linearEquationSolverFactory, ModelCheckerHint const& hint = ModelCheckerHint());
                
                static std::vector<ValueType> computeNextProbabilities(Environment const& env, storm::storage::SparseMatrix<ValueType> const& transitionMatrix, storm::storage::BitVector const& nextStates, storm::solver::LinearEquationSolverFactory<ValueType> const& linearEquationSolverFactory);
                
                static std::vector<ValueType> computeUntilProbabilities(Environment const& env, storm::solver::SolveGoal<ValueType>&& goal, storm::storage::SparseMatrix<ValueType> const& transitionMatrix, storm::storage::SparseMatrix<ValueType> const& backwardTransitions, storm::storage::BitVector const& phiStates, storm::storage::BitVector const& psiStates, bool qualitative, storm::solver::LinearEquationSolverFactory<ValueType> const& linearEquationSolverFactory, ModelCheckerHint const& hint = ModelCheckerHint());

                static std::vector<ValueType> computeGloballyProbabilities(Environment const& env, storm::solver::SolveGoal<ValueType>&& goal, storm::storage::SparseMatrix<ValueType> const& transitionMatrix, storm::storage::SparseMatrix<ValueType> const& backwardTransitions, storm::storage::BitVector const& psiStates, bool qualitative, storm::solver::LinearEquationSolverFactory<ValueType> const& linearEquationSolverFactory);
                
                static std::vector<ValueType> computeCumulativeRewards(Environment const& env, storm::solver::SolveGoal<ValueType>&& goal, storm::storage::SparseMatrix<ValueType> const& transitionMatrix, RewardModelType const& rewardModel, uint_fast64_t stepBound, storm::solver::LinearEquationSolverFactory<ValueType> const& linearEquationSolverFactory);
                
                static std::vector<ValueType> computeInstantaneousRewards(Environment const& env, storm::solver::SolveGoal<ValueType>&& goal, storm::storage::SparseMatrix<ValueType> const& transitionMatrix, RewardModelType const& rewardModel, uint_fast64_t stepCount, storm::solver::LinearEquationSolverFactory<ValueType> const& linearEquationSolverFactory);

                static std::vector<ValueType> computeReachabilityRewards(Environment const& env, storm::solver::SolveGoal<ValueType>&& goal, storm::storage::SparseMatrix<ValueType> const& transitionMatrix, storm::storage::SparseMatrix<ValueType> const& backwardTransitions, RewardModelType const& rewardModel, storm::storage::BitVector const& targetStates, bool qualitative, storm::solver::LinearEquationSolverFactory<ValueType> const& linearEquationSolverFactory, ModelCheckerHint const& hint = ModelCheckerHint());
                
                static std::vector<ValueType> computeReachabilityRewards(Environment const& env, storm::solver::SolveGoal<ValueType>&& goal, storm::storage::SparseMatrix<ValueType> const& transitionMatrix, storm::storage::SparseMatrix<ValueType> const& backwardTransitions, std::vector<ValueType> const& totalStateRewardVector, storm::storage::BitVector const& targetStates, bool qualitative, storm::solver::LinearEquationSolverFactory<ValueType> const& linearEquationSolverFactory, ModelCheckerHint const& hint = ModelCheckerHint());

                static std::vector<ValueType> computeLongRunAverageProbabilities(Environment const& env, storm::solver::SolveGoal<ValueType>&& goal, storm::storage::SparseMatrix<ValueType> const& transitionMatrix, storm::storage::BitVector const& psiStates, storm::solver::LinearEquationSolverFactory<ValueType> const& linearEquationSolverFactory);

                static std::vector<ValueType> computeLongRunAverageRewards(Environment const& env, storm::solver::SolveGoal<ValueType>&& goal,  storm::storage::SparseMatrix<ValueType> const& transitionMatrix, RewardModelType const& rewardModel, storm::solver::LinearEquationSolverFactory<ValueType> const& linearEquationSolverFactory);

                static std::vector<ValueType> computeLongRunAverageRewards(Environment const& env, storm::solver::SolveGoal<ValueType>&& goal, storm::storage::SparseMatrix<ValueType> const& transitionMatrix, std::vector<ValueType> const& stateRewards, storm::solver::LinearEquationSolverFactory<ValueType> const& linearEquationSolverFactory);

                static std::vector<ValueType> computeConditionalProbabilities(Environment const& env, storm::solver::SolveGoal<ValueType>&& goal, storm::storage::SparseMatrix<ValueType> const& transitionMatrix, storm::storage::SparseMatrix<ValueType> const& backwardTransitions, storm::storage::BitVector const& targetStates, storm::storage::BitVector const& conditionStates, bool qualitative, storm::solver::LinearEquationSolverFactory<ValueType> const& linearEquationSolverFactory);
                
                static std::vector<ValueType> computeConditionalRewards(Environment const& env, storm::solver::SolveGoal<ValueType>&& goal, storm::storage::SparseMatrix<ValueType> const& transitionMatrix, storm::storage::SparseMatrix<ValueType> const& backwardTransitions, RewardModelType const& rewardModel, storm::storage::BitVector const& targetStates, storm::storage::BitVector const& conditionStates, bool qualitative, storm::solver::LinearEquationSolverFactory<ValueType> const& linearEquationSolverFactory);
                
            private:
                static std::vector<ValueType> computeReachabilityRewards(Environment const& env, storm::solver::SolveGoal<ValueType>&& goal, storm::storage::SparseMatrix<ValueType> const& transitionMatrix, storm::storage::SparseMatrix<ValueType> const& backwardTransitions, std::function<std::vector<ValueType>(uint_fast64_t, storm::storage::SparseMatrix<ValueType> const&, storm::storage::BitVector const&)> const& totalStateRewardVectorGetter, storm::storage::BitVector const& targetStates, bool qualitative, storm::solver::LinearEquationSolverFactory<ValueType> const& linearEquationSolverFactory, ModelCheckerHint const& hint = ModelCheckerHint());

                struct BaierTransformedModel {
                    BaierTransformedModel() : noTargetStates(false) {
                        // Intentionally left empty.
                    }

                    storm::storage::BitVector getNewRelevantStates(storm::storage::BitVector const& oldRelevantStates) const;
                    storm::storage::BitVector getNewRelevantStates() const;

                    storm::storage::BitVector beforeStates;
                    boost::optional<storm::storage::SparseMatrix<ValueType>> transitionMatrix;
                    boost::optional<storm::storage::BitVector> targetStates;
                    boost::optional<std::vector<ValueType>> stateRewards;
                    bool noTargetStates;
                };
                
                static BaierTransformedModel computeBaierTransformation(Environment const& env, storm::storage::SparseMatrix<ValueType> const& transitionMatrix, storm::storage::SparseMatrix<ValueType> const& backwardTransitions, storm::storage::BitVector const& targetStates, storm::storage::BitVector const& conditionStates, boost::optional<std::vector<ValueType>> const& stateRewards, storm::solver::LinearEquationSolverFactory<ValueType> const& linearEquationSolverFactory);
            };
        }
    }
}

#endif /* STORM_MODELCHECKER_SPARSE_DTMC_PRCTL_MODELCHECKER_HELPER_H_ */
