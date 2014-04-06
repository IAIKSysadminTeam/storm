/*
 * Program.cpp
 *
 *  Created on: 12.01.2013
 *      Author: Christian Dehnert
 */

#include <sstream>
#include <iostream>

#include "Program.h"
#include "exceptions/InvalidArgumentException.h"
#include "src/exceptions/OutOfRangeException.h"

#include "log4cplus/logger.h"
#include "log4cplus/loggingmacros.h"
extern log4cplus::Logger logger;

namespace storm {
    namespace ir {
        
        Program::Program() : modelType(UNDEFINED), booleanUndefinedConstantExpressions(), integerUndefinedConstantExpressions(), doubleUndefinedConstantExpressions(), modules(), rewards(), actions(), actionsToModuleIndexMap(), variableToModuleIndexMap() {
            // Nothing to do here.
        }
        
        Program::Program(ModelType modelType,
                         std::map<std::string, std::unique_ptr<storm::ir::expressions::BooleanConstantExpression>> const& booleanUndefinedConstantExpressions,
                         std::map<std::string, std::unique_ptr<storm::ir::expressions::IntegerConstantExpression>> const& integerUndefinedConstantExpressions,
                         std::map<std::string, std::unique_ptr<storm::ir::expressions::DoubleConstantExpression>> const& doubleUndefinedConstantExpressions,
                         std::vector<BooleanVariable> const& globalBooleanVariables,
                         std::vector<IntegerVariable> const& globalIntegerVariables,
                         std::map<std::string, uint_fast64_t> const& globalBooleanVariableToIndexMap,
                         std::map<std::string, uint_fast64_t> const& globalIntegerVariableToIndexMap,
                         std::vector<storm::ir::Module> const& modules,
                         std::map<std::string, storm::ir::RewardModel> const& rewards,
                         std::map<std::string, std::unique_ptr<storm::ir::expressions::BaseExpression>> const& labels)
        : modelType(modelType), globalBooleanVariables(globalBooleanVariables), globalIntegerVariables(globalIntegerVariables),
        globalBooleanVariableToIndexMap(globalBooleanVariableToIndexMap), globalIntegerVariableToIndexMap(globalIntegerVariableToIndexMap),
        modules(modules), rewards(rewards), actionsToModuleIndexMap(), variableToModuleIndexMap() {
            
            // Perform a deep-copy of the maps.
            for (auto const& booleanUndefinedConstant : booleanUndefinedConstantExpressions) {
                this->booleanUndefinedConstantExpressions[booleanUndefinedConstant.first] = std::unique_ptr<storm::ir::expressions::BooleanConstantExpression>(new storm::ir::expressions::BooleanConstantExpression(*booleanUndefinedConstant.second));
            }
            for (auto const& integerUndefinedConstant : integerUndefinedConstantExpressions) {
                this->integerUndefinedConstantExpressions[integerUndefinedConstant.first] = std::unique_ptr<storm::ir::expressions::IntegerConstantExpression>(new storm::ir::expressions::IntegerConstantExpression(*integerUndefinedConstant.second));
            }
            for (auto const& doubleUndefinedConstant : doubleUndefinedConstantExpressions) {
                this->doubleUndefinedConstantExpressions[doubleUndefinedConstant.first] = std::unique_ptr<storm::ir::expressions::DoubleConstantExpression>(new storm::ir::expressions::DoubleConstantExpression(*doubleUndefinedConstant.second));
            }
            for (auto const& label : labels) {
                this->labels[label.first] = label.second->clone();
            }
            
            // Now build the mapping from action names to module indices so that the lookup can later be performed quickly.
            for (unsigned int moduleIndex = 0; moduleIndex < this->modules.size(); moduleIndex++) {
                Module const& module = this->modules[moduleIndex];
                
                for (auto const& action : module.getActions()) {
                    if (this->actionsToModuleIndexMap.count(action) == 0) {
                        this->actionsToModuleIndexMap[action] = std::set<uint_fast64_t>();
                    }
                    this->actionsToModuleIndexMap[action].insert(moduleIndex);
                    this->actions.insert(action);
                }
                
                // Put in the appropriate entries for the mapping from variable names to module index.
                for (uint_fast64_t booleanVariableIndex = 0; booleanVariableIndex < module.getNumberOfBooleanVariables(); ++booleanVariableIndex) {
                    this->variableToModuleIndexMap[module.getBooleanVariable(booleanVariableIndex).getName()] = moduleIndex;
                }
                for (uint_fast64_t integerVariableIndex = 0; integerVariableIndex < module.getNumberOfIntegerVariables(); ++integerVariableIndex) {
                    this->variableToModuleIndexMap[module.getIntegerVariable(integerVariableIndex).getName()] = moduleIndex;
                }
            }
        }
        
        Program::Program(Program const& otherProgram) : modelType(otherProgram.modelType), globalBooleanVariables(otherProgram.globalBooleanVariables),
        globalIntegerVariables(otherProgram.globalIntegerVariables), globalBooleanVariableToIndexMap(otherProgram.globalBooleanVariableToIndexMap),
        globalIntegerVariableToIndexMap(otherProgram.globalIntegerVariableToIndexMap), modules(otherProgram.modules), rewards(otherProgram.rewards),
        actions(otherProgram.actions), actionsToModuleIndexMap(), variableToModuleIndexMap() {
            // Perform deep-copy of the maps.
            for (auto const& booleanUndefinedConstant : otherProgram.booleanUndefinedConstantExpressions) {
                this->booleanUndefinedConstantExpressions[booleanUndefinedConstant.first] = std::unique_ptr<storm::ir::expressions::BooleanConstantExpression>(new storm::ir::expressions::BooleanConstantExpression(*booleanUndefinedConstant.second));
            }
            for (auto const& integerUndefinedConstant : otherProgram.integerUndefinedConstantExpressions) {
                this->integerUndefinedConstantExpressions[integerUndefinedConstant.first] = std::unique_ptr<storm::ir::expressions::IntegerConstantExpression>(new storm::ir::expressions::IntegerConstantExpression(*integerUndefinedConstant.second));
            }
            for (auto const& doubleUndefinedConstant : otherProgram.doubleUndefinedConstantExpressions) {
                this->doubleUndefinedConstantExpressions[doubleUndefinedConstant.first] = std::unique_ptr<storm::ir::expressions::DoubleConstantExpression>(new storm::ir::expressions::DoubleConstantExpression(*doubleUndefinedConstant.second));
            }
            for (auto const& label : otherProgram.labels) {
                this->labels[label.first] = label.second->clone();
            }
        }
        
        Program& Program::operator=(Program const& otherProgram) {
            if (this != &otherProgram) {
                this->modelType = otherProgram.modelType;
                this->globalBooleanVariables = otherProgram.globalBooleanVariables;
                this->globalIntegerVariables = otherProgram.globalIntegerVariables;
                this->globalBooleanVariableToIndexMap = otherProgram.globalBooleanVariableToIndexMap;
                this->globalIntegerVariableToIndexMap = otherProgram.globalIntegerVariableToIndexMap;
                this->modules = otherProgram.modules;
                this->rewards = otherProgram.rewards;
                this->actions = otherProgram.actions;
                this->actionsToModuleIndexMap = otherProgram.actionsToModuleIndexMap;
                this->variableToModuleIndexMap = otherProgram.variableToModuleIndexMap;
                
                // Perform deep-copy of the maps.
                for (auto const& booleanUndefinedConstant : otherProgram.booleanUndefinedConstantExpressions) {
                    this->booleanUndefinedConstantExpressions[booleanUndefinedConstant.first] = std::unique_ptr<storm::ir::expressions::BooleanConstantExpression>(new storm::ir::expressions::BooleanConstantExpression(*booleanUndefinedConstant.second));
                }
                for (auto const& integerUndefinedConstant : otherProgram.integerUndefinedConstantExpressions) {
                    this->integerUndefinedConstantExpressions[integerUndefinedConstant.first] = std::unique_ptr<storm::ir::expressions::IntegerConstantExpression>(new storm::ir::expressions::IntegerConstantExpression(*integerUndefinedConstant.second));
                }
                for (auto const& doubleUndefinedConstant : otherProgram.doubleUndefinedConstantExpressions) {
                    this->doubleUndefinedConstantExpressions[doubleUndefinedConstant.first] = std::unique_ptr<storm::ir::expressions::DoubleConstantExpression>(new storm::ir::expressions::DoubleConstantExpression(*doubleUndefinedConstant.second));
                }
                for (auto const& label : otherProgram.labels) {
                    this->labels[label.first] = label.second->clone();
                }
            }
            
            return *this;
        }
        
        Program::ModelType Program::getModelType() const {
            return modelType;
        }
        
        std::string Program::toString() const {
            std::stringstream result;
            switch (modelType) {
                case UNDEFINED: result << "undefined"; break;
                case DTMC: result << "dtmc"; break;
                case CTMC: result << "ctmc"; break;
                case MDP: result << "mdp"; break;
                case CTMDP: result << "ctmdp"; break;
            }
            result << std::endl;
            
            for (auto const& element : booleanUndefinedConstantExpressions) {
                result << "const bool " << element.second->toString() << ";" << std::endl;
            }
            for (auto const& element : integerUndefinedConstantExpressions) {
                result << "const int " << element.second->toString() << ";" << std::endl;
            }
            for (auto const& element : doubleUndefinedConstantExpressions) {
                result << "const double " << element.second->toString() << ";" << std::endl;
            }
            result << std::endl;
            
            for (auto const& element : globalBooleanVariables) {
                result << "global " << element.toString() << std::endl;
            }
            for (auto const& element : globalIntegerVariables) {
                result << "global " << element.toString() << std::endl;
            }
            result << std::endl;
            
            for (auto const& module : modules) {
                result << module.toString() << std::endl;
            }
            
            for (auto const& rewardModel : rewards) {
                result << rewardModel.second.toString() << std::endl;
            }
            
            for (auto const& label : labels) {
                result << "label \"" << label.first << "\" = " << label.second->toString() <<";" << std::endl;
            }
            
            return result.str();
        }
        
        storm::ir::BooleanVariable const& Program::getGlobalBooleanVariable(uint_fast64_t index) const {
            return this->globalBooleanVariables[index];
        }
        
        storm::ir::IntegerVariable const& Program::getGlobalIntegerVariable(uint_fast64_t index) const {
            return this->globalIntegerVariables[index];
        }
        
        uint_fast64_t Program::getNumberOfModules() const {
            return this->modules.size();
        }
        
        storm::ir::Module const& Program::getModule(uint_fast64_t index) const {
            return this->modules[index];
        }
        
        std::set<std::string> const& Program::getActions() const {
            return this->actions;
        }
        
        std::set<uint_fast64_t> const& Program::getModulesByAction(std::string const& action) const {
            auto actionModuleSetPair = this->actionsToModuleIndexMap.find(action);
            if (actionModuleSetPair == this->actionsToModuleIndexMap.end()) {
                LOG4CPLUS_ERROR(logger, "Action name '" << action << "' does not exist.");
                throw storm::exceptions::OutOfRangeException() << "Action name '" << action << "' does not exist.";
            }
            return actionModuleSetPair->second;
        }
        
        uint_fast64_t Program::getModuleIndexForVariable(std::string const& variableName) const {
            auto variableNameToModuleIndexPair = this->variableToModuleIndexMap.find(variableName);
            if (variableNameToModuleIndexPair != this->variableToModuleIndexMap.end()) {
                return variableNameToModuleIndexPair->second;
            }
            throw storm::exceptions::OutOfRangeException() << "Variable '" << variableName << "' does not exist.";
        }
        
        uint_fast64_t Program::getNumberOfGlobalBooleanVariables() const {
            return this->globalBooleanVariables.size();
        }
        
        uint_fast64_t Program::getNumberOfGlobalIntegerVariables() const {
            return this->globalIntegerVariables.size();
        }
        
        storm::ir::RewardModel const& Program::getRewardModel(std::string const& name) const {
            auto nameRewardModelPair = this->rewards.find(name);
            if (nameRewardModelPair == this->rewards.end()) {
                LOG4CPLUS_ERROR(logger, "Reward model '" << name << "' does not exist.");
                throw storm::exceptions::OutOfRangeException() << "Reward model '" << name << "' does not exist.";
            }
            return nameRewardModelPair->second;
        }
        
        std::map<std::string, std::unique_ptr<storm::ir::expressions::BaseExpression>> const& Program::getLabels() const {
            return this->labels;
        }
        
        bool Program::hasUndefinedBooleanConstant(std::string const& constantName) const {
            return this->booleanUndefinedConstantExpressions.find(constantName) != this->booleanUndefinedConstantExpressions.end();
        }
        
        std::unique_ptr<storm::ir::expressions::BooleanConstantExpression> const& Program::getUndefinedBooleanConstantExpression(std::string const& constantName) const {
            auto constantExpressionPair = this->booleanUndefinedConstantExpressions.find(constantName);
            if (constantExpressionPair != this->booleanUndefinedConstantExpressions.end()) {
                return constantExpressionPair->second;
            } else {
                throw storm::exceptions::InvalidArgumentException() << "Unknown undefined boolean constant " << constantName << ".";
            }
        }
        
        bool Program::hasUndefinedIntegerConstant(std::string const& constantName) const {
            return this->integerUndefinedConstantExpressions.find(constantName) != this->integerUndefinedConstantExpressions.end();
        }
        
        std::unique_ptr<storm::ir::expressions::IntegerConstantExpression> const& Program::getUndefinedIntegerConstantExpression(std::string const& constantName) const {
            auto constantExpressionPair = this->integerUndefinedConstantExpressions.find(constantName);
            if (constantExpressionPair != this->integerUndefinedConstantExpressions.end()) {
                return constantExpressionPair->second;
            } else {
                throw storm::exceptions::InvalidArgumentException() << "Unknown undefined integer constant " << constantName << ".";
            }
        }
        
        bool Program::hasUndefinedDoubleConstant(std::string const& constantName) const {
            return this->doubleUndefinedConstantExpressions.find(constantName) != this->doubleUndefinedConstantExpressions.end();
        }
        
        std::unique_ptr<storm::ir::expressions::DoubleConstantExpression> const& Program::getUndefinedDoubleConstantExpression(std::string const& constantName) const {
            auto constantExpressionPair = this->doubleUndefinedConstantExpressions.find(constantName);
            if (constantExpressionPair != this->doubleUndefinedConstantExpressions.end()) {
                return constantExpressionPair->second;
            } else {
                throw storm::exceptions::InvalidArgumentException() << "Unknown undefined double constant " << constantName << ".";
            }
        }
        
        std::map<std::string, std::unique_ptr<storm::ir::expressions::BooleanConstantExpression>> const& Program::getBooleanUndefinedConstantExpressionsMap() const {
            return this->booleanUndefinedConstantExpressions;
        }
        
        std::map<std::string, std::unique_ptr<storm::ir::expressions::IntegerConstantExpression>> const& Program::getIntegerUndefinedConstantExpressionsMap() const {
            return this->integerUndefinedConstantExpressions;
        }
        
        std::map<std::string, std::unique_ptr<storm::ir::expressions::DoubleConstantExpression>> const& Program::getDoubleUndefinedConstantExpressionsMap() const {
            return this->doubleUndefinedConstantExpressions;
        }
        
        uint_fast64_t Program::getGlobalIndexOfBooleanVariable(std::string const& variableName) const {
            return this->globalBooleanVariableToIndexMap.at(variableName);
        }
        
        uint_fast64_t Program::getGlobalIndexOfIntegerVariable(std::string const& variableName) const {
            return this->globalIntegerVariableToIndexMap.at(variableName);
        }
        
        void Program::restrictCommands(boost::container::flat_set<uint_fast64_t> const& indexSet) {
            for (auto& module : modules) {
                module.restrictCommands(indexSet);
            }
        }
        
    } // namespace ir
} // namepsace storm