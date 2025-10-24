#include <gflags/gflags.h>
#include <iostream>

#include "QueryExecutor.h"

DEFINE_string(query, "", "Path to JSON query file");
DEFINE_string(out, "result.txt", "Output file path");
DEFINE_string(conn, "dbname=regiondb user=postgres host=localhost", "DB connection string");
DEFINE_bool(debug, false, "Print SQL");

int main(int argc, char** argv) {
    gflags::SetUsageMessage("Usage: region_query --query=JSON [--out=result.txt] [--conn=...]");
    gflags::ParseCommandLineFlags(&argc, &argv, true);

    if (FLAGS_query.empty()) {
        std::cerr << "[Error] --query is required\n";
        std::cerr << gflags::ProgramUsage() << "\n";
        return 1;
    }

    return runQuery(FLAGS_conn, FLAGS_query, FLAGS_out, FLAGS_debug);
}
