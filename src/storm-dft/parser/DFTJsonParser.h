#pragma  once

#include <map>

#include "storm/storage/expressions/ExpressionManager.h"
#include "storm-parsers/parser/ExpressionParser.h"
#include "storm/storage/expressions/ExpressionEvaluator.h"

#include "storm-dft/storage/dft/DFT.h"
#include "storm-dft/builder/DFTBuilder.h"

// JSON parser
#include "json.hpp"

using json = nlohmann::json;

namespace storm {
    namespace parser {

        template<typename ValueType>
        class DFTJsonParser {
            storm::builder::DFTBuilder<ValueType> builder;

            std::shared_ptr<storm::expressions::ExpressionManager> manager;

            storm::parser::ExpressionParser parser;

            storm::expressions::ExpressionEvaluator<ValueType> evaluator;

            std::unordered_map<std::string, storm::expressions::Expression> identifierMapping;

        public:
            DFTJsonParser() : manager(new storm::expressions::ExpressionManager()), parser(*manager), evaluator(*manager) {
            }

            storm::storage::DFT<ValueType> parseJsonFromString(std::string const& jsonString);

            storm::storage::DFT<ValueType> parseJsonFromFile(std::string const& filename);

        private:
            storm::storage::DFT<ValueType> parseJson(json const& jsonInput);

            std::string generateUniqueName(std::string const& id, std::string const& name);

            ValueType parseRationalExpression(std::string const& expr);

            std::string parseJsonNumber(json number);
        };
    }
}
