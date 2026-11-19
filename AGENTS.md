# ETL — agent instructions

`ETL` is "yet another" lightweight STL replacement for Arduino: containers and utilities
(`etl::vector`, `array`, `lookup`, `queue`, `optional`, `etl::filter` filters,
`etl::led`, `etl::settings`, `etl::little_fs`, `etl::wifi`, `etl::webui`,
`etl::mqtt`, `etl::espnow`). The library itself lives in `src/etl/`, while `src/main.cpp`
is a test application that runs `etl::unittest::test_all(Serial)` on the device.

## Build and verification

`pio` is not in PATH — call it by full path:

```powershell
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run                      # all environments
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run -e esp8266
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run -e esp32c3
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run -e esp32-wroom-32u
```

- The library must build in **all three** environments: ESP8266 and the two ESP32 ones.
  Before saying "done", show the result for each env (RAM/Flash, errors).
- Read compilation errors from the whole output: the first error is the cause, the rest are
  usually its consequences.
- Uploading and monitoring require a connected board — **ask the user first**:

```powershell
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run -e esp32-wroom-32u -t upload
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" device monitor -b 115200
```

- There are no host-side tests: the tests live in `src/etl/etl_test.cpp` and run on the device
  through `src/main.cpp`. The correctness criterion is a build of every env plus a test run on
  hardware (when one is connected).
- Full clean: `... pio run -t clean`. Do not wipe `.pio` without a reason — rebuilds get slower.

## Platform and dependencies

- ESP32 environments use `platform = espressif32 @ ^8.0.0` (Arduino core 3.x). Do not unpin it:
  core 3.x has no `ledcSetup`/`ledcAttachPin`; `etl_led` drives PWM per pin through
  `ledcAttach` / `ledcWrite(_pin, v)` / `ledcDetach`.
- `lib_ldf_mode = deep` (not `deep+`): otherwise the system FS and Network libraries are not
  pulled into the build, and the ESP32 build fails with `Network.h: No such file or directory`.
- Dependencies are pinned (GTimer, FileData, ArduinoJson, PubSubClient). Add a new library only
  on request; do not bump versions "while at it".

## Platform-specific code: ETL_ESP32 / ETL_ESP8266 only

**Rule: anything that differs between platforms must go through the `ETL_ESP32` and
`ETL_ESP8266` macros from `src/etl/etl_defines.h`.**

```cpp
#include "etl_defines.h"

#if defined(ETL_ESP32)
    ledcAttach(_pin, frequency, resolution);
#elif defined(ETL_ESP8266)
    analogWriteFreq(frequency);
#else
    #pragma message("ERROR: led::init_pwm board not defined")
#endif
```

- Since v1.1.3 the ESP32 platform is `espressif32 @ ^8.0.0` — Arduino core 3.x (currently
  `framework-arduinoespressif32` 3.3.12). **`ESP32` is no longer defined there**, so the old
  `#ifdef ESP32` yields an empty build: the code inside is simply not compiled, leaving a
  `#pragma message` stub or a body-less function, and the failure only shows up on the device.
  That is exactly how `etl_led` broke when moving to 8.x.
- `ESP8266` is still defined, but use `ETL_ESP8266` anyway — so both branches look the same and
  do not depend on how a particular core defines its macros.
- Start a new platform-specific file with `#include "etl_defines.h"` (it is idempotent), not with
  `#include <Arduino.h>`.
- If the log shows `#warning "ETL: Platform not detected. ETL_ESP8266 / ETL_ESP32 are undefined."`,
  that is an environment configuration error (or a forgotten include), not a warning to deal with
  later: platform-specific code does not work in such a build.
- Core 3.x API: `ledcAttach`/`ledcAttachChannel`, `ledcWrite`/`ledcWriteChannel`, `ledcDetach`.
  The old `ledcSetup`/`ledcAttachPin` are gone.

## What must not break

- ESP8266 and ESP32 compatibility at the same time: platform calls go under
  `#if defined(ETL_ESP32)` / `#elif defined(ETL_ESP8266)`.
- No STL: only `etl::` (`vector`, `array`, `lookup`, ...), no exceptions and no RTTI, no
  unnecessary dynamic allocations — the library is about saving RAM.
- `library.json`: keep `headers` and `build.srcFilter` (the latter excludes `main.cpp` from the
  export) in sync when adding or removing files — this defines the package contents in the
  PlatformIO Registry.
- Version: `src/etl/etl_version.h` is the source of truth; `sync_version.py` (a build pre-script)
  copies it into `library.json`, `library.properties`, `package.json`. PATCH for regular changes,
  MINOR for new features. After changing the version by hand run `python sync_version.py`.
- `README.md` is the version history: on release add a section in Russian, in the existing style.
- `littlefs/*.csv` are partition schemes, `examples/*.xlsx` are filter calculation workbooks.
  Do not touch them without a reason.

## Style

- Minimal diff: same style, same names; never reformat someone else's file wholesale.
- Comments — to the point and in Russian; do not comment the obvious.
- Never commit or push to git yourself. On request, prepare a commit message (short, in Russian,
  in the style of the history: `v1.1.0 feat: API core ESP32 3.x upgrade`) and show it — the user
  pushes it after review.

## Workflow

1. Read the affected files (and `README.md`) first, only then edit.
2. Propose the solution and wait for approval first — never change code without permission.
3. Change one logical thing at a time, and build all envs after each change.
4. Use subagents for code search and for reviewing others' changes, but make the edits yourself.
5. If a build fails for an environment reason (no toolchain, no board, no network), say so
   directly; do not present it as a code bug.
