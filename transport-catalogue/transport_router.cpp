#include <utility>
#include <string_view>
#include <vector>
#include <iterator>

#include "transport_router.h"
#include "transport_catalogue.h"
#include "graph.h"
#include "router.h"
#include "domain.h"

namespace transport_router {
	void TransportRouter::Initialize() {
		InitializeGraph();		
		InitializeArrivalAndDepartureStationsId();
		InitializeWaitingEdgesId();
		InitializeMovingEdgesId();
		graph::Router<double> router(graph_);
		router_ = std::move(router);
	}

	TransportRouter::TransportRouter(const transport_catalogue::TransportCatalogue& catalogue) {
		catalogue_ = &catalogue;
	}

	RouteBetweenTwoStationsInfo TransportRouter::BuildRoute(const std::string_view from, const std::string_view to) const {
		RouteBetweenTwoStationsInfo info;
		const domain::Station* from_ptr = catalogue_->Station(from);
		const domain::Station* to_ptr = catalogue_->Station(to);
		const size_t from_id = id_by_arrival_stations_.at(from_ptr);
		const size_t to_id = id_by_arrival_stations_.at(to_ptr);
		auto route_info = router_.BuildRoute(from_id, to_id);

		if (!route_info) {
			return info;
		}

		info.total_time = route_info.value().weight;

		std::vector<PassengerActivity> activities;
		const auto* const edges = &route_info.value().edges;
		for (const auto& edge_id : *edges) {
			const EdgeInfo edge_info = edges_info_.at(edge_id);
			if (edge_info.type == EdgeType::MOVING) {
				MovingActivity activity;
				activity.route = edge_info.name;
				activity.span_count = edge_info.span_count;
				activity.time = edge_info.time;
				activities.emplace_back(std::move(activity));
			}
			if (edge_info.type == EdgeType::WAITING) {
				WaitingActivity activity;
				activity.stop_name = edge_info.name;
				activity.time = edge_info.time;
				activities.emplace_back(std::move(activity));
			}
		}
		info.items = std::move(activities);

		return info;
	}

	void TransportRouter::SetRoutingSettings(RoutingSettings&& routing_settings) {
		const double METERS_PER_MINUTE_SPEED_CONVERSION_FACTOR = 1000.0 / 60.0;
		routing_settings_.bus_velocity = std::move(routing_settings.bus_velocity);
		routing_settings_.bus_wait_time = std::move(routing_settings.bus_wait_time);
		routing_settings_.bus_velocity *= METERS_PER_MINUTE_SPEED_CONVERSION_FACTOR;
	}

	transport_router::TransportRouter::DepartureStation TransportRouter::CreateDepartureStationFromArrivalStation(const ArrivalStation* station) const {
		return DepartureStation{"Departure: " + station->name};
	}

	void TransportRouter::InitializeArrivalAndDepartureStationsId() {
		arrival_stations_by_id_.clear();
		id_by_arrival_stations_.clear();
		departure_stations_by_id_.clear();
		id_by_departure_stations_.clear();

		const auto* const catalogue_map_stations_ptr = catalogue_->AllStationsAsMap();		
		size_t station_id = 0;

		for (const auto& [key, station] : *catalogue_map_stations_ptr) {
			arrival_stations_by_id_[station_id] = station;
			id_by_arrival_stations_[station] = station_id;
			++station_id;

			departure_stations_.push_back(CreateDepartureStationFromArrivalStation(station));
			departure_stations_by_id_[station_id] = &departure_stations_.back();
			id_by_departure_stations_[&departure_stations_.back()] = station_id;
			departure_stations_by_arrival_stations_[station] = &departure_stations_.back();
			departure_station_id_by_arrival_station_[station] = station_id;
			++station_id;			
		}
	}

	void TransportRouter::InitializeMovingEdgesId() {
		const auto* const catalogue_map_routes_ptr = catalogue_->AllRoutesAsMap();
		for (const auto& [route_name, route] : *catalogue_map_routes_ptr) {			
			for (auto from_it = route->stations.begin(); from_it != route->stations.end(); ++from_it) {
				const size_t from_station_id = departure_station_id_by_arrival_station_.at(*from_it);
				int span_count = 0;
				double edge_weight = 0;				
				std::string from_station_name = (*from_it)->name; 

				for (auto to_it = std::next(from_it); to_it != route->stations.end(); ++to_it) {
					const size_t to_station_id = id_by_arrival_stations_.at(*to_it);
					edge_weight += catalogue_->DistanceBetweenStations(from_station_name, (*to_it)->name);
					from_station_name = (*to_it)->name;
					const double weight_as_time = edge_weight / routing_settings_.bus_velocity;
					const graph::Edge<double> moving_edge{ .from = from_station_id , .to = to_station_id , .weight = weight_as_time };
					const graph::EdgeId edge_id = graph_.AddEdge(moving_edge);
					++span_count;
					const EdgeInfo edge_info{ .type = EdgeType::MOVING, .name = route_name, .time = weight_as_time , .span_count = span_count };
					edges_info_[edge_id] = edge_info;
				}
			}
		}
	}

	void TransportRouter::InitializeWaitingEdgesId() {
		for (const auto& [from_station_id, arrival_station] : arrival_stations_by_id_) {
			const size_t to_station_id = departure_station_id_by_arrival_station_.at(arrival_station);
			const graph::Edge<double> waiting_edge{ from_station_id , to_station_id , routing_settings_.bus_wait_time };
			const graph::EdgeId edge_id = graph_.AddEdge(waiting_edge);
			const EdgeInfo edge_info{ .type = EdgeType::WAITING, .name = arrival_station->name, .time = routing_settings_.bus_wait_time };
			edges_info_[edge_id] = edge_info;
		}
	}

	void TransportRouter::InitializeGraph() {
		const size_t graph_size = 2 * catalogue_->AllStationsAsMap()->size();
		graph::DirectedWeightedGraph<double> graph(graph_size);
		graph_ = std::move(graph);
	}
} // namespace transport_router