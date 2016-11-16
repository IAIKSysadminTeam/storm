#include "gtest/gtest.h"
#include "storm-config.h"

#ifdef STORM_HAVE_HYPRO

#include "src/modelchecker/multiobjective/pcaa.h"
#include "src/modelchecker/results/ExplicitQuantitativeCheckResult.h"
#include "src/modelchecker/results/ExplicitQualitativeCheckResult.h"
#include "src/models/sparse/Mdp.h"
#include "src/settings/modules/GeneralSettings.h"
#include "src/settings/SettingsManager.h"
#include "src/utility/storm.h"


TEST(SparseMdpPcaaModelCheckerTest, consensus) {
    
    std::string programFile = STORM_CPP_BASE_PATH "/examples/multiobjective/mdp/consensus/consensus2_3_2.nm";
    std::string formulasAsString = "multi(Pmax=? [ F \"one_proc_err\" ], P>=0.8916673903 [ G \"one_coin_ok\" ]) "; // numerical
    formulasAsString += "; \n multi(P>=0.1 [ F \"one_proc_err\" ], P>=0.8916673903 [ G \"one_coin_ok\" ])"; // achievability (true)
    formulasAsString += "; \n multi(P>=0.11 [ F \"one_proc_err\" ], P>=0.8916673903 [ G \"one_coin_ok\" ])"; // achievability (false)
    
    // programm, model,  formula
    storm::prism::Program program = storm::parseProgram(programFile);
    program = storm::utility::prism::preprocess(program, "");
    std::vector<std::shared_ptr<storm::logic::Formula const>> formulas = storm::parseFormulasForPrismProgram(formulasAsString, program);
    std::shared_ptr<storm::models::sparse::Mdp<double>> mdp = storm::buildSparseModel<double>(program, formulas, true)->as<storm::models::sparse::Mdp<double>>();
    uint_fast64_t const initState = *mdp->getInitialStates().begin();;
    
    storm::modelchecker::SparseMdpPrctlModelChecker<storm::models::sparse::Mdp<double>> checker(*mdp);
    
    std::unique_ptr<storm::modelchecker::CheckResult> result = storm::modelchecker::multiobjective::performPcaa(*mdp, formulas[0]->asMultiObjectiveFormula());
    ASSERT_TRUE(result->isExplicitQuantitativeCheckResult());
    EXPECT_NEAR(0.10833260970000025, result->asExplicitQuantitativeCheckResult<double>()[initState], storm::settings::getModule<storm::settings::modules::GeneralSettings>().getPrecision());
    
    result = storm::modelchecker::multiobjective::performPcaa(*mdp, formulas[1]->asMultiObjectiveFormula());
    ASSERT_TRUE(result->isExplicitQualitativeCheckResult());
    EXPECT_TRUE(result->asExplicitQualitativeCheckResult()[initState]);
    
    result = storm::modelchecker::multiobjective::performPcaa(*mdp, formulas[2]->asMultiObjectiveFormula());
    ASSERT_TRUE(result->isExplicitQualitativeCheckResult());
    EXPECT_FALSE(result->asExplicitQualitativeCheckResult()[initState]);
    
}

TEST(SparseMdpPcaaModelCheckerTest, zeroconf) {
    
    std::string programFile = STORM_CPP_BASE_PATH "/examples/multiobjective/mdp/zeroconf/zeroconf4.nm";
    std::string formulasAsString = "multi(Pmax=? [ F l=4 & ip=1 ] , P>=0.993141[ G (error=0) ]) "; // numerical
    
    // programm, model,  formula
    storm::prism::Program program = storm::parseProgram(programFile);
    program = storm::utility::prism::preprocess(program, "");
    std::vector<std::shared_ptr<storm::logic::Formula const>> formulas = storm::parseFormulasForPrismProgram(formulasAsString, program);
    std::shared_ptr<storm::models::sparse::Mdp<double>> mdp = storm::buildSparseModel<double>(program, formulas, true)->as<storm::models::sparse::Mdp<double>>();
    uint_fast64_t const initState = *mdp->getInitialStates().begin();
    
    storm::modelchecker::SparseMdpPrctlModelChecker<storm::models::sparse::Mdp<double>> checker(*mdp);
    
    std::unique_ptr<storm::modelchecker::CheckResult> result = storm::modelchecker::multiobjective::performPcaa(*mdp, formulas[0]->asMultiObjectiveFormula());
    ASSERT_TRUE(result->isExplicitQuantitativeCheckResult());
    EXPECT_NEAR(0.0003075787401574803, result->asExplicitQuantitativeCheckResult<double>()[initState], storm::settings::getModule<storm::settings::modules::GeneralSettings>().getPrecision());
}

TEST(SparseMdpPcaaModelCheckerTest, team3with3objectives) {
    
    std::string programFile = STORM_CPP_BASE_PATH "/examples/multiobjective/mdp/team/team3obj_3.nm";
    std::string formulasAsString = "multi(Pmax=? [ F task1_completed ], R{\"w_1_total\"}>=2.210204082 [ C ],  P>=0.5 [ F task2_completed ])"; // numerical
    
    // programm, model,  formula
    storm::prism::Program program = storm::parseProgram(programFile);
    program = storm::utility::prism::preprocess(program, "");
    std::vector<std::shared_ptr<storm::logic::Formula const>> formulas = storm::parseFormulasForPrismProgram(formulasAsString, program);
    std::shared_ptr<storm::models::sparse::Mdp<double>> mdp = storm::buildSparseModel<double>(program, formulas, true)->as<storm::models::sparse::Mdp<double>>();
    uint_fast64_t const initState = *mdp->getInitialStates().begin();
    
    storm::modelchecker::SparseMdpPrctlModelChecker<storm::models::sparse::Mdp<double>> checker(*mdp);
    
    std::unique_ptr<storm::modelchecker::CheckResult> result = storm::modelchecker::multiobjective::performPcaa(*mdp, formulas[0]->asMultiObjectiveFormula());
    ASSERT_TRUE(result->isExplicitQuantitativeCheckResult());
    EXPECT_NEAR(0.7448979591841851, result->asExplicitQuantitativeCheckResult<double>()[initState], storm::settings::getModule<storm::settings::modules::GeneralSettings>().getPrecision());
}

TEST(SparseMdpPcaaModelCheckerTest, scheduler) {
    
    std::string programFile = STORM_CPP_BASE_PATH "/examples/multiobjective/mdp/scheduler/scheduler05.nm";
    std::string formulasAsString = "multi(R{\"time\"}<= 11.778[ F \"tasks_complete\" ], R{\"energy\"}<=1.45 [  F \"tasks_complete\" ]) ";
    
    // programm, model,  formula
    storm::prism::Program program = storm::parseProgram(programFile);
    program = storm::utility::prism::preprocess(program, "");
    std::vector<std::shared_ptr<storm::logic::Formula const>> formulas = storm::parseFormulasForPrismProgram(formulasAsString, program);
    std::shared_ptr<storm::models::sparse::Mdp<double>> mdp = storm::buildSparseModel<double>(program, formulas, true)->as<storm::models::sparse::Mdp<double>>();
    uint_fast64_t const initState = *mdp->getInitialStates().begin();
    
    storm::modelchecker::SparseMdpPrctlModelChecker<storm::models::sparse::Mdp<double>> checker(*mdp);
    
    std::unique_ptr<storm::modelchecker::CheckResult> result = storm::modelchecker::multiobjective::performPcaa(*mdp, formulas[0]->asMultiObjectiveFormula());
    EXPECT_TRUE(result->asExplicitQualitativeCheckResult()[initState]);
}

TEST(SparseMdpPcaaModelCheckerTest, dpm) {
    
    std::string programFile = STORM_CPP_BASE_PATH "/examples/multiobjective/mdp/dpm/dpm100.nm";
    std::string formulasAsString = "multi(R{\"power\"}min=? [ C<=100 ], R{\"queue\"}<=70 [ C<=100 ])"; // numerical
    
    // programm, model,  formula
    storm::prism::Program program = storm::parseProgram(programFile);
    program = storm::utility::prism::preprocess(program, "");
    std::vector<std::shared_ptr<storm::logic::Formula const>> formulas = storm::parseFormulasForPrismProgram(formulasAsString, program);
    std::shared_ptr<storm::models::sparse::Mdp<double>> mdp = storm::buildSparseModel<double>(program, formulas, true)->as<storm::models::sparse::Mdp<double>>();
    uint_fast64_t const initState = *mdp->getInitialStates().begin();
    
    storm::modelchecker::SparseMdpPrctlModelChecker<storm::models::sparse::Mdp<double>> checker(*mdp);
    
    std::unique_ptr<storm::modelchecker::CheckResult> result = storm::modelchecker::multiobjective::performPcaa(*mdp, formulas[0]->asMultiObjectiveFormula());
    ASSERT_TRUE(result->isExplicitQuantitativeCheckResult());
    EXPECT_NEAR(121.61288420945114, result->asExplicitQuantitativeCheckResult<double>()[initState], storm::settings::getModule<storm::settings::modules::GeneralSettings>().getPrecision());
}




#endif /* STORM_HAVE_HYPRO */
