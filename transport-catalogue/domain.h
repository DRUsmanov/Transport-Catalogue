#pragma once

#include <string>
#include <vector>

#include "geo.h"

namespace domain {
    struct Station {
        std::string name;
        geo::Coordinates coordinates;
    };

    struct Route {
        std::string name;
        std::vector<const Station*> stations;
        bool is_roundtrip;
    };
}

