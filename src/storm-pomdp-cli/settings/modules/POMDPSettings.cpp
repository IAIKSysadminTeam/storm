#include "storm-pomdp-cli/settings/modules/POMDPSettings.h"

#include "storm/settings/SettingsManager.h"
#include "storm/settings/SettingMemento.h"
#include "storm/settings/Option.h"
#include "storm/settings/OptionBuilder.h"
#include "storm/settings/ArgumentBuilder.h"

#include "storm/exceptions/InvalidArgumentException.h"

namespace storm {
    namespace settings {
        namespace modules {
            
            const std::string POMDPSettings::moduleName = "pomdp";
            const std::string exportAsParametricModelOption = "parametric-drn";
            const std::string qualitativeReductionOption = "qualitativereduction";
            const std::string analyzeUniqueObservationsOption = "uniqueobservations";
            const std::string mecReductionOption = "mecreduction";
            const std::string selfloopReductionOption = "selfloopreduction";
            const std::string memoryBoundOption = "memorybound";
            const std::string memoryPatternOption = "memorypattern";
            std::vector<std::string> memoryPatterns = {"trivial", "fixedcounter", "selectivecounter", "ring", "fixedring", "settablebits", "full"};
            const std::string fscmode = "fscmode";
            std::vector<std::string> fscModes = {"standard", "simple-linear", "simple-linear-inverse"};
            const std::string transformBinaryOption = "transformbinary";
            const std::string transformSimpleOption = "transformsimple";

            POMDPSettings::POMDPSettings() : ModuleSettings(moduleName) {
                this->addOption(storm::settings::OptionBuilder(moduleName, exportAsParametricModelOption, false, "Export the parametric file.").addArgument(storm::settings::ArgumentBuilder::createStringArgument("filename", "The name of the file to which to write the model.").build()).build());
                this->addOption(storm::settings::OptionBuilder(moduleName, qualitativeReductionOption, false, "Reduces the model size by performing qualitative analysis (E.g. merge states with prob. 1.").build());
                this->addOption(storm::settings::OptionBuilder(moduleName, analyzeUniqueObservationsOption, false, "Computes the states with a unique observation").build());
                this->addOption(storm::settings::OptionBuilder(moduleName, mecReductionOption, false, "Reduces the model size by analyzing maximal end components").build());
                this->addOption(storm::settings::OptionBuilder(moduleName, selfloopReductionOption, false, "Reduces the model size by removing self loop actions").build());
                this->addOption(storm::settings::OptionBuilder(moduleName, memoryBoundOption, false, "Sets the maximal number of allowed memory states (1 means memoryless schedulers).").addArgument(storm::settings::ArgumentBuilder::createUnsignedIntegerArgument("bound", "The maximal number of memory states.").setDefaultValueUnsignedInteger(1).addValidatorUnsignedInteger(storm::settings::ArgumentValidatorFactory::createUnsignedGreaterValidator(0)).build()).build());
                this->addOption(storm::settings::OptionBuilder(moduleName, memoryPatternOption, false, "Sets the pattern of the considered memory structure").addArgument(storm::settings::ArgumentBuilder::createStringArgument("name", "Pattern name.").addValidatorString(ArgumentValidatorFactory::createMultipleChoiceValidator(memoryPatterns)).setDefaultValueString("full").build()).build());
                this->addOption(storm::settings::OptionBuilder(moduleName, fscmode, false, "Sets the way the pMC is obtained").addArgument(storm::settings::ArgumentBuilder::createStringArgument("type", "type name").addValidatorString(ArgumentValidatorFactory::createMultipleChoiceValidator(fscModes)).setDefaultValueString("standard").build()).build());
                this->addOption(storm::settings::OptionBuilder(moduleName, transformBinaryOption, false, "Transforms the pomdp to a binary pomdp.").build());
                this->addOption(storm::settings::OptionBuilder(moduleName, transformSimpleOption, false, "Transforms the pomdp to a binary and simple pomdp.").build());
            }

            bool POMDPSettings::isExportToParametricSet() const {
                return this->getOption(exportAsParametricModelOption).getHasOptionBeenSet();
            }

            std::string POMDPSettings::getExportToParametricFilename() const {
                return this->getOption(exportAsParametricModelOption).getArgumentByName("filename").getValueAsString();
            }
            
            bool POMDPSettings::isQualitativeReductionSet() const {
                return this->getOption(qualitativeReductionOption).getHasOptionBeenSet();
            }
            
            bool POMDPSettings::isAnalyzeUniqueObservationsSet() const {
                return this->getOption(analyzeUniqueObservationsOption).getHasOptionBeenSet();
            }
            
            bool POMDPSettings::isMecReductionSet() const {
                return this->getOption(mecReductionOption).getHasOptionBeenSet();
            }
            
            bool POMDPSettings::isSelfloopReductionSet() const {
                return this->getOption(selfloopReductionOption).getHasOptionBeenSet();
            }
            
            uint64_t POMDPSettings::getMemoryBound() const {
                return this->getOption(memoryBoundOption).getArgumentByName("bound").getValueAsUnsignedInteger();
            }
            
            storm::storage::PomdpMemoryPattern POMDPSettings::getMemoryPattern() const {
                auto pattern = this->getOption(memoryPatternOption).getArgumentByName("name").getValueAsString();
                if (pattern == "trivial") {
                    return storm::storage::PomdpMemoryPattern::Trivial;
                } else if (pattern == "fixedcounter") {
                    return storm::storage::PomdpMemoryPattern::FixedCounter;
                } else if (pattern == "selectivecounter") {
                    return storm::storage::PomdpMemoryPattern::SelectiveCounter;
                } else if (pattern == "ring") {
                    return storm::storage::PomdpMemoryPattern::SelectiveRing;
                } else if (pattern == "fixedring") {
                    return storm::storage::PomdpMemoryPattern::FixedRing;
                } else if (pattern == "settablebits") {
                    return storm::storage::PomdpMemoryPattern::SettableBits;
                } else if (pattern == "full") {
                    return storm::storage::PomdpMemoryPattern::Full;
                }
                STORM_LOG_THROW(false, storm::exceptions::InvalidArgumentException, "The name of the memory pattern is unknown.");
            }

            std::string POMDPSettings::getFscApplicationTypeString() const {
                return this->getOption(fscmode).getArgumentByName("type").getValueAsString();
            }
            
            bool POMDPSettings::isTransformBinarySet() const {
                return this->getOption(transformBinaryOption).getHasOptionBeenSet();
            }

            bool POMDPSettings::isTransformSimpleSet() const {
                return this->getOption(transformSimpleOption).getHasOptionBeenSet();
            }
            
            void POMDPSettings::finalize() {
            }

            bool POMDPSettings::check() const {
                STORM_LOG_THROW(getMemoryPattern() != storm::storage::PomdpMemoryPattern::Trivial || getMemoryBound() == 1, storm::exceptions::InvalidArgumentException, "Memory bound greater one is not possible with the trivial memory pattern.");
                return true;
            }
            
        } // namespace modules
    } // namespace settings
} // namespace storm
