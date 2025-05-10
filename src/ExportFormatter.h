#ifndef EXPORT_FORMATTER_H
#define EXPORT_FORMATTER_H

#include <string>
#include <vector>
#include <memory>
#include <fstream>
#include "Token.h"

// Base class for token export formatters
class TokenExporter {
public:
    virtual ~TokenExporter() = default;
    
    // Export tokens to string
    virtual std::string exportToString(const std::vector<Token>& tokens) const = 0;
    
    // Export tokens to file
    virtual bool exportToFile(const std::vector<Token>& tokens, const std::string& filename) const {
        std::string content = exportToString(tokens);
        std::ofstream file(filename);
        if (!file.is_open()) {
            return false;
        }
        file << content;
        return file.good();
    }
};

// JSON exporter
class JsonExporter : public TokenExporter {
public:
    std::string exportToString(const std::vector<Token>& tokens) const override;
};

// XML exporter
class XmlExporter : public TokenExporter {
private:
    std::string escapeXml(const std::string& input) const;
public:
    std::string exportToString(const std::vector<Token>& tokens) const override;
};

// CSV exporter
class CsvExporter : public TokenExporter {
private:
    bool includeHeaders;
    char delimiter;
    std::string escapeCsv(const std::string& input) const;
public:
    CsvExporter(bool includeHeaders = true, char delimiter = ',')
        : includeHeaders(includeHeaders), delimiter(delimiter) {}
        
    std::string exportToString(const std::vector<Token>& tokens) const override;
};

// HTML exporter for visual representation
class HtmlExporter : public TokenExporter {
private:
    bool includeStyles;
    bool includeTokenDetails;
    std::string escapeHtml(const std::string& input) const;
public:
    HtmlExporter(bool includeStyles = true, bool includeTokenDetails = true)
        : includeStyles(includeStyles), includeTokenDetails(includeTokenDetails) {}
        
    std::string exportToString(const std::vector<Token>& tokens) const override;
};

// Factory class to create exporters
class ExporterFactory {
public:
    enum class Format {
        JSON,
        XML,
        CSV,
        HTML
    };
    
    static std::unique_ptr<TokenExporter> createExporter(Format format);
};

#endif // EXPORT_FORMATTER_H 