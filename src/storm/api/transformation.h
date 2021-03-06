#pragma once

#include "storm/transformer/ContinuousToDiscreteTimeModelTransformer.h"
#include "storm/transformer/SymbolicToSparseTransformer.h"

#include "storm/utility/macros.h"
#include "storm/utility/builder.h"
#include "storm/exceptions/InvalidOperationException.h"
#include "storm/exceptions/NotSupportedException.h"

namespace storm {
    namespace api {
        
        /*!
         * Transforms the given continuous model to a discrete time model.
         * If such a transformation does not preserve one of the given formulas, a warning is issued.
         */
        template <typename ValueType>
        std::pair<std::shared_ptr<storm::models::sparse::Model<ValueType>>, std::vector<std::shared_ptr<storm::logic::Formula const>>>  transformContinuousToDiscreteTimeSparseModel(std::shared_ptr<storm::models::sparse::Model<ValueType>> const& model, std::vector<std::shared_ptr<storm::logic::Formula const>> const& formulas) {
            
            storm::transformer::ContinuousToDiscreteTimeModelTransformer<ValueType> transformer;
            
            std::string timeRewardName = "_time";
            while(model->hasRewardModel(timeRewardName)) {
                timeRewardName += "_";
            }
            auto newFormulas = transformer.checkAndTransformFormulas(formulas, timeRewardName);
            STORM_LOG_WARN_COND(newFormulas.size() == formulas.size(), "Transformation of a " << model->getType() << " to a discrete time model does not preserve all properties.");
            
            if (model->isOfType(storm::models::ModelType::Ctmc)) {
                return std::make_pair(transformer.transform(*model->template as<storm::models::sparse::Ctmc<ValueType>>(), timeRewardName), newFormulas);
            } else if (model->isOfType(storm::models::ModelType::MarkovAutomaton)) {
                return std::make_pair(transformer.transform(*model->template as<storm::models::sparse::MarkovAutomaton<ValueType>>(), timeRewardName), newFormulas);
            } else {
                STORM_LOG_THROW(false, storm::exceptions::NotSupportedException, "Transformation of a " << model->getType() << " to a discrete time model is not supported");
            }
            return std::make_pair(nullptr, newFormulas);;
        }

        /*!
         * Transforms the given continuous model to a discrete time model IN PLACE.
         * This means that the input continuous time model is replaced by the new discrete time model.
         * If such a transformation does not preserve one of the given formulas, an error is issued.
         */
        template <typename ValueType>
        std::pair<std::shared_ptr<storm::models::sparse::Model<ValueType>>, std::vector<std::shared_ptr<storm::logic::Formula const>>> transformContinuousToDiscreteTimeSparseModel(storm::models::sparse::Model<ValueType>&& model, std::vector<std::shared_ptr<storm::logic::Formula const>> const& formulas) {
            
            storm::transformer::ContinuousToDiscreteTimeModelTransformer<ValueType> transformer;
            
             std::string timeRewardName = "_time";
            while(model.hasRewardModel(timeRewardName)) {
                timeRewardName += "_";
            }
            auto newFormulas = transformer.checkAndTransformFormulas(formulas, timeRewardName);
            STORM_LOG_WARN_COND(newFormulas.size() == formulas.size(), "Transformation of a " << model.getType() << " to a discrete time model does not preserve all properties.");
           
            if (model.isOfType(storm::models::ModelType::Ctmc)) {
                return std::make_pair(transformer.transform(std::move(*model.template as<storm::models::sparse::Ctmc<ValueType>>()), timeRewardName), newFormulas);
            } else if (model.isOfType(storm::models::ModelType::MarkovAutomaton)) {
                return std::make_pair(transformer.transform(std::move(*model.template as<storm::models::sparse::MarkovAutomaton<ValueType>>()), timeRewardName), newFormulas);
            } else {
                STORM_LOG_THROW(false, storm::exceptions::NotSupportedException, "Transformation of a " << model.getType() << " to a discrete time model is not supported.");
            }
            return std::make_pair(nullptr, newFormulas);;
            
        }
        
        /*!
         * Transforms the given symbolic model to a sparse model.
         */
        template<storm::dd::DdType Type, typename ValueType>
        std::shared_ptr<storm::models::sparse::Model<ValueType>> transformSymbolicToSparseModel(std::shared_ptr<storm::models::symbolic::Model<Type, ValueType>> const& symbolicModel, std::vector<std::shared_ptr<storm::logic::Formula const>> const& formulas = std::vector<std::shared_ptr<storm::logic::Formula const>>()) {
            switch (symbolicModel->getType()) {
                case storm::models::ModelType::Dtmc:
                    return storm::transformer::SymbolicDtmcToSparseDtmcTransformer<Type, ValueType>().translate(*symbolicModel->template as<storm::models::symbolic::Dtmc<Type, ValueType>>(), formulas);
                case storm::models::ModelType::Mdp:
                    return storm::transformer::SymbolicMdpToSparseMdpTransformer<Type, ValueType>::translate(*symbolicModel->template as<storm::models::symbolic::Mdp<Type, ValueType>>(), formulas);
                case storm::models::ModelType::Ctmc:
                    return storm::transformer::SymbolicCtmcToSparseCtmcTransformer<Type, ValueType>::translate(*symbolicModel->template as<storm::models::symbolic::Ctmc<Type, ValueType>>(), formulas);
                default:
                    STORM_LOG_THROW(false, storm::exceptions::NotSupportedException, "Transformation of symbolic " << symbolicModel->getType() << " to sparse model is not supported.");
            }
            return nullptr;
        }
        
        template <typename ValueType>
        std::shared_ptr<storm::models::sparse::Model<ValueType>> transformToNondeterministicModel(storm::models::sparse::Model<ValueType>&& model) {
            storm::storage::sparse::ModelComponents<ValueType> components(std::move(model.getTransitionMatrix()), std::move(model.getStateLabeling()), std::move(model.getRewardModels()));
            components.choiceLabeling = std::move(model.getOptionalChoiceLabeling());
            components.stateValuations = std::move(model.getOptionalStateValuations());
            components.choiceOrigins = std::move(model.getOptionalChoiceOrigins());
            if (model.isOfType(storm::models::ModelType::Dtmc)) {
                return storm::utility::builder::buildModelFromComponents(storm::models::ModelType::Mdp, std::move(components));
            } else if (model.isOfType(storm::models::ModelType::Ctmc)) {
                components.rateTransitions = true;
                components.markovianStates = storm::storage::BitVector(components.transitionMatrix.getRowGroupCount(), true);
                return storm::utility::builder::buildModelFromComponents(storm::models::ModelType::MarkovAutomaton, std::move(components));
            } else {
                STORM_LOG_THROW(false, storm::exceptions::InvalidOperationException, "Cannot transform model of type " << model.getType() << " to a nondeterministic model.");
            }
        }
        
    }
}
