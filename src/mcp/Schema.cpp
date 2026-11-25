/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "mcp/Schema.h"
#ifndef DOXYGEN_SHOULD_SKIP_THIS
#include <exception>
#include <map>
#include <nlohmann/json-schema.hpp>
#include <nlohmann/json.hpp>
#include <string>
#endif
namespace mcp {
    namespace {
        nlohmann::json_schema::json_validator schemaValidator() {
            nlohmann::json_schema::json_validator validator(
                nullptr,
                [](const std::string&, const std::string&) {
                },
                [](const std::string&, const std::string&, const Json&) {
                });
            validator.set_default_dialect(nlohmann::json_schema::schema_dialect::draft2020_12);
            nlohmann::json_schema::validation_limits limits;
            limits.max_schemas = 1024;
            validator.set_validation_limits(limits);

            return validator;
        }

        class SchemaError final : public nlohmann::json_schema::error_handler {
        public:
            explicit SchemaError(std::string& message);
            void error(const Json::json_pointer& pointer, const Json&, const std::string& reason) final;

        private:
            std::string& message;
        };

        SchemaError::SchemaError(std::string& message)
            : message(message) {
        }

        void SchemaError::error(const Json::json_pointer& pointer, const Json&, const std::string& reason) {
            if (message.empty()) {
                message = pointer.to_string() + ": " + reason;
            }
        }

    } // namespace

    bool Schema::check(const Json& schema, std::string& error) {
        bool good = false;
        error.clear();
        try {
            if (schema.dump().size() <= 16384) {
                auto validator = schemaValidator();
                validator.validate_schema(schema);
                validator.set_root_schema(schema);
                good = true;
            } else {
                error = "schema exceeds 16384-byte limit";
            }
        } catch (const std::exception& exception) {
            error = exception.what();
        }

        return good;
    }

    bool Schema::validate(const Json& schema, const Json& value, std::string& error) {
        bool good = false;
        error.clear();
        try {
            auto validator = schemaValidator();
            validator.set_root_schema(schema);
            SchemaError handler(error);
            validator.validate(value, handler);
            good = error.empty();
        } catch (const std::exception& exception) {
            error = exception.what();
        }

        return good;
    }

} // namespace mcp
