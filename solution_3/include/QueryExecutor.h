#ifndef REGION_QUERY_QUERY_EXECUTOR_H
#define REGION_QUERY_QUERY_EXECUTOR_H

#include <string>

int runQuery(const std::string& conninfo,
             const std::string& queryFile,
             const std::string& outFile,
             bool debug);

#endif // REGION_QUERY_QUERY_EXECUTOR_H
