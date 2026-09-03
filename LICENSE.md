# License

This repository, **loramon** (LoRaMon — the per-on-air-frame LoRa monitor for
reticulous, as an LCD app and a browser window), is released under the
**Apache License, Version 2.0**.

Full license text: <https://www.apache.org/licenses/LICENSE-2.0>

Copyright (c) 2026 by reticulous project contributors.

## Third-party software

### Vendored in this repository

None. Both surfaces draw their own plot — the LCD app into an RGB565 canvas,
the browser window into a 2D canvas — so there is no charting library here to
carry a license.

### Build-time dependencies

Declared in `esp-idf/idf_component.yml` and `browser/package.json`:

| Component / package | Source | License |
|---|---|---|
| ESP-IDF (platform) | espressif/esp-idf | Apache-2.0 |
| Browser peer deps (Vue, Quasar, Pinia) | npm | MIT |
