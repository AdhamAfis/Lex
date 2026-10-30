#ifndef EXPORT_FORMATTER_H
#define EXPORT_FORMATTER_H

#include <string>
#include <vector>
#include <memory>
#include <fstream>
#include "Token.h"
#include "FileUtil.h"

class TokenExporter {
public:
    virtual ~TokenExporter() = default;

    virtual std::string exportToString(const std::vector<Token>& tokens) const = 0;

    virtual bool exportToFile(const std::vector<Token>& tokens, const std::string& filename) const {
        return writeFileContents(filename, exportToString(tokens));
    }
};

class JsonExporter : public TokenExporter {
public:
    std::string exportToString(const std::vector<Token>& tokens) const override;
};

class XmlExporter : public TokenExporter {
private:
    std::string escapeXml(const std::string& input) const;
public:
    std::string exportToString(const std::vector<Token>& tokens) const override;
};

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

#endif
