#include <sstream>
#include <vector>
#include <stdexcept>

#include "QueryBuilder.h"

QueryBuilder::QueryBuilder(const Rect& valid) : valid_(valid) {}

// scan the tree to determine need proper or not
bool QueryBuilder::needsProper(const json& node) const {
    if (node.contains("operator_crop")) {
        const auto& val = node.at("operator_crop");
        return val.contains("proper") && val.at("proper").get<bool>();
    }

    if (node.contains("operator_and")) {
        for (const auto& val : node.at("operator_and"))
            if (needsProper(val)) return true;
    }

    if (node.contains("operator_or")) {
        for (const auto& val : node.at("operator_or"))
            if (needsProper(val)) return true;
    }

    return false;
}

// return the sql of selecting eligible ids
std::string QueryBuilder::buildCropSql(const json& crop) const {
    if (!crop.contains("region"))
        throw std::runtime_error("missing region");
    Rect region = readRect(crop.at("region"));

    bool hasCategory = crop.contains("category");
    int category = hasCategory ? crop.at("category").get<int>() : 0;

    bool hasGroups = crop.contains("one_of_groups");
    std::vector<long long> groups;
    if (hasGroups) {
        for (const auto& g : crop.at("one_of_groups"))
            groups.push_back(g.get<long long>());
        if (groups.empty()) hasGroups = false;
    }

    bool proper = crop.contains("proper") ? crop.at("proper").get<bool>() : false;

    std::ostringstream s;
    s << "SELECT id FROM inspection_region "
      << "WHERE coord_x BETWEEN " << region.xmin << " AND " << region.xmax
      << " AND coord_y BETWEEN " << region.ymin << " AND " << region.ymax;
    if (hasCategory) s << " AND category = " << category;
    if (hasGroups) {
        s << " AND group_id IN (";
        for (size_t i = 0; i < groups.size(); ++i) {
            if (i) s << ",";
            s << groups[i];
        }
        s << ")";
    }
    if (proper)
        s << " AND group_id IN (SELECT group_id FROM proper_groups)";
    return "(" + s.str() + ")";
}

std::string QueryBuilder::buildSetSql(const json& node) const {
    // crop node is supposed to be leaf node only
    if (node.contains("operator_crop"))
        return buildCropSql(node.at("operator_crop"));

    if (node.contains("operator_and")) {
        const auto& arr = node.at("operator_and");
        if (!arr.is_array() || arr.empty())
            throw std::runtime_error("invalid AND array");
        std::ostringstream s;
        for (size_t i = 0; i < arr.size(); ++i) {
            if (i) s << " INTERSECT ";
            s << buildSetSql(arr[i]);
        }
        return "(" + s.str() + ")";
    }

    if (node.contains("operator_or")) {
        const auto& arr = node.at("operator_or");
        if (!arr.is_array() || arr.empty())
            throw std::runtime_error("invalid OR array");
        std::ostringstream s;
        for (size_t i = 0; i < arr.size(); ++i) {
            if (i) s << " UNION ";
            s << buildSetSql(arr[i]);
        }
        return "(" + s.str() + ")";
    }

    throw std::runtime_error("unknown operator");
}

std::string QueryBuilder::buildSql(const json& root, bool needProper) const {
    std::string idSet = buildSetSql(root);
    std::ostringstream sql;

    sql << "WITH ";
    if (needProper) {
        sql << "proper_groups AS ("
            << "SELECT group_id FROM inspection_region "
            << "GROUP BY group_id "
            << "HAVING MIN(coord_x)>=" << valid_.xmin
            << " AND MAX(coord_x)<=" << valid_.xmax
            << " AND MIN(coord_y)>=" << valid_.ymin
            << " AND MAX(coord_y)<=" << valid_.ymax
            << "), ";
    }

    sql << "ids AS " << idSet << " "
        << "SELECT r.coord_x AS x, r.coord_y AS y "
        << "FROM inspection_region r JOIN ids s ON s.id=r.id "
        << "ORDER BY y, x;";

    return sql.str();
}
