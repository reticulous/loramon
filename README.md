# loramon — LoRaMon, the per-on-air-frame monitor

```
open LoRaMon (LCD launcher tile, or the Dock icon in the browser)
 ↓
the device starts recording          → one storage node per frame, from now on
 ↓
graph fills from open forward         (there is no history before you opened it)
 ↓  loop: 1 Hz rebuild, redraw
pick a window pill 10s…1hr            → what the plot spans
drag across the plot                  → zoom to that span; back pill leaves it
tap a frame                           → what it was and who it was with, along the top
 ↓
background it (another app, Home)     → the session continues; recording does not pause
 ↓
stop it (recents swipe-up, or close
the browser window)                   → the device stops recording and drops the subtree
```

Three things that follow from the ladder, and surprise people otherwise:

- **The graph starts empty on every open.** Recording is gated on an app being
  open, because a device recording per-frame telemetry nobody is reading is
  paying a 1 Hz radio sample, a 1 Hz interface-task beat and up to half a
  megabyte of tree nodes for nothing. (The records themselves are unprefixed
  keys — in RAM, never written to flash.)
- **Both surfaces show the same thing.** The LCD app and the browser window read
  the same records and draw the same axes and colours. The browser glides and
  the LCD steps, and that is the only difference you should be able to see.
- **Stopping it deletes the records; backgrounding it does not.** The session is
  the app's life, not its visibility — reaching for Settings mid-watch and coming
  back must not cost the history you were watching accumulate, so the sentinel is
  written from `onCreate`/`onClose` rather than `onShow`/`onHide`. What ends a
  session on the device is a recents swipe-up (or a memory-pressure eviction,
  which runs the same `onClose`), and in the browser closing the window. Nothing
  accumulates between sessions.
- **A backgrounded session keeps costing.** Left in recents with the screen off,
  it holds the 1 Hz sample and the 1 Hz interface beat against light sleep —
  around 0.2–0.5 mA on top of the radio's own standing RX draw, so a few percent
  of an idle node. The heap is the larger figure: the subtree grows toward
  `LORA_MON_CAP` (4096 nodes per radio, roughly half a megabyte of PSRAM at the
  ceiling) and stays there until the app is stopped.

## What it is

One graph per radio showing every frame that went over the air. Each frame is a
bar spanning its time-on-air, placed by transmit power or received signal on a
dBm axis, over a window from ten seconds to an hour. Colour is direction and
audience: red is what this radio put on the air, blue what arrived, light for a
broadcast anyone may read and dark for a frame with one addressee; grey is
somebody else's unicast, overheard, and purple a frame whose CRC failed.

A run of frames sharing a peer is named by a pill beside it. With a
frequency-agility regime in force, the channels the radio detours to get their
own lanes under the main plot, showing both the traffic there and — as a shaded
run — where the radio was actually listening.

It is two surfaces of one app: an `LcdApp` on the device screen, and a Dock
window in the browser SPA.

## Where it fits

This straddle is the **reader**. [iface-lora](../iface-lora) is the recorder: it
publishes one node per on-air frame at `lora.<n>.packets.<ms>`, the channel-RSSI
series at `lora.<n>.rssi`, the rolling hour at `lora.<n>.air1h.{rx,tx}` and the
neighbourhood at `lora.<n>.peers.<slot>`. Those keys are iface-lora's and are
documented in [its README](../iface-lora/README.md#storage-variables).

iface-lora stages this straddle by default (`additional_installs:`), so a build
with the radio has the monitor. Drop it with:

```
spangap build <buildable> --without loramon
```

That is not a cosmetic removal. iface-lora's whole recording half is gated on
this straddle's presence symbol (`CONFIG_STRADDLE_LORAMON`), so a build without
it has no per-frame storage nodes, no neighbourhood rows, no channel-RSSI
series, no airtime rollup, no 1 Hz sampling beat and no 16 KB-per-radio expiry
FIFO — not idle versions of them, absent ones. The radio is otherwise unchanged,
and the settings pane and status bar keep everything they read.

## Storage variables

This straddle owns no settings and publishes no telemetry. It writes three
command sentinels, all of them "a viewer is looking":

| Key | Written by | Meaning |
|---|---|---|
| `sys.stats.lcd_loramon` | the LCD app, from `onCreate`/`onClose` | `1` while the on-device app is running — foreground or background, until it is stopped or evicted |
| `sys.stats.web_loramon` | the browser window, while visible | `1` while the browser window is up |
| `sys.stats.web_peers` | the browser window, while visible | `1` while a reader wants `lora.<n>.peers.<slot>` — the browser only; the LCD app asks the peer table directly |

Everything the two apps read is published by iface-lora under `lora.<n>.*`.

## Using it

**On the device**, LoRaMon is a launcher tile. Tabs across the top pick the
radio on a multi-radio board; the pill row picks the window. Touch a frame to
have it named along the top of the plot; drag across the plot to zoom, and the
back pill pops one level. `attr` at the right-hand end of the pill row turns the
peer labels off, for when the traffic is dense enough that they are in the way.

**In the browser**, the same view is a Dock app. Hover reads a frame out;
drag-select zooms. The window has to be front-most before the plot takes a
press, so reaching for an occluded LoRaMon costs you a raise and not a zoom.

## Dependencies

- [iface-lora](../iface-lora) — hard. It is the recorder, and the LCD app asks
  its peer table to name the node behind a frame's tag.
- [spangap-lcd](../spangap-lcd) — soft, and not declared: the LCD app lives in
  `esp-idf/conditional/spangap-lcd/` and its service entry is `when:`-gated, so
  a build without the screen simply has the browser window.
- [spangap-web](../spangap-web) — soft in the same way, through the browser
  half. A headless build compiles nothing from this straddle at all.

## What it does NOT own

- **The records.** Their format, their expiry, what a `desc` code means and what
  gets a `tag` are all iface-lora's, and change there.
- **Anything about the radio.** The two apps write three watch keys and nothing
  else; opening a monitor must not change what goes on the air.

## Read next

- [INTERNALS.md](INTERNALS.md) — the axes and the colour scheme and why they
  are what they are, the peer-pill placement rules, the zoom stack, the live-edge
  lag, and where the two surfaces deliberately differ.
- [iface-lora INTERNALS §12](../iface-lora/INTERNALS.md) — the recorder: what is
  written, when, and how it expires.
