#pragma once
#include <iostream>

namespace storm {
    namespace logic {
        
        class RewardAccumulation {
        public:
            RewardAccumulation(bool steps, bool time, bool exit);
            RewardAccumulation(RewardAccumulation const& other) = default;
            
            bool isStepsSet() const; // If set, choice rewards and transition rewards are accumulated upon taking the transition
            bool isTimeSet() const; // If set, state rewards are accumulated over time (assuming 0 time passes in discrete-time model states)
            bool isExitSet() const; // If set, state rewards are accumulated upon exiting the state
            
            // Returns true iff accumulation for all types of reward is disabled.
            bool isEmpty() const;
            
        private:
            bool time, steps, exit;
        };

        std::ostream& operator<<(std::ostream& out, RewardAccumulation const& acc);

    }
}

