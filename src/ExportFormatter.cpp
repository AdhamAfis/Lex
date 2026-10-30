#include <vector>
#include <memory>
#include <string>
#include "ExportFormatter.h"
#include <sstream>
#include <iomanip>
#include "../include/json.hpp"

std::string JsonExporter::exportToString(const std::vector<Token>& tokens) const {
    nlohmann::json output = {{"tokens", nlohmann::json::array()}};
    for (const auto& token : tokens) {
        nlohmann::json item = {{"type", token.typeToString()}, {"lexeme", token.lexeme},
                               {"line", token.location.line}, {"column", token.location.column}};
        if (!token.location.filename.empty()) item["filename"] = token.location.filename;
        if (token.attribute) item["attributes"] = token.attribute->toString();
        output["tokens"].push_back(item);
    }
    return output.dump(2) + "\n";
}

std::string XmlExporter::exportToString(const std::vector<Token>& tokens) const {
    std::stringstream ss;

    ss << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    ss << "<tokens>\n";

    for (const auto& token : tokens) {
        ss << "  <token>\n";
        ss << "    <type>" << token.typeToString() << "</type>\n";
        ss << "    <lexeme>" << escapeXml(token.lexeme) << "</lexeme>\n";
        ss << "    <location>\n";
        ss << "      <line>" << token.location.line << "</line>\n";
        ss << "      <column>" << token.location.column << "</column>\n";

        if (!token.location.filename.empty()) {
            ss << "      <filename>" << escapeXml(token.location.filename) << "</filename>\n";
        }

        ss << "    </location>\n";

        if (token.attribute) {
            ss << "    <attributes>" << escapeXml(token.attribute->toString()) << "</attributes>\n";
        }

        ss << "  </token>\n";
    }

    ss << "</tokens>\n";

    return ss.str();
}

std::string XmlExporter::escapeXml(const std::string& input) const {
    std::string output;
    output.reserve(input.size());

    for (char c : input) {
        if (static_cast<unsigned char>(c) < 0x20 && c != '\t' && c != '\n' && c != '\r')
            throw std::runtime_error("XML cannot represent control characters");
        switch (c) {
            case '&': output += "&amp;"; break;
            case '<': output += "&lt;"; break;
            case '>': output += "&gt;"; break;
            case '\"': output += "&quot;"; break;
            case '\'': output += "&apos;"; break;
            default: output += c; break;
        }
    }

    return output;
}

std::string CsvExporter::exportToString(const std::vector<Token>& tokens) const {
    std::stringstream ss;

    if (includeHeaders) {
        ss << "Type" << delimiter
           << "Lexeme" << delimiter
           << "Line" << delimiter
           << "Column" << delimiter
           << "Filename" << delimiter
           << "Attributes" << "\n";
    }

    for (const auto& token : tokens) {
        ss << token.typeToString() << delimiter
           << "\"" << escapeCsv(token.lexeme) << "\"" << delimiter
           << token.location.line << delimiter
           << token.location.column << delimiter
           << "\"" << escapeCsv(token.location.filename) << "\"" << delimiter;

        if (token.attribute) {
            ss << "\"" << escapeCsv(token.attribute->toString()) << "\"";
        }

        ss << "\n";
    }

    return ss.str();
}

std::string CsvExporter::escapeCsv(const std::string& input) const {
    std::string output;
    output.reserve(input.size());

    for (char c : input) {
        output += c;
        if (c == '\"') {
            output += '\"';
        }
    }

    return output;
}

std::string HtmlExporter::exportToString(const std::vector<Token>& tokens) const {
    std::stringstream ss;

    ss << "<!DOCTYPE html>\n";
    ss << "<html>\n";
    ss << "<head>\n";
    ss << "  <title>Token Visualization</title>\n";

    if (includeStyles) {
        ss << "  <style>\n";
        ss << "    body { font-family: Arial, sans-serif; margin: 20px; }\n";
        ss << "    table { border-collapse: collapse; width: 100%; }\n";
        ss << "    th, td { border: 1px solid #ddd; padding: 8px; text-align: left; }\n";
        ss << "    th { background-color: #f2f2f2; }\n";
        ss << "    tr:nth-child(even) { background-color: #f9f9f9; }\n";
        ss << "    .token-INTEGER { color: #0066cc; }\n";
        ss << "    .token-FLOAT, .token-SCIENTIFIC { color: #006699; }\n";
        ss << "    .token-HEX, .token-OCTAL, .token-BINARY { color: #9900cc; }\n";
        ss << "    .token-STRING_LITERAL, .token-CHAR_LITERAL { color: #cc6600; }\n";
        ss << "    .token-IDENTIFIER { color: #000000; }\n";
        ss << "    .token-KEYWORD { color: #0000cc; font-weight: bold; }\n";
        ss << "    .token-OPERATOR, .token-ASSIGNMENT_OPERATOR, .token-ARITHMETIC_OPERATOR,\n";
        ss << "    .token-LOGICAL_OPERATOR, .token-BITWISE_OPERATOR, .token-COMPARISON_OPERATOR { color: #cc0000; }\n";
        ss << "    .token-DELIMITER, .token-PARENTHESIS, .token-BRACKET, .token-BRACE,\n";
        ss << "    .token-SEMICOLON, .token-COMMA, .token-DOT { color: #666666; }\n";
        ss << "    .token-PREPROCESSOR { color: #009900; font-style: italic; }\n";
        ss << "    .token-COMMENT { color: #999999; font-style: italic; }\n";
        ss << "    .token-ERROR, .token-UNKNOWN { color: #ff0000; background-color: #ffeeee; }\n";
        ss << "    .details { font-size: 0.8em; color: #666; margin-top: 5px; }\n";
        ss << "  </style>\n";
    }

    ss << "</head>\n";
    ss << "<body>\n";
    ss << "  <h1>Token Visualization</h1>\n";
    ss << "  <table>\n";
    ss << "    <tr>\n";
    ss << "      <th>Type</th>\n";
    ss << "      <th>Lexeme</th>\n";

    if (includeTokenDetails) {
        ss << "      <th>Line</th>\n";
        ss << "      <th>Column</th>\n";
        ss << "      <th>Filename</th>\n";
        ss << "      <th>Attributes</th>\n";
    }

    ss << "    </tr>\n";

    for (const auto& token : tokens) {
        ss << "    <tr>\n";
        ss << "      <td>" << token.typeToString() << "</td>\n";
        ss << "      <td class=\"token-" << token.typeToString() << "\">"
           << escapeHtml(token.lexeme) << "</td>\n";

        if (includeTokenDetails) {
            ss << "      <td>" << token.location.line << "</td>\n";
            ss << "      <td>" << token.location.column << "</td>\n";
            ss << "      <td>" << escapeHtml(token.location.filename) << "</td>\n";
            ss << "      <td>";

            if (token.attribute) {
                ss << escapeHtml(token.attribute->toString());
            }

            ss << "</td>\n";
        }

        ss << "    </tr>\n";
    }

    ss << "  </table>\n";

    ss << "  <h2>Token Stream</h2>\n";
    ss << "  <div class=\"token-stream\">\n";

    for (const auto& token : tokens) {
        if (token.type == TokenType::EOF_TOKEN) {
            continue;
        }

        ss << "    <span class=\"token-" << token.typeToString() << "\">"
           << escapeHtml(token.lexeme);

        if (includeTokenDetails) {
            ss << "<span class=\"details\">[" << token.typeToString() << "]</span>";
        }

        ss << "</span>\n";
    }

    ss << "  </div>\n";
    ss << "</body>\n";
    ss << "</html>\n";

    return ss.str();
}

std::string HtmlExporter::escapeHtml(const std::string& input) const {
    std::stringstream ss;

    for (char c : input) {
        switch (c) {
            case '&': ss << "&amp;"; break;
            case '<': ss << "&lt;"; break;
            case '>': ss << "&gt;"; break;
            case '\"': ss << "&quot;"; break;
            case '\'': ss << "&#39;"; break;
            case '\n': ss << "<br>"; break;
            case '\t': ss << "&nbsp;&nbsp;&nbsp;&nbsp;"; break;
            case ' ': ss << "&nbsp;"; break;
            default: ss << c; break;
        }
    }

    return ss.str();
}

std::unique_ptr<TokenExporter> ExporterFactory::createExporter(Format format) {
    switch (format) {
        case Format::JSON:
            return std::make_unique<JsonExporter>();
        case Format::XML:
            return std::make_unique<XmlExporter>();
        case Format::CSV:
            return std::make_unique<CsvExporter>();
        case Format::HTML:
            return std::make_unique<HtmlExporter>();
        default:
            return nullptr;
    }
}
