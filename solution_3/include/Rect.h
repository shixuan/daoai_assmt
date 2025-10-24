#ifndef REGION_QUERY_RECT_H
#define REGION_QUERY_RECT_H

#include <nlohmann/json.hpp>
#include <stdexcept>

using json = nlohmann::json;

struct Rect {
    double xmin, ymin, xmax, ymax;
};

inline Rect readRect(const json& j) {
    Rect r;
    r.xmin = j.at("p_min").at("x").get<double>();
    r.ymin = j.at("p_min").at("y").get<double>();
    r.xmax = j.at("p_max").at("x").get<double>();
    r.ymax = j.at("p_max").at("y").get<double>();
    if (r.xmin > r.xmax || r.ymin > r.ymax)
        throw std::runtime_error("invalid rectangle");
    return r;
}

#endif // REGION_QUERY_RECT_H
