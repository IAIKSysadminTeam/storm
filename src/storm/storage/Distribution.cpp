#include "storm/storage/Distribution.h"

#include <algorithm>
#include <iostream>

#include "storm/utility/macros.h"
#include "storm/utility/constants.h"
#include "storm/utility/ConstantsComparator.h"

#include "storm/settings/SettingsManager.h"

#include "storm/adapters/CarlAdapter.h"

namespace storm {
    namespace storage {
        
        template<typename ValueType, typename StateType>
        Distribution<ValueType, StateType>::Distribution() {
            // Intentionally left empty.
        }
        
        template<typename ValueType, typename StateType>
        void Distribution<ValueType, StateType>::add(Distribution const& other) {
            container_type newDistribution;
            std::set_union(this->distribution.begin(), this->distribution.end(), other.distribution.begin(), other.distribution.end(), std::inserter(newDistribution, newDistribution.begin()));
            this->distribution = std::move(newDistribution);
        }
        
        template<typename ValueType, typename StateType>
        bool Distribution<ValueType, StateType>::equals(Distribution<ValueType, StateType> const& other, storm::utility::ConstantsComparator<ValueType> const& comparator) const {
            // We need to check equality by ourselves, because we need to account for epsilon differences.
            if (this->distribution.size() != other.distribution.size()) {
                return false;
            }
            
            auto first1 = this->distribution.begin();
            auto last1 = this->distribution.end();
            auto first2 = other.distribution.begin();
            
            for (; first1 != last1; ++first1, ++first2) {
                if (first1->first != first2->first) {
                    return false;
                }
                if (!comparator.isEqual(first1->second, first2->second)) {
                    return false;
                }
            }
            
            return true;
        }
        
        template<typename ValueType, typename StateType>
        void Distribution<ValueType, StateType>::addProbability(StateType const& state, ValueType const& probability) {
            auto it = this->distribution.find(state);
            if (it == this->distribution.end()) {
                this->distribution.emplace_hint(it, state, probability);
            } else {
                it->second += probability;
            }
        }
        
        template<typename ValueType, typename StateType>
        void Distribution<ValueType, StateType>::removeProbability(StateType const& state, ValueType const& probability, storm::utility::ConstantsComparator<ValueType> const& comparator) {
            auto it = this->distribution.find(state);
            STORM_LOG_ASSERT(it != this->distribution.end(), "Cannot remove probability, because the state is not in the support of the distribution.");
            it->second -= probability;
            if (comparator.isZero(it->second)) {
                this->distribution.erase(it);
            }
        }
        
        template<typename ValueType, typename StateType>
        void Distribution<ValueType, StateType>::shiftProbability(StateType const& fromState, StateType const& toState, ValueType const& probability, storm::utility::ConstantsComparator<ValueType> const& comparator) {
            removeProbability(fromState, probability, comparator);
            addProbability(toState, probability);
        }
        
        template<typename ValueType, typename StateType>
        typename Distribution<ValueType, StateType>::iterator Distribution<ValueType, StateType>::begin() {
            return this->distribution.begin();
        }

        template<typename ValueType, typename StateType>
        typename Distribution<ValueType, StateType>::const_iterator Distribution<ValueType, StateType>::begin() const {
            return this->distribution.begin();
        }
        
        template<typename ValueType, typename StateType>
        typename Distribution<ValueType, StateType>::const_iterator Distribution<ValueType, StateType>::cbegin() const {
            return this->begin();
        }
        
        template<typename ValueType, typename StateType>
        typename Distribution<ValueType, StateType>::iterator Distribution<ValueType, StateType>::end() {
            return this->distribution.end();
        }
        
        template<typename ValueType, typename StateType>
        typename Distribution<ValueType, StateType>::const_iterator Distribution<ValueType, StateType>::end() const {
            return this->distribution.end();
        }

        template<typename ValueType, typename StateType>
        typename Distribution<ValueType, StateType>::const_iterator Distribution<ValueType, StateType>::cend() const {
            return this->end();
        }
        
        template<typename ValueType, typename StateType>
        void Distribution<ValueType, StateType>::scale(StateType const& state) {
            auto probabilityIterator = this->distribution.find(state);
            if (probabilityIterator != this->distribution.end()) {
                ValueType scaleValue = storm::utility::one<ValueType>() / probabilityIterator->second;
                this->distribution.erase(probabilityIterator);
                
                for (auto& entry : this->distribution) {
                    entry.second *= scaleValue;
                }
            }
        }
                
        template<typename ValueType, typename StateType>
        std::size_t Distribution<ValueType, StateType>::size() const {
            return this->distribution.size();
        }
        
        template<typename ValueType, typename StateType>
        std::ostream& operator<<(std::ostream& out, Distribution<ValueType, StateType> const& distribution) {
            out << "{";
            for (auto const& entry : distribution) {
                out << "[" << entry.second << ": " << entry.first << "], ";
            }
            out << "}";
            
            return out;
        }
        
        template<typename ValueType, typename StateType>
        bool Distribution<ValueType, StateType>::less(Distribution<ValueType, StateType> const& other, storm::utility::ConstantsComparator<ValueType> const& comparator) const {
            if (this->size() != other.size()) {
                return this->size() < other.size();
            }
            auto firstIt = this->begin();
            auto firstIte = this->end();
            auto secondIt = other.begin();
            for (; firstIt != firstIte; ++firstIt, ++secondIt) {
                // If the two blocks already differ, we can decide which distribution is smaller.
                if (firstIt->first != secondIt->first) {
                    return firstIt->first < secondIt->first;
                }
                
                // If the blocks are the same, but the probability differs, we can also decide which distribution is smaller.
                if (!comparator.isEqual(firstIt->second, secondIt->second)) {
                    return comparator.isLess(firstIt->second, secondIt->second);
                }
            }
            return false;
        }
        
        template class Distribution<double>;
        template std::ostream& operator<<(std::ostream& out, Distribution<double> const& distribution);

#ifdef STORM_HAVE_CARL
        template class Distribution<storm::RationalNumber>;
        template std::ostream& operator<<(std::ostream& out, Distribution<storm::RationalNumber> const& distribution);

        template class Distribution<storm::RationalFunction>;
        template std::ostream& operator<<(std::ostream& out, Distribution<storm::RationalFunction> const& distribution);
#endif
    }
}