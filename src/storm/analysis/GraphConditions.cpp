
#include "storm/models/sparse/MarkovAutomaton.h"
#include "storm/models/sparse/Ctmc.h"
#include "GraphConditions.h"
#include "storm/utility/constants.h"
#include "storm/exceptions/NotImplementedException.h"
#include "storm/models/sparse/StandardRewardModel.h"

namespace storm {
    namespace analysis {


        template <typename ValueType>
        ConstraintCollector<ValueType>::ConstraintCollector(storm::models::sparse::Model<ValueType> const& model) {
            process(model);
        }

        template <typename ValueType>
        std::unordered_set<typename ConstraintType<ValueType>::val> const& ConstraintCollector<ValueType>::getWellformedConstraints() const {
            return this->wellformedConstraintSet;
        }

        template <typename ValueType>
        std::unordered_set<typename ConstraintType<ValueType>::val> const& ConstraintCollector<ValueType>::getGraphPreservingConstraints() const {
            return this->graphPreservingConstraintSet;
        }

        template <typename ValueType>
        std::set<storm::RationalFunctionVariable> const& ConstraintCollector<ValueType>::getVariables() const {
            return this->variableSet;
        }


        template <typename ValueType>
        void ConstraintCollector<ValueType>::wellformedRequiresNonNegativeEntries(std::vector<ValueType> const& vec) {
            for(auto const& entry : vec) {
                if (!storm::utility::isConstant(entry)) {
                    auto const& transitionVars = entry.gatherVariables();
                    variableSet.insert(transitionVars.begin(), transitionVars.end());
                    if (entry.denominator().isConstant()) {
                        if (entry.denominatorAsNumber() > 0) {
                            wellformedConstraintSet.emplace(entry.nominator().polynomialWithCoefficient(), storm::CompareRelation::GEQ);
                        } else if (entry.denominatorAsNumber() < 0) {
                            wellformedConstraintSet.emplace(entry.nominator().polynomialWithCoefficient(), storm::CompareRelation::LEQ);
                        } else {
                            assert(false); // Should fail before.
                        }
                    } else {
                        wellformedConstraintSet.emplace(entry.denominator().polynomialWithCoefficient(), storm::CompareRelation::NEQ);
                        STORM_LOG_THROW(false, storm::exceptions::NotImplementedException, "Rational functions at state rewards not yet supported.");
                    }
                }
            }
        }

        template <typename ValueType>
        void ConstraintCollector<ValueType>::process(storm::models::sparse::Model<ValueType> const& model) {
            for(uint_fast64_t action = 0; action < model.getTransitionMatrix().getRowCount(); ++action) {
                ValueType sum = storm::utility::zero<ValueType>();

                for (auto transitionIt = model.getTransitionMatrix().begin(action); transitionIt != model.getTransitionMatrix().end(action); ++transitionIt) {
                    auto const& transition = *transitionIt;
                    sum += transition.getValue();
                    if (!storm::utility::isConstant(transition.getValue())) {
                        auto const& transitionVars = transition.getValue().gatherVariables();
                        variableSet.insert(transitionVars.begin(), transitionVars.end());
                        // Assert: 0 <= transition <= 1
                        if (transition.getValue().denominator().isConstant()) {
                            assert(transition.getValue().denominator().constantPart() != 0);
                            if (transition.getValue().denominator().constantPart() > 0) {
                                // Assert: nom <= denom
                                wellformedConstraintSet.emplace((transition.getValue().nominator() - transition.getValue().denominator()).polynomialWithCoefficient(), storm::CompareRelation::LEQ);
                                // Assert: nom >= 0
                                wellformedConstraintSet.emplace(transition.getValue().nominator().polynomialWithCoefficient(), storm::CompareRelation::GEQ);
                            } else if (transition.getValue().denominator().constantPart() < 0) {
                                // Assert nom >= denom
                                wellformedConstraintSet.emplace((transition.getValue().nominator() - transition.getValue().denominator()).polynomialWithCoefficient(), storm::CompareRelation::GEQ);
                                // Assert: nom <= 0
                                wellformedConstraintSet.emplace(transition.getValue().nominator().polynomialWithCoefficient(), storm::CompareRelation::LEQ);
                            } else {
                                STORM_LOG_ASSERT(false, "Denominator must not equal 0.");
                            }
                        } else {
                            // Assert: denom != 0
                            wellformedConstraintSet.emplace(transition.getValue().denominator().polynomialWithCoefficient(), storm::CompareRelation::NEQ);
                             // Assert: transition >= 0 <==> if denom > 0 then nom >= 0 else nom <= 0
                            wellformedConstraintSet.emplace(carl::FormulaType::ITE, typename ConstraintType<ValueType>::val(transition.getValue().denominator().polynomialWithCoefficient(), storm::CompareRelation::GREATER), typename ConstraintType<ValueType>::val(transition.getValue().nominator().polynomialWithCoefficient(), storm::CompareRelation::GEQ), typename ConstraintType<ValueType>::val(transition.getValue().nominator().polynomialWithCoefficient(), storm::CompareRelation::LEQ));
                             // TODO: Assert: transition <= 1 <==> if denom > 0 then nom - denom <= 0 else nom - denom >= 0
                        }
                        // Assert: transition > 0
                        graphPreservingConstraintSet.emplace(transition.getValue().nominator().polynomialWithCoefficient(), storm::CompareRelation::NEQ);
                    }
                }
                STORM_LOG_ASSERT(!storm::utility::isConstant(sum) || storm::utility::isOne(sum), "If the sum is a constant, it must be equal to 1.");
                if(!storm::utility::isConstant(sum)) {
                    // Assert: sum == 1
                    wellformedConstraintSet.emplace((sum.nominator() - sum.denominator()).polynomialWithCoefficient(), storm::CompareRelation::EQ);
                }
            }

            if (model.getType() == storm::models::ModelType::Ctmc) {
                auto const& exitRateVector = static_cast<storm::models::sparse::Ctmc<ValueType> const&>(model).getExitRateVector();
                wellformedRequiresNonNegativeEntries(exitRateVector);
            } else if (model.getType() == storm::models::ModelType::MarkovAutomaton) {
                auto const& exitRateVector = static_cast<storm::models::sparse::MarkovAutomaton<ValueType> const&>(model).getExitRates();
                wellformedRequiresNonNegativeEntries(exitRateVector);
            }

            for(auto const& rewModelEntry : model.getRewardModels()) {
                if (rewModelEntry.second.hasStateRewards()) {
                    wellformedRequiresNonNegativeEntries(rewModelEntry.second.getStateRewardVector());
                }
                if (rewModelEntry.second.hasStateActionRewards()) {
                    wellformedRequiresNonNegativeEntries(rewModelEntry.second.getStateActionRewardVector());
                }
                if (rewModelEntry.second.hasTransitionRewards()) {
                    for (auto const& entry : rewModelEntry.second.getTransitionRewardMatrix()) {
                        if(!entry.getValue().isConstant()) {
                            if (entry.getValue().denominator().isConstant()) {
                                if (entry.getValue().denominatorAsNumber() > 0) {
                                    wellformedConstraintSet.emplace(entry.getValue().nominator().polynomialWithCoefficient(), storm::CompareRelation::GEQ);
                                } else if (entry.getValue().denominatorAsNumber() < 0) {
                                    wellformedConstraintSet.emplace(entry.getValue().nominator().polynomialWithCoefficient(), storm::CompareRelation::LEQ);
                                } else {
                                    assert(false); // Should fail before.
                                }
                            } else {
                                wellformedConstraintSet.emplace(entry.getValue().denominator().polynomialWithCoefficient(), storm::CompareRelation::NEQ);
                                STORM_LOG_THROW(false, storm::exceptions::NotImplementedException, "Rational functions at transition rewards are not yet supported.");
                            }
                        }
                    }
                }
            }

        }

        template <typename ValueType>
        void ConstraintCollector<ValueType>::operator()(storm::models::sparse::Model<ValueType> const& model) {
            process(model);
        }

        template class ConstraintCollector<storm::RationalFunction>;
    }
}
