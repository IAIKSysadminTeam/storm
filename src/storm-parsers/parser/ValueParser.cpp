#include "storm-parsers/parser/ValueParser.h"

#include "storm/exceptions/NotSupportedException.h"

namespace storm {
    namespace parser {

        template<typename ValueType>
        void ValueParser<ValueType>::addParameter(std::string const& parameter) {
            STORM_LOG_THROW(false, storm::exceptions::NotSupportedException, "Parameters are not supported in this build.");
        }

        template<>
        void ValueParser<storm::RationalFunction>::addParameter(std::string const& parameter) {
            storm::expressions::Variable var = manager->declareRationalVariable(parameter);
            identifierMapping.emplace(var.getName(), var);
            parser.setIdentifierMapping(identifierMapping);
            STORM_LOG_TRACE("Added parameter: " << var.getName());
        }

        template<>
        double ValueParser<double>::parseValue(std::string const& value) const {
            return NumberParser<double>::parse(value);
        }

        template<>
        storm::RationalFunction ValueParser<storm::RationalFunction>::parseValue(std::string const& value) const {
            storm::RationalFunction rationalFunction = evaluator.asRational(parser.parseFromString(value));
            STORM_LOG_TRACE("Parsed expression: " << rationalFunction);
            return rationalFunction;
        }

        // Template instantiations.
        template class ValueParser<double>;
        template class ValueParser<storm::RationalFunction>;

    } // namespace parser
} // namespace storm
