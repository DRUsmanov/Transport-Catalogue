#pragma once

#include <algorithm>
#include <cstdlib>
#include <optional>
#include <vector>
#include <set>
#include <utility>

#include "domain.h"
#include "geo.h"
#include "svg.h"
#include "transport_catalogue.h"

namespace map_renderer {

    inline const double EPSILON = 1e-6;
    inline bool IsZero(double value) {
        return std::abs(value) < EPSILON;
    }

    class SphereProjector {
    public:
        SphereProjector() = default;
        template <typename PointInputIt>        
        SphereProjector(PointInputIt points_begin, PointInputIt points_end,
            double max_width, double max_height, double padding)
            : padding_(padding)
        {
            if (points_begin == points_end) {
                return;
            }

            const auto [left_it, right_it] = std::minmax_element(
                points_begin, points_end,
                [](auto lhs, auto rhs) { return lhs->lng < rhs->lng; });
            min_lon_ = (*left_it)->lng;
            const double max_lon = (*right_it)->lng;

            const auto [bottom_it, top_it] = std::minmax_element(
                points_begin, points_end,
                [](auto lhs, auto rhs) { return lhs->lat < rhs->lat; });
            const double min_lat = (*bottom_it)->lat;
            max_lat_ = (*top_it)->lat;

            std::optional<double> width_zoom;
            if (!IsZero(max_lon - min_lon_)) {
                width_zoom = (max_width - 2 * padding) / (max_lon - min_lon_);
            }

            std::optional<double> height_zoom;
            if (!IsZero(max_lat_ - min_lat)) {
                height_zoom = (max_height - 2 * padding) / (max_lat_ - min_lat);
            }

            if (width_zoom && height_zoom) {
                zoom_coeff_ = std::min(*width_zoom, *height_zoom);
            }
            else if (width_zoom) {
                zoom_coeff_ = *width_zoom;
            }
            else if (height_zoom) {
                zoom_coeff_ = *height_zoom;
            }
        }

        svg::Point operator()(geo::Coordinates coords) const {
            return {
                (coords.lng - min_lon_) * zoom_coeff_ + padding_,
                (max_lat_ - coords.lat) * zoom_coeff_ + padding_
            };
        }

    private:
        double padding_ = 0;
        double min_lon_ = 0;
        double max_lat_ = 0;
        double zoom_coeff_ = 0;
    };
    
    struct RenderSettings {
        double width; //ширина изображения
        double height; //высота изображения
        double padding; //отступ от краев изображения
        double line_width; //толщина линий, которыми рисуются маршруты
        double stop_radius; //радиус окружностей, которыми обзначаются остановки
        int bus_label_font_size; //размер текста, которым написаны названия автобусных маршрутов
        svg::Point bus_label_offset; //смещение надписи с названием маршрута относительно координат конечной остановки на карте
        int stop_label_font_size; //размер текста, которым отображаются названия остановок
        svg::Point stop_label_offset; //смещение названия остановки относительно её координат на карте
        svg::Color underlayer_color; //цвет подложки под названиями остановок и маршрутов
        double underlayer_width; //толщина подложки под названиями остановок и маршрутов
        std::vector<svg::Color> color_palette; //цветовая палитра
    };

    class MapRederer {
    public:
        void InitializeSphereProjector();

        void SetRoutes(std::set<const domain::Route*, transport_catalogue::route_comporator::Less>&& routes);
        void SetStations(std::set<const domain::Station*, transport_catalogue::station_comporator::Less>&& stations);
        void SetRenderSettings(RenderSettings&& render_settings);

        void CreateSvgDocument();
        void PrintMap(std::ostream& out);

    private:
        svg::Polyline CreateRoutePolyline(const domain::Route* route, const svg::Color& route_color);
        std::pair<svg::Text, svg::Text> CreateRouteText(const domain::Route* route, const svg::Color& text_color, const geo::Coordinates pos);
        std::pair<svg::Text, svg::Text> CreateStationText(const domain::Station* station);
        svg::Color GetColorFromPalette(int current_color_index);

        SphereProjector sphere_projector_;
        svg::Document svg_document_;

        std::set<const domain::Route*, transport_catalogue::route_comporator::Less> routes_in_lexicographic_order_;
        std::set<const domain::Station*, transport_catalogue::station_comporator::Less> stations_in_lelexicographic_order_;
        RenderSettings render_settings_;
    };
}

