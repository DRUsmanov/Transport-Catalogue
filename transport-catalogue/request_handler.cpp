#include <sstream>
#include <variant>
#include <vector>
#include <unordered_map>
#include <string>
#include <utility>
#include <set>

#include "request_handler.h"
#include "transport_catalogue.h"
#include "map_renderer.h"

void request_handler::RequestHandler::SetCatalogue(transport_catalogue::TransportCatalogue& catalogue) noexcept {
	catalogue_ = &catalogue;
}

void request_handler::RequestHandler::SetRouter(transport_router::TransportRouter& transport_router) noexcept {
	transport_router_ = &transport_router;
}

void request_handler::RequestHandler::SetMapRenderer(map_renderer::MapRederer& map_renderer) noexcept {
	map_renderer_ = &map_renderer;
}

void request_handler::RequestHandler::SetResponceReciver(std::function<void(AnswerLine&& answer_line)>&& responce_reciver){
	responce_reciver_ = std::move(responce_reciver);
}

void request_handler::RequestHandler::SetRequestsLine(RequestsLine&& requests_line) noexcept {
	request_line_ = std::move(requests_line);
}

void request_handler::RequestHandler::SendRenderSettingsToMapRenderer(map_renderer::RenderSettings&& render_settings) noexcept {
	map_renderer_->SetRenderSettings(std::move(render_settings));
}

void request_handler::RequestHandler::SendRoutingSettingsToTransportRouter(transport_router::RoutingSettings&& routing_settings) noexcept {
	transport_router_->SetRoutingSettings(std::move(routing_settings));
}

void request_handler::RequestHandler::SendAnswerToResponceReciver() {
	if (responce_reciver_) {
		responce_reciver_(std::move(answer_line_));
	}
}

void request_handler::RequestHandler::ProcessModificationRequests() {
	if (request_line_.empty()) {
		return;
	}
	// Добавляем остановки в каталог 
	for (const auto& request : request_line_) {
		if (std::holds_alternative<AddStationCommand>(request)) {
			const AddStationCommand command = std::get<AddStationCommand>(request);
			domain::Station station{ command.name, command.coordinate };
			catalogue_->AddStation(std::move(station));
		}
	}
	// Добавляем растояния между остановками в каталог
	for (const auto& request : request_line_) {
		if (std::holds_alternative<AddStationCommand>(request)) {
			const AddStationCommand command = std::get<AddStationCommand>(request);
			if (command.road_distances.has_value()) {
				const std::unordered_map<std::string, int>* road_distances_ptr = &command.road_distances.value();
				for (const auto& [neibor_station_name, distance_to_neibor_station] : *road_distances_ptr) {
					catalogue_->AddDistanceBetweenStations(command.name, neibor_station_name, distance_to_neibor_station);
				}
			}
		}
	}
	// Добавляем маршруты в каталог
	for (const auto& request : request_line_) {
		if (std::holds_alternative<AddRouteCommand>(request)) {
			AddRouteCommand command = std::get<AddRouteCommand>(request);
			std::vector<const domain::Station*> stations_on_route;
			if (command.stations_on_route.has_value()) {
				const std::vector<std::string>* stations_on_route_ptr = &command.stations_on_route.value();
				for (const auto& station_name : *stations_on_route_ptr) {
					stations_on_route.emplace_back(catalogue_->Station(station_name));
				}
				if (!command.is_roundtrip) {
					auto stations_on_route_copy = stations_on_route;
					for (auto it = next(stations_on_route_copy.rbegin()); it != stations_on_route_copy.rend(); ++it) {
						stations_on_route.emplace_back(*it);
					}
				}				
			}
			domain::Route route{ std::move(command.name), std::move(stations_on_route), std::move(command.is_roundtrip) };
			catalogue_->AddRoute(std::move(route));
		}
	}
	
}

void request_handler::RequestHandler::ProcessReadRequests() {
	for (const auto& request : request_line_) {
		if (std::holds_alternative<ReadCommand>(request)) {
			const ReadCommand command = std::get<ReadCommand>(request);
			if (command.type == "Stop") {
				const std::optional<std::set <const domain::Route*, transport_catalogue::route_comporator::Less>> routes_through_station = catalogue_->StationsRoutes(command.name.value());

				OnStationRequestAnswer answer{
					command.id,
					routes_through_station };

				if (routes_through_station) {
					answer.is_found_in_catalogue = true;
				}

				answer_line_.emplace_back(answer);
			}

			if (command.type == "Bus") {
				transport_catalogue::RouteInfo info = catalogue_->RouteInformation(command.name.value());

				OnRouteRequestAnswer answer{
					command.id,
					info.total_station_number,
					info.unique_station_number,
					info.geographical_route_length,
					info.total_route_length,
					info.curvature };

				if (info) {
					answer.is_found_in_catalogue = true;
				}

				answer_line_.emplace_back(answer);
			}

			if (command.type == "Map") {
				std::set<const domain::Route*, transport_catalogue::route_comporator::Less> routes_in_lexicographic_order;
				std::set<const domain::Station*, transport_catalogue::station_comporator::Less> stations_in_lelexicographic_order;
				const auto catalogue_map_stations_ptr = catalogue_->AllRoutesAsMap();
				for (const auto& [key, route] : *catalogue_map_stations_ptr) {
					if (route->stations.empty()) {
						continue;
					}
					routes_in_lexicographic_order.insert(route);
					for (const auto& station : route->stations) {
						stations_in_lelexicographic_order.insert(station);
					}
				}
				map_renderer_->SetRoutes(std::move(routes_in_lexicographic_order));
				map_renderer_->SetStations(std::move(stations_in_lelexicographic_order));
				map_renderer_->InitializeSphereProjector();
				map_renderer_->CreateSvgDocument();
				std::ostringstream s;
				map_renderer_->PrintMap(s);

				OnMapRequestAnswer answer{
					command.id,
					s.str() };

				answer_line_.emplace_back(answer);
			}

			if (command.type == "Route") {
				transport_router::RouteBetweenTwoStationsInfo info = transport_router_->BuildRoute(command.from.value(), command.to.value());
				OnRouteBetweenTwoStationsAnswer answer;
				answer.request_id = command.id;
				if (info) {
					answer.total_time = info.total_time;
					answer.items = std::move(info.items);
					answer.is_route_possible = true;
				}
				answer_line_.emplace_back(answer);
			}
		}
	}
	request_line_.clear();
}

