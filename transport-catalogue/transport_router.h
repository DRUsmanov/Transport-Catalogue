#pragma once

#include <unordered_map>
#include <variant>
#include <vector>
#include <string>
#include <optional>
#include <string_view>

#include "transport_catalogue.h"
#include "domain.h"
#include "graph.h"
#include "router.h"


namespace transport_router {
	
	struct WaitingActivity {
		std::string stop_name;
		double time;
	};

	struct MovingActivity {
		std::string route;
		size_t span_count;
		double time;
	};

	using PassengerActivity = std::variant <std::monostate, WaitingActivity, MovingActivity>;

	struct RouteBetweenTwoStationsInfo {
		std::optional<double> total_time;
		std::optional<std::vector<PassengerActivity>> items;

		operator bool() const{
			return total_time.has_value();
		}
	};

	struct RoutingSettings {
		double bus_wait_time;
		double bus_velocity;
	};		

	class TransportRouter {
	public:
		using ArrivalStation = domain::Station; //В качестве остановки прибытия применяется реальная остановка

		explicit TransportRouter(const transport_catalogue::TransportCatalogue& catalogue);		
		[[nodiscard]] RouteBetweenTwoStationsInfo BuildRoute(const std::string_view from, const std::string_view to) const;
		void SetRoutingSettings(RoutingSettings&& routing_settings);
		void Initialize();

	private:
		struct DepartureStation {
			std::string name;
		};

		enum class EdgeType {
			WAITING,
			MOVING
		};

		struct EdgeInfo {
			EdgeType type;
			std::string_view name;
			double time;
			int span_count = 0;
		};

		DepartureStation CreateDepartureStationFromArrivalStation(const ArrivalStation* station) const; //Создает фейковую остановку отправления для реальной остановки
		
		void InitializeArrivalAndDepartureStationsId();
		void InitializeMovingEdgesId();
		void InitializeWaitingEdgesId();
		void InitializeGraph();

		std::vector<DepartureStation> departure_stations_;
		std::unordered_map<graph::EdgeId, EdgeInfo> edges_info_;

		std::unordered_map<size_t, const ArrivalStation*> arrival_stations_by_id_;
		std::unordered_map<const ArrivalStation*, size_t> id_by_arrival_stations_;
		std::unordered_map<size_t, const DepartureStation*> departure_stations_by_id_;
		std::unordered_map<const DepartureStation*, size_t> id_by_departure_stations_;
		std::unordered_map<const ArrivalStation*, size_t> departure_station_id_by_arrival_station_;
		std::unordered_map<const ArrivalStation*, const DepartureStation*> departure_stations_by_arrival_stations_;

		const transport_catalogue::TransportCatalogue* catalogue_;
		graph::DirectedWeightedGraph<double> graph_;
		graph::Router<double> router_;
		RoutingSettings routing_settings_;
	};
} // namespace transport_router
