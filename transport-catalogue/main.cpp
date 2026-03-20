#include <iostream>

#include "json_reader.h"
#include "transport_catalogue.h"
#include "map_renderer.h"
#include "request_handler.h"

int main() {
    transport_catalogue::TransportCatalogue catalogue;
    json_reader::JsonReader reader;
    map_renderer::MapRederer map_renderer;
    request_handler::RequestHandler request_handler;
    transport_router::TransportRouter router(catalogue); // Инициализация пустого роутера связанного с каталогом
    

    reader.SetRequestHandler(request_handler); 
    request_handler.SetCatalogue(catalogue);
    request_handler.SetMapRenderer(map_renderer);   
    request_handler.SetRouter(router); // Обработчик запросов связан с пустым роутером

    std::cin >> reader; 
    reader.ParseInputDocument();
    reader.SendRequestToRequestHandler(); // После получения запросов от ридера обработчик запросов передаст настройки в роутер
    request_handler.ProcessModificationRequests(); // Чтени запросов, заполнение каталога
    router.Initialize(); // Инициализация роутера по заполненному каталогу и настройкам, теперь роутер готов к работе
    request_handler.ProcessReadRequests(); // Выполнение запросов на чтени, в том числе на формирование маршрута между двумя остановками
    request_handler.SendAnswerToResponceReciver();
    std::cout << reader;    
}