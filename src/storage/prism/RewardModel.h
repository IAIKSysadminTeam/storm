#ifndef STORM_STORAGE_PRISM_REWARDMODEL_H_
#define STORM_STORAGE_PRISM_REWARDMODEL_H_

#include <string>
#include <vector>
#include <map>

#include "src/storage/prism/StateReward.h"
#include "src/storage/prism/TransitionReward.h"
#include "src/utility/OsDetection.h"

namespace storm {
    namespace prism {
        class RewardModel : public LocatedInformation {
        public:
            /*!
             * Creates a reward model with the given name, state and transition rewards.
             *
             * @param rewardModelName The name of the reward model.
             * @param stateRewards A vector of state-based rewards.
             * @param transitionRewards A vector of transition-based rewards.
             * @param filename The filename in which the reward model is defined.
             * @param lineNumber The line number in which the reward model is defined.
             */
            RewardModel(std::string const& rewardModelName, std::vector<storm::prism::StateReward> const& stateRewards, std::vector<storm::prism::TransitionReward> const& transitionRewards, std::string const& filename = "", uint_fast64_t lineNumber = 0);
            
            // Create default implementations of constructors/assignment.
            RewardModel() = default;
            RewardModel(RewardModel const& other) = default;
            RewardModel& operator=(RewardModel const& other)= default;
#ifndef WINDOWS
            RewardModel(RewardModel&& other) = default;
            RewardModel& operator=(RewardModel&& other) = default;
#endif
            
            /*!
             * Retrieves the name of the reward model.
             *
             * @return The name of the reward model.
             */
            std::string const& getName() const;
            
            /*!
             * Checks whether the reward model is empty, i.e. contains no state or transition rewards.
             *
             * @return True iff the reward model is empty.
             */
            bool empty() const;
            
            /*!
             * Retrieves whether there are any state rewards.
             *
             * @return True iff there are any state rewards.
             */
            bool hasStateRewards() const;
            
            /*!
             * Retrieves all state rewards associated with this reward model.
             *
             * @return The state rewards associated with this reward model.
             */
            std::vector<storm::prism::StateReward> const& getStateRewards() const;
            
            /*!
             * Retrieves whether there are any transition rewards.
             *
             * @return True iff there are any transition rewards.
             */
            bool hasTransitionRewards() const;
            
            /*!
             * Retrieves all transition rewards associated with this reward model.
             *
             * @return The transition rewards associated with this reward model.
             */
            std::vector<storm::prism::TransitionReward> const& getTransitionRewards() const;
            
            /*!
             * Substitutes all identifiers in the reward model according to the given map.
             *
             * @param substitution The substitution to perform.
             * @return The resulting reward model.
             */
            RewardModel substitute(std::map<std::string, storm::expressions::Expression> const& substitution) const;
            
            friend std::ostream& operator<<(std::ostream& stream, RewardModel const& rewardModel);

        private:
            // The name of the reward model.
            std::string rewardModelName;
            
            // The state-based rewards associated with this reward model.
            std::vector<storm::prism::StateReward> stateRewards;
            
            // The transition-based rewards associated with this reward model.
            std::vector<storm::prism::TransitionReward> transitionRewards;
        };
        
    } // namespace prism
} // namespace storm

#endif /* STORM_STORAGE_PRISM_REWARDMODEL_H_ */
