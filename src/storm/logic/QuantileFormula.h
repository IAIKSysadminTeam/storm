#pragma once

#include "storm/logic/Formula.h"
#include "storm/solver/OptimizationDirection.h"

namespace storm {
    namespace logic {
        class QuantileFormula : public Formula {
        public:
            QuantileFormula(std::vector<std::pair<storm::solver::OptimizationDirection, storm::expressions::Variable>> const& boundVariables, std::shared_ptr<Formula const> subformula);
            
            virtual ~QuantileFormula();
            
            virtual bool isQuantileFormula() const override;
            
            virtual bool hasQuantitativeResult() const override; // Result is numerical or a pareto curve
            virtual bool hasNumericalResult() const; // Result is numerical
            virtual bool hasParetoCurveResult() const; // Result is a pareto curve
            
            Formula const& getSubformula() const;
            uint64_t getDimension() const;
            bool isMultiDimensional() const;
            
            storm::expressions::Variable const& getBoundVariable() const;
            storm::expressions::Variable const& getBoundVariable(uint64_t index) const;
            std::vector<std::pair<storm::solver::OptimizationDirection, storm::expressions::Variable>> const& getBoundVariables() const;
            storm::solver::OptimizationDirection const& getOptimizationDirection() const;
            storm::solver::OptimizationDirection const& getOptimizationDirection(uint64_t index) const;
            
            virtual boost::any accept(FormulaVisitor const& visitor, boost::any const& data) const override;
            virtual void gatherAtomicExpressionFormulas(std::vector<std::shared_ptr<AtomicExpressionFormula const>>& atomicExpressionFormulas) const override;
            virtual void gatherAtomicLabelFormulas(std::vector<std::shared_ptr<AtomicLabelFormula const>>& atomicLabelFormulas) const override;
            virtual void gatherReferencedRewardModels(std::set<std::string>& referencedRewardModels) const override;
            
            virtual std::ostream& writeToStream(std::ostream& out) const override;
        private:
            std::vector<std::pair<storm::solver::OptimizationDirection, storm::expressions::Variable>> boundVariables;
            std::shared_ptr<Formula const> subformula;
        };
    }
}