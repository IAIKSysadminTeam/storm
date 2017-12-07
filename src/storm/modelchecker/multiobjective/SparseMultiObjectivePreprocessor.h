#pragma once

#include <memory>
#include <string>

#include "storm/logic/Formulas.h"
#include "storm/storage/BitVector.h"
#include "storm/modelchecker/multiobjective/SparseMultiObjectivePreprocessorResult.h"
#include "storm/modelchecker/multiobjective/SparseMultiObjectivePreprocessorTask.h"
#include "storm/storage/memorystructure/MemoryStructure.h"

namespace storm {
    namespace modelchecker {
        namespace multiobjective {
            
            /*
             * This class invokes the necessary preprocessing for the constraint based multi-objective model checking algorithm
             */
            template <class SparseModelType>
            class SparseMultiObjectivePreprocessor {
            public:
                typedef typename SparseModelType::ValueType ValueType;
                typedef typename SparseModelType::RewardModelType RewardModelType;
                typedef SparseMultiObjectivePreprocessorResult<SparseModelType> ReturnType;
                
                /*!
                 * Preprocesses the given model w.r.t. the given formulas
                 * @param originalModel The considered model
                 * @param originalFormula the considered formula. The subformulas should only contain one OperatorFormula at top level.
                 */
                static ReturnType preprocess(SparseModelType const& originalModel, storm::logic::MultiObjectiveFormula const& originalFormula);
                
            private:
                
                struct PreprocessorData {
                    SparseModelType const& originalModel;
                    std::vector<std::shared_ptr<Objective<ValueType>>> objectives;
                    std::vector<std::shared_ptr<SparseMultiObjectivePreprocessorTask<SparseModelType>>> tasks;
                    std::shared_ptr<storm::storage::MemoryStructure> memory;
                    
                    // Indices of the objectives that require a check for finite reward
                    storm::storage::BitVector finiteRewardCheckObjectives;
                    
                    // Indices of the objectives for which we need to compute an upper bound for the result
                    storm::storage::BitVector upperResultBoundObjectives;
                    
                    std::string memoryLabelPrefix;
                    std::string rewardModelNamePrefix;
                    
                    PreprocessorData(SparseModelType const& model);
                };
                
                /*!
                 * Apply the neccessary preprocessing for the given formula.
                 * @param formula the current (sub)formula
                 * @param opInfo the information of the resulting operator formula
                 * @param data the current data. The currently processed objective is located at data.objectives.back()
                 * @param optionalRewardModelName the reward model name that is considered for the formula (if available)
                 */
                static void preprocessOperatorFormula(storm::logic::OperatorFormula const& formula, PreprocessorData& data);
                static void preprocessProbabilityOperatorFormula(storm::logic::ProbabilityOperatorFormula const& formula, storm::logic::OperatorInformation const& opInfo, PreprocessorData& data);
                static void preprocessRewardOperatorFormula(storm::logic::RewardOperatorFormula const& formula, storm::logic::OperatorInformation const& opInfo, PreprocessorData& data);
                static void preprocessTimeOperatorFormula(storm::logic::TimeOperatorFormula const& formula, storm::logic::OperatorInformation const& opInfo, PreprocessorData& data);
                static void preprocessUntilFormula(storm::logic::UntilFormula const& formula, storm::logic::OperatorInformation const& opInfo, PreprocessorData& data, std::shared_ptr<storm::logic::Formula const> subformula = nullptr);
                static void preprocessBoundedUntilFormula(storm::logic::BoundedUntilFormula const& formula, storm::logic::OperatorInformation const& opInfo, PreprocessorData& data);
                static void preprocessGloballyFormula(storm::logic::GloballyFormula const& formula, storm::logic::OperatorInformation const& opInfo, PreprocessorData& data);
                static void preprocessEventuallyFormula(storm::logic::EventuallyFormula const& formula, storm::logic::OperatorInformation const& opInfo, PreprocessorData& data, boost::optional<std::string> const& optionalRewardModelName = boost::none);
                static void preprocessCumulativeRewardFormula(storm::logic::CumulativeRewardFormula const& formula, storm::logic::OperatorInformation const& opInfo, PreprocessorData& data, boost::optional<std::string> const& optionalRewardModelName = boost::none);
                static void preprocessTotalRewardFormula(storm::logic::OperatorInformation const& opInfo, PreprocessorData& data, boost::optional<std::string> const& optionalRewardModelName = boost::none); // The total reward formula itself does not need to be provided as it is unique.
                
                
                /*!
                 * Builds the result from preprocessing
                 */
                static ReturnType buildResult(SparseModelType const& originalModel, storm::logic::MultiObjectiveFormula const& originalFormula, PreprocessorData& data, std::shared_ptr<SparseModelType> const& preprocessedModel, storm::storage::SparseMatrix<ValueType> const& backwardTransitions);
                
                /*!
                 * Returns the query type
                 */
                 static typename ReturnType::QueryType getQueryType(std::vector<Objective<ValueType>> const& objectives);
                
                
                /*!
                 * Computes the set of states that have zero expected reward w.r.t. all expected reward objectives
                 */
                static void setReward0States(ReturnType& result, storm::storage::SparseMatrix<ValueType> const& backwardTransitions);

                
                /*!
                 * Checks whether the occurring expected rewards are finite and sets the RewardFinitenessType accordingly
                 * Returns the set of states for which a scheduler exists that induces finite reward for all objectives
                 */
                static void checkRewardFiniteness(ReturnType& result, storm::storage::BitVector const& finiteRewardCheckObjectives, storm::storage::SparseMatrix<ValueType> const& backwardTransitions);
                
                /*!
                 * Finds an upper bound for the expected reward of the objective with the given index (assuming it considers an expected reward objective)
                 */
                static boost::optional<ValueType> computeUpperResultBound(ReturnType const& result, uint64_t objIndex, storm::storage::SparseMatrix<ValueType> const& backwardTransitions);

                
            };
        }
    }
}

