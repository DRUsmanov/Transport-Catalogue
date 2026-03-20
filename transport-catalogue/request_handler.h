#pragma once

#include <functional>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>
#include <variant>

#include "domain.h"
#include "geo.h"
#include "map_renderer.h"
#include "transport_catalogue.h"
#include "transport_router.h"



namespace request_handler {
    struct AddStationCommand {
        std::string name;
        geo::Coordinates coordinate;
        std::optional<std::unordered_map<std::string, int>> road_distances;
    };

    struct AddRouteCommand {
        std::string name;
        std::optional<std::vector<std::string>> stations_on_route;
        bool is_roundtrip;
    };

    struct ReadCommand {
        size_t id;
        std::string type;
        std::optional<std::string> name;
        std::optional<std::string> from;
        std::optional<std::string> to;
    };

    struct OnStationRequestAnswer {
        size_t request_id;
        std::optional<std::set <const domain::Route*, transport_catalogue::route_comporator::Less>> routes_through_station;
        bool is_found_in_catalogue = false;
    };

    struct OnRouteBetweenTwoStationsAnswer {
        size_t request_id;
        std::optional<double> total_time;
        std::optional<std::vector<transport_router::PassengerActivity>> items;
        bool is_route_possible = false;
    };

    struct OnRouteRequestAnswer {
        size_t request_id;
        std::optional<size_t> total_station_number;
        std::optional<size_t> unique_station_number;
        std::optional<double> geographical_route_length;
        std::optional<double> total_route_length;
        std::optional<double> curvature;
        bool is_found_in_catalogue = false;
    };

    struct OnMapRequestAnswer {
        size_t request_id;
        std::string rendered_map;
    };    

    using Request = std::variant<std::monostate, AddStationCommand, AddRouteCommand, ReadCommand/*, BuildRouteCommand*/>;
    using Answer = std::variant<std::monostate, OnStationRequestAnswer, OnRouteRequestAnswer, OnMapRequestAnswer, OnRouteBetweenTwoStationsAnswer>;
    using RequestsLine = std::vector<Request>;
    using AnswerLine = std::vector<Answer>;


    class RequestHandler {
    public:
        void SetCatalogue(transport_catalogue::TransportCatalogue& catalogue) noexcept;
        void SetMapRenderer(map_renderer::MapRederer& map_renderer) noexcept;
        void SetRouter(transport_router::TransportRouter& transport_router) noexcept;
        void SetResponceReciver(std::function<void(AnswerLine&& answer_line)>&& responce_reciver);
        void SetRequestsLine(RequestsLine&& requests_line) noexcept;
        void SendRenderSettingsToMapRenderer(map_renderer::RenderSettings&& render_settings) noexcept;
        void SendRoutingSettingsToTransportRouter(transport_router::RoutingSettings&& routing_settings) noexcept;
        void ProcessModificationRequests();
        void ProcessReadRequests();
        void SendAnswerToResponceReciver();
        

    private:
        transport_catalogue::TransportCatalogue* catalogue_;
        map_renderer::MapRederer* map_renderer_;
        transport_router::TransportRouter* transport_router_;

        RequestsLine request_line_;
        AnswerLine answer_line_;

        std::function<void(AnswerLine&& answer_line)> responce_reciver_;        
    };
} //request_handler

