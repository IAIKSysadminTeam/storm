#include "ExplicitDFTModelBuilder.h"

#include <map>

#include "storm/models/sparse/MarkovAutomaton.h"
#include "storm/models/sparse/Ctmc.h"
#include "storm/utility/constants.h"
#include "storm/utility/vector.h"
#include "storm/utility/bitoperations.h"
#include "storm/utility/ProgressMeasurement.h"
#include "storm/exceptions/UnexpectedException.h"
#include "storm/settings/SettingsManager.h"
#include "storm/logic/AtomicLabelFormula.h"
#include "storm-dft/settings/modules/FaultTreeSettings.h"


namespace storm {
    namespace builder {

        template<typename ValueType, typename StateType>
        ExplicitDFTModelBuilder<ValueType, StateType>::ModelComponents::ModelComponents() : transitionMatrix(), stateLabeling(), markovianStates(), exitRates(), choiceLabeling() {
            // Intentionally left empty.
        }

        template<typename ValueType, typename StateType>
        ExplicitDFTModelBuilder<ValueType, StateType>::MatrixBuilder::MatrixBuilder(bool canHaveNondeterminism) : mappingOffset(0), stateRemapping(), currentRowGroup(0), currentRow(0), canHaveNondeterminism((canHaveNondeterminism)) {
            // Create matrix builder
            builder = storm::storage::SparseMatrixBuilder<ValueType>(0, 0, 0, false, canHaveNondeterminism, 0);
        }

        template<typename ValueType, typename StateType>
        ExplicitDFTModelBuilder<ValueType, StateType>::LabelOptions::LabelOptions(std::vector<std::shared_ptr<const storm::logic::Formula>> properties, bool buildAllLabels) : buildFailLabel(true), buildFailSafeLabel(false), buildAllLabels(buildAllLabels) {
            // Get necessary labels from properties
            std::vector<std::shared_ptr<storm::logic::AtomicLabelFormula const>> atomicLabels;
            for (auto property : properties) {
                property->gatherAtomicLabelFormulas(atomicLabels);
            }
            // Set usage of necessary labels
            for (auto atomic : atomicLabels) {
                std::string label = atomic->getLabel();
                std::size_t foundIndex = label.find("_fail");
                if (foundIndex != std::string::npos) {
                    elementLabels.insert(label.substr(0, foundIndex));
                } else if (label.compare("failed") == 0) {
                    buildFailLabel = true;
                } else if (label.compare("failsafe") == 0) {
                    buildFailSafeLabel = true;
                } else {
                    STORM_LOG_WARN("Label '" << label << "' not known.");
                }
            }
        }

        template<typename ValueType, typename StateType>
        ExplicitDFTModelBuilder<ValueType, StateType>::ExplicitDFTModelBuilder(storm::storage::DFT<ValueType> const& dft, storm::storage::DFTIndependentSymmetries const& symmetries, bool enableDC) :
                dft(dft),
                stateGenerationInfo(std::make_shared<storm::storage::DFTStateGenerationInfo>(dft.buildStateGenerationInfo(symmetries))),
                enableDC(enableDC),
                usedHeuristic(storm::settings::getModule<storm::settings::modules::FaultTreeSettings>().getApproximationHeuristic()),
                generator(dft, *stateGenerationInfo, enableDC, mergeFailedStates),
                matrixBuilder(!generator.isDeterministicModel()),
                stateStorage(dft.stateBitVectorSize()),
                // TODO Matthias: make choosable
                //explorationQueue(dft.nrElements()+1, 0, 1)
                explorationQueue(200, 0, 0.9, false)
        {
            // Intentionally left empty.
            // TODO Matthias: remove again
            usedHeuristic = storm::builder::ApproximationHeuristic::DEPTH;

            // Compute independent subtrees
            if (dft.topLevelType() == storm::storage::DFTElementType::OR) {
                // We only support this for approximation with top level element OR
                for (auto const& child : dft.getGate(dft.getTopLevelIndex())->children()) {
                    // Consider all children of the top level gate
                    std::vector<size_t> isubdft;
                    if (child->nrParents() > 1 || child->hasOutgoingDependencies()) {
                        STORM_LOG_TRACE("child " << child->name() << "does not allow modularisation.");
                        isubdft.clear();
                    } else if (dft.isGate(child->id())) {
                        isubdft = dft.getGate(child->id())->independentSubDft(false);
                    } else {
                        STORM_LOG_ASSERT(dft.isBasicElement(child->id()), "Child is no BE.");
                        if(dft.getBasicElement(child->id())->hasIngoingDependencies()) {
                            STORM_LOG_TRACE("child " << child->name() << "does not allow modularisation.");
                            isubdft.clear();
                        } else {
                            isubdft = {child->id()};
                        }
                    }
                    if(isubdft.empty()) {
                        subtreeBEs.clear();
                        break;
                    } else {
                        // Add new independent subtree
                        std::vector<size_t> subtree;
                        for (size_t id : isubdft) {
                            if (dft.isBasicElement(id)) {
                                subtree.push_back(id);
                            }
                        }
                        subtreeBEs.push_back(subtree);
                    }
                }
            }
            if (subtreeBEs.empty()) {
                // Consider complete tree
                std::vector<size_t> subtree;
                for (size_t id = 0; id < dft.nrElements(); ++id) {
                    if (dft.isBasicElement(id)) {
                        subtree.push_back(id);
                    }
                }
                subtreeBEs.push_back(subtree);
            }

            for (auto tree : subtreeBEs) {
                std::stringstream stream;
                stream << "Subtree: ";
                for (size_t id : tree) {
                    stream << id << ", ";
                }
                STORM_LOG_TRACE(stream.str());
            }
        }

        template<typename ValueType, typename StateType>
        void ExplicitDFTModelBuilder<ValueType, StateType>::buildModel(LabelOptions const& labelOpts, size_t iteration, double approximationThreshold) {
            STORM_LOG_TRACE("Generating DFT state space");

            if (iteration < 1) {
                // Initialize
                modelComponents.markovianStates = storm::storage::BitVector(INITIAL_BITVECTOR_SIZE);

                if(mergeFailedStates) {
                    // Introduce explicit fail state
                    storm::generator::StateBehavior<ValueType, StateType> behavior = generator.createMergeFailedState([this] (DFTStatePointer const& state) {
                        this->failedStateId = newIndex++;
                        matrixBuilder.stateRemapping.push_back(0);
                        return this->failedStateId;
                    } );

                    matrixBuilder.setRemapping(failedStateId);
                    STORM_LOG_ASSERT(!behavior.empty(), "Behavior is empty.");
                    matrixBuilder.newRowGroup();
                    setMarkovian(behavior.begin()->isMarkovian());

                    // Now add self loop.
                    // TODO Matthias: maybe use general method.
                    STORM_LOG_ASSERT(behavior.getNumberOfChoices() == 1, "Wrong number of choices for failed state.");
                    STORM_LOG_ASSERT(behavior.begin()->size() == 1, "Wrong number of transitions for failed state.");
                    std::pair<StateType, ValueType> stateProbabilityPair = *(behavior.begin()->begin());
                    STORM_LOG_ASSERT(stateProbabilityPair.first == failedStateId, "No self loop for failed state.");
                    STORM_LOG_ASSERT(storm::utility::isOne<ValueType>(stateProbabilityPair.second), "Probability for failed state != 1.");
                    matrixBuilder.addTransition(stateProbabilityPair.first, stateProbabilityPair.second);
                    matrixBuilder.finishRow();
                }

                // Build initial state
                this->stateStorage.initialStateIndices = generator.getInitialStates(std::bind(&ExplicitDFTModelBuilder::getOrAddStateIndex, this, std::placeholders::_1));
                STORM_LOG_ASSERT(stateStorage.initialStateIndices.size() == 1, "Only one initial state assumed.");
                initialStateIndex = stateStorage.initialStateIndices[0];
                STORM_LOG_TRACE("Initial state: " << initialStateIndex);
                // Initialize heuristic values for inital state
                STORM_LOG_ASSERT(!statesNotExplored.at(initialStateIndex).second, "Heuristic for initial state is already initialized");
                ExplorationHeuristicPointer heuristic = std::make_shared<ExplorationHeuristic>(initialStateIndex);
                heuristic->markExpand();
                statesNotExplored[initialStateIndex].second = heuristic;
                explorationQueue.push(heuristic);
            } else {
                initializeNextIteration();
            }

            if (approximationThreshold > 0) {
                switch (usedHeuristic) {
                    case storm::builder::ApproximationHeuristic::NONE:
                        // Do not change anything
                        approximationThreshold = dft.nrElements()+10;
                        break;
                    case storm::builder::ApproximationHeuristic::DEPTH:
                        approximationThreshold = iteration + 1;
                        break;
                    case storm::builder::ApproximationHeuristic::PROBABILITY:
                    case storm::builder::ApproximationHeuristic::BOUNDDIFFERENCE:
                        approximationThreshold = 10 * std::pow(2, iteration);
                        break;
                }
            }
            exploreStateSpace(approximationThreshold);

            size_t stateSize = stateStorage.getNumberOfStates() + (mergeFailedStates ? 1 : 0);
            modelComponents.markovianStates.resize(stateSize);
            modelComponents.deterministicModel = generator.isDeterministicModel();

            // Fix the entries in the transition matrix according to the mapping of ids to row group indices
            STORM_LOG_ASSERT(matrixBuilder.getRemapping(initialStateIndex) == initialStateIndex, "Initial state should not be remapped.");
            // TODO Matthias: do not consider all rows?
            STORM_LOG_TRACE("Remap matrix: " << matrixBuilder.stateRemapping << ", offset: " << matrixBuilder.mappingOffset);
            matrixBuilder.remap();

            STORM_LOG_TRACE("State remapping: " << matrixBuilder.stateRemapping);
            STORM_LOG_TRACE("Markovian states: " << modelComponents.markovianStates);
            STORM_LOG_DEBUG("Model has " << stateSize << " states");
            STORM_LOG_DEBUG("Model is " << (generator.isDeterministicModel() ? "deterministic" : "non-deterministic"));

            // Build transition matrix
            modelComponents.transitionMatrix = matrixBuilder.builder.build(stateSize, stateSize);
            if (stateSize <= 15) {
                STORM_LOG_TRACE("Transition matrix: " << std::endl << modelComponents.transitionMatrix);
            } else {
                STORM_LOG_TRACE("Transition matrix: too big to print");
            }

            buildLabeling(labelOpts);
        }

        template<typename ValueType, typename StateType>
        void ExplicitDFTModelBuilder<ValueType, StateType>::initializeNextIteration() {
            STORM_LOG_TRACE("Refining DFT state space");

            // TODO Matthias: should be easier now as skipped states all are at the end of the matrix
            // Push skipped states to explore queue
            // TODO Matthias: remove
            for (auto const& skippedState : skippedStates) {
                statesNotExplored[skippedState.second.first->getId()] = skippedState.second;
                explorationQueue.push(skippedState.second.second);
            }

            // Initialize matrix builder again
            // TODO Matthias: avoid copy
            std::vector<uint_fast64_t> copyRemapping = matrixBuilder.stateRemapping;
            matrixBuilder = MatrixBuilder(!generator.isDeterministicModel());
            matrixBuilder.stateRemapping = copyRemapping;
            StateType nrStates = modelComponents.transitionMatrix.getRowGroupCount();
            STORM_LOG_ASSERT(nrStates == matrixBuilder.stateRemapping.size(), "No. of states does not coincide with mapping size.");

            // Start by creating a remapping from the old indices to the new indices
            std::vector<StateType> indexRemapping = std::vector<StateType>(nrStates, 0);
            auto iterSkipped = skippedStates.begin();
            size_t skippedBefore = 0;
            for (size_t i = 0; i < indexRemapping.size(); ++i) {
                while (iterSkipped != skippedStates.end() && iterSkipped->first <= i) {
                    STORM_LOG_ASSERT(iterSkipped->first < indexRemapping.size(), "Skipped is too high.");
                    ++skippedBefore;
                    ++iterSkipped;
                }
                indexRemapping[i] = i - skippedBefore;
            }

            // Set remapping
            size_t nrExpandedStates = nrStates - skippedBefore;
            matrixBuilder.mappingOffset = nrStates;
            STORM_LOG_TRACE("# expanded states: " << nrExpandedStates);
            StateType skippedIndex = nrExpandedStates;
            std::map<StateType, std::pair<DFTStatePointer, ExplorationHeuristicPointer>> skippedStatesNew;
            for (size_t id = 0; id < matrixBuilder.stateRemapping.size(); ++id) {
                StateType index = matrixBuilder.getRemapping(id);
                auto itFind = skippedStates.find(index);
                if (itFind != skippedStates.end()) {
                    // Set new mapping for skipped state
                    matrixBuilder.stateRemapping[id] = skippedIndex;
                    skippedStatesNew[skippedIndex] = itFind->second;
                    indexRemapping[index] = skippedIndex;
                    ++skippedIndex;
                } else {
                    // Set new mapping for expanded state
                    matrixBuilder.stateRemapping[id] = indexRemapping[index];
                }
            }
            STORM_LOG_TRACE("New state remapping: " << matrixBuilder.stateRemapping);
            std::stringstream ss;
            ss << "Index remapping:" << std::endl;
            for (auto tmp : indexRemapping) {
                ss << tmp << " ";
            }
            STORM_LOG_TRACE(ss.str());

            // Remap markovian states
            storm::storage::BitVector markovianStatesNew = storm::storage::BitVector(modelComponents.markovianStates.size(), true);
            // Iterate over all not set bits
            modelComponents.markovianStates.complement();
            size_t index = modelComponents.markovianStates.getNextSetIndex(0);
            while (index < modelComponents.markovianStates.size()) {
                markovianStatesNew.set(indexRemapping[index], false);
                index = modelComponents.markovianStates.getNextSetIndex(index+1);
            }
            STORM_LOG_ASSERT(modelComponents.markovianStates.size() - modelComponents.markovianStates.getNumberOfSetBits() == markovianStatesNew.getNumberOfSetBits(), "Remapping of markovian states is wrong.");
            STORM_LOG_ASSERT(markovianStatesNew.size() == nrStates, "No. of states does not coincide with markovian size.");
            modelComponents.markovianStates = markovianStatesNew;

            // Build submatrix for expanded states
            // TODO Matthias: only use row groups when necessary
            for (StateType oldRowGroup = 0; oldRowGroup < modelComponents.transitionMatrix.getRowGroupCount(); ++oldRowGroup) {
                if (indexRemapping[oldRowGroup] < nrExpandedStates) {
                    // State is expanded -> copy to new matrix
                    matrixBuilder.newRowGroup();
                    for (StateType oldRow = modelComponents.transitionMatrix.getRowGroupIndices()[oldRowGroup]; oldRow < modelComponents.transitionMatrix.getRowGroupIndices()[oldRowGroup+1]; ++oldRow) {
                        for (typename storm::storage::SparseMatrix<ValueType>::const_iterator itEntry = modelComponents.transitionMatrix.begin(oldRow); itEntry != modelComponents.transitionMatrix.end(oldRow); ++itEntry) {
                            auto itFind = skippedStates.find(itEntry->getColumn());
                            if (itFind != skippedStates.end()) {
                                // Set id for skipped states as we remap it later
                                matrixBuilder.addTransition(matrixBuilder.mappingOffset + itFind->second.first->getId(), itEntry->getValue());
                            } else {
                                // Set newly remapped index for expanded states
                                matrixBuilder.addTransition(indexRemapping[itEntry->getColumn()], itEntry->getValue());
                            }
                        }
                        matrixBuilder.finishRow();
                    }
                }
            }

            skippedStates = skippedStatesNew;

            STORM_LOG_ASSERT(matrixBuilder.getCurrentRowGroup() == nrExpandedStates, "Row group size does not match.");
            skippedStates.clear();
        }

        template<typename ValueType, typename StateType>
        void ExplicitDFTModelBuilder<ValueType, StateType>::exploreStateSpace(double approximationThreshold) {
            size_t nrExpandedStates = 0;
            size_t nrSkippedStates = 0;
            storm::utility::ProgressMeasurement progress("explored states");
            progress.startNewMeasurement(0);
            // TODO Matthias: do not empty queue every time but break before
            while (!explorationQueue.empty()) {
                // Get the first state in the queue
                ExplorationHeuristicPointer currentExplorationHeuristic = explorationQueue.popTop();
                StateType currentId = currentExplorationHeuristic->getId();
                auto itFind = statesNotExplored.find(currentId);
                STORM_LOG_ASSERT(itFind != statesNotExplored.end(), "Id " << currentId << " not found");
                DFTStatePointer currentState = itFind->second.first;
                STORM_LOG_ASSERT(currentExplorationHeuristic == itFind->second.second, "Exploration heuristics do not match");
                STORM_LOG_ASSERT(currentState->getId() == currentId, "Ids do not match");
                // Remove it from the list of not explored states
                statesNotExplored.erase(itFind);
                STORM_LOG_ASSERT(stateStorage.stateToId.contains(currentState->status()), "State is not contained in state storage.");
                STORM_LOG_ASSERT(stateStorage.stateToId.getValue(currentState->status()) == currentId, "Ids of states do not coincide.");

                // Get concrete state if necessary
                if (currentState->isPseudoState()) {
                    // Create concrete state from pseudo state
                    currentState->construct();
                }
                STORM_LOG_ASSERT(!currentState->isPseudoState(), "State is pseudo state.");

                // Remember that the current row group was actually filled with the transitions of a different state
                matrixBuilder.setRemapping(currentId);

                matrixBuilder.newRowGroup();

                // Try to explore the next state
                generator.load(currentState);

                //if (approximationThreshold > 0.0 && nrExpandedStates > approximationThreshold && !currentExplorationHeuristic->isExpand()) {
                if (approximationThreshold > 0.0 && currentExplorationHeuristic->isSkip(approximationThreshold)) {
                    // Skip the current state
                    ++nrSkippedStates;
                    STORM_LOG_TRACE("Skip expansion of state: " << dft.getStateString(currentState));
                    setMarkovian(true);
                    // Add transition to target state with temporary value 0
                    // TODO Matthias: what to do when there is no unique target state?
                    matrixBuilder.addTransition(failedStateId, storm::utility::zero<ValueType>());
                    // Remember skipped state
                    skippedStates[matrixBuilder.getCurrentRowGroup() - 1] = std::make_pair(currentState, currentExplorationHeuristic);
                    matrixBuilder.finishRow();
                } else {
                    // Explore the current state
                    ++nrExpandedStates;
                    storm::generator::StateBehavior<ValueType, StateType> behavior = generator.expand(std::bind(&ExplicitDFTModelBuilder::getOrAddStateIndex, this, std::placeholders::_1));
                    STORM_LOG_ASSERT(!behavior.empty(), "Behavior is empty.");
                    setMarkovian(behavior.begin()->isMarkovian());

                    // Now add all choices.
                    for (auto const& choice : behavior) {
                        // Add the probabilistic behavior to the matrix.
                        for (auto const& stateProbabilityPair : choice) {
                            STORM_LOG_ASSERT(!storm::utility::isZero(stateProbabilityPair.second), "Probability zero.");
                            // Set transition to state id + offset. This helps in only remapping all previously skipped states.
                            matrixBuilder.addTransition(matrixBuilder.mappingOffset + stateProbabilityPair.first, stateProbabilityPair.second);
                            // Set heuristic values for reached states
                            auto iter = statesNotExplored.find(stateProbabilityPair.first);
                            if (iter != statesNotExplored.end()) {
                                // Update heuristic values
                                DFTStatePointer state = iter->second.first;
                                if (!iter->second.second) {
                                    // Initialize heuristic values
                                    ExplorationHeuristicPointer heuristic = std::make_shared<ExplorationHeuristic>(stateProbabilityPair.first, *currentExplorationHeuristic, stateProbabilityPair.second, choice.getTotalMass());
                                    iter->second.second = heuristic;
                                    if (state->hasFailed(dft.getTopLevelIndex()) || state->isFailsafe(dft.getTopLevelIndex()) || state->nrFailableDependencies() > 0 || (state->nrFailableDependencies() == 0 && state->nrFailableBEs() == 0)) {
                                        // Do not skip absorbing state or if reached by dependencies
                                        iter->second.second->markExpand();
                                    }
                                    if (usedHeuristic == storm::builder::ApproximationHeuristic::BOUNDDIFFERENCE) {
                                        // Compute bounds for heuristic now
                                        if (state->isPseudoState()) {
                                            // Create concrete state from pseudo state
                                            state->construct();
                                        }
                                        STORM_LOG_ASSERT(!currentState->isPseudoState(), "State is pseudo state.");

                                        // Initialize bounds
                                        // TODO Mathias: avoid hack
                                        ValueType lowerBound = getLowerBound(state);
                                        ValueType upperBound = getUpperBound(state);
                                        heuristic->setBounds(lowerBound, upperBound);
                                    }

                                    explorationQueue.push(heuristic);
                                } else if (!iter->second.second->isExpand()) {
                                    double oldPriority = iter->second.second->getPriority();
                                    if (iter->second.second->updateHeuristicValues(*currentExplorationHeuristic, stateProbabilityPair.second, choice.getTotalMass())) {
                                        // Update priority queue
                                        explorationQueue.update(iter->second.second, oldPriority);
                                    }
                                }
                            }
                        }
                        matrixBuilder.finishRow();
                    }
                }
                // Output number of currently explored states
                if (nrExpandedStates % 100 == 0) {
                    progress.updateProgress(nrExpandedStates);
                }
            } // end exploration

            STORM_LOG_INFO("Expanded " << nrExpandedStates << " states");
            STORM_LOG_INFO("Skipped " << nrSkippedStates << " states");
            STORM_LOG_ASSERT(nrSkippedStates == skippedStates.size(), "Nr skipped states is wrong");
        }

        template<typename ValueType, typename StateType>
        void ExplicitDFTModelBuilder<ValueType, StateType>::buildLabeling(LabelOptions const& labelOpts) {
            // Build state labeling
            modelComponents.stateLabeling = storm::models::sparse::StateLabeling(modelComponents.transitionMatrix.getRowGroupCount());
            // Initial state
            modelComponents.stateLabeling.addLabel("init");
            STORM_LOG_ASSERT(matrixBuilder.getRemapping(initialStateIndex) == initialStateIndex, "Initial state should not be remapped.");
            modelComponents.stateLabeling.addLabelToState("init", initialStateIndex);
            // Label all states corresponding to their status (failed, failsafe, failed BE)
            if(labelOpts.buildFailLabel) {
                modelComponents.stateLabeling.addLabel("failed");
            }
            if(labelOpts.buildFailSafeLabel) {
                modelComponents.stateLabeling.addLabel("failsafe");
            }

            // Collect labels for all necessary elements
            for (size_t id = 0; id < dft.nrElements(); ++id) {
                std::shared_ptr<storage::DFTElement<ValueType> const> element = dft.getElement(id);
                if (labelOpts.buildAllLabels || labelOpts.elementLabels.count(element->name()) > 0) {
                    modelComponents.stateLabeling.addLabel(element->name() + "_fail");
                }
            }

            // Set labels to states
            if (mergeFailedStates) {
                modelComponents.stateLabeling.addLabelToState("failed", failedStateId);
            }
            for (auto const& stateIdPair : stateStorage.stateToId) {
                storm::storage::BitVector state = stateIdPair.first;
                size_t stateId = matrixBuilder.getRemapping(stateIdPair.second);
                if (!mergeFailedStates && labelOpts.buildFailLabel && dft.hasFailed(state, *stateGenerationInfo)) {
                    modelComponents.stateLabeling.addLabelToState("failed", stateId);
                }
                if (labelOpts.buildFailSafeLabel && dft.isFailsafe(state, *stateGenerationInfo)) {
                    modelComponents.stateLabeling.addLabelToState("failsafe", stateId);
                }
                // Set fail status for each necessary element
                for (size_t id = 0; id < dft.nrElements(); ++id) {
                    std::shared_ptr<storage::DFTElement<ValueType> const> element = dft.getElement(id);
                    if ((labelOpts.buildAllLabels || labelOpts.elementLabels.count(element->name()) > 0) && storm::storage::DFTState<ValueType>::hasFailed(state, stateGenerationInfo->getStateIndex(element->id()))) {
                        modelComponents.stateLabeling.addLabelToState(element->name() + "_fail", stateId);
                    }
                }
            }

            STORM_LOG_TRACE(modelComponents.stateLabeling);
        }

        template<typename ValueType, typename StateType>
        std::shared_ptr<storm::models::sparse::Model<ValueType>> ExplicitDFTModelBuilder<ValueType, StateType>::getModel() {
            STORM_LOG_ASSERT(skippedStates.size() == 0, "Concrete model has skipped states");
            return createModel(false);
        }

        template<typename ValueType, typename StateType>
        std::shared_ptr<storm::models::sparse::Model<ValueType>> ExplicitDFTModelBuilder<ValueType, StateType>::getModelApproximation(bool lowerBound, bool expectedTime) {
            if (expectedTime) {
                // TODO Matthias: handle case with no skipped states
                changeMatrixBound(modelComponents.transitionMatrix, lowerBound);
                return createModel(true);
            } else {
                // Change model for probabilities
                // TODO Matthias: make nicer
                storm::storage::SparseMatrix<ValueType> matrix = modelComponents.transitionMatrix;
                storm::models::sparse::StateLabeling labeling = modelComponents.stateLabeling;
                if (lowerBound) {
                    // Set self loop for lower bound
                    for (auto it = skippedStates.begin(); it != skippedStates.end(); ++it) {
                        auto matrixEntry = matrix.getRow(it->first, 0).begin();
                        STORM_LOG_ASSERT(matrixEntry->getColumn() == failedStateId, "Transition has wrong target state.");
                        STORM_LOG_ASSERT(!it->second.first->isPseudoState(), "State is still pseudo state.");
                        matrixEntry->setValue(storm::utility::one<ValueType>());
                        matrixEntry->setColumn(it->first);
                    }
                } else {
                    // Make skipped states failed states for upper bound
                    storm::storage::BitVector failedStates = modelComponents.stateLabeling.getStates("failed");
                    for (auto it = skippedStates.begin(); it != skippedStates.end(); ++it) {
                        failedStates.set(it->first);
                    }
                    labeling.setStates("failed", failedStates);
                }

                std::shared_ptr<storm::models::sparse::Model<ValueType>> model;
                if (modelComponents.deterministicModel) {
                    model = std::make_shared<storm::models::sparse::Ctmc<ValueType>>(std::move(matrix), std::move(labeling));
                } else {
                    // Build MA
                    // Compute exit rates
                    // TODO Matthias: avoid computing multiple times
                    modelComponents.exitRates = std::vector<ValueType>(modelComponents.markovianStates.size());
                    std::vector<typename storm::storage::SparseMatrix<ValueType>::index_type> indices = matrix.getRowGroupIndices();
                    for (StateType stateIndex = 0; stateIndex < modelComponents.markovianStates.size(); ++stateIndex) {
                        if (modelComponents.markovianStates[stateIndex]) {
                            modelComponents.exitRates[stateIndex] = matrix.getRowSum(indices[stateIndex]);
                        } else {
                            modelComponents.exitRates[stateIndex] = storm::utility::zero<ValueType>();
                        }
                    }
                    STORM_LOG_TRACE("Exit rates: " << modelComponents.exitRates);

                    storm::storage::sparse::ModelComponents<ValueType> maComponents(std::move(matrix), std::move(labeling));
                    maComponents.rateTransitions = true;
                    maComponents.markovianStates = modelComponents.markovianStates;
                    maComponents.exitRates = modelComponents.exitRates;
                    std::shared_ptr<storm::models::sparse::MarkovAutomaton<ValueType>> ma = std::make_shared<storm::models::sparse::MarkovAutomaton<ValueType>>(std::move(maComponents));
                    if (ma->hasOnlyTrivialNondeterminism()) {
                        // Markov automaton can be converted into CTMC
                        // TODO Matthias: change components which were not moved accordingly
                        model = ma->convertToCtmc();
                    } else {
                        model = ma;
                    }

                }

                if (model->getNumberOfStates() <= 15) {
                    STORM_LOG_TRACE("Transition matrix: " << std::endl << model->getTransitionMatrix());
                } else {
                    STORM_LOG_TRACE("Transition matrix: too big to print");
                }
                return model;
            }
        }

        template<typename ValueType, typename StateType>
        std::shared_ptr<storm::models::sparse::Model<ValueType>> ExplicitDFTModelBuilder<ValueType, StateType>::createModel(bool copy) {
            std::shared_ptr<storm::models::sparse::Model<ValueType>> model;

            if (modelComponents.deterministicModel) {
                // Build CTMC
                if (copy) {
                    model = std::make_shared<storm::models::sparse::Ctmc<ValueType>>(modelComponents.transitionMatrix, modelComponents.stateLabeling);
                } else {
                    model = std::make_shared<storm::models::sparse::Ctmc<ValueType>>(std::move(modelComponents.transitionMatrix), std::move(modelComponents.stateLabeling));
                }
            } else {
                // Build MA
                // Compute exit rates
                // TODO Matthias: avoid computing multiple times
                modelComponents.exitRates = std::vector<ValueType>(modelComponents.markovianStates.size());
                std::vector<typename storm::storage::SparseMatrix<ValueType>::index_type> indices = modelComponents.transitionMatrix.getRowGroupIndices();
                for (StateType stateIndex = 0; stateIndex < modelComponents.markovianStates.size(); ++stateIndex) {
                    if (modelComponents.markovianStates[stateIndex]) {
                        modelComponents.exitRates[stateIndex] = modelComponents.transitionMatrix.getRowSum(indices[stateIndex]);
                    } else {
                        modelComponents.exitRates[stateIndex] = storm::utility::zero<ValueType>();
                    }
                }
                STORM_LOG_TRACE("Exit rates: " << modelComponents.exitRates);

                std::shared_ptr<storm::models::sparse::MarkovAutomaton<ValueType>> ma;
                if (copy) {
                    storm::storage::sparse::ModelComponents<ValueType> maComponents(modelComponents.transitionMatrix, modelComponents.stateLabeling);
                    maComponents.rateTransitions = true;
                    maComponents.markovianStates = modelComponents.markovianStates;
                    maComponents.exitRates = modelComponents.exitRates;
                    ma = std::make_shared<storm::models::sparse::MarkovAutomaton<ValueType>>(std::move(maComponents));
                } else {
                    storm::storage::sparse::ModelComponents<ValueType> maComponents(std::move(modelComponents.transitionMatrix), std::move(modelComponents.stateLabeling));
                    maComponents.rateTransitions = true;
                    maComponents.markovianStates = std::move(modelComponents.markovianStates);
                    maComponents.exitRates = std::move(modelComponents.exitRates);
                    ma = std::make_shared<storm::models::sparse::MarkovAutomaton<ValueType>>(std::move(maComponents));
                }
                if (ma->hasOnlyTrivialNondeterminism()) {
                    // Markov automaton can be converted into CTMC
                    // TODO Matthias: change components which were not moved accordingly
                    model = ma->convertToCtmc();
                } else {
                    model = ma;
                }
            }

            if (model->getNumberOfStates() <= 15) {
                STORM_LOG_TRACE("Transition matrix: " << std::endl << model->getTransitionMatrix());
            } else {
                STORM_LOG_TRACE("Transition matrix: too big to print");
            }
            return model;
        }

        template<typename ValueType, typename StateType>
        void ExplicitDFTModelBuilder<ValueType, StateType>::changeMatrixBound(storm::storage::SparseMatrix<ValueType> & matrix, bool lowerBound) const {
            // Set lower bound for skipped states
            for (auto it = skippedStates.begin(); it != skippedStates.end(); ++it) {
                auto matrixEntry = matrix.getRow(it->first, 0).begin();
                STORM_LOG_ASSERT(matrixEntry->getColumn() == failedStateId, "Transition has wrong target state.");
                STORM_LOG_ASSERT(!it->second.first->isPseudoState(), "State is still pseudo state.");

                ExplorationHeuristicPointer heuristic = it->second.second;
                if (storm::utility::isZero(heuristic->getUpperBound())) {
                    // Initialize bounds
                    ValueType lowerBound = getLowerBound(it->second.first);
                    ValueType upperBound = getUpperBound(it->second.first);
                    heuristic->setBounds(lowerBound, upperBound);
                }

                // Change bound
                if (lowerBound) {
                    matrixEntry->setValue(it->second.second->getLowerBound());
                } else {
                    matrixEntry->setValue(it->second.second->getUpperBound());
                }
            }
        }

        template<typename ValueType, typename StateType>
        ValueType ExplicitDFTModelBuilder<ValueType, StateType>::getLowerBound(DFTStatePointer const& state) const {
            // Get the lower bound by considering the failure of all possible BEs
            ValueType lowerBound = storm::utility::zero<ValueType>();
            for (size_t index = 0; index < state->nrFailableBEs(); ++index) {
                lowerBound += state->getFailableBERate(index);
            }
            return lowerBound;
        }

        template<typename ValueType, typename StateType>
        ValueType ExplicitDFTModelBuilder<ValueType, StateType>::getUpperBound(DFTStatePointer const& state) const {
            if (state->hasFailed(dft.getTopLevelIndex())) {
                return storm::utility::zero<ValueType>();
            }

            // Get the upper bound by considering the failure of all BEs
            ValueType upperBound = storm::utility::one<ValueType>();
            ValueType rateSum = storm::utility::zero<ValueType>();

            // Compute for each independent subtree
            for (std::vector<size_t> const& subtree : subtreeBEs) {
                // Get all possible rates
                std::vector<ValueType> rates;
                storm::storage::BitVector coldBEs(subtree.size(), false);
                for (size_t i = 0; i < subtree.size(); ++i) {
                    size_t id = subtree[i];
                    if (state->isOperational(id)) {
                        // Get BE rate
                        ValueType rate = state->getBERate(id);
                        if (storm::utility::isZero<ValueType>(rate)) {
                            // Get active failure rate for cold BE
                            rate = dft.getBasicElement(id)->activeFailureRate();
                            if (storm::utility::isZero<ValueType>(rate)) {
                                // Ignore BE which cannot fail
                                continue;
                            }
                            // Mark BE as cold
                            coldBEs.set(i, true);
                        }
                        rates.push_back(rate);
                        rateSum += rate;
                    }
                }

                STORM_LOG_ASSERT(rates.size() > 0, "No rates failable");

                // Sort rates
                std::sort(rates.begin(), rates.end());
                std::vector<size_t> rateCount(rates.size(), 0);
                size_t lastIndex = 0;
                for (size_t i = 0; i < rates.size(); ++i) {
                    if (rates[lastIndex] != rates[i]) {
                        lastIndex = i;
                    }
                    ++rateCount[lastIndex];
                }

                for (size_t i = 0; i < rates.size(); ++i) {
                    // Cold BEs cannot fail in the first step
                    if (!coldBEs.get(i) && rateCount[i] > 0) {
                        std::iter_swap(rates.begin() + i, rates.end() - 1);
                        // Compute AND MTTF of subtree without current rate and scale with current rate
                        upperBound += rates.back() * rateCount[i] * computeMTTFAnd(rates, rates.size() - 1);
                        // Swap back
                        // TODO try to avoid swapping back
                        std::iter_swap(rates.begin() + i, rates.end() - 1);
                    }
                }
            }

            STORM_LOG_TRACE("Upper bound is " << (rateSum / upperBound) << " for state " << state->getId());
            STORM_LOG_ASSERT(!storm::utility::isZero(upperBound), "Upper bound is 0");
            STORM_LOG_ASSERT(!storm::utility::isZero(rateSum), "State is absorbing");
            return rateSum / upperBound;
        }

        template<typename ValueType, typename StateType>
        ValueType ExplicitDFTModelBuilder<ValueType, StateType>::computeMTTFAnd(std::vector<ValueType> const& rates, size_t size) const {
            ValueType result = storm::utility::zero<ValueType>();
            if (size == 0) {
                return result;
            }

            // TODO Matthias: works only for <64 BEs!
            // WARNING: this code produces wrong results for more than 32 BEs
            /*for (size_t i = 1; i < 4 && i <= rates.size(); ++i) {
                size_t permutation = smallestIntWithNBitsSet(static_cast<size_t>(i));
                ValueType sum = storm::utility::zero<ValueType>();
                do {
                    ValueType permResult = storm::utility::zero<ValueType>();
                    for(size_t j = 0; j < rates.size(); ++j) {
                        if(permutation & static_cast<size_t>(1 << static_cast<size_t>(j))) {
                            // WARNING: if the first bit is set, it also recognizes the 32nd bit as set
                            // TODO: fix
                            permResult += rates[j];
                        }
                    }
                    permutation = nextBitPermutation(permutation);
                    STORM_LOG_ASSERT(!storm::utility::isZero(permResult), "PermResult is 0");
                    sum += storm::utility::one<ValueType>() / permResult;
                } while(permutation < (static_cast<size_t>(1) << rates.size()) && permutation != 0);
                if (i % 2 == 0) {
                    result -= sum;
                } else {
                    result += sum;
                }
            }*/

            // Compute result with permutations of size <= 3
            ValueType one = storm::utility::one<ValueType>();
            for (size_t i1 = 0; i1 < size; ++i1) {
                // + 1/a
                ValueType sum = rates[i1];
                result += one / sum;
                for (size_t i2 = 0; i2 < i1; ++i2) {
                    // - 1/(a+b)
                    ValueType sum2 = sum + rates[i2];
                    result -= one / sum2;
                    for (size_t i3 = 0; i3 < i2; ++i3) {
                        // + 1/(a+b+c)
                        result += one / (sum2 + rates[i3]);
                    }
                }
            }

            STORM_LOG_ASSERT(!storm::utility::isZero(result), "UpperBound is 0");
            return result;
        }

        template<typename ValueType, typename StateType>
        StateType ExplicitDFTModelBuilder<ValueType, StateType>::getOrAddStateIndex(DFTStatePointer const& state) {
            StateType stateId;
            bool changed = false;

            if (stateGenerationInfo->hasSymmetries()) {
                // Order state by symmetry
                STORM_LOG_TRACE("Check for symmetry: " << dft.getStateString(state));
                changed = state->orderBySymmetry();
                STORM_LOG_TRACE("State " << (changed ? "changed to " : "did not change") << (changed ? dft.getStateString(state) : ""));
            }

            if (stateStorage.stateToId.contains(state->status())) {
                // State already exists
                stateId = stateStorage.stateToId.getValue(state->status());
                STORM_LOG_TRACE("State " << dft.getStateString(state) << " with id " << stateId << " already exists");
                if (!changed) {
                    // Check if state is pseudo state
                    // If state is explored already the possible pseudo state was already constructed
                    auto iter = statesNotExplored.find(stateId);
                    if (iter != statesNotExplored.end() && iter->second.first->isPseudoState()) {
                        // Create pseudo state now
                        STORM_LOG_ASSERT(iter->second.first->getId() == stateId, "Ids do not match.");
                        STORM_LOG_ASSERT(iter->second.first->status() == state->status(), "Pseudo states do not coincide.");
                        state->setId(stateId);
                        // Update mapping to map to concrete state now
                        // TODO Matthias: just change pointer?
                        statesNotExplored[stateId] = std::make_pair(state, iter->second.second);
                        // We do not push the new state on the exploration queue as the pseudo state was already pushed
                        STORM_LOG_TRACE("Created pseudo state " << dft.getStateString(state));
                    }
                }
            } else {
                // State does not exist yet
                STORM_LOG_ASSERT(state->isPseudoState() == changed, "State type (pseudo/concrete) wrong.");
                // Create new state
                state->setId(newIndex++);
                stateId = stateStorage.stateToId.findOrAdd(state->status(), state->getId());
                STORM_LOG_ASSERT(stateId == state->getId(), "Ids do not match.");
                // Insert state as not yet explored
                ExplorationHeuristicPointer nullHeuristic;
                statesNotExplored[stateId] = std::make_pair(state, nullHeuristic);
                // Reserve one slot for the new state in the remapping
                matrixBuilder.stateRemapping.push_back(0);
                STORM_LOG_TRACE("New " << (state->isPseudoState() ? "pseudo" : "concrete") << " state: " << dft.getStateString(state));
            }
            return stateId;
        }

        template<typename ValueType, typename StateType>
        void ExplicitDFTModelBuilder<ValueType, StateType>::setMarkovian(bool markovian) {
            if (matrixBuilder.getCurrentRowGroup() > modelComponents.markovianStates.size()) {
                // Resize BitVector
                modelComponents.markovianStates.resize(modelComponents.markovianStates.size() + INITIAL_BITVECTOR_SIZE);
            }
            modelComponents.markovianStates.set(matrixBuilder.getCurrentRowGroup() - 1, markovian);
        }

        template<typename ValueType, typename StateType>
        void ExplicitDFTModelBuilder<ValueType, StateType>::printNotExplored() const {
            std::cout << "states not explored:" << std::endl;
            for (auto it : statesNotExplored) {
                std::cout << it.first << " -> " << dft.getStateString(it.second.first) << std::endl;
            }
        }


        // Explicitly instantiate the class.
        template class ExplicitDFTModelBuilder<double>;

#ifdef STORM_HAVE_CARL
        template class ExplicitDFTModelBuilder<storm::RationalFunction>;
#endif

    } // namespace builder
} // namespace storm


