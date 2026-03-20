#pragma once

#include <iostream>

#include "json.h"
#include "map_renderer.h"
#include "request_handler.h"
#include "transport_catalogue.h"
#include "svg.h"

namespace json_reader {
	class JsonReader {
	public:
		friend std::istream& operator>>(std::istream& in, JsonReader& reader);
		friend std::ostream& operator<<(std::ostream& out, const JsonReader& reader);

		void SetRequestHandler(request_handler::RequestHandler& request_handler) noexcept;
		void ParseInputDocument();
		void SendRequestToRequestHandler();

	private:
		request_handler::AddStationCommand FormAddStationCommandForRequestHandler(const json::Dict* dict);
		request_handler::AddRouteCommand FormAddRouteCommandForRequestHandler(const json::Dict* dict);
		transport_router::RoutingSettings FormRoutingSettingsForRequestHandler(const json::Dict* dict);
		request_handler::ReadCommand FormReadCommandForRequestHandler(const json::Dict* dict);
		map_renderer::RenderSettings FormRenderSettingsForRequestHandler(const json::Dict* dict);

		void CreateOutputDocument(request_handler::AnswerLine&& answer_line);

		request_handler::RequestHandler* request_handler_;
		request_handler::RequestsLine request_line_for_request_handler_;
		map_renderer::RenderSettings render_settings_for_request_handler_;
		transport_router::RoutingSettings routing_settings_for_request_handler_;
		json::Document input_document_;
		json::Document output_document_;
	};

	inline svg::Color GetColorFromJsonNode(const json::Node& node) {
		if (node.IsString()) {
			return node.AsString();
		}
		if (node.IsArray()) {
			const auto* color_array = &node.AsArray();
			if (color_array->size() == 3) {

				return svg::Rgb{
					static_cast<uint8_t>(color_array->at(0).AsInt()),
					static_cast<uint8_t>(color_array->at(1).AsInt()),
					static_cast<uint8_t>(color_array->at(2).AsInt())
				};
			}
			if (color_array->size() == 4) {
				return svg::Rgba{
					static_cast<uint8_t>(color_array->at(0).AsInt()),
					static_cast<uint8_t>(color_array->at(1).AsInt()),
					static_cast<uint8_t>(color_array->at(2).AsInt()),
					color_array->at(3).AsDouble()
				};
			}
		}
		return svg::Color();
	}

	inline std::istream& operator>>(std::istream& in, JsonReader& reader) {
		reader.input_document_ = json::Load(in);
		return in;
	}

	inline std::ostream& operator<<(std::ostream& out, const JsonReader& reader) {
		Print(reader.output_document_, out);
		return out;
	}
}
