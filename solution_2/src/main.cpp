#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <stdexcept>
#include <pqxx/pqxx>
#include <gflags/gflags.h>
#include <array>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

DEFINE_string(query, "", "Path to JSON query file");
DEFINE_string(out, "result.txt", "Output text file path");
DEFINE_string(conn, "dbname=regiondb user=postgres host=localhost", "DB connection string");

struct Rect {
    double xmin, ymin, xmax, ymax;
};

static Rect readRect(const json& j) {
    Rect r{};
    r.xmin = j.at("p_min").at("x").get<double>();
    r.ymin = j.at("p_min").at("y").get<double>();
    r.xmax = j.at("p_max").at("x").get<double>();
    r.ymax = j.at("p_max").at("y").get<double>();
    if (r.xmin > r.xmax || r.ymin > r.ymax) {
        throw std::runtime_error("Invalid rectangle: p_min > p_max");
    }
    return r;
}

int main(int argc, char** argv) {
    gflags::SetUsageMessage("Usage: region_query --query=JSON [--out=result.txt] [--conn=...]");
    gflags::ParseCommandLineFlags(&argc, &argv, true);

    if (FLAGS_query.empty()) {
        std::cerr << "[Error] --query is required\n";
        std::cerr << gflags::ProgramUsage() << "\n";
        return 1;
    }

    try {
        std::ifstream fin(FLAGS_query);
        if (!fin.is_open())
            throw std::runtime_error("Failed to open query file: " + FLAGS_query);
        json qj;
        fin >> qj;

        if (!qj.contains("valid_region"))
            throw std::runtime_error("JSON missing 'valid_region'");
        Rect valid = readRect(qj.at("valid_region"));

        // parse operator_crop
        if (!qj.contains("query") || !qj.at("query").contains("operator_crop"))
            throw std::runtime_error("JSON missing 'query.operator_crop'");
        const auto& crop = qj.at("query").at("operator_crop");
        if (!crop.contains("region"))
            throw std::runtime_error("operator_crop.region is required");

        Rect region = readRect(crop.at("region"));

        // optional
        bool has_category = crop.contains("category");
        int category = has_category ? crop.at("category").get<int>() : 0;

        bool has_groups = crop.contains("one_of_groups");
        std::vector<long long> groups;
        if (has_groups) {
            for (const auto& g : crop.at("one_of_groups"))
                groups.push_back(g.get<long long>());
            if (groups.empty())
                has_groups = false;
        }

        bool proper = crop.contains("proper") ? crop.at("proper").get<bool>() : false;

        // connect to db
        pqxx::connection conn(FLAGS_conn);
        if (!conn.is_open())
            throw std::runtime_error("Cannot open DB connection");
        pqxx::work txn(conn);

        // construct SQL
        std::ostringstream sql;
        sql << "WITH ";

        if (proper) {
            sql << "proper_groups AS ("
                << "  SELECT group_id"
                << "  FROM inspection_region"
                << "  GROUP BY group_id"
                << "  HAVING MIN(coord_x) >= " << valid.xmin
                << "     AND MAX(coord_x) <= " << valid.xmax
                << "     AND MIN(coord_y) >= " << valid.ymin
                << "     AND MAX(coord_y) <= " << valid.ymax
                << "), ";
        }

        sql << "crop AS ("
            << "  SELECT coord_x AS x, coord_y AS y, category, group_id"
            << "  FROM inspection_region"
            << "  WHERE coord_x BETWEEN " << region.xmin << " AND " << region.xmax
            << "    AND coord_y BETWEEN " << region.ymin << " AND " << region.ymax;

        if (has_category) {
            sql << " AND category = " << category;
        }

        if (has_groups) {
            sql << " AND group_id IN (";
            for (size_t i = 0; i < groups.size(); ++i) {
                if (i) sql << ",";
                sql << groups[i];
            }
            sql << ")";
        }

        if (proper) {
            sql << " AND group_id IN (SELECT group_id FROM proper_groups)";
        }

        sql << ") "
               "SELECT x, y FROM crop ORDER BY y ASC, x ASC;";

        pqxx::result r = txn.exec(sql.str());
        txn.commit();

        // output
        std::ofstream fout(FLAGS_out);
        if (!fout.is_open()) throw std::runtime_error("Failed to open output file: " + FLAGS_out);

        for (const auto& row : r) {
            double x = row[0].as<double>();
            double y = row[1].as<double>();
            fout << x << " " << y << "\n";
        }
        fout.close();

        std::cout << "Wrote " << r.size() << " points to " << FLAGS_out << "\n";
        return 0;

    } catch (const std::exception& e) {
        std::cerr << "[ERROR] " << e.what() << "\n";
        return 1;
    }
}
