#include "storm/environment/modelchecker/MultiObjectiveModelCheckerEnvironment.h"

#include "storm/settings/SettingsManager.h"
#include "storm/settings/modules/MultiObjectiveSettings.h"
#include "storm/utility/constants.h"
#include "storm/utility/macros.h"

namespace storm {
        
    MultiObjectiveModelCheckerEnvironment::MultiObjectiveModelCheckerEnvironment() {
        auto const& multiobjectiveSettings = storm::settings::getModule<storm::settings::modules::MultiObjectiveSettings>();
        method = multiobjectiveSettings.getMultiObjectiveMethod();
        if (multiobjectiveSettings.isExportPlotSet()) {
            plotPathUnderApprox = multiobjectiveSettings.getExportPlotDirectory() + "underapproximation.csv";
            plotPathOverApprox = multiobjectiveSettings.getExportPlotDirectory() + "overapproximation.csv";
            plotPathParetoPoints = multiobjectiveSettings.getExportPlotDirectory() + "paretopoints.csv";
        }
        
        precision = storm::utility::convertNumber<storm::RationalNumber>(multiobjectiveSettings.getPrecision());
        if (multiobjectiveSettings.isMaxStepsSet()) {
            maxSteps = multiobjectiveSettings.getMaxSteps();
        }
    }
    
    MultiObjectiveModelCheckerEnvironment::~MultiObjectiveModelCheckerEnvironment() {
        // Intentionally left empty
    }
    
    storm::modelchecker::multiobjective::MultiObjectiveMethod const& MultiObjectiveModelCheckerEnvironment::getMethod() const {
        return this->method;
    }
    
    void MultiObjectiveModelCheckerEnvironment::setMethod(storm::modelchecker::multiobjective::MultiObjectiveMethod value) {
        this->method = value;
    }
    
    bool MultiObjectiveModelCheckerEnvironment::isExportPlotSet() const {
        return this->plotPathUnderApprox.is_initialized() || this->plotPathOverApprox.is_initialized() || this->plotPathParetoPoints.is_initialized();
    }
    
    boost::optional<std::string> MultiObjectiveModelCheckerEnvironment::getPlotPathUnderApproximation() const {
        return plotPathUnderApprox;
    }
    
    void MultiObjectiveModelCheckerEnvironment::setPlotPathUnderApproximation(std::string const& path) {
        plotPathUnderApprox = path;
    }
    
    void MultiObjectiveModelCheckerEnvironment::unsetPlotPathUnderApproximation() {
        plotPathUnderApprox = boost::none;
    }
    
    boost::optional<std::string> MultiObjectiveModelCheckerEnvironment::getPlotPathOverApproximation() const {
        return plotPathOverApprox;
    }
    
    void MultiObjectiveModelCheckerEnvironment::setPlotPathOverApproximation(std::string const& path) {
        plotPathOverApprox = path;
    }
    
    void MultiObjectiveModelCheckerEnvironment::unsetPlotPathOverApproximation() {
        plotPathOverApprox = boost::none;
    }
    
    boost::optional<std::string> MultiObjectiveModelCheckerEnvironment::getPlotPathParetoPoints() const {
        return plotPathParetoPoints;
    }
    
    void MultiObjectiveModelCheckerEnvironment::setPlotPathParetoPoints(std::string const& path) {
        plotPathParetoPoints = path;
    }
    
    void MultiObjectiveModelCheckerEnvironment::unsetPlotPathParetoPoints() {
        plotPathParetoPoints = boost::none;
    }
    
    storm::RationalNumber const& MultiObjectiveModelCheckerEnvironment::getPrecision() const {
        return precision;
    }
    
    void MultiObjectiveModelCheckerEnvironment::setPrecision(storm::RationalNumber const& value) {
        precision = value;
    }
    
    bool MultiObjectiveModelCheckerEnvironment::isMaxStepsSet() const {
        return maxSteps.is_initialized();
    }
    
    uint64_t const& MultiObjectiveModelCheckerEnvironment::getMaxSteps() const {
        return maxSteps.get();
    }
    
    void MultiObjectiveModelCheckerEnvironment::setMaxSteps(uint64_t const& value) {
        maxSteps = value;
    }
    
    void MultiObjectiveModelCheckerEnvironment::unsetMaxSteps() {
        maxSteps = boost::none;
    }
}