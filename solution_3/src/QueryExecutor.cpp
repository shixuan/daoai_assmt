#include <fstream>
#include <iostream>
#include <pqxx/pqxx>
#include <nlohmann/json.hpp>

#include "QueryExecutor.h"
#include "QueryBuilder.h"
#include "Rect.h"

int runQuery(const std::string& conninfo,
             const std::string& queryFile,
             const std::string& outFile,
             bool debug) {
    try {
        std::ifstream fin(queryFile);
        if (!fin.is_open())
            throw std::runtime_error("cannot open query file");

        json j;
        fin >> j;

        Rect valid = readRect(j.at("valid_region"));
        const json& root = j.at("query");

        QueryBuilder qb(valid);
        bool needProper = qb.needsProper(root);
        std::string sql = qb.buildSql(root, needProper);

        if (debug)
            std::cerr << sql << "\n";

        // connect to db
        pqxx::connection conn(conninfo);
        pqxx::work txn(conn);
        pqxx::result r = txn.exec(sql);
        txn.commit();

        // output
        std::ofstream fout(outFile);
        if (!fout.is_open())
            throw std::runtime_error("cannot open output");
        for (const auto& row : r)
            fout << row[0].as<double>() << " " << row[1].as<double>() << "\n";
        fout.close();

        std::cout << "Wrote " << r.size() << " points\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] " << e.what() << "\n";
        return 1;
    }
}
