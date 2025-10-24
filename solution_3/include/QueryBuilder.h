#ifndef REGION_QUERY_QUERY_BUILDER_H
#define REGION_QUERY_QUERY_BUILDER_H

#include <string>
#include <nlohmann/json.hpp>
#include "Rect.h"

class QueryBuilder {
public:
    explicit QueryBuilder(const Rect& valid);
    bool needsProper(const json& node) const;
    std::string buildCropSql(const json& crop) const;
    std::string buildSetSql(const json& node) const;
    std::string buildSql(const json& root, bool needProper) const;

private:
    Rect valid_;
};

#endif // REGION_QUERY_QUERY_BUILDER_H
