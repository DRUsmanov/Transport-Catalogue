#include <string>
#include <unordered_map>
#include <variant>

#include "geo.h"
#include "json.h"
#include "json_reader.h"
#include "request_handler.h"
#include "svg.h"
#include "json_builder.h"

namespace json_reader {
	void JsonReader::SetRequestHandler(request_handler::RequestHandler& request_handler) noexcept {
		request_handler_ = &request_handler;
	}

	void JsonReader::ParseInputDocument() {
		const json::Array* base_requests_in_json = nullptr;
		const json::Array* stat_requests_in_json = nullptr;
		const json::Dict* render_settings_in_json = nullptr;
		const json::Dict* routing_settings_in_json = nullptr;
		if (input_document_.GetRoot().AsDict().contains("base_requests")) {
			base_requests_in_json = &input_document_.GetRoot().AsDict().at("base_requests").AsArray();
		}
		if (input_document_.GetRoot().AsDict().contains("stat_requests")) {
			stat_requests_in_json = &input_document_.GetRoot().AsDict().at("stat_requests").AsArray();
		}
		if (input_document_.GetRoot().AsDict().contains("render_settings")) {
			render_settings_in_json = &input_document_.GetRoot().AsDict().at("render_settings").AsDict();
		}
		if (input_document_.GetRoot().AsDict().contains("routing_settings")) {
			routing_settings_in_json = &input_document_.GetRoot().AsDict().at("routing_settings").AsDict();
		}

		if (base_requests_in_json) {
			for (const auto& request : *base_requests_in_json) {
				const auto* request_as_map = &request.AsDict();
				std::string request_type = request_as_map->at("type").AsString();
				if (request_type == "Stop") {
					request_handler::AddStationCommand command = FormAddStationCommandForRequestHandler(request_as_map);
					request_line_for_request_handler_.emplace_back(std::move(command));
				}
				if (request_type == "Bus") {
					request_handler::AddRouteCommand command = FormAddRouteCommandForRequestHandler(request_as_map);
					request_line_for_request_handler_.emplace_back(std::move(command));
				}
			}
		}		

		if (stat_requests_in_json) {
			for (const auto& request : *stat_requests_in_json) {
				const auto* request_as_map = &request.AsDict();
				request_handler::ReadCommand command = FormReadCommandForRequestHandler(request_as_map);
				request_line_for_request_handler_.emplace_back(std::move(command));
			}
		}
		
		if (render_settings_in_json) {
			render_settings_for_request_handler_ = FormRenderSettingsForRequestHandler(render_settings_in_json);
		}

		if (routing_settings_in_json) {
			routing_settings_for_request_handler_ = FormRoutingSettingsForRequestHandler(routing_settings_in_json);			
		}

		input_document_.Clear();
	}

	void JsonReader::SendRequestToRequestHandler() {
		request_handler_->SetRequestsLine(std::move(request_line_for_request_handler_));
		request_handler_->SendRenderSettingsToMapRenderer(std::move(render_settings_for_request_handler_));
		request_handler_->SendRoutingSettingsToTransportRouter(std::move(routing_settings_for_request_handler_));
		request_handler_->SetResponceReciver([this](request_handler::AnswerLine&& answer_line) {CreateOutputDocument(std::move(answer_line));});
	}

	request_handler::AddStationCommand JsonReader::FormAddStationCommandForRequestHandler(const json::Dict* dict) {
		request_handler::AddStationCommand command;
		std::string name = dict->at("name").AsString();
		const double lat = dict->at("latitude").AsDouble();
		const double lng = dict->at("longitude").AsDouble();
		command.name = name;
		command.coordinate = { lat, lng };
						
		if (!dict->at("road_distances").AsDict().empty()) {
			std::unordered_map<std::string, int> road_distances;
			const auto* road_distances_ptr = &dict->at("road_distances").AsDict();
			for (const auto& [neibor_station_name, distance_to_neibor_station] : *road_distances_ptr) {
				road_distances[neibor_station_name] = distance_to_neibor_station.AsInt();
			}
			command.road_distances = road_distances;
		}

		return command;
	}

	request_handler::AddRouteCommand JsonReader::FormAddRouteCommandForRequestHandler(const json::Dict* dict) {
		request_handler::AddRouteCommand command;
		const std::string name = dict->at("name").AsString();
		const bool is_roundtrip = dict->at("is_roundtrip").AsBool();
		command.name = name;
		command.is_roundtrip = is_roundtrip;
		
		if (!dict->at("stops").AsArray().empty()) {
			std::vector<std::string> stations_on_route;
			const auto* stations_on_route_ptr = &dict->at("stops").AsArray();
			for (const auto& station : *stations_on_route_ptr) {
				stations_on_route.push_back(station.AsString());
			}
			command.stations_on_route = stations_on_route;
		}

		return command;		
	}

	transport_router::RoutingSettings JsonReader::FormRoutingSettingsForRequestHandler(const json::Dict* dict) {
		transport_router::RoutingSettings settings;
		int bus_wait_time = dict->at("bus_wait_time").AsInt();
		int bus_velocity = dict->at("bus_velocity").AsInt();
		settings.bus_wait_time = bus_wait_time;
		settings.bus_velocity = bus_velocity;
		return settings;
	}

	request_handler::ReadCommand JsonReader::FormReadCommandForRequestHandler(const json::Dict* dict) {
		request_handler::ReadCommand command;
		const size_t id = dict->at("id").AsInt();
		const std::string type = dict->at("type").AsString();
		command.id = id;
		command.type = type;
		if (type != "Map" && type != "Route") {
			const std::string name = dict->at("name").AsString();
			command.name = name;
		}
		if (type == "Route") {
			const std::string from = dict->at("from").AsString();
			const std::string to = dict->at("to").AsString();
			command.from = from;
			command.to = to;
		}
		return command;
	}

	map_renderer::RenderSettings JsonReader::FormRenderSettingsForRequestHandler(const json::Dict* render_settings) {
		map_renderer::RenderSettings settings;
		settings.width = render_settings->at("width").AsDouble();
		settings.height = render_settings->at("height").AsDouble();
		settings.padding = render_settings->at("padding").AsDouble();
		settings.line_width = render_settings->at("line_width").AsDouble();
		settings.stop_radius = render_settings->at("stop_radius").AsDouble();

		settings.bus_label_font_size = render_settings->at("bus_label_font_size").AsInt();
		settings.bus_label_offset.x = render_settings->at("bus_label_offset").AsArray().at(0).AsDouble();
		settings.bus_label_offset.y = render_settings->at("bus_label_offset").AsArray().at(1).AsDouble();

		settings.stop_label_font_size = render_settings->at("stop_label_font_size").AsInt();
		settings.stop_label_offset.x = render_settings->at("stop_label_offset").AsArray().at(0).AsDouble();
		settings.stop_label_offset.y = render_settings->at("stop_label_offset").AsArray().at(1).AsDouble();

		settings.underlayer_color = GetColorFromJsonNode(render_settings->at("underlayer_color"));
		settings.underlayer_width = render_settings->at("underlayer_width").AsDouble();

		const auto* color_palette = &render_settings->at("color_palette").AsArray();
		for (const auto& color : *color_palette) {
			settings.color_palette.push_back(GetColorFromJsonNode(color));
		}

		return settings;
	}

	void JsonReader::CreateOutputDocument(request_handler::AnswerLine&& answer_line) {
		using namespace json;	
       
		if (answer_line.empty()) {
			return;
		}
        Builder builder;
		builder.StartArray();
		for (const auto& answer : answer_line) {
			if (std::holds_alternative<request_handler::OnStationRequestAnswer>(answer)) {
				const auto* on_station_request_answer = &std::get<request_handler::OnStationRequestAnswer>(answer);
				builder.StartDict();
				if (!on_station_request_answer->is_found_in_catalogue) {
					builder.Key("error_message").Value((std::string)"not found");
				}
				else {
					builder.Key("buses").StartArray();
					if (on_station_request_answer->routes_through_station.has_value()) {
						for (const auto& route : on_station_request_answer->routes_through_station.value()) {
							builder.Value((*route).name);
						}
					}
					builder.EndArray();
				}
				builder.Key("request_id").Value((int)on_station_request_answer->request_id).EndDict();						
			}

			if (std::holds_alternative<request_handler::OnRouteRequestAnswer>(answer)) {
				const auto* on_route_request_answer = &std::get<request_handler::OnRouteRequestAnswer>(answer);
				builder.StartDict();
				if (!on_route_request_answer->is_found_in_catalogue) {
					builder.Key("error_message").Value((std::string)"not found");
				}
				else {
					builder.Key("stop_count").Value((int)on_route_request_answer->total_station_number.value());
					builder.Key("unique_stop_count").Value((int)on_route_request_answer->unique_station_number.value());
					builder.Key("route_length").Value((double)on_route_request_answer->total_route_length.value());
					builder.Key("curvature").Value((double)on_route_request_answer->curvature.value());		
				}
				builder.Key("request_id").Value((int)on_route_request_answer->request_id).EndDict();
			}

			if (std::holds_alternative<request_handler::OnMapRequestAnswer>(answer)) {
				const auto* on_map_request_answer = &std::get<request_handler::OnMapRequestAnswer>(answer);
				builder.StartDict();
				builder.Key("map").Value(on_map_request_answer->rendered_map);
				builder.Key("request_id").Value((int)on_map_request_answer->request_id);
				builder.EndDict();
			}

			if (std::holds_alternative<request_handler::OnRouteBetweenTwoStationsAnswer>(answer)) {
				const auto* on_route_between_two_stations_answer = &std::get<request_handler::OnRouteBetweenTwoStationsAnswer>(answer);
				builder.StartDict();
				builder.Key("request_id").Value((int)on_route_between_two_stations_answer->request_id);
				if (!on_route_between_two_stations_answer->is_route_possible) {
					builder.Key("error_message").Value((std::string)"not found");
				}
				else {
					builder.Key("total_time").Value((double)on_route_between_two_stations_answer->total_time.value());
					builder.Key("items").StartArray();
					for (const auto& activity : on_route_between_two_stations_answer->items.value()) {
						builder.StartDict();
						if (std::holds_alternative<transport_router::WaitingActivity>(activity)) {
							const auto* waiting_activity = &std::get<transport_router::WaitingActivity>(activity);
							builder.Key("type").Value((std::string)"Wait");
							builder.Key("stop_name").Value((std::string)waiting_activity->stop_name);
							builder.Key("time").Value((double)waiting_activity->time);
						}
						if (std::holds_alternative<transport_router::MovingActivity>(activity)) {
							const auto* moving_activity = &std::get<transport_router::MovingActivity>(activity);
							builder.Key("type").Value((std::string)"Bus");
							builder.Key("bus").Value((std::string)moving_activity->route);
							builder.Key("span_count").Value((double)moving_activity->span_count);
							builder.Key("time").Value((double)moving_activity->time);
						}
						builder.EndDict();
					}
					builder.EndArray();
				}
				builder.EndDict();
			}
		}
		builder.EndArray();
		output_document_ = json::Document(builder.Build());
	}

}

