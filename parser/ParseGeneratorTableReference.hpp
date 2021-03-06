/**
 *   Copyright 2016, Quickstep Research Group, Computer Sciences Department,
 *   University of Wisconsin—Madison.
 *   Copyright 2016 Pivotal Software, Inc.
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 **/

#ifndef QUICKSTEP_PARSER_PARSE_GENERATOR_TABLE_REFERENCE_HPP_
#define QUICKSTEP_PARSER_PARSE_GENERATOR_TABLE_REFERENCE_HPP_

#include <memory>
#include <string>
#include <vector>

#include "parser/ParseBasicExpressions.hpp"
#include "parser/ParseTableReference.hpp"
#include "utility/Macros.hpp"

namespace quickstep {

class ParseTreeNode;

/** \addtogroup Parser
 *  @{
 */

/**
 * @brief A table that is generated by a generator function.
 */
class ParseGeneratorTableReference : public ParseTableReference {
 public:
  /**
   * @brief Constructor. Takes ownership of \p generator_function.
   *
   * @param line_number The line number of the first token of the table reference.
   * @param column_number The column number of the first token of the table reference.
   * @param generator_function_ The parsed generator function.
   */
  ParseGeneratorTableReference(const int line_number,
                               const int column_number,
                               ParseFunctionCall *generator_function)
      : ParseTableReference(line_number, column_number),
        generator_function_(generator_function) {
  }

  /**
   * @brief Destructor.
   */
  ~ParseGeneratorTableReference() override {}

  TableReferenceType getTableReferenceType() const override {
    return kGeneratorTableReference;
  }

  std::string getName() const override { return "TableGenerator"; }

  /**
   * @return The parsed generator function.
   */
  const ParseFunctionCall* generator_function() const { return generator_function_.get(); }

 protected:
  void getFieldStringItems(
      std::vector<std::string> *inline_field_names,
      std::vector<std::string> *inline_field_values,
      std::vector<std::string> *non_container_child_field_names,
      std::vector<const ParseTreeNode*> *non_container_child_fields,
      std::vector<std::string> *container_child_field_names,
      std::vector<std::vector<const ParseTreeNode*>> *container_child_fields) const override;

 private:
  std::unique_ptr<ParseFunctionCall> generator_function_;

  DISALLOW_COPY_AND_ASSIGN(ParseGeneratorTableReference);
};

/** @} */

}  // namespace quickstep

#endif /* QUICKSTEP_PARSER_PARSE_GENERATOR_TABLE_REFERENCE_HPP_ */
