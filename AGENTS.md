# ETL — инструкции для агентов

`ETL` — «ещё одна» лёгкая замена STL для Arduino: контейнеры и утилиты
(`etl::vector`, `array`, `lookup`, `queue`, `optional`, фильтры `etl::filter`,
`etl::led`, `etl::settings`, `etl::little_fs`, `etl::wifi`, `etl::webui`,
`etl::mqtt`, `etl::espnow`). Сама библиотека лежит в `src/etl/`, а `src/main.cpp` —
тестовое приложение, которое на устройстве прогоняет `etl::unittest::test_all(Serial)`.

## Сборка и проверка

`pio` не в PATH — вызывай полный путь:

```powershell
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run                      # все окружения
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run -e esp8266
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run -e esp32c3
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run -e esp32-wroom-32u
```

- Библиотека обязана собираться **во всех трёх** окружениях: ESP8266 и два ESP32.
  Перед тем как сказать «готово», покажи итог по каждому env (RAM/Flash, ошибки).
- Ошибки компиляции читай из вывода целиком: сначала первая ошибка, остальные часто её следствие.
- Прошивка и монитор требуют подключённой платы — **спроси пользователя**:

```powershell
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run -e esp32-wroom-32u -t upload
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" device monitor -b 115200
```

- Тестов на ПК нет: тесты живут в `src/etl/etl_test.cpp` и выполняются на плате
  через `src/main.cpp`. Критерий корректности — сборка всех env плюс прогон тестов
  на устройстве (когда оно подключено).
- Полная очистка: `... pio run -t clean`. Не чисти `.pio` без причины — сборка станет дольше.

## Платформа и зависимости

- ESP32-окружения — `platform = espressif32 @ ^8.0.0` (Arduino core 3.x). Не открепляй:
  в ядре 3.x нет `ledcSetup`/`ledcAttachPin`, `etl_led` работает по пину через
  `ledcAttach` / `ledcWrite(_pin, v)` / `ledcDetach`.
- `lib_ldf_mode = deep` (не `deep+`): иначе в сборку не подтягиваются системные
  библиотеки FS и Network, и сборка ESP32 падает на `Network.h: No such file or directory`.
- Платформу определяем только макросами `ETL_ESP32` / `ETL_ESP8266` из `etl_defines.h`:
  в core 3.x `#ifdef ESP32` не срабатывает, и код на ШИМ молча выпадает из сборки.
- Зависимости зафиксированы (GTimer, FileData, ArduinoJson, PubSubClient). Новую
  библиотеку добавляй только по просьбе; версии не поднимай «заодно».

## Что важно не сломать

- Совместимость ESP8266 и ESP32 одновременно: платформенные вызовы — под
  `#if defined(ETL_ESP32)` / `#elif defined(ETL_ESP8266)`.
- Никакого STL: только `etl::` (`vector`, `array`, `lookup`, ...), без исключений и RTTI,
  без лишних динамических аллокаций — библиотека про экономию RAM.
- `library.json`: списки `headers` и `build.srcFilter` (там `main.cpp` исключён из экспорта)
  правь при добавлении/удалении файлов — от этого зависит состав пакета в PlatformIO Registry.
- Версия: `src/etl/etl_version.h` — источник истины, `sync_version.py` (pre-script сборки)
  подставляет её в `library.json`, `library.properties`, `package.json`. PATCH — обычные
  правки, MINOR — новые возможности. После смены версии вручную: `python sync_version.py`.
- `README.md` — история версий: при релизе добавь раздел на русском в существующем стиле.
- `littlefs/*.csv` — схемы разделов, `examples/*.xlsx` — расчётные таблицы фильтров.
  Без необходимости не трогай.

## Стиль

- Минимальный diff: тот же стиль, те же имена, не переформатируй чужие файлы целиком.
- Комментарии — по делу и на русском; не комментируй очевидное.
- Ничего не выкладывай в git самостоятельно. По просьбе подготовь текст коммита
  (короткий, на русском, в стиле истории: `v1.1.0 feat: API core ESP32 3.x upgrade`)
  и покажи его — пользователь выложит сам после корректировки.

## Порядок работы

1. Сначала прочитай затронутые файлы (и `README.md`), потом правь.
2. Сначала предложи решение и дождись подтверждения — без разрешения код не меняем.
3. Меняй по одной логической вещи за раз, после каждой — сборка всех env.
4. Для поиска по коду и проверки чужих правок используй сабагентов, но правки делай сам.
5. Если сборка падает по причине окружения (нет тулчейна, нет платы, нет сети) — скажи
   об этом прямо, не выдавай это за ошибку кода.
