#include <unordered_set>
#include <string_view>
#include <algorithm>
#include <vector>
#include <utility>

#include "geo.h"
#include "transport_catalogue.h"
#include "domain.h"

namespace transport_catalogue {
    using namespace std;

    void TransportCatalogue::AddStation(const domain::Station&& station) {
        stations_.emplace_back(station);
        ref_stations_.insert({ stations_.back().name, &stations_.back() });
    }

    void TransportCatalogue::AddRoute(const domain::Route&& route) {
        routes_.emplace_back(route);
        ref_routes_.insert({ routes_.back().name, &routes_.back() });
    }

    void TransportCatalogue::AddDistanceBetweenStations(string_view from, string_view to, int distance) {
        const domain::Station* station_from = Station(from);
        const domain::Station* station_to = Station(to);
        auto key = make_pair(station_from, station_to);
        distances_between_stations_[key] = distance;
    }

    const domain::Station* TransportCatalogue::Station(string_view name) const {
        return ref_stations_.at(name);
    }

    const domain::Route* TransportCatalogue::Route(string_view name) const {
        return ref_routes_.at(name);
    }

    int TransportCatalogue::DistanceBetweenStations(string_view from, string_view to) const {
        const domain::Station* station_from = Station(from);
        const domain::Station* station_to = Station(to);
        auto key = make_pair(station_from, station_to);
        if (!distances_between_stations_.contains(key)) {
            key = make_pair(station_to, station_from);
        }
        return distances_between_stations_.at(key);
    }

    const std::unordered_map<std::string_view, const domain::Route*>* TransportCatalogue::AllRoutesAsMap() const {
        return &ref_routes_;
    }

    const std::unordered_map<std::string_view, const domain::Station*>* TransportCatalogue::AllStationsAsMap() const
    {
        return &ref_stations_;
    }

    const RouteInfo TransportCatalogue::RouteInformation(string_view name) const {
        if (!ref_routes_.contains(name)) {
            return {};
        }
        auto stations = Route(name)->stations;

        size_t total_station_number = stations.size();

        unordered_set<const domain::Station*> unique_stations(stations.begin(), stations.end());
        size_t unique_station_number = unique_stations.size();

        double geographical_route_length = 0;
        double total_route_length = 0;

        for (size_t i = 0; i < stations.size() - 1; ++i) {
            geographical_route_length += geo::ComputeDistance(stations[i]->coordinates, stations[i + 1]->coordinates);
            total_route_length += DistanceBetweenStations(stations[i]->name, stations[i + 1]->name);
        }
        double curvature = total_route_length / geographical_route_length;
        return RouteInfo{ total_station_number, unique_station_number, geographical_route_length, total_route_length, curvature };
    }

    const std::optional<std::set <const domain::Route*, route_comporator::Less>> TransportCatalogue::StationsRoutes(string_view name) const {
        if (!ref_stations_.contains(name)) {
            return {};
        }
        set <const domain::Route*, route_comporator::Less> routes_through_station;
        for (const auto& [route_name, route] : ref_routes_) {
            auto stations = route->stations;
            if (ranges::any_of(stations, [&](const domain::Station* station) { return station->name == name;})) {
                routes_through_station.insert(route);
            }
        }
        return routes_through_station;
    }

    size_t TransportCatalogue::StationsNumber() const {
        return stations_.size();
    }
}

