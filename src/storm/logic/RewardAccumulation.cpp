#include "storm/logic/RewardAccumulation.h"

namespace storm {
    namespace logic {
        
        RewardAccumulation::RewardAccumulation(bool steps, bool time, bool exit) : time(time), steps(steps), exit(exit){
            // Intentionally left empty
        }
        
        bool RewardAccumulation::isStepsSet() const {
            return steps;
        }
        
        bool RewardAccumulation::isTimeSet() const {
            return time;
        }
        
        bool RewardAccumulation::isExitSet() const {
            return exit;
        }
        
        bool RewardAccumulation::isEmpty() const {
            return !isStepsSet() && !isTimeSet() && !isExitSet();
        }

        std::ostream& operator<<(std::ostream& out, RewardAccumulation const& acc) {
            bool hasEntry = false;
            if (acc.isStepsSet()) {
                out << "steps";
                hasEntry = true;
            }
            if (acc.isTimeSet()) {
                if (hasEntry) {
                    out << ", ";
                }
                out << "time";
                hasEntry = true;
            }
            if (acc.isExitSet()) {
                if (hasEntry) {
                    out << ", ";
                }
                out << "exit";
                hasEntry = true;
            }
            
            return out;
        }
        
    }
}
