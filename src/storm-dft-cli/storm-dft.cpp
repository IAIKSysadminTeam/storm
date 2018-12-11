#include "storm-dft/api/storm-dft.h"

#include "storm-dft/settings/DftSettings.h"
#include "storm-dft/settings/modules/DftIOSettings.h"
#include "storm-dft/settings/modules/FaultTreeSettings.h"
#include "storm-dft/settings/modules/DftGspnSettings.h"
#include "storm/settings/modules/IOSettings.h"
#include "storm/settings/modules/ResourceSettings.h"
#include "storm/settings/modules/GeneralSettings.h"


#include "storm-parsers/api/storm-parsers.h"
#include "storm/utility/initialize.h"
#include "storm-cli-utilities/cli.h"

/*!
 * Process commandline options and start computations.
 */
template<typename ValueType>
void processOptions() {
    // Start by setting some urgent options (log levels, resources, etc.)
    storm::cli::setUrgentOptions();

    storm::settings::modules::DftIOSettings const& dftIOSettings = storm::settings::getModule<storm::settings::modules::DftIOSettings>();
    storm::settings::modules::FaultTreeSettings const& faultTreeSettings = storm::settings::getModule<storm::settings::modules::FaultTreeSettings>();
    storm::settings::modules::IOSettings const& ioSettings = storm::settings::getModule<storm::settings::modules::IOSettings>();
    storm::settings::modules::DftGspnSettings const& dftGspnSettings = storm::settings::getModule<storm::settings::modules::DftGspnSettings>();


    if (!dftIOSettings.isDftFileSet() && !dftIOSettings.isDftJsonFileSet()) {
        STORM_LOG_THROW(false, storm::exceptions::InvalidSettingsException, "No input model.");
    }

    // Build DFT from given file
    std::shared_ptr<storm::storage::DFT<ValueType>> dft;
    if (dftIOSettings.isDftJsonFileSet()) {
        STORM_LOG_DEBUG("Loading DFT from file " << dftIOSettings.getDftJsonFilename());
        dft = storm::api::loadDFTJsonFile<ValueType>(dftIOSettings.getDftJsonFilename());
    } else {
        STORM_LOG_DEBUG("Loading DFT from file " << dftIOSettings.getDftFilename());
        dft = storm::api::loadDFTGalileoFile<ValueType>(dftIOSettings.getDftFilename());
    }

    if (dftIOSettings.isDisplayStatsSet()) {
        dft->writeStatsToStream(std::cout);
    }

    if (dftIOSettings.isExportToJson()) {
        // Export to json
        storm::api::exportDFTToJsonFile<ValueType>(*dft, dftIOSettings.getExportJsonFilename());
        return;
    }

    // Check well-formedness of DFT
    std::stringstream stream;
    if (!dft->checkWellFormedness(stream)) {
        STORM_LOG_THROW(false, storm::exceptions::WrongFormatException, "DFT is not well-formed: " << stream.str());
    }

    if (dftGspnSettings.isTransformToGspn()) {
        // Transform to GSPN
        std::pair<std::shared_ptr<storm::gspn::GSPN>, uint64_t> pair = storm::api::transformToGSPN(*dft);
        std::shared_ptr<storm::gspn::GSPN> gspn = pair.first;
        uint64_t toplevelFailedPlace = pair.second;

        // Export
        storm::api::handleGSPNExportSettings(*gspn);

        // Transform to Jani
        std::shared_ptr<storm::jani::Model> model = storm::api::transformToJani(*gspn, toplevelFailedPlace);
        return;
    }


#ifdef STORM_HAVE_Z3
    if (faultTreeSettings.solveWithSMT()) {
        // Solve with SMT
        STORM_LOG_DEBUG("Running DFT analysis with use of SMT");
        storm::api::exportDFTToSMT(*dft, "test.smt2");
        STORM_LOG_THROW(false, storm::exceptions::NotSupportedException, "Only exported to SMT file 'test.smt2' but analysis is not supported.");
        return;
    }
#endif

    // Set min or max
    std::string optimizationDirection = "min";
    if (dftIOSettings.isComputeMaximalValue()) {
        STORM_LOG_THROW(!dftIOSettings.isComputeMinimalValue(), storm::exceptions::InvalidSettingsException, "Cannot compute minimal and maximal values at the same time.");
        optimizationDirection = "max";
    }

    // Construct properties to analyse
    std::vector<std::string> properties;
    if (ioSettings.isPropertySet()) {
        properties.push_back(ioSettings.getProperty());
    }
    if (dftIOSettings.usePropExpectedTime()) {
        properties.push_back("T" + optimizationDirection + "=? [F \"failed\"]");
    }
    if (dftIOSettings.usePropProbability()) {
        properties.push_back("P" + optimizationDirection + "=? [F \"failed\"]");
    }
    if (dftIOSettings.usePropTimebound()) {
        std::stringstream stream;
        stream << "P" << optimizationDirection << "=? [F<=" << dftIOSettings.getPropTimebound() << " \"failed\"]";
        properties.push_back(stream.str());
    }
    if (dftIOSettings.usePropTimepoints()) {
        for (double timepoint : dftIOSettings.getPropTimepoints()) {
            std::stringstream stream;
            stream << "P" << optimizationDirection << "=? [F<=" << timepoint << " \"failed\"]";
            properties.push_back(stream.str());
        }
    }

    // Build properties
    STORM_LOG_THROW(!properties.empty(), storm::exceptions::InvalidSettingsException, "No property given.");
    std::string propString = properties[0];
    for (size_t i = 1; i < properties.size(); ++i) {
        propString += ";" + properties[i];
    }
    std::vector<std::shared_ptr<storm::logic::Formula const>> props = storm::api::extractFormulasFromProperties(storm::api::parseProperties(propString));
    STORM_LOG_ASSERT(props.size() > 0, "No properties found.");

    // Carry out the actual analysis
    if (faultTreeSettings.isApproximationErrorSet()) {
        // Approximate analysis
        storm::api::analyzeDFTApprox<ValueType>(*dft, props, faultTreeSettings.useSymmetryReduction(), faultTreeSettings.useModularisation(), !faultTreeSettings.isDisableDC(),
                                                faultTreeSettings.getApproximationError(), true);
    } else {
        storm::api::analyzeDFT<ValueType>(*dft, props, faultTreeSettings.useSymmetryReduction(), faultTreeSettings.useModularisation(), !faultTreeSettings.isDisableDC(), true);
    }
}

/*!
 * Entry point for Storm-DFT.
 *
 * @param argc The argc argument of main().
 * @param argv The argv argument of main().
 * @return Return code, 0 if successfull, not 0 otherwise.
 */
int main(const int argc, const char** argv) {
    try {
        storm::utility::setUp();
        storm::cli::printHeader("Storm-dft", argc, argv);
        storm::settings::initializeDftSettings("Storm-dft", "storm-dft");

        storm::utility::Stopwatch totalTimer(true);
        if (!storm::cli::parseOptions(argc, argv)) {
            return -1;
        }

        storm::settings::modules::GeneralSettings const& generalSettings = storm::settings::getModule<storm::settings::modules::GeneralSettings>();
        if (generalSettings.isParametricSet()) {
            processOptions<storm::RationalFunction>();
        } else {
            processOptions<double>();
        }

        totalTimer.stop();
        if (storm::settings::getModule<storm::settings::modules::ResourceSettings>().isPrintTimeAndMemorySet()) {
            storm::cli::printTimeAndMemoryStatistics(totalTimer.getTimeInMilliseconds());
        }

        // All operations have now been performed, so we clean up everything and terminate.
        storm::utility::cleanUp();
        return 0;
    } catch (storm::exceptions::BaseException const& exception) {
        STORM_LOG_ERROR("An exception caused Storm-DFT to terminate. The message of the exception is: " << exception.what());
        return 1;
    } catch (std::exception const& exception) {
        STORM_LOG_ERROR("An unexpected exception occurred and caused Storm-DFT to terminate. The message of this exception is: " << exception.what());
        return 2;
    }
}
