#include "JSONExporter.h"

#include <iostream>
#include <fstream>
#include <vector>

#include "storm/utility/macros.h"
#include "storm/utility/file.h"
#include "storm/exceptions/FileIoException.h"
#include "storm/exceptions/NotSupportedException.h"

#include "storm/exceptions/InvalidJaniException.h"
#include "storm/exceptions/NotImplementedException.h"
#include "storm/exceptions/InvalidPropertyException.h"

#include "storm/storage/expressions/RationalLiteralExpression.h"
#include "storm/storage/expressions/IntegerLiteralExpression.h"
#include "storm/storage/expressions/BooleanLiteralExpression.h"
#include "storm/storage/expressions/UnaryBooleanFunctionExpression.h"
#include "storm/storage/expressions/UnaryNumericalFunctionExpression.h"
#include "storm/storage/expressions/BinaryBooleanFunctionExpression.h"
#include "storm/storage/expressions/BinaryNumericalFunctionExpression.h"
#include "storm/storage/expressions/IfThenElseExpression.h"
#include "storm/storage/expressions/BinaryRelationExpression.h"
#include "storm/storage/expressions/VariableExpression.h"
#include "storm/storage/expressions/ExpressionManager.h"

#include "storm/logic/Formulas.h"

#include "storm/storage/jani/AutomatonComposition.h"
#include "storm/storage/jani/ParallelComposition.h"
#include "storm/storage/jani/Property.h"
#include "storm/storage/jani/traverser/RewardModelInformation.h"
#include "storm/storage/jani/expressions/JaniReduceNestingExpressionVisitor.h"
#include "storm/storage/jani/FunctionEliminator.h"
#include "storm/storage/jani/expressions/JaniExpressionSubstitutionVisitor.h"

namespace storm {
    namespace jani {

        modernjson::json anyToJson(boost::any&& input) {
            boost::any tmp(std::move(input));
            modernjson::json res = std::move(*boost::any_cast<modernjson::json>(&tmp));
            return res;
        }
        
        modernjson::json buildExpression(storm::expressions::Expression const& exp,  std::vector<storm::jani::Constant> const& constants, VariableSet const& globalVariables = VariableSet(), VariableSet const& localVariables = VariableSet(), std::unordered_set<std::string> const& auxiliaryVariables = {}) {
            STORM_LOG_TRACE("Exporting " << exp);
            return ExpressionToJson::translate(exp,  constants, globalVariables, localVariables, auxiliaryVariables);
        }

        class CompositionJsonExporter : public CompositionVisitor {
        public:
            CompositionJsonExporter(bool allowRecursion) : allowRecursion(allowRecursion){
                
            }
            
            static modernjson::json translate(storm::jani::Composition const& comp, bool allowRecursion = true) {
                CompositionJsonExporter visitor(allowRecursion);
                return anyToJson(comp.accept(visitor, boost::none));
            }
            
            virtual boost::any visit(AutomatonComposition const& composition, boost::any const&) {
                modernjson::json compDecl;
                modernjson::json autDecl;
                autDecl["automaton"] = composition.getAutomatonName();
                std::vector<modernjson::json> elements;
                elements.push_back(autDecl);
                compDecl["elements"] = elements;
                return compDecl;
            }
            
            virtual boost::any visit(ParallelComposition const& composition, boost::any const& data) {
                modernjson::json compDecl;
                
                std::vector<modernjson::json> elems;
                for (auto const& subcomp : composition.getSubcompositions()) {
                    modernjson::json elemDecl;
                    if (subcomp->isAutomatonComposition()) {
                        elemDecl["automaton"] = std::static_pointer_cast<AutomatonComposition>(subcomp)->getAutomatonName();
                    } else {
                        STORM_LOG_THROW(allowRecursion, storm::exceptions::InvalidJaniException, "Nesting composition " << *subcomp << " is not supported by JANI.");
                        elemDecl = anyToJson(subcomp->accept(*this, data));
                    }
                    elems.push_back(elemDecl);
                }
                compDecl["elements"] = elems;
                std::vector<modernjson::json> synElems;
                for (auto const& syncs : composition.getSynchronizationVectors()) {
                    modernjson::json syncDecl;
                    syncDecl["synchronise"] = std::vector<std::string>();
                    for (auto const& syncIn : syncs.getInput()) {
                        if (syncIn == SynchronizationVector::NO_ACTION_INPUT) {
                            syncDecl["synchronise"].push_back(nullptr);
                        } else {
                            syncDecl["synchronise"].push_back(syncIn);
                        }
                    }
                    syncDecl["result"] = syncs.getOutput();
                    synElems.push_back(syncDecl);
                }
                if (!synElems.empty()) {
                    compDecl["syncs"] = synElems;
                }
                
                return compDecl;
            }
            
        private:
            bool allowRecursion;
        };
        
        std::string comparisonTypeToJani(storm::logic::ComparisonType ct) {
            switch(ct) {
                case storm::logic::ComparisonType::Less:
                    return "<";
                case storm::logic::ComparisonType::LessEqual:
                    return "≤";
                case storm::logic::ComparisonType::Greater:
                    return ">";
                case storm::logic::ComparisonType::GreaterEqual:
                    return "≥";
                default:
                    assert(false);
            }
        }
        
        modernjson::json numberToJson(storm::RationalNumber rn) {
            modernjson::json numDecl;
            numDecl = storm::utility::convertNumber<double>(rn);
//            if(carl::isOne(carl::getDenom(rn))) {
//                numDecl = modernjson::json(carl::toString(carl::getNum(rn)));
//            } else {
//                numDecl["op"] = "/";
//                // TODO set json lib to work with arbitrary precision ints.
//                assert(carl::toInt<int64_t>(carl::getNum(rn)) == carl::getNum(rn));
//                assert(carl::toInt<int64_t>(carl::getDenom(rn)) == carl::getDenom(rn));
//                numDecl["left"] = carl::toInt<int64_t>(carl::getNum(rn));
//                numDecl["right"] = carl::toInt<int64_t>(carl::getDenom(rn));
//            }
            return numDecl;
        }
        
        
        modernjson::json FormulaToJaniJson::constructPropertyInterval(boost::optional<storm::expressions::Expression> const& lower, boost::optional<bool> const& lowerExclusive, boost::optional<storm::expressions::Expression> const& upper, boost::optional<bool> const& upperExclusive) const {
            STORM_LOG_THROW((lower.is_initialized() || upper.is_initialized()), storm::exceptions::InvalidJaniException, "PropertyInterval needs either a lower or an upper bound, but none was given.");
            STORM_LOG_THROW((lower.is_initialized() || !lowerExclusive.is_initialized()), storm::exceptions::InvalidJaniException, "PropertyInterval defines wether the lower bound is exclusive but no lower bound is given.");
            STORM_LOG_THROW((upper.is_initialized() || !upperExclusive.is_initialized()), storm::exceptions::InvalidJaniException, "PropertyInterval defines wether the upper bound is exclusive but no upper bound is given.");
            
            modernjson::json iDecl;
            if (lower) {
                iDecl["lower"] = buildExpression(*lower, model.getConstants(), model.getGlobalVariables());
                if (lowerExclusive) {
                    iDecl["lower-exclusive"] = *lowerExclusive;
                }
            }
            if (upper) {
                iDecl["upper"] = buildExpression(*upper, model.getConstants(), model.getGlobalVariables());
                if (upperExclusive) {
                    iDecl["upper-exclusive"] = *upperExclusive;
                }
            }
            return iDecl;
        }
        
        modernjson::json FormulaToJaniJson::constructRewardAccumulation(storm::logic::RewardAccumulation const& rewardAccumulation, std::string const& rewardModelName) const {
            storm::jani::RewardModelInformation info(model, rewardModelName);
            
            bool steps = rewardAccumulation.isStepsSet() && (info.hasActionRewards() || info.hasTransitionRewards());
            bool time = rewardAccumulation.isTimeSet() && !model.isDiscreteTimeModel() && info.hasStateRewards();
            bool exit = rewardAccumulation.isExitSet() && info.hasStateRewards();
            
            return constructRewardAccumulation(storm::logic::RewardAccumulation(steps, time, exit));
        }
        
        modernjson::json FormulaToJaniJson::constructRewardAccumulation(storm::logic::RewardAccumulation const& rewardAccumulation) const {
            std::vector<std::string> res;
            if (rewardAccumulation.isStepsSet()) {
                res.push_back("steps");
            }
            if (rewardAccumulation.isTimeSet()) {
                res.push_back("time");
            }
            if (rewardAccumulation.isExitSet()) {
                stateExitRewards = true;
                res.push_back("exit");
            }
            return res;
        }

        modernjson::json FormulaToJaniJson::constructStandardRewardAccumulation(std::string const& rewardModelName) const {
            if (model.isDiscreteTimeModel()) {
                return constructRewardAccumulation(storm::logic::RewardAccumulation(true, false, true), rewardModelName);
            } else {
                return constructRewardAccumulation(storm::logic::RewardAccumulation(true, true, false), rewardModelName);
            }
        }
        
        modernjson::json FormulaToJaniJson::translate(storm::logic::Formula const& formula, storm::jani::Model const& model, storm::jani::ModelFeatures& modelFeatures) {
            FormulaToJaniJson translator(model);
            auto result = anyToJson(formula.accept(translator));
            if (translator.containsStateExitRewards()) {
                modelFeatures.add(storm::jani::ModelFeature::StateExitRewards);
            }
            return result;
        }
        
        bool FormulaToJaniJson::containsStateExitRewards() const {
            return stateExitRewards;
        }
        
        boost::any FormulaToJaniJson::visit(storm::logic::AtomicExpressionFormula const& f, boost::any const&) const {
            return buildExpression(f.getExpression(), model.getConstants(), model.getGlobalVariables());
        }
        
        boost::any FormulaToJaniJson::visit(storm::logic::AtomicLabelFormula const& f, boost::any const&) const {
            modernjson::json opDecl(f.getLabel());
            return opDecl;
        }
        boost::any FormulaToJaniJson::visit(storm::logic::BinaryBooleanStateFormula const& f, boost::any const& data) const{
            modernjson::json opDecl;
            storm::logic::BinaryBooleanStateFormula::OperatorType op = f.getOperator();
            opDecl["op"] = op == storm::logic::BinaryBooleanStateFormula::OperatorType::And ? "∧" : "∨";
            opDecl["left"] = anyToJson(f.getLeftSubformula().accept(*this, data));
            opDecl["right"] = anyToJson(f.getRightSubformula().accept(*this, data));
            return opDecl;
        }
        boost::any FormulaToJaniJson::visit(storm::logic::BooleanLiteralFormula const& f, boost::any const&) const {
            modernjson::json opDecl(f.isTrueFormula() ? true : false);
            return opDecl;
        }
        boost::any FormulaToJaniJson::visit(storm::logic::BoundedUntilFormula const& f, boost::any const& data) const {
            STORM_LOG_THROW(!f.hasMultiDimensionalSubformulas(), storm::exceptions::NotSupportedException, "Jani export of multi-dimensional bounded until formulas is not supported.");
            modernjson::json opDecl;
            opDecl["op"] = "U";
            opDecl["left"] = anyToJson(f.getLeftSubformula().accept(*this, data));
            opDecl["right"] = anyToJson(f.getRightSubformula().accept(*this, data));
            
            bool hasStepBounds(false), hasTimeBounds(false);
            std::vector<modernjson::json> rewardBounds;
            
            for (uint64_t i = 0; i < f.getDimension(); ++i) {
                boost::optional<storm::expressions::Expression> lower, upper;
                boost::optional<bool> lowerExclusive, upperExclusive;
                if (f.hasLowerBound(i)) {
                    lower = f.getLowerBound(i);
                    lowerExclusive = f.isLowerBoundStrict(i);
                }
                if (f.hasUpperBound(i)) {
                    upper = f.getUpperBound(i);
                    upperExclusive = f.isUpperBoundStrict(i);
                }
                modernjson::json propertyInterval = constructPropertyInterval(lower, lowerExclusive, upper, upperExclusive);
    
                auto tbr = f.getTimeBoundReference(i);
                if (tbr.isStepBound() || (model.isDiscreteTimeModel() && tbr.isTimeBound())) {
                    STORM_LOG_THROW(!hasStepBounds, storm::exceptions::NotSupportedException, "Jani export of until formulas with multiple step bounds is not supported.");
                    hasStepBounds = true;
                    opDecl["step-bounds"] = propertyInterval;
                } else if(tbr.isRewardBound()) {
                    modernjson::json rewbound;
                    rewbound["exp"] = buildExpression(model.getRewardModelExpression(tbr.getRewardName()), model.getConstants(), model.getGlobalVariables());
                    if (tbr.hasRewardAccumulation()) {
                        rewbound["accumulate"] = constructRewardAccumulation(tbr.getRewardAccumulation(), tbr.getRewardName());
                    } else {
                        rewbound["accumulate"] = constructStandardRewardAccumulation(tbr.getRewardName());
                    }
                    rewbound["bounds"] = propertyInterval;
                    rewardBounds.push_back(std::move(rewbound));
                } else {
                    STORM_LOG_THROW(!hasTimeBounds, storm::exceptions::NotSupportedException, "Jani export of until formulas with multiple time bounds is not supported.");
                    hasTimeBounds = true;
                    opDecl["time-bounds"] = propertyInterval;
                }
            }
            if (!rewardBounds.empty()) {
                opDecl["reward-bounds"] = modernjson::json(rewardBounds);
            }
            return opDecl;

        }
        
        boost::any FormulaToJaniJson::visit(storm::logic::ConditionalFormula const&, boost::any const&) const {
            STORM_LOG_THROW(false, storm::exceptions::NotSupportedException, "Jani currently does not support conditional formulae");
        }
        
        boost::any FormulaToJaniJson::visit(storm::logic::CumulativeRewardFormula const&, boost::any const&) const {
            STORM_LOG_THROW(false, storm::exceptions::NotSupportedException, "Storm currently does not support translating cumulative reward formulae");
        }
        
        boost::any FormulaToJaniJson::visit(storm::logic::EventuallyFormula const& f, boost::any const& data) const {
            modernjson::json opDecl;
            opDecl["op"] = "U";
            opDecl["left"] = anyToJson(f.getTrueFormula()->accept(*this, data));
            opDecl["right"] = anyToJson(f.getSubformula().accept(*this, data));
            return opDecl;
        }
        
        boost::any FormulaToJaniJson::visit(storm::logic::TimeOperatorFormula const& f, boost::any const& data) const {
            modernjson::json opDecl;
            
            // Create standard reward accumulation for time operator formulas.
            storm::logic::RewardAccumulation rewAcc(model.isDiscreteTimeModel(), !model.isDiscreteTimeModel(), false);
            if (f.getSubformula().isEventuallyFormula() && f.getSubformula().asEventuallyFormula().hasRewardAccumulation()) {
                rewAcc = f.getSubformula().asEventuallyFormula().getRewardAccumulation();
            }
            auto rewAccJson = constructRewardAccumulation(rewAcc);

            if(f.hasBound()) {
                auto bound = f.getBound();
                opDecl["op"] = comparisonTypeToJani(bound.comparisonType);
                if(f.hasOptimalityType()) {
                    opDecl["left"]["op"] = f.getOptimalityType() == storm::solver::OptimizationDirection::Minimize ? "Emin" : "Emax";
                    if (f.getSubformula().isEventuallyFormula()) {
                        opDecl["left"]["reach"] = anyToJson(f.getSubformula().asEventuallyFormula().getSubformula().accept(*this, data));
                    } else {
                        STORM_LOG_THROW(false, storm::exceptions::NotSupportedException, "Unsupported subformula for time operator formula " << f);
                    }
                } else {
                    opDecl["left"]["op"] = (bound.comparisonType == storm::logic::ComparisonType::Less || bound.comparisonType == storm::logic::ComparisonType::LessEqual) ? "Emax" : "Emin";
                    opDecl["left"]["reach"] = anyToJson(f.getSubformula().asEventuallyFormula().getSubformula().accept(*this, data));
                }
                opDecl["left"]["exp"] = modernjson::json(1);
                opDecl["left"]["accumulate"] = rewAccJson;
                opDecl["right"] = buildExpression(bound.threshold, model.getConstants(), model.getGlobalVariables());
            } else {
                if(f.hasOptimalityType()) {
                    opDecl["op"] = f.getOptimalityType() == storm::solver::OptimizationDirection::Minimize ? "Emin" : "Emax";
                    if (f.getSubformula().isEventuallyFormula()) {
                        opDecl["reach"] = anyToJson(f.getSubformula().asEventuallyFormula().getSubformula().accept(*this, data));
                    } else {
                        STORM_LOG_THROW(false, storm::exceptions::NotSupportedException, "Unsupported subformula for time operator formula " << f);
                    }
                } else {
                    // TODO add checks
                    opDecl["op"] = "Emin";
                    opDecl["reach"] = anyToJson(f.getSubformula().asEventuallyFormula().getSubformula().accept(*this, data));
                }
                opDecl["exp"] = modernjson::json(1);
                opDecl["accumulate"] = rewAccJson;
            }
            return opDecl;
        }
        
        boost::any FormulaToJaniJson::visit(storm::logic::GloballyFormula const& f, boost::any const& data) const {
            modernjson::json opDecl;
            opDecl["op"] = "G";
            opDecl["exp"] = anyToJson(f.getSubformula().accept(*this, data));
            return opDecl;
        }
        
        boost::any FormulaToJaniJson::visit(storm::logic::InstantaneousRewardFormula const&, boost::any const&) const {
            STORM_LOG_THROW(false, storm::exceptions::NotImplementedException, "Jani currently does not support conversion of an instanteneous reward formula");
        }
        
        boost::any FormulaToJaniJson::visit(storm::logic::LongRunAverageOperatorFormula const& f, boost::any const& data) const {
            modernjson::json opDecl;
            if(f.hasBound()) {
                auto bound = f.getBound();
                opDecl["op"] = comparisonTypeToJani(bound.comparisonType);
                if(f.hasOptimalityType()) {
                    opDecl["left"]["op"] = f.getOptimalityType() == storm::solver::OptimizationDirection::Minimize ? "Smin" : "Smax";
                    opDecl["left"]["exp"] = anyToJson(f.getSubformula().accept(*this, data));
                } else {
                    opDecl["left"]["op"] = (bound.comparisonType == storm::logic::ComparisonType::Less || bound.comparisonType == storm::logic::ComparisonType::LessEqual) ? "Smax" : "Smin";
                    opDecl["left"]["exp"] = anyToJson(f.getSubformula().accept(*this, data));
                }
                opDecl["right"] = buildExpression(bound.threshold, model.getConstants(), model.getGlobalVariables());
            } else {
                if(f.hasOptimalityType()) {
                    opDecl["op"] = f.getOptimalityType() == storm::solver::OptimizationDirection::Minimize ? "Smin" : "Smax";
                    opDecl["exp"] = anyToJson(f.getSubformula().accept(*this, data));
                    
                } else {
                    // TODO add checks
                    opDecl["op"] = "Smin";
                    opDecl["exp"] = anyToJson(f.getSubformula().accept(*this, data));
                }
            }
            return opDecl;
        }
        
        boost::any FormulaToJaniJson::visit(storm::logic::LongRunAverageRewardFormula const&, boost::any const&) const {
//            modernjson::json opDecl;
//            if(f.()) {
//                auto bound = f.getBound();
//                opDecl["op"] = comparisonTypeToJani(bound.comparisonType);
//                if(f.hasOptimalityType()) {
//                    opDecl["left"]["op"] = f.getOptimalityType() == storm::solver::OptimizationDirection::Minimize ? "Smin" : "Smax";
//                    opDecl["left"]["exp"] = anyToJson(f.getSubformula().accept(*this, boost::none));
//                } else {
//                    opDecl["left"]["op"] = (bound.comparisonType == storm::logic::ComparisonType::Less || bound.comparisonType == storm::logic::ComparisonType::LessEqual) ? "Smax" : "Smin";
//                    opDecl["left"]["exp"] = anyToJson(f.getSubformula().accept(*this, boost::none));
//                }
//                opDecl["right"] = ExpressionToJson::translate(bound.threshold);
//            } else {
//                if(f.hasOptimalityType()) {
//                    opDecl["op"] = f.getOptimalityType() == storm::solver::OptimizationDirection::Minimize ? "Smin" : "Smax";
//                    opDecl["exp"] = anyToJson(f.getSubformula().accept(*this, boost::none));
//                    
//                } else {
//                    // TODO add checks
//                    opDecl["op"] = "Pmin";
//                    opDecl["exp"] = anyToJson(f.getSubformula().accept(*this, boost::none));
//                }
//            }
//            return opDecl;
            
            STORM_LOG_THROW(false, storm::exceptions::NotImplementedException, "Jani currently does not support conversion of an LRA reward formula");
        }
        
        
        boost::any FormulaToJaniJson::visit(storm::logic::MultiObjectiveFormula const&, boost::any const&) const {
            STORM_LOG_THROW(false, storm::exceptions::NotImplementedException, "Jani currently does not support conversion of a multi-objective formula");
        }
        
        boost::any FormulaToJaniJson::visit(storm::logic::QuantileFormula const&, boost::any const&) const {
            STORM_LOG_THROW(false, storm::exceptions::NotImplementedException, "Jani currently does not support conversion of a Quantile formula");
        }
        
        boost::any FormulaToJaniJson::visit(storm::logic::NextFormula const& f, boost::any const& data) const {
            modernjson::json opDecl;
            opDecl["op"] = "U";
            opDecl["left"] = anyToJson(f.getTrueFormula()->accept(*this, data));
            opDecl["right"] = anyToJson(f.getSubformula().accept(*this, data));
            auto intervalExpressionManager = std::make_shared<storm::expressions::ExpressionManager>();
            opDecl["step-bounds"] = constructPropertyInterval(intervalExpressionManager->integer(1), false, intervalExpressionManager->integer(1), false);
            return opDecl;
        }
        
      
        
        
        boost::any FormulaToJaniJson::visit(storm::logic::ProbabilityOperatorFormula const& f, boost::any const& data) const {
            modernjson::json opDecl;
            
            if(f.hasBound()) {
                auto bound = f.getBound();
                opDecl["op"] = comparisonTypeToJani(bound.comparisonType);
                if(f.hasOptimalityType()) {
                    opDecl["left"]["op"] = f.getOptimalityType() == storm::solver::OptimizationDirection::Minimize ? "Pmin" : "Pmax";
                    opDecl["left"]["exp"] = anyToJson(f.getSubformula().accept(*this, data));
                } else {
                    opDecl["left"]["op"] = (bound.comparisonType == storm::logic::ComparisonType::Less || bound.comparisonType == storm::logic::ComparisonType::LessEqual) ? "Pmax" : "Pmin";
                    opDecl["left"]["exp"] = anyToJson(f.getSubformula().accept(*this, data));
                }
                opDecl["right"] = buildExpression(bound.threshold, model.getConstants(), model.getGlobalVariables());
            } else {
                if(f.hasOptimalityType()) {
                    opDecl["op"] = f.getOptimalityType() == storm::solver::OptimizationDirection::Minimize ? "Pmin" : "Pmax";
                    opDecl["exp"] = anyToJson(f.getSubformula().accept(*this, data));
                    
                } else {
                    // TODO add checks
                    opDecl["op"] = "Pmin";
                    opDecl["exp"] = anyToJson(f.getSubformula().accept(*this, data));
                }
            }
            return opDecl;
        }
        
        boost::any FormulaToJaniJson::visit(storm::logic::RewardOperatorFormula const& f, boost::any const& data) const {
            modernjson::json opDecl;
            
            std::string instantName;
            if (model.isDiscreteTimeModel()) {
                instantName = "step-instant";
            } else {
                instantName = "time-instant";
            }

            std::string rewardModelName;
            if (f.hasRewardModelName()) {
                rewardModelName = f.getRewardModelName();
            } else {
                if (model.getGlobalVariables().getNumberOfRealTransientVariables() == 1) {
                    for (auto const& variable : model.getGlobalVariables().getRealVariables()) {
                        if (variable.isTransient()) {
                            rewardModelName = variable.getName();
                            STORM_LOG_WARN("Reward model name was not given, assuming the only global real transient variable '" << rewardModelName << "' to measure the reward.");
                            break;
                        }
                    }
                }
            }
            STORM_LOG_THROW(!rewardModelName.empty(), storm::exceptions::NotSupportedException, "Reward name has to be specified for Jani-conversion");

            std::string opString = "";
            if (f.getSubformula().isLongRunAverageRewardFormula()) {
                opString = "S";
            } else {
                opString = "E";
            }
            if (f.hasOptimalityType()) {
                opString += storm::solver::minimize(f.getOptimalityType()) ? "min" : "max";
            } else if (f.hasBound()) {
                opString += storm::logic::isLowerBound(f.getBound().comparisonType) ? "min" : "max";
            } else {
                opString += "min";
            }

            opDecl["op"] = opString;

            if (f.getSubformula().isEventuallyFormula()) {
                opDecl["reach"] = anyToJson(f.getSubformula().asEventuallyFormula().getSubformula().accept(*this, data));
                if (f.getSubformula().asEventuallyFormula().hasRewardAccumulation()) {
                    opDecl["accumulate"] = constructRewardAccumulation(f.getSubformula().asEventuallyFormula().getRewardAccumulation(), rewardModelName);
                } else {
                    opDecl["accumulate"] = constructStandardRewardAccumulation(rewardModelName);
                }
            } else if (f.getSubformula().isCumulativeRewardFormula()) {
                // TODO: support for reward bounded formulas
                STORM_LOG_WARN_COND(!f.getSubformula().asCumulativeRewardFormula().getTimeBoundReference().isRewardBound(), "Export for cumulative reward formulas with reward instant currently unsupported.");
                opDecl[instantName] = buildExpression(f.getSubformula().asCumulativeRewardFormula().getBound(), model.getConstants(), model.getGlobalVariables());
                if (f.getSubformula().asCumulativeRewardFormula().hasRewardAccumulation()) {
                        opDecl["accumulate"] = constructRewardAccumulation(f.getSubformula().asCumulativeRewardFormula().getRewardAccumulation(), rewardModelName);
                } else {
                    opDecl["accumulate"] = constructStandardRewardAccumulation(rewardModelName);
                }
            } else if (f.getSubformula().isInstantaneousRewardFormula()) {
                opDecl[instantName] = buildExpression(f.getSubformula().asInstantaneousRewardFormula().getBound(), model.getConstants(), model.getGlobalVariables());
            } else if (f.getSubformula().isLongRunAverageRewardFormula()) {
                if (f.getSubformula().asLongRunAverageRewardFormula().hasRewardAccumulation()) {
                    opDecl["accumulate"] = constructRewardAccumulation(f.getSubformula().asLongRunAverageRewardFormula().getRewardAccumulation(), rewardModelName);
                } else {
                    opDecl["accumulate"] = constructStandardRewardAccumulation(rewardModelName);
                }
            }
            opDecl["exp"] = buildExpression(model.getRewardModelExpression(rewardModelName), model.getConstants(), model.getGlobalVariables());
            
            if(f.hasBound()) {
                modernjson::json compDecl;
                auto bound = f.getBound();
                compDecl["op"] = comparisonTypeToJani(bound.comparisonType);
                compDecl["left"] = std::move(opDecl);
                compDecl["right"] = buildExpression(bound.threshold, model.getConstants(), model.getGlobalVariables());
                return compDecl;
            } else {
                return opDecl;
            }
        }
        
        boost::any FormulaToJaniJson::visit(storm::logic::TotalRewardFormula const&, boost::any const&) const {
            STORM_LOG_THROW(false, storm::exceptions::NotImplementedException, "Jani currently does not support a total reward formula");
        }
        
        boost::any FormulaToJaniJson::visit(storm::logic::UnaryBooleanStateFormula const& f, boost::any const& data) const {
            modernjson::json opDecl;
            storm::logic::UnaryBooleanStateFormula::OperatorType op = f.getOperator();
            assert(op == storm::logic::UnaryBooleanStateFormula::OperatorType::Not);
            opDecl["op"] = "¬";
            opDecl["exp"] = anyToJson(f.getSubformula().accept(*this, data));
            return opDecl;
        }
        
        boost::any FormulaToJaniJson::visit(storm::logic::UntilFormula const& f, boost::any const& data) const {
            modernjson::json opDecl;
            opDecl["op"] = "U";
            opDecl["left"] = anyToJson(f.getLeftSubformula().accept(*this, data));
            opDecl["right"] = anyToJson(f.getRightSubformula().accept(*this, data));
            return opDecl;
        }
        
        std::string operatorTypeToJaniString(storm::expressions::OperatorType optype) {
            
            using OpType = storm::expressions::OperatorType;
            switch(optype) {
                case OpType::And:
                    return "∧";
                case OpType::Or:
                    return "∨";
                case OpType::Xor:
                    return "≠";
                case OpType::Implies:
                    return "⇒";
                case OpType::Iff:
                    return "=";
                case OpType::Plus:
                    return "+";
                case OpType::Minus:
                    return "-";
                case OpType::Times:
                    return "*";
                case OpType::Divide:
                    return "/";
                case OpType::Min:
                    return "min";
                case OpType::Max:
                    return "max";
                case OpType::Power:
                    return "pow";
                case OpType::Modulo:
                    return "%";
                case OpType::Equal:
                    return "=";
                case OpType::NotEqual:
                    return "≠";
                case OpType::Less:
                    return "<";
                case OpType::LessOrEqual:
                    return "≤";
                case OpType::Greater:
                    return ">";
                case OpType::GreaterOrEqual:
                    return "≥";
                case OpType::Not:
                    return "¬";
                case OpType::Floor:
                    return "floor";
                case OpType::Ceil:
                    return "ceil";
                case OpType::Ite:
                    return "ite";
                default:
                    STORM_LOG_THROW(false, storm::exceptions::InvalidJaniException, "Operator not supported by Jani");
            }
        }
        
        modernjson::json ExpressionToJson::translate(storm::expressions::Expression const& expr, std::vector<storm::jani::Constant> const& constants, VariableSet const& globalVariables, VariableSet const& localVariables, std::unordered_set<std::string> const& auxiliaryVariables) {
            
            // Simplify the expression first and reduce the nesting
            auto simplifiedExpr = storm::jani::reduceNestingInJaniExpression(expr.simplify());
            
            ExpressionToJson visitor(constants, globalVariables, localVariables, auxiliaryVariables);
            return anyToJson(simplifiedExpr.accept(visitor, boost::none));
        }
        
        boost::any ExpressionToJson::visit(storm::expressions::IfThenElseExpression const& expression, boost::any const& data) {
            modernjson::json opDecl;
            opDecl["op"] = "ite";
            opDecl["if"] = anyToJson(expression.getCondition()->accept(*this, data));
            opDecl["then"] = anyToJson(expression.getThenExpression()->accept(*this, data));
            opDecl["else"] = anyToJson(expression.getElseExpression()->accept(*this, data));
            return opDecl;
        }
        boost::any ExpressionToJson::visit(storm::expressions::BinaryBooleanFunctionExpression const& expression, boost::any const& data) {
            modernjson::json opDecl;
            opDecl["op"] = operatorTypeToJaniString(expression.getOperator());
            opDecl["left"] = anyToJson(expression.getOperand(0)->accept(*this, data));
            opDecl["right"] = anyToJson(expression.getOperand(1)->accept(*this, data));
            return opDecl;
        }
        boost::any ExpressionToJson::visit(storm::expressions::BinaryNumericalFunctionExpression const& expression, boost::any const& data) {
            modernjson::json opDecl;
            opDecl["op"] = operatorTypeToJaniString(expression.getOperator());
            opDecl["left"] = anyToJson(expression.getOperand(0)->accept(*this, data));
            opDecl["right"] = anyToJson(expression.getOperand(1)->accept(*this, data));
            return opDecl;
        }
        boost::any ExpressionToJson::visit(storm::expressions::BinaryRelationExpression const& expression, boost::any const& data) {
            modernjson::json opDecl;
            opDecl["op"] = operatorTypeToJaniString(expression.getOperator());
            opDecl["left"] = anyToJson(expression.getOperand(0)->accept(*this, data));
            opDecl["right"] = anyToJson(expression.getOperand(1)->accept(*this, data));
            return opDecl;
        }
        boost::any ExpressionToJson::visit(storm::expressions::VariableExpression const& expression, boost::any const&) {
            if (auxiliaryVariables.count(expression.getVariableName())) {
                return modernjson::json(expression.getVariableName());
            } else if (globalVariables.hasVariable(expression.getVariable())) {
                return modernjson::json(globalVariables.getVariable(expression.getVariable()).getName());
            } else if (localVariables.hasVariable(expression.getVariable())) {
                return modernjson::json(localVariables.getVariable(expression.getVariable()).getName());
            } else {
                for (auto const& constant : constants) {
                    if (constant.getExpressionVariable() == expression.getVariable()) {
                        return modernjson::json(constant.getName());
                    }
                }
            }
            STORM_LOG_THROW(false, storm::exceptions::InvalidJaniException, "Expression variable '" << expression.getVariableName() << "' not known in Jani data structures.");
            return modernjson::json(); // should not reach this point.
        }
        boost::any ExpressionToJson::visit(storm::expressions::UnaryBooleanFunctionExpression const& expression, boost::any const& data) {
            modernjson::json opDecl;
            opDecl["op"] = operatorTypeToJaniString(expression.getOperator());
            opDecl["exp"] = anyToJson(expression.getOperand()->accept(*this, data));
            return opDecl;
        }
        boost::any ExpressionToJson::visit(storm::expressions::UnaryNumericalFunctionExpression const& expression, boost::any const& data) {
            modernjson::json opDecl;
            opDecl["op"] = operatorTypeToJaniString(expression.getOperator());
            opDecl["exp"] = anyToJson(expression.getOperand()->accept(*this, data));
            return opDecl;
        }
        boost::any ExpressionToJson::visit(storm::expressions::BooleanLiteralExpression const& expression, boost::any const&) {
            return modernjson::json(expression.getValue());
        }
        boost::any ExpressionToJson::visit(storm::expressions::IntegerLiteralExpression const& expression, boost::any const&) {
            return modernjson::json(expression.getValue());
        }
        boost::any ExpressionToJson::visit(storm::expressions::RationalLiteralExpression const& expression, boost::any const&) {
            return modernjson::json(expression.getValueAsDouble());
        }
        
        boost::any ExpressionToJson::visit(storm::expressions::ValueArrayExpression const& expression, boost::any const& data) {
            modernjson::json opDecl;
            opDecl["op"] = "av";
            std::vector<modernjson::json> elements;
            uint64_t size = expression.size()->evaluateAsInt();
            for (uint64_t i = 0; i < size; ++i) {
                elements.push_back(anyToJson(expression.at(i)->accept(*this, data)));
            }
            opDecl["elements"] = std::move(elements);
            return opDecl;
        }
        
        boost::any ExpressionToJson::visit(storm::expressions::ConstructorArrayExpression const& expression, boost::any const& data) {
            modernjson::json opDecl;
            opDecl["op"] = "ac";
            opDecl["var"] = expression.getIndexVar().getName();
            opDecl["length"] = anyToJson(expression.size()->accept(*this, data));
            bool inserted = auxiliaryVariables.insert(expression.getIndexVar().getName()).second;
            opDecl["exp"] = anyToJson(expression.getElementExpression()->accept(*this, data));
            if (inserted) {
                auxiliaryVariables.erase(expression.getIndexVar().getName());
            }
            return opDecl;
        }
        
        boost::any ExpressionToJson::visit(storm::expressions::ArrayAccessExpression const& expression, boost::any const& data) {
            modernjson::json opDecl;
            opDecl["op"] = "aa";
            opDecl["exp"] = anyToJson(expression.getOperand(0)->accept(*this, data));
            opDecl["index"] = anyToJson(expression.getOperand(1)->accept(*this, data));
            return opDecl;
        }
        
        boost::any ExpressionToJson::visit(storm::expressions::FunctionCallExpression const& expression, boost::any const& data) {
            modernjson::json opDecl;
            opDecl["op"] = "call";
            opDecl["function"] = expression.getFunctionIdentifier();
            std::vector<modernjson::json> arguments;
            for (uint64_t i = 0; i < expression.getNumberOfArguments(); ++i) {
                arguments.push_back(anyToJson(expression.getArgument(i)->accept(*this, data)));
            }
            opDecl["args"] = std::move(arguments);
            return opDecl;
        }
        
        void JsonExporter::toFile(storm::jani::Model const& janiModel, std::vector<storm::jani::Property> const& formulas, std::string const& filepath, bool checkValid, bool compact) {
            std::ofstream stream;
            storm::utility::openFile(filepath, stream, false, true);
            toStream(janiModel, formulas, stream, checkValid, compact);
            storm::utility::closeFile(stream);
        }
        
        void JsonExporter::toStream(storm::jani::Model const& janiModel,  std::vector<storm::jani::Property> const& formulas, std::ostream& os, bool checkValid, bool compact) {
            if(checkValid) {
                janiModel.checkValid();
            }
            JsonExporter exporter;
            STORM_LOG_INFO("Started to convert model " << janiModel.getName() << ".");
            exporter.convertModel(janiModel, !compact);
            STORM_LOG_INFO("Started to convert properties of model " << janiModel.getName() << ".");
            exporter.convertProperties(formulas, janiModel);
            if (compact) {
                // Dump without line breaks/indents
                STORM_LOG_INFO("Producing compact json output... " << janiModel.getName() << ".");
                os << exporter.finalize().dump() << std::endl;
            } else {
                // Dump with line breaks and indention with 4 spaces
                STORM_LOG_INFO("Producing json output... " << janiModel.getName() << ".");
                os << exporter.finalize().dump(4) << std::endl;
            }
            STORM_LOG_INFO("Conversion completed " << janiModel.getName() << ".");
        }
        
        modernjson::json buildActionArray(std::vector<storm::jani::Action> const& actions) {
            std::vector<modernjson::json> actionReprs;
            uint64_t actIndex = 0;
            for(auto const& act : actions) {
                if(actIndex == storm::jani::Model::SILENT_ACTION_INDEX) {
                    actIndex++;
                    continue;
                }
                actIndex++;
                modernjson::json actEntry;
                actEntry["name"] = act.getName();
                actionReprs.push_back(actEntry);
            }
            
            return modernjson::json(actionReprs);
            
        }
        
        modernjson::json buildTypeDescription(storm::expressions::Type const& type) {
            modernjson::json typeDescr;
            if (type.isIntegerType()) {
                typeDescr = "int";
            } else if (type.isRationalType()) {
                typeDescr = "real";
            } else if (type.isBooleanType()) {
                typeDescr = "bool";
            } else if (type.isArrayType()) {
                typeDescr["kind"] = "array";
                typeDescr["base"] = buildTypeDescription(type.getElementType());
            } else {
                assert(false);
            }
            return typeDescr;
        }
        
        void getBoundsFromConstraints(modernjson::json& typeDesc, storm::expressions::Variable const& var, storm::expressions::Expression const& constraint, std::vector<storm::jani::Constant> const& constants) {
            if (constraint.getBaseExpression().isBinaryBooleanFunctionExpression() && constraint.getBaseExpression().getOperator() == storm::expressions::OperatorType::And) {
                getBoundsFromConstraints(typeDesc, var, constraint.getBaseExpression().getOperand(0), constants);
                getBoundsFromConstraints(typeDesc, var, constraint.getBaseExpression().getOperand(1), constants);
            } else if (constraint.getBaseExpression().isBinaryRelationExpression()) {
                bool leq = constraint.getBaseExpression().getOperator() == storm::expressions::OperatorType::LessOrEqual;
                bool geq = constraint.getBaseExpression().getOperator() == storm::expressions::OperatorType::GreaterOrEqual;
                std::vector<bool> varOps(2, false);
                for (int i : {0,1}) {
                    varOps[i] = constraint.getOperand(i).isVariable() && constraint.getOperand(i).getBaseExpression().asVariableExpression().getVariable() == var;
                }
                storm::expressions::Expression boundExpr = varOps[0] ? constraint.getOperand(1) : constraint.getOperand(0);
                if ((leq && varOps[0]) || (geq && varOps[1])) {
                    typeDesc["upper-bound"] = buildExpression(boundExpr, constants);
                } else if ((leq && varOps[1]) || (geq && varOps[0])) {
                    typeDesc["lower-bound"] = buildExpression(boundExpr, constants);
                } else {
                    STORM_LOG_THROW(false, storm::exceptions::NotSupportedException, "Jani Export for constant constraint " << constraint << " is not supported.");
                }
            } else {
                STORM_LOG_THROW(false, storm::exceptions::NotSupportedException, "Jani Export for constant constraint " << constraint << " is not supported.");
            }
        }
        
        modernjson::json buildConstantsArray(std::vector<storm::jani::Constant> const& constants) {
            std::vector<modernjson::json> constantDeclarations;
            for(auto const& constant : constants) {
                modernjson::json constantEntry;
                constantEntry["name"] = constant.getName();
                modernjson::json typeDesc;
                if (constant.hasConstraint()) {
                    typeDesc["kind"] = "bounded";
                    typeDesc["base"] = buildTypeDescription(constant.getType());
                    getBoundsFromConstraints(typeDesc, constant.getExpressionVariable(), constant.getConstraintExpression(), constants);
                } else {
                    typeDesc = buildTypeDescription(constant.getType());
                }
                constantEntry["type"] = typeDesc;
                if(constant.isDefined()) {
                    constantEntry["value"] = buildExpression(constant.getExpression(), constants);
                }
                constantDeclarations.push_back(constantEntry);
            }
            return modernjson::json(constantDeclarations);
        }
        
        
        modernjson::json buildVariablesArray(storm::jani::VariableSet const& varSet, std::vector<storm::jani::Constant> const& constants, VariableSet const& globalVariables, VariableSet const& localVariables = VariableSet()) {
            modernjson::json variableDeclarations = std::vector<modernjson::json>();
            for(auto const& variable : varSet) {
                modernjson::json varEntry;
                varEntry["name"] = variable.getName();
                if (variable.isTransient()) {
                    varEntry["transient"] = variable.isTransient();
                }
                modernjson::json typeDesc;
                if(variable.isBooleanVariable()) {
                    typeDesc = "bool";
                } else if (variable.isRealVariable()) {
                    typeDesc = "real";
                } else if (variable.isUnboundedIntegerVariable()) {
                    typeDesc = "int";
                } else if (variable.isBoundedIntegerVariable()) {
                    typeDesc["kind"] = "bounded";
                    typeDesc["base"] = "int";
                    typeDesc["lower-bound"] = buildExpression(variable.asBoundedIntegerVariable().getLowerBound(), constants, globalVariables, localVariables);
                    typeDesc["upper-bound"] = buildExpression(variable.asBoundedIntegerVariable().getUpperBound(), constants, globalVariables, localVariables);
                } else if (variable.isArrayVariable()) {
                    typeDesc["kind"] = "array";
                    switch (variable.asArrayVariable().getElementType()) {
                        case storm::jani::ArrayVariable::ElementType::Bool:
                            typeDesc["base"] = "bool";
                            break;
                        case storm::jani::ArrayVariable::ElementType::Real:
                            typeDesc["base"] = "real";
                            break;
                        case storm::jani::ArrayVariable::ElementType::Int:
                            if (variable.asArrayVariable().hasElementTypeBound()) {
                                modernjson::json baseTypeDescr;
                                baseTypeDescr["kind"] = "bounded";
                                baseTypeDescr["base "] = "int";
                                if (variable.asArrayVariable().hasLowerElementTypeBound()) {
                                    baseTypeDescr["lower-bound"] = buildExpression(variable.asArrayVariable().getLowerElementTypeBound(), constants, globalVariables, localVariables);
                                }
                                if (variable.asArrayVariable().hasUpperElementTypeBound()) {
                                    baseTypeDescr["upper-bound"] = buildExpression(variable.asArrayVariable().getUpperElementTypeBound(), constants, globalVariables, localVariables);
                                }
                                typeDesc["base"] = baseTypeDescr;
                            } else {
                                typeDesc["base"] = "int";
                            }
                            break;
                    }
                } else {
                    assert(variable.isClockVariable());
                    typeDesc = "clock";
                }
                varEntry["type"] = typeDesc;
                if (variable.hasInitExpression()) {
                    varEntry["initial-value"] = buildExpression(variable.getInitExpression(), constants, globalVariables, localVariables);
                }
                variableDeclarations.push_back(std::move(varEntry));
            }
            return variableDeclarations;
        }

        modernjson::json buildFunctionsArray(std::unordered_map<std::string, FunctionDefinition> const& functionDefinitions, std::vector<storm::jani::Constant> const& constants, VariableSet const& globalVariables, VariableSet const& localVariables = VariableSet()) {
            modernjson::json functionDeclarations = std::vector<modernjson::json>();
            for (auto const& nameFunDef : functionDefinitions) {
                storm::jani::FunctionDefinition const& funDef = nameFunDef.second;
                modernjson::json funDefJson;
                funDefJson["name"] = nameFunDef.first;
                funDefJson["type"] = buildTypeDescription(funDef.getType());
                std::vector<modernjson::json> parameterDeclarations;
                std::unordered_set<std::string> parameterNames;
                for (auto const& p : funDef.getParameters()) {
                    modernjson::json parDefJson;
                    parDefJson["name"] = p.getName();
                    parameterNames.insert(p.getName());
                    parDefJson["type"] = buildTypeDescription(p.getType());
                    parameterDeclarations.push_back(parDefJson);
                }
                funDefJson["parameters"] = parameterDeclarations;
                funDefJson["body"] = buildExpression(funDef.getFunctionBody(), constants, globalVariables, localVariables, parameterNames);
                functionDeclarations.push_back(std::move(funDefJson));
            }
            return functionDeclarations;
        }
        
        modernjson::json buildAssignmentArray(storm::jani::OrderedAssignments const& orderedAssignments, std::vector<storm::jani::Constant> const& constants, VariableSet const& globalVariables, VariableSet const& localVariables, bool commentExpressions) {
            modernjson::json assignmentDeclarations = std::vector<modernjson::json>();
            bool addIndex = orderedAssignments.hasMultipleLevels();
            for(auto const& assignment : orderedAssignments) {
                modernjson::json assignmentEntry;
                if (assignment.getLValue().isVariable()) {
                    assignmentEntry["ref"] = assignment.getVariable().getName();
                } else {
                    STORM_LOG_ASSERT(assignment.getLValue().isArrayAccess(), "Unhandled LValue " << assignment.getLValue());
                    modernjson::json arrayAccess;
                    arrayAccess["op"] = "aa";
                    arrayAccess["exp"] = assignment.getLValue().getArray().getName();
                    arrayAccess["index"] = buildExpression(assignment.getLValue().getArrayIndex(), constants, globalVariables, localVariables);
                    assignmentEntry["ref"] = std::move(arrayAccess);
                }
                assignmentEntry["value"] = buildExpression(assignment.getAssignedExpression(), constants, globalVariables, localVariables);
                if(addIndex) {
                    assignmentEntry["index"] = assignment.getLevel();
                }
                if (commentExpressions) {
                    assignmentEntry["comment"] = assignment.getVariable().getName() + " <- " + assignment.getAssignedExpression().toString();
                }
                assignmentDeclarations.push_back(std::move(assignmentEntry));
            }
            return assignmentDeclarations;
        }
        
        modernjson::json buildLocationsArray(std::vector<storm::jani::Location> const& locations, std::vector<storm::jani::Constant> const& constants, VariableSet const& globalVariables, VariableSet const& localVariables, bool commentExpressions) {
            modernjson::json locationDeclarations = std::vector<modernjson::json>();
            for(auto const& location : locations) {
                modernjson::json locEntry;
                locEntry["name"] = location.getName();
                if (location.hasTimeProgressInvariant()) {
                    modernjson::json timeProg;
                    timeProg["exp"] = buildExpression(location.getTimeProgressInvariant(), constants, globalVariables, localVariables);
                    if (commentExpressions) {
                        timeProg["comment"] = location.getTimeProgressInvariant().toString();
                    }
                    locEntry["time-progress"] = std::move(timeProg);
                }
                if (!location.getAssignments().empty()) {
                    locEntry["transient-values"] = buildAssignmentArray(location.getAssignments(), constants, globalVariables, localVariables, commentExpressions);
                }
                locationDeclarations.push_back(std::move(locEntry));
            }
            return locationDeclarations;
        }
        
        modernjson::json buildInitialLocations(storm::jani::Automaton const& automaton) {
            std::vector<std::string> names;
            for(auto const& initLocIndex : automaton.getInitialLocationIndices()) {
                names.push_back(automaton.getLocation(initLocIndex).getName());
            }
            return modernjson::json(names);
        }
        
        modernjson::json buildDestinations(std::vector<EdgeDestination> const& destinations, std::map<uint64_t, std::string> const& locationNames, std::vector<storm::jani::Constant> const& constants, VariableSet const& globalVariables, VariableSet const& localVariables, bool commentExpressions) {
            assert(destinations.size() > 0);
            modernjson::json destDeclarations = std::vector<modernjson::json>();
            for(auto const& destination : destinations) {
                modernjson::json destEntry;
                destEntry["location"] = locationNames.at(destination.getLocationIndex());
                bool prob1 = false;
                if(destination.getProbability().isLiteral()) {
                    if(destination.getProbability().evaluateAsDouble() == 1) {
                        prob1 = true;
                    }
                }
                if (!prob1) {
                    destEntry["probability"]["exp"] = buildExpression(destination.getProbability(), constants, globalVariables, localVariables);
                    if (commentExpressions) {
                        destEntry["probability"]["comment"] = destination.getProbability().toString();
                    }
                }
                if (!destination.getOrderedAssignments().empty()) {
                    destEntry["assignments"] = buildAssignmentArray(destination.getOrderedAssignments(), constants, globalVariables, localVariables, commentExpressions);
                }
                destDeclarations.push_back(std::move(destEntry));
            }
            return destDeclarations;
        }
        
        modernjson::json buildEdges(std::vector<Edge> const& edges , std::map<uint64_t, std::string> const& actionNames, std::map<uint64_t, std::string> const& locationNames, std::vector<storm::jani::Constant> const& constants, VariableSet const& globalVariables, VariableSet const& localVariables, bool commentExpressions) {
            modernjson::json edgeDeclarations = std::vector<modernjson::json>();
            for(auto const& edge : edges) {
                if (edge.getGuard().isFalse()) {
                    continue;
                }
                STORM_LOG_THROW(edge.getDestinations().size() > 0, storm::exceptions::InvalidJaniException, "An edge without destinations is not allowed.");
                modernjson::json edgeEntry;
                edgeEntry["location"] = locationNames.at(edge.getSourceLocationIndex());
                if(!edge.hasSilentAction()) {
                    edgeEntry["action"] = actionNames.at(edge.getActionIndex());
                }
                if(edge.hasRate()) {
                    edgeEntry["rate"]["exp"] = buildExpression(edge.getRate(), constants, globalVariables, localVariables);
                    if (commentExpressions) {
                        edgeEntry["rate"]["comment"] = edge.getRate().toString();
                    }
                }
                if (!edge.getGuard().isTrue()) {
                    edgeEntry["guard"]["exp"] = buildExpression(edge.getGuard(), constants, globalVariables, localVariables);
                    if (commentExpressions) {
                        edgeEntry["guard"]["comment"] = edge.getGuard().toString();
                    }
                }
                edgeEntry["destinations"] = buildDestinations(edge.getDestinations(), locationNames, constants, globalVariables, localVariables, commentExpressions);
                if (!edge.getAssignments().empty()) {
                    edgeEntry["assignments"] = buildAssignmentArray(edge.getAssignments(), constants, globalVariables, localVariables, commentExpressions);
                }
                
                edgeDeclarations.push_back(std::move(edgeEntry));
            }
            return edgeDeclarations;
        }
        
        modernjson::json buildAutomataArray(std::vector<storm::jani::Automaton> const& automata, std::map<uint64_t, std::string> const& actionNames, std::vector<storm::jani::Constant> const& constants, VariableSet const& globalVariables, bool commentExpressions) {
            modernjson::json automataDeclarations = std::vector<modernjson::json>();
            for(auto const& automaton : automata) {
                modernjson::json autoEntry;
                autoEntry["name"] = automaton.getName();
                autoEntry["variables"] = buildVariablesArray(automaton.getVariables(), constants, globalVariables, automaton.getVariables());
                if (!automaton.getFunctionDefinitions().empty()) {
                    autoEntry["functions"] = buildFunctionsArray(automaton.getFunctionDefinitions(), constants, globalVariables, automaton.getVariables());
                }
                if(automaton.hasRestrictedInitialStates()) {
                    autoEntry["restrict-initial"]["exp"] = buildExpression(automaton.getInitialStatesRestriction(), constants, globalVariables, automaton.getVariables());
                }
                autoEntry["locations"] = buildLocationsArray(automaton.getLocations(), constants, globalVariables, automaton.getVariables(), commentExpressions);
                autoEntry["initial-locations"] = buildInitialLocations(automaton);
                autoEntry["edges"] = buildEdges(automaton.getEdges(), actionNames, automaton.buildIdToLocationNameMap(), constants, globalVariables, automaton.getVariables(), commentExpressions);
                automataDeclarations.push_back(std::move(autoEntry));
            }
            return automataDeclarations;
        }
        
        void JsonExporter::convertModel(storm::jani::Model const& janiModel, bool commentExpressions) {
            modelFeatures = janiModel.getModelFeatures();
            jsonStruct["jani-version"] = janiModel.getJaniVersion();
            jsonStruct["name"] = janiModel.getName();
            jsonStruct["type"] = to_string(janiModel.getModelType());
            jsonStruct["actions"] = buildActionArray(janiModel.getActions());
            jsonStruct["constants"] = buildConstantsArray(janiModel.getConstants());
            jsonStruct["variables"] = buildVariablesArray(janiModel.getGlobalVariables(), janiModel.getConstants(), janiModel.getGlobalVariables());
            if (!janiModel.getGlobalFunctionDefinitions().empty()) {
                jsonStruct["functions"] = buildFunctionsArray(janiModel.getGlobalFunctionDefinitions(), janiModel.getConstants(), janiModel.getGlobalVariables());
            }
            jsonStruct["restrict-initial"]["exp"] = buildExpression(janiModel.getInitialStatesRestriction(), janiModel.getConstants(), janiModel.getGlobalVariables());
            jsonStruct["automata"] = buildAutomataArray(janiModel.getAutomata(), janiModel.getActionIndexToNameMap(), janiModel.getConstants(), janiModel.getGlobalVariables(), commentExpressions);
            jsonStruct["system"] = CompositionJsonExporter::translate(janiModel.getSystemComposition());
        }
        
        std::string janiFilterTypeString(storm::modelchecker::FilterType const& ft) {
            switch(ft) {
                case storm::modelchecker::FilterType::MIN:
                    return "min";
                case storm::modelchecker::FilterType::MAX:
                    return "max";
                case storm::modelchecker::FilterType::SUM:
                    return "sum";
                case storm::modelchecker::FilterType::AVG:
                    return "avg";
                case storm::modelchecker::FilterType::COUNT:
                    return "count";
                case storm::modelchecker::FilterType::EXISTS:
                    return "∃";
                case storm::modelchecker::FilterType::FORALL:
                    return "∀";
                case storm::modelchecker::FilterType::ARGMIN:
                    return "argmin";
                case storm::modelchecker::FilterType::ARGMAX:
                    return "argmax";
                case storm::modelchecker::FilterType::VALUES:
                    return "values";
                default:
                    assert(false);
                    
            }
        }
        
        modernjson::json convertFilterExpression(storm::jani::FilterExpression const& fe, storm::jani::Model const& model, storm::jani::ModelFeatures& modelFeatures) {
            modernjson::json propDecl;
            propDecl["states"]["op"] = "initial";
            propDecl["op"] = "filter";
            propDecl["fun"] = janiFilterTypeString(fe.getFilterType());
            propDecl["values"] = FormulaToJaniJson::translate(*fe.getFormula(), model, modelFeatures);
            return propDecl;
        }
        
        
        void JsonExporter::convertProperties( std::vector<storm::jani::Property> const& formulas, storm::jani::Model const& model) {
            modernjson::json properties;
            
            // Unset model-features that only relate to properties. These are only set if such properties actually exist.
            modelFeatures.remove(storm::jani::ModelFeature::StateExitRewards);
            if (formulas.empty()) {
                jsonStruct["properties"] = modernjson::json(modernjson::json::value_t::array);
                return;
            }

            uint64_t index = 0;
            for(auto const& f : formulas) {
                modernjson::json propDecl;
                propDecl["name"] = f.getName();
                propDecl["expression"] = convertFilterExpression(f.getFilter(), model, modelFeatures);
                ++index;
                properties.push_back(std::move(propDecl));
            }
            jsonStruct["properties"] = std::move(properties);
        }
        
        modernjson::json JsonExporter::finalize() {
            jsonStruct["features"] = modernjson::json::parse(modelFeatures.toString());
            return jsonStruct;
        }

    }
}
