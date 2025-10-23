#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <unordered_set>
#include <filesystem>
#include <pqxx/pqxx>
#include <gflags/gflags.h>

DEFINE_string(data_directory, "", "Path to directory containing points.txt / categories.txt / groups.txt");
DEFINE_string(conn, "dbname=regiondb user=postgres host=localhost", "PostgreSQL connection string");
DEFINE_bool(truncate, false, "Clean existing data before insert");

struct Region {
    long long id;
    double x, y;
    int category;
    long long group_id;
};

int main(int argc, char** argv) {
    gflags::SetUsageMessage("Usage: region_loader --data_directory=PATH [--conn=CONNINFO]");
    gflags::ParseCommandLineFlags(&argc, &argv, true);

    if (FLAGS_data_directory.empty()) {
        std::cerr << "[Error] --data_directory is required.\n";
        std::cerr << gflags::ProgramUsage();
        return 1;
    }

    try {
        std::string data_dir  = FLAGS_data_directory;
        std::string conninfo  = FLAGS_conn;
        bool do_truncate      = FLAGS_truncate;

        std::string path_points = (std::filesystem::path(data_dir) / "points.txt").string();
        std::string path_cats   = (std::filesystem::path(data_dir) / "categories.txt").string();
        std::string path_groups = (std::filesystem::path(data_dir) / "groups.txt").string();

        std::ifstream f_points(path_points);
        std::ifstream f_cats(path_cats);
        std::ifstream f_groups(path_groups);
        if (!f_points.is_open() || !f_cats.is_open() || !f_groups.is_open())
            throw std::runtime_error("Failed to open one or more input files in " + data_dir);


        std::vector<Region> records;
        records.reserve(100000);

        std::string line_p, line_c, line_g;
        long long line_idx = 0;
        while (true) {
            bool okp = static_cast<bool>(std::getline(f_points, line_p));
            bool okc = static_cast<bool>(std::getline(f_cats,   line_c));
            bool okg = static_cast<bool>(std::getline(f_groups, line_g));
            if (!okp && !okc && !okg) break;    // EOF
            if (!(okp && okc && okg))
                throw std::runtime_error("Input files have different line counts.");

            // points
            std::istringstream isp(line_p);
            double x, y;
            if (!(isp >> x >> y))
                throw std::runtime_error("Bad format in points.txt at line " + std::to_string(line_idx + 1));

            // category
            int category;
            std::istringstream isc(line_c);
            if (!(isc >> category))
                throw std::runtime_error("Bad format in categories.txt at line " + std::to_string(line_idx + 1));

            // group id
            long long gid;
            std::istringstream isg(line_g);
            if (!(isg >> gid))
                throw std::runtime_error("Bad format in groups.txt at line " + std::to_string(line_idx + 1));

            Region r;
            ++line_idx;
            r.x = x;
            r.y = y;
            r.category = category;
            r.group_id = gid;
            records.push_back(r);
        }

        std::cout << "Loaded " << records.size() << " rows from " << data_dir << std::endl;

        // connect to db
        pqxx::connection conn(conninfo);
        if (!conn.is_open())
            throw std::runtime_error("Cannot connect to database.");

        pqxx::work txn(conn);

        if (do_truncate) {
            std::cout << "Truncating existing tables..." << std::endl;
            txn.exec("TRUNCATE TABLE inspection_region RESTART IDENTITY CASCADE;");
            txn.exec("TRUNCATE TABLE inspection_group RESTART IDENTITY CASCADE;");
        }

        // insert groups
        std::unordered_set<long long> group_ids;
        for (const auto& r : records)
            group_ids.insert(r.group_id);

        txn.conn().prepare(
            "ins_group",
            "INSERT INTO inspection_group(id) VALUES($1) ON CONFLICT (id) DO NOTHING");
        for (auto gid : group_ids)
            txn.exec_prepared("ins_group", gid);

        // insert regions
        txn.conn().prepare(
            "ins_region",
            "INSERT INTO inspection_region(group_id, coord_x, coord_y, category) "
            "VALUES ($1, $2, $3, $4)");
        for (const auto& r : records)
            txn.exec_prepared("ins_region", r.group_id, r.x, r.y, r.category);

        txn.commit();
        std::cout << "Inserted " << records.size() << " records successfully." << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "[ERROR] " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
