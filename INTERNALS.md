# loramon — INTERNALS

```
straddle staged?        no  → iface-lora compiles its recorder out; no keys, no apps, and stop
 ↓ yes
app opened (LCD tile / Dock icon)
 ↓
set sys.stats.{lcd,web}_loramon = 1     → iface-lora starts recording (and web_peers, browser only)
 ↓
read lora.<n>.packets.<ms> subtree      → one record per on-air frame + one per dwell
 ↓  LCD: storageForEach on a 1 s tick   browser: mirrored subtree, rebuilt at 1 Hz
build the view for the selected window  → LAG_MS behind "now", so nothing is redrawn twice
 ↓  loop: repaint (LCD on the tick, browser on requestAnimationFrame)
touch/drag → zoom stack; tap → inspect one frame
 ↓
app closed → set the watch keys to 0    → iface-lora drops the whole packets subtree
```

Three rules the ladder rests on:

- **The device records only while an app here says it is looking.** The watch
  keys are the whole contract in that direction, and the falling edge deletes
  the subtree — so there is never pre-open history and every graph fills from
  open forward.
- **Storage is the model.** Neither viewer holds a record the device did not
  publish, and neither asks the device a question: no ITS port, no request/reply.
  Expiry reaches the browser because `storageDeleteTree` emits an explicit
  delete op.
- **The two apps implement one model.** Same records, same axes, same colour
  scheme, same zoom semantics. Where they differ it is stated below, and it is
  always about the surface — repaint cadence, label budget, stack depth — never
  about what a frame means.

## 1. What is here

| Path | Half | What it is |
|---|---|---|
| `browser/src/panels/LoraMonWindow.vue` | browser | the window: canvas, hit test, zoom stack, pills, hover readout |
| `browser/src/modules/loramon.ts` | browser | `registerLoraMon()` — the Dock app entry and its self-mounted window |
| `esp-idf/assets/lcd-icons/loramon.svg` | both | the icon, once: the build copies it to the LCD launcher tile and to the web Dock |
| `esp-idf/conditional/spangap-lcd/src/loramon_lcd.cpp` | firmware | the LcdApp: the same view painted by hand into one RGB565 canvas |
| `esp-idf/conditional/spangap-lcd/include/loramon_app.h` | firmware | `LoraMonApp`, the `when: spangap/spangap-lcd` service entry |

The firmware half is **entirely** conditional. A headless build stages this
straddle, compiles no source from it at all, and gets the browser window; a
build without `spangap-web` gets the LCD app; a build with neither still stages
it harmlessly, and the only thing that removes the recorder from iface-lora is
`--without loramon`.

Nothing here is a header anyone else includes, and nothing here exports a
symbol into the firmware beyond the one service class. The dependency on
iface-lora is one call — `loraNameForTag`, the peer table asked directly rather
than through a published copy, because the LCD app is in the same binary as the
table and serialising a fact so the same firmware can parse it back is a round
trip for nothing. The browser has no such shortcut and reads
`lora.<n>.peers.<slot>` instead.

## 2. The view

What the device publishes, and when, is [iface-lora's INTERNALS](../iface-lora/INTERNALS.md)
§12 (records and expiry) and §18.3 (the channel-RSSI series). This section is
what the two viewers do with it.

The browser reads the mirrored `lora.<n>.packets` subtree; the LCD app iterates
it via `storageForEach` on a 1 s redraw. Both
show **one graph carrying both directions** over one selected window, picked
from a pill row (10s / 1m / 5m / 10m / 30m / 1hr). Each frame is a horizontal
line spanning its time-on-air, placed on **one of two dBm axes sharing the same
four gradient bands**: transmit power −10…+30 dBm in **10 dB** steps down the
left gutter, received strength −128…0 dBm in **32 dB** steps down the right. The
receive axis is the reportable range itself rather than a comfortable subset of
it: packet strength is a byte read as −value/2 dBm, so 0 is the strongest level
a receiver can state and −127.5 the weakest, and the round 32 dB step falls out
of that. A pair on one bench reads well above −30, and an axis stopping there
put every one of those in the top band with nothing to tell them apart.
The steps differ because the ranges do (128 dB against 40) and both have to land
on the same band edges for one grid to serve them; the axis names stand in the
pill strip over their own gutter, which is what pushes the pills inward.
Colour says **direction and audience**, and nothing tints the background: red is
what this radio put on the air and blue what arrived, light for a broadcast
anyone may read (`#F08080` / `#80B8F0`) and dark for a frame with one addressee
(`#B02020` / `#2060C0`); grey `#8A8A8A` is somebody else's unicast — on the air,
but not our conversation — and purple `#8050C8` a frame whose CRC failed, which
is neither, since nothing in it was readable. Protocol left the colour scheme
when the records gained a description: a viewer that can be *asked* what a frame
is has no need to spend its one visual channel saying it, and audience is the
question a monitor is actually opened to answer. The caption carries **tx
airtime** and **channel busy** (tx + rx — the radio is half duplex, so the two
never overlap) plus a colour-keyed legend, since a colour with no key is a
decoration. A tab row above the graph selects the radio on a multi-radio
board (discovered from `lora.<n>.state`, hidden when there is only one);
switching resets the zoom stack, because a span selected on one radio's traffic
means nothing on another's. With a frequency-agility regime in force the browser
stacks **one quarter-height graph per agile channel** under that main graph on
the same time axis — see [iface-lora INTERNALS](../iface-lora/INTERNALS.md) §18.6.

**Pills.** A pill is a note stuck on the graph rather than part of it: cream,
black text, rounded, in the smaller face. One style serves both the peer name on
a train and the noise floor's figure, because two label styles for one job is one
too many.

**In the browser a peer pill is LEFT-ALIGNED on the moment its name becomes
true**, and stands until the next one contradicts it — the same reading as a
name on a timeline. Anchored to a point and not fitted to a run: a run's width
changes with the zoom, so a label fitted to one moves every time the view does,
and a label that will not fit vanishes from traffic that is plainly there.

One rule places them — **mark wherever the attribution changes** — and the two
kinds of channel fall out of it because they carry the answer in different
records:

- The **hailing channel** holds no meetings, so only frames are tagged: the
  first attributable frame *on screen* gets a pill (nothing preceding it in view
  established the name) and after that every change does. The pill starts **at**
  the frame — the traffic runs on to its right, and the name is the head of that
  run.
- A **detour channel** is attended in slots, and a slot belongs to its peer for
  its whole width even when nothing arrives in it. The dwell's `<tag>` carries
  that, so a slot is named on its own beginning rather than on whichever frame
  happened to be first inside it, and a slot that passed in silence is still
  named. The pill sits just **before** the slot opens, clear of it: a listening
  window is a bounded thing with its own left edge, and a label laid over that
  edge hides where the window starts — the one thing not to cover on a lane
  whose whole point is *we were listening from here to here*. A slot whose pill
  has no room to its left (the view's own edge, or the pill before it) goes
  unnamed rather than sliding inward.

A dwell additionally opens a new pill when it does not continue the previous
stay (`SLOT_JOIN_MS` of slack against beat quantisation), so two consecutive
slots with the *same* peer read as two slots. A frame never does: an untagged
frame between two of a peer's frames — somebody else's broadcast, overheard — is
not a change of who the lane is with, and clearing the run on it would re-label
the same conversation every time the air was shared. A pill that would collide
with the one before it, or run past the right gutter, is dropped rather than
moved: a name that has slid off the moment it belongs to is worse than no name.

The LCD still labels **runs** — consecutive frames sharing a `<tag>` — with a
fixed pool of four floating labels (the canvas has no glyph blitter), widest run
first: a view with more nameable runs than that has them shoulder to shoulder
and unreadable anyway.

The two viewers repaint differently, and the difference is visible. The browser
rebuilds from storage at 1 Hz but repaints off `requestAnimationFrame` against an
extrapolated device clock, so a live window *glides*; the LCD repaints on its 1 s
tick (and on a touch event), so the same motion is stepped. Nothing in the model
differs — only how often it is drawn.

**Zoom stack.** Touching/clicking the plot anchors a highlighted span. The
anchor is stored as a **device time, not a pixel** — which is what makes holding
still on a live graph *widen* the highlight: the anchor stays put while "now"
advances, so it drifts left under a stationary pointer.

**Every lane carries a light-grey bezel around its plot area.** A lane the radio
never visited is veiled end to end and has no traffic to give it shape, so
without one it is a rectangle of dark against a dark window — indistinguishable
from the gap between two lanes, and from no lane at all. The bezel is what says a
graph is there and *empty* rather than absent, which is why it is drawn before
the traffic as well as after: the case it exists for is the lane with nothing to
plot, and that is exactly the case both viewers return early on. The noise floor
stops at the gutters for the matching reason — a level drawn on the plot that
runs on through the scales strikes out the very numbers it is read against.

**The live edge stops short of "now" by `LAG_MS` (1.5 s).** The most recent
moment is not a finished picture: a stay on one channel is one record that
*grows*, so the background arrives a step at a time, and frame records are
batched, so a packet lands in a column the graph has already drawn as empty.
Both read as the graph correcting itself in public — blocks filling in at the
right edge, bars appearing out of nowhere behind them. Holding the edge back past
the slowest of those publishers shows only what has settled. On a plot whose
shortest window is ten seconds, a second and a half of latency is not a cost
anybody is watching for. Both viewers apply it in `view()`, so everything derived
from the span — the hit test, the copy button, the airtime figures — agrees with
what is drawn.

**The hover hairline is the opposite, and deliberately so.** It is stored as a
*fraction of the plot width* and turned into an instant at draw time, so on a
live graph the line stands where the hand is and the frames slide under it. A
selection is a span of time and must keep meaning the same span; a cursor is
where you are pointing, and a time-anchored one walks off to the left while the
mouse has not moved. That the instant it reports advances is the point — the
pointer is asking what is there *now*, not holding a bookmark.

Releasing pushes
`{t0,t1}` on a zoom stack and the view becomes that fixed span; the window pills
give way to a single **back** pill. A zoomed view stands still, so selecting
inside it zooms further, pushing another level. Back pops one level, and
emptying the stack returns to whichever moving window was active. Depth is not
printed anywhere — it is the back pill's colour: **blue while one level is
left** (so it leads back to the live window), grey while another frozen view
stands behind. A release that selected under 1 % of the visible span (under 5 ms
either way in the browser) is a tap, not a zoom, and is discarded — otherwise
every stray touch would freeze the graph on a span nobody can read. Both apps implement the same model (`zoomStack`
/ `sel` in the Vue component, `s.zoom[]` / `s.selActive` behind `canvasEventCb`
on the LCD); the LCD additionally caps the stack at 8 levels, where the browser
lets it grow. In the browser the plot ignores a press taken while the window is *not*
front-most (`focusedWindowId`): that press only raises the window, matching the
click swallow `FloatingWindow` already does — pointer events bypass it, so
without the check, reaching for an occluded LoRaMon would cost a zoom. Airtime follows
the view: a zoomed span is always computed locally, and only the *live* 1-hour
window uses the firmware's published rollup.

**Timescale, frozen views only.** A zoomed view stands still, so it carries a
grid: **1-px vertical lines in `#242424`, the darkest tone of the band
gradient**, on round multiples of one division — dark enough to read as part of
the background rather than as something drawn over it, and drawn **first** —
below the traffic, below the pills, below everything else a lane carries. It is
the ruling on the paper: part of the background, never crossing anything drawn
on top of it. The span
and what a
division is worth are stated beside the back pill in both viewers. A live view
gets none — the grid is anchored to absolute time and would
crawl across a sliding graph. The division is the smallest **1-2-5-10** step at
least `DIV_PX_MIN` pixels wide (70 CSS px on the web, 40 px on the LCD, whose
plot is a third the width), floored at 1 ms because the records are ms-stamped.
That ladder's widest gap is **×2.5** (2 → 5), which is the whole constraint on
the band: any allowed pixels-per-division range spanning a factor of ≥ 2.5
contains a step for *every* span and canvas width, so each viewer only states
its minimum and the 2.5× above it is implied.

## 3. Tasks and ownership

The browser half runs where every other panel does: the SPA's event loop, no
worker. The 1 Hz rebuild is a pass over the mirrored subtree the device store
already pushes; the repaint is `requestAnimationFrame` against an extrapolated
device clock, which is why a live window glides rather than steps.

The LCD half runs on the **lcd task**, like every `LcdApp`, and touches nothing
else: `onShow`/`onHide` write one storage key each, and the 1 s redraw does a
`storageForEach` and paints. It holds no lock, spawns no task, and never calls
into the radio. The one cross-straddle call, `loraNameForTag`, reads the peer
table under the table's own lock.

Neither half writes anything the device reads except the two watch keys and
`sys.stats.web_peers`. That is deliberate: a viewer that could change what the
radio does would make "open the monitor" an action with side effects on the air.

## 4. Pitfalls

- **The watch key must be cleared on close, and close means every way out.**
  The LCD app writes it from `onHide`, which the launcher calls for a back-out,
  a sleep and an app switch alike; the browser writes it from the window's
  visibility watcher and from `onUnmounted`. A key left at 1 leaves the device
  recording — and holding a 1 Hz RSSI beat — for nobody.
- **`sys.stats.web_peers` is the browser's alone.** The LCD app must not set it:
  it is in the same binary as the peer table and asks it directly, and setting
  the key would make the device serialise every peer row on every stats beat for
  a reader that does not exist.
- **Records are keyed by device uptime, and uptime restarts.** The browser
  anchors "now" to the newest record it has seen and never pulls that anchor
  backward, so a device reboot leaves the graph frozen at the old anchor until
  the new records overtake it. That is the deliberate choice — a monotonic
  anchor cannot be dragged backward by one late arrival — and the cost is a
  stale-looking window across a reboot, not a wrong one.
- **A tap is not a zoom.** A release under 1 % of the visible span (under 5 ms
  either way in the browser) is discarded, or every stray touch freezes the
  graph on a span nobody can read.
- **The LCD canvas has no glyph blitter**, which is why its peer labels come
  from a fixed pool of four and the browser's do not. Raising the pool is not a
  constant change; it is a layout problem.
