# Описание
Приложение реализует функционал транспортного каталога. Инициализация транспортной сети выполняется на основе входного JSON-файла, после чего система готова к обработке запросов на построение маршрутов. Позволяет находить кратчайший маршрут между остановками.
# Сборка
Проект не требует внешних библиотек. Для сборки используйте любой С++17-совместимый компилятор.
Проект проверен на работу с MSVC и GCC.

Пример сборки:
~~~
mkdir build && cd build
cmake ..
cmake --build .
~~~
После сборки получается исполняемый файл transport_catalogue.exe

## Запуск приложения:
Программа читает JSON из std::cin и выводит результат в std::cout.

Разместите в корневой директории проекта файл input.json (название может отличаться), расширение может быть изменено на .txt, но содержимое должно быть валидным формату JSON, иначе будет выброшенно исключение при чтении файла.

## Запуск из корневой директории:
Linux:
~~~
./build/transport_catalogue < input.json > output.json
~~~
## Windows:
~~~
./build/Debug/transport_catalogue.exe < input.json > output.json
~~~
Входной документ должен содержать обязательные поля:
~~~
{
  "base_requests": [ ... ],      // массив объектов для заполнения базы
  "render_settings": { ... },     // настройки отрисовщика карты
  "routing_settings": { ... },    // настройки маршрутизатора
  "stat_requests": [ ... ]        // массив запросов на получение информации
}
~~~
## Поддерживаются запросы:

+ __Bus__ - информация о маршруте (количество остановок, уникальные остановки, длина, кривизна).
+ __Stop__ - список маршрутов, проходящих через остановку.
+ __Map__ - SVG-карта (в ответе возвращается строка).
+ __Route__ - построение маршрута между двумя остановками (from и to). Если маршрут не найден вернёт пустой массив JSON, "items": [].
### Пример входного файла:
~~~
  {
      "base_requests": [
          {
              "is_roundtrip": true,
              "name": "297",
              "stops": [
                  "Biryulyovo Zapadnoye",
                  "Biryulyovo Tovarnaya",
                  "Universam",
                  "Biryulyovo Zapadnoye"
              ],
              "type": "Bus"
          },
          {
              "is_roundtrip": false,
              "name": "635",
              "stops": [
                  "Biryulyovo Tovarnaya",
                  "Universam",
                  "Prazhskaya"
              ],
              "type": "Bus"
          },
          {
              "latitude": 55.574371,
              "longitude": 37.6517,
              "name": "Biryulyovo Zapadnoye",
              "road_distances": {
                  "Biryulyovo Tovarnaya": 2600
              },
              "type": "Stop"
          },
          {
              "latitude": 55.587655,
              "longitude": 37.645687,
              "name": "Universam",
              "road_distances": {
                  "Biryulyovo Tovarnaya": 1380,
                  "Biryulyovo Zapadnoye": 2500,
                  "Prazhskaya": 4650
              },
              "type": "Stop"
          },
          {
              "latitude": 55.592028,
              "longitude": 37.653656,
              "name": "Biryulyovo Tovarnaya",
              "road_distances": {
                  "Universam": 890
              },
              "type": "Stop"
          },
          {
              "latitude": 55.611717,
              "longitude": 37.603938,
              "name": "Prazhskaya",
              "road_distances": {},
              "type": "Stop"
          }
      ],
      "render_settings": {
          "bus_label_font_size": 20,
          "bus_label_offset": [
              7,
              15
          ],
          "color_palette": [
              "green",
              [
                  255,
                  160,
                  0
              ],
              "red"
          ],
          "height": 200,
          "line_width": 14,
          "padding": 30,
          "stop_label_font_size": 20,
          "stop_label_offset": [
              7,
              -3
          ],
          "stop_radius": 5,
          "underlayer_color": [
              255,
              255,
              255,
              0.85
          ],
          "underlayer_width": 3,
          "width": 200
      },
      "routing_settings": {
          "bus_velocity": 40,
          "bus_wait_time": 6
      },
      "stat_requests": [
          {
              "id": 1,
              "name": "297",
              "type": "Bus"
          },
          {
              "id": 2,
              "name": "635",
              "type": "Bus"
          },
          {
              "id": 3,
              "name": "Universam",
              "type": "Stop"
          },
          {
              "from": "Biryulyovo Zapadnoye",
              "id": 4,
              "to": "Universam",
              "type": "Route"
          },
          {
              "from": "Biryulyovo Zapadnoye",
              "id": 5,
              "to": "Prazhskaya",
              "type": "Route"
          }
      ]
  }
~~~
### Ожидаемый ответ:
~~~
  [
    {
        "curvature": 1.42963,
        "request_id": 1,
        "route_length": 5990,
        "stop_count": 4,
        "unique_stop_count": 3
    },
    {
        "curvature": 1.30156,
        "request_id": 2,
        "route_length": 11570,
        "stop_count": 5,
        "unique_stop_count": 3
    },
    {
        "buses": [
            "297",
            "635"
        ],
        "request_id": 3
    },
    {
        "items": [
            {
                "stop_name": "Biryulyovo Zapadnoye",
                "time": 6,
                "type": "Wait"
            },
            {
                "bus": "297",
                "span_count": 2,
                "time": 5.235,
                "type": "Bus"
            }
        ],
        "request_id": 4,
        "total_time": 11.235
    },
    {
        "items": [
            {
                "stop_name": "Biryulyovo Zapadnoye",
                "time": 6,
                "type": "Wait"
            },
            {
                "bus": "297",
                "span_count": 2,
                "time": 5.235,
                "type": "Bus"
            },
            {
                "stop_name": "Universam",
                "time": 6,
                "type": "Wait"
            },
            {
                "bus": "635",
                "span_count": 1,
                "time": 6.975,
                "type": "Bus"
            }
        ],
        "request_id": 5,
        "total_time": 24.21
    }
]
~~~
