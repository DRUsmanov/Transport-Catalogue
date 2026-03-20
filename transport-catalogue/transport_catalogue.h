#pragma once

#include <deque>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>

#include "domain.h"

namespace transport_catalogue {
    struct RouteInfo {
        std::optional<size_t> total_station_number;
        std::optional<size_t> unique_station_number;
        std::optional<double> geographical_route_length;
        std::optional<double> total_route_length;
        std::optional<double> curvature;

        operator bool() {
            return total_station_number.has_value();
        }
    };    

    namespace route_comporator {
        struct Less {

            bool operator()(const domain::Route* lhs, const domain::Route* rhs) const {
                return lhs->name < rhs->name;
            }
        };
    }

    namespace station_comporator {
        struct Less {
            bool operator()(const domain::Station* lhs, const domain::Station* rhs) const {
                return lhs->name < rhs->name;
            }
        };
    }

    struct StationInfo {
        std::optional<std::set <const domain::Route*, route_comporator::Less>> routes_through_station;

        operator bool() {
            return routes_through_station.has_value();
        }
    };

    struct DistanceBetweenStationsMapHasher {
        size_t operator()(const std::pair<const domain::Station*, const domain::Station*>& key) const {
            auto hash_part1 = std::hash<const domain::Station*>{}(key.first);
            auto hash_part2 = std::hash<const domain::Station*>{}(key.second);
            return hash_part1 + hash_part2 * 37;
        }
    };

    class TransportCatalogue {
    public:
        void AddStation(const domain::Station&& station);
        void AddRoute(const domain::Route&& route);
        void AddDistanceBetweenStations(std::string_view from, std::string_view to, int distance);

        [[nodiscard]] const domain::Station* Station(std::string_view name) const;
        [[nodiscard]] const domain::Route* Route(std::string_view name) const;
        [[nodiscard]] int DistanceBetweenStations(std::string_view from, std::string_view to) const;
        [[nodiscard]] const std::unordered_map<std::string_view, const domain::Route*>* AllRoutesAsMap() const;
        [[nodiscard]] const std::unordered_map<std::string_view, const domain::Station*>* AllStationsAsMap() const;
        [[nodiscard]] const RouteInfo RouteInformation(std::string_view name) const;
        [[nodiscard]] const std::optional<std::set <const domain::Route*, route_comporator::Less>> StationsRoutes(std::string_view name) const;
        [[nodiscard]] size_t StationsNumber() const;

    private:
        std::deque<domain::Station> stations_;
        std::deque<domain::Route> routes_;
        std::unordered_map<std::string_view, const domain::Station*> ref_stations_;
        std::unordered_map<std::string_view, const domain::Route*> ref_routes_;
        std::unordered_map<std::pair<const domain::Station*, const domain::Station*>, int, DistanceBetweenStationsMapHasher> distances_between_stations_;
    };
} // namespace transport_catalogue


