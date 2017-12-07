#include "storm/storage/dd/BisimulationDecomposition.h"

#include "storm/storage/dd/bisimulation/Partition.h"
#include "storm/storage/dd/bisimulation/PartitionRefiner.h"
#include "storm/storage/dd/bisimulation/MdpPartitionRefiner.h"
#include "storm/storage/dd/bisimulation/QuotientExtractor.h"
#include "storm/storage/dd/bisimulation/PartialQuotientExtractor.h"

#include "storm/models/symbolic/Model.h"
#include "storm/models/symbolic/Mdp.h"
#include "storm/models/symbolic/StandardRewardModel.h"

#include "storm/settings/SettingsManager.h"
#include "storm/settings/modules/GeneralSettings.h"

#include "storm/utility/macros.h"
#include "storm/exceptions/InvalidOperationException.h"
#include "storm/exceptions/NotSupportedException.h"

namespace storm {
    namespace dd {
        
        using namespace bisimulation;

        template <storm::dd::DdType DdType, typename ValueType>
        std::unique_ptr<PartitionRefiner<DdType, ValueType>> createRefiner(storm::models::symbolic::Model<DdType, ValueType> const& model, Partition<DdType, ValueType> const& initialPartition) {
            if (model.isOfType(storm::models::ModelType::Mdp)) {
                return std::make_unique<MdpPartitionRefiner<DdType, ValueType>>(*model.template as<storm::models::symbolic::Mdp<DdType, ValueType>>(), initialPartition);
            } else {
                return std::make_unique<PartitionRefiner<DdType, ValueType>>(model, initialPartition);
            }
        }
        
        template <storm::dd::DdType DdType, typename ValueType>
        BisimulationDecomposition<DdType, ValueType>::BisimulationDecomposition(storm::models::symbolic::Model<DdType, ValueType> const& model, storm::storage::BisimulationType const& bisimulationType) : model(model), preservationInformation(model), refiner(createRefiner(model, Partition<DdType, ValueType>::create(model, bisimulationType, preservationInformation))) {
            this->initialize();
        }
      
        template <storm::dd::DdType DdType, typename ValueType>
        BisimulationDecomposition<DdType, ValueType>::BisimulationDecomposition(storm::models::symbolic::Model<DdType, ValueType> const& model, storm::storage::BisimulationType const& bisimulationType, bisimulation::PreservationInformation<DdType, ValueType> const& preservationInformation) : model(model), preservationInformation(preservationInformation), refiner(createRefiner(model, Partition<DdType, ValueType>::create(model, bisimulationType, preservationInformation))) {
            this->initialize();
        }
  
        template <storm::dd::DdType DdType, typename ValueType>
        BisimulationDecomposition<DdType, ValueType>::BisimulationDecomposition(storm::models::symbolic::Model<DdType, ValueType> const& model, std::vector<std::shared_ptr<storm::logic::Formula const>> const& formulas, storm::storage::BisimulationType const& bisimulationType) : model(model), preservationInformation(model, formulas), refiner(createRefiner(model, Partition<DdType, ValueType>::create(model, bisimulationType, formulas))) {
            this->initialize();
        }
        
        template <storm::dd::DdType DdType, typename ValueType>
        BisimulationDecomposition<DdType, ValueType>::BisimulationDecomposition(storm::models::symbolic::Model<DdType, ValueType> const& model, Partition<DdType, ValueType> const& initialPartition, bisimulation::PreservationInformation<DdType, ValueType> const& preservationInformation) : model(model), preservationInformation(preservationInformation), refiner(createRefiner(model, initialPartition)) {
            this->initialize();
        }
        
        template <storm::dd::DdType DdType, typename ValueType>
        BisimulationDecomposition<DdType, ValueType>::~BisimulationDecomposition() = default;
        
        template <storm::dd::DdType DdType, typename ValueType>
        void BisimulationDecomposition<DdType, ValueType>::initialize() {
            auto const& generalSettings = storm::settings::getModule<storm::settings::modules::GeneralSettings>();
            showProgress = generalSettings.isVerboseSet();
            showProgressDelay = generalSettings.getShowProgressDelay();
            this->refineWrtRewardModels();
            
            STORM_LOG_TRACE("Initial partition has " << refiner->getStatePartition().getNumberOfBlocks() << " blocks.");
#ifndef NDEBUG
            STORM_LOG_TRACE("Initial partition has " << refiner->getStatePartition().getNodeCount() << " nodes.");
#endif
        }
        
        template <storm::dd::DdType DdType, typename ValueType>
        void BisimulationDecomposition<DdType, ValueType>::compute(bisimulation::SignatureMode const& mode) {
            STORM_LOG_ASSERT(refiner, "No suitable refiner.");
            STORM_LOG_ASSERT(this->refiner->getStatus() != Status::FixedPoint, "Can only proceed if no fixpoint has been reached yet.");

            auto start = std::chrono::high_resolution_clock::now();
            auto timeOfLastMessage = start;
            uint64_t iterations = 0;
            bool refined = true;
            while (refined) {
                refined = refiner->refine(mode);

                ++iterations;
                STORM_LOG_TRACE("After iteration " << iterations << " partition has " << refiner->getStatePartition().getNumberOfBlocks() << " blocks.");
                
                if (showProgress) {
                    auto now = std::chrono::high_resolution_clock::now();
                    auto durationSinceLastMessage = std::chrono::duration_cast<std::chrono::seconds>(now - timeOfLastMessage).count();
                    if (static_cast<uint64_t>(durationSinceLastMessage) >= showProgressDelay) {
                        auto durationSinceStart = std::chrono::duration_cast<std::chrono::seconds>(now - start).count();
                        STORM_LOG_INFO("State partition after " << iterations << " iterations (" << durationSinceStart << "s) has " << refiner->getStatePartition().getNumberOfBlocks() << " blocks.");
                        timeOfLastMessage = std::chrono::high_resolution_clock::now();
                    }
                }
            }
            auto end = std::chrono::high_resolution_clock::now();
            
            STORM_LOG_DEBUG("Partition refinement completed in " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "ms (" << iterations << " iterations).");
        }

        template <storm::dd::DdType DdType, typename ValueType>
        bool BisimulationDecomposition<DdType, ValueType>::compute(uint64_t steps, bisimulation::SignatureMode const& mode) {
            STORM_LOG_ASSERT(refiner, "No suitable refiner.");
            STORM_LOG_ASSERT(this->refiner->getStatus() != Status::FixedPoint, "Can only proceed if no fixpoint has been reached yet.");
            STORM_LOG_ASSERT(steps > 0, "Can only perform positive number of steps.");

            auto start = std::chrono::high_resolution_clock::now();
            auto timeOfLastMessage = start;
            uint64_t iterations = 0;
            bool refined = true;
            while (refined && iterations < steps) {
                refined = refiner->refine(mode);
                
                ++iterations;
                
                if (showProgress) {
                    auto now = std::chrono::high_resolution_clock::now();
                    auto durationSinceLastMessage = std::chrono::duration_cast<std::chrono::seconds>(now - timeOfLastMessage).count();
                    if (static_cast<uint64_t>(durationSinceLastMessage) >= showProgressDelay) {
                        auto durationSinceStart = std::chrono::duration_cast<std::chrono::seconds>(now - start).count();
                        STORM_LOG_INFO("State partition after " << iterations << " iterations (" << durationSinceStart << "ms) has " << refiner->getStatePartition().getNumberOfBlocks() << " blocks.");
                        timeOfLastMessage = std::chrono::high_resolution_clock::now();
                    }
                }
            }
            
            return !refined;
        }
        
        template <storm::dd::DdType DdType, typename ValueType>
        bool BisimulationDecomposition<DdType, ValueType>::getReachedFixedPoint() const {
            return this->refiner->getStatus() == Status::FixedPoint;
        }
        
        template <storm::dd::DdType DdType, typename ValueType>
        std::shared_ptr<storm::models::Model<ValueType>> BisimulationDecomposition<DdType, ValueType>::getQuotient() const {
            std::shared_ptr<storm::models::Model<ValueType>> quotient;
            if (this->refiner->getStatus() == Status::FixedPoint) {
                STORM_LOG_TRACE("Starting full quotient extraction.");
                QuotientExtractor<DdType, ValueType> extractor;
                quotient = extractor.extract(model, refiner->getStatePartition(), preservationInformation);
            } else {
                STORM_LOG_THROW(model.getType() == storm::models::ModelType::Dtmc || model.getType() == storm::models::ModelType::Mdp, storm::exceptions::InvalidOperationException, "Can only extract partial quotient for discrete-time models.");
                
                STORM_LOG_TRACE("Starting partial quotient extraction.");
                if (!partialQuotientExtractor) {
                    partialQuotientExtractor = std::make_unique<bisimulation::PartialQuotientExtractor<DdType, ValueType>>(model);
                }

                quotient = partialQuotientExtractor->extract(refiner->getStatePartition(), preservationInformation);
            }

            STORM_LOG_TRACE("Quotient extraction done.");
            return quotient;
        }
        
        template <storm::dd::DdType DdType, typename ValueType>
        void BisimulationDecomposition<DdType, ValueType>::refineWrtRewardModels() {
            for (auto const& rewardModelName : this->preservationInformation.getRewardModelNames()) {
                auto const& rewardModel = this->model.getRewardModel(rewardModelName);
                refiner->refineWrtRewardModel(rewardModel);
            }
        }
        
        template class BisimulationDecomposition<storm::dd::DdType::CUDD, double>;

        template class BisimulationDecomposition<storm::dd::DdType::Sylvan, double>;
        template class BisimulationDecomposition<storm::dd::DdType::Sylvan, storm::RationalNumber>;
        template class BisimulationDecomposition<storm::dd::DdType::Sylvan, storm::RationalFunction>;
        
    }
}
