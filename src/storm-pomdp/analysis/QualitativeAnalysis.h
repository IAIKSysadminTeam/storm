#include "storm/models/sparse/Pomdp.h"
#include "storm/storage/BitVector.h"
#include "storm/logic/Formulas.h"
namespace storm {
    namespace analysis {
        template<typename ValueType>
        class QualitativeAnalysis {
        public:

            QualitativeAnalysis(storm::models::sparse::Pomdp<ValueType> const& pomdp);
            storm::storage::BitVector analyseProb0(storm::logic::ProbabilityOperatorFormula const& formula) const;
            storm::storage::BitVector analyseProb1(storm::logic::ProbabilityOperatorFormula const& formula) const;
            
        private:
            storm::storage::BitVector analyseProb0or1(storm::logic::ProbabilityOperatorFormula const& formula, bool prob0) const;
            storm::storage::BitVector analyseProb0Max(storm::logic::UntilFormula const& formula) const;
            storm::storage::BitVector analyseProb0Min(storm::logic::UntilFormula const& formula) const;
            storm::storage::BitVector analyseProb1Max(storm::logic::UntilFormula const& formula) const;
            storm::storage::BitVector analyseProb1Min(storm::logic::UntilFormula const& formula) const;
            
            storm::storage::BitVector checkPropositionalFormula(storm::logic::Formula const& propositionalFormula) const;
            
            storm::models::sparse::Pomdp<ValueType> const& pomdp;
        };
    }
}

