#include <vector>
#include <utility>

#include "map_renderer.h"
#include "geo.h"
#include "svg.h"

void map_renderer::MapRederer::InitializeSphereProjector() {
	std::vector<const geo::Coordinates*> stations_coordinate_range;
	for (const auto& station : stations_in_lelexicographic_order_) {
		stations_coordinate_range.push_back(&station->coordinates);
	}

	sphere_projector_ = SphereProjector(
		stations_coordinate_range.begin(),
		stations_coordinate_range.end(),
		render_settings_.width,
		render_settings_.height,
		render_settings_.padding);
}

void map_renderer::MapRederer::SetRoutes(std::set<const domain::Route*, transport_catalogue::route_comporator::Less>&& routes) {
	routes_in_lexicographic_order_ = std::move(routes);
}

void map_renderer::MapRederer::SetStations(std::set<const domain::Station*, transport_catalogue::station_comporator::Less>&& stations) {
	stations_in_lelexicographic_order_ = std::move(stations);
}

void map_renderer::MapRederer::SetRenderSettings(RenderSettings&& render_settings) {
	render_settings_ = std::move(render_settings);
}

void map_renderer::MapRederer::CreateSvgDocument() {
	int current_color_index = 0;
	for (const auto& route : routes_in_lexicographic_order_) {		
		svg::Color route_color = GetColorFromPalette(current_color_index);
		svg::Polyline route_polyline = CreateRoutePolyline(route, route_color);
		svg_document_.Add(route_polyline);
		++current_color_index;
	}
	
	current_color_index = 0;
	for (const auto& route : routes_in_lexicographic_order_) {
		svg::Color text_color = GetColorFromPalette(current_color_index);
		geo::Coordinates start_station_pos = route->stations.front()->coordinates;
		std::pair<svg::Text, svg::Text> route_text = CreateRouteText(route, text_color, start_station_pos);
		svg_document_.Add(route_text.first);
		svg_document_.Add(route_text.second);
		++current_color_index;
		if (!route->is_roundtrip) {
			geo::Coordinates final_station_pos = (route->stations.at(route->stations.size() / 2))->coordinates;
			if (start_station_pos == final_station_pos) {
				continue;
			}
			std::pair<svg::Text, svg::Text> route_text = CreateRouteText(route, text_color, final_station_pos);
			svg_document_.Add(route_text.first);
			svg_document_.Add(route_text.second);
		}
	}
	
	for (const auto& station : stations_in_lelexicographic_order_) {
		svg::Circle station_circle;
		auto circle_center = sphere_projector_(station->coordinates);
		station_circle
			.SetCenter(circle_center)
			.SetRadius(render_settings_.stop_radius)
			.SetFillColor("white");
		svg_document_.Add(station_circle);
	}
	
	for (const auto& station : stations_in_lelexicographic_order_) {
		std::pair<svg::Text, svg::Text> station_text = CreateStationText(station);
		svg_document_.Add(station_text.first);
		svg_document_.Add(station_text.second);
	}
}

void map_renderer::MapRederer::PrintMap(std::ostream& out) {
	svg_document_.Render(out);
}

svg::Polyline map_renderer::MapRederer::CreateRoutePolyline(const domain::Route* route, const svg::Color& route_color) {
	svg::Polyline route_line;
	for (const auto& station : route->stations) {
		route_line.AddPoint(sphere_projector_(station->coordinates));
	}
	route_line
		.SetStrokeColor(route_color)
		.SetStrokeWidth(render_settings_.line_width)
		.SetStrokeLineCap(svg::StrokeLineCap::ROUND)
		.SetStrokeLineJoin(svg::StrokeLineJoin::ROUND)
		.SetFillColor(svg::NoneColor);
	return route_line;
}

std::pair<svg::Text, svg::Text> map_renderer::MapRederer::CreateRouteText(const domain::Route* route, const svg::Color& text_color, const geo::Coordinates pos) {
	svg::Text route_text;
	svg::Text route_text_with_background;
	const svg::Point route_text_pos = sphere_projector_(pos);
	route_text
		.SetData(route->name)
		.SetPosition(route_text_pos)
		.SetOffset(render_settings_.bus_label_offset)
		.SetFontSize(render_settings_.bus_label_font_size)
		.SetFontFamily("Verdana")
		.SetFontWeight("bold")
		.SetFillColor(text_color);

	route_text_with_background
		.SetData(route->name)
		.SetPosition(route_text_pos)
		.SetOffset(render_settings_.bus_label_offset)
		.SetFontSize(render_settings_.bus_label_font_size)
		.SetFontFamily("Verdana")
		.SetFontWeight("bold")
		.SetFillColor(render_settings_.underlayer_color)
		.SetStrokeColor(render_settings_.underlayer_color)
		.SetStrokeWidth(render_settings_.underlayer_width)
		.SetStrokeLineCap(svg::StrokeLineCap::ROUND)
		.SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
	return { route_text_with_background, route_text };
}

std::pair<svg::Text, svg::Text> map_renderer::MapRederer::CreateStationText(const domain::Station* station) {
	svg::Text station_text;
	svg::Text station_text_with_background;
	const svg::Point station_text_pos = sphere_projector_(station->coordinates);
	station_text
		.SetData(station->name)
		.SetPosition(station_text_pos)
		.SetOffset(render_settings_.stop_label_offset)
		.SetFontSize(render_settings_.stop_label_font_size)
		.SetFontFamily("Verdana")
		.SetFillColor("black");

	station_text_with_background
		.SetData(station->name)
		.SetPosition(station_text_pos)
		.SetOffset(render_settings_.stop_label_offset)
		.SetFontSize(render_settings_.stop_label_font_size)
		.SetFontFamily("Verdana")
		.SetFillColor(render_settings_.underlayer_color)
		.SetStrokeColor(render_settings_.underlayer_color)
		.SetStrokeWidth(render_settings_.underlayer_width)
		.SetStrokeLineCap(svg::StrokeLineCap::ROUND)
		.SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
	return { station_text_with_background , station_text };
}

svg::Color map_renderer::MapRederer::GetColorFromPalette(int current_color_index) {
	const size_t color_palette_size = render_settings_.color_palette.size();
	svg::Color route_color = render_settings_.color_palette[current_color_index % color_palette_size];
	return route_color;
}
