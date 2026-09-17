# Jacquard Serial Notebook

Defect log and Serial2 wire-protocol reference between the ESP32-S3 front panel and the Jacquard loom's main board.

-
- **Link**: Serial2, UART, 230400 8N1, GPIO16 (RX) / GPIO17 (TX) on both boards
- **Front panel**: `ESP32_S3_FRONT.ino`
- **Main board**: `ESP32 DEV MODULE mainboard/Main_Board.ino`

---

## Overview

The machine splits into two microcontrollers joined by a single UART. Neither is useful alone: the front panel has no storage and can't drive the loom; the main board has no screen or keypad. Every defect below eventually traced back to what one board assumed the other already knew.

The front panel owns the human interface: menu navigation, entering a running design's pick number, browsing a USB drive, copying or deleting files. The main board owns the loom: it reads the active design row-by-row off its SD card in time with two mechanical pulse sensors, and is the only board that can create, delete, or write a file. Every action the operator takes on the front panel becomes a short command sent down the wire; the main board acts on it and, for most commands, sends back a one- or two-byte acknowledgement.

---

## Defect log

### JAC-01 — USB drive mounts, but no folders ever appear
**Board:** Front panel · **Status:** Fixed

- **Symptom:** Insert a drive, wait for "USB Ready" — the folder browser then shows nothing.
- **Cause:** `usbScanFolders()` opened the USB root into a local `usbroot` handle, then read and closed the unrelated global `root` — a leftover SD-card handle that was never opened, so the scan silently found zero entries. A duplicate variable declaration in the same function also failed to compile.
- **Fix:** Read and close `usbroot` throughout; removed the duplicate declaration. (`ESP32_S3_FRONT.ino`)

### JAC-02 — Single-file copy skips the destination and then fails mid-transfer
**Board:** Both · **Status:** Fixed

- **Symptom:** Picking a file on the drive copies immediately with no folder prompt, then the transfer stalls and aborts on the first card.
- **Cause:** Two independent gaps. The front panel never sent the folder-select command before opening the file, so the main board's destination context stayed empty and it tried to write to `//filename.bmp`. Separately, the main board firmware actually running on the hardware differed from the version checked into the repo — it appends a CRC16 to every card's payload and rejects any transfer missing one, a step the front panel's sender had dropped in an earlier refactor.
- **Fix:** Send `ABf1` for the chosen folder before the file open; add a `crc16_ccitt()` matching the main board's exactly, appended big-endian after each card's payload. (`ESP32_S3_FRONT.ino`)

### JAC-03 — Two sketches, one compile
**Board:** Main board · **Status:** Fixed

- **Symptom:** Compiling threw dozens of "redefinition of `setup()`" / "redefinition of `File root`" errors after the main board file was dropped into the front panel's project folder.
- **Cause:** Arduino compiles every `.ino` file in a sketch's folder as one program. Two firmwares sharing a folder means two `setup()`s, two `loop()`s, and duplicate globals — not a code defect, a project-layout one.
- **Fix:** Moved the main board firmware into its own sketch folder, compiled and flashed independently.

### JAC-04 — A five-second pause between picking a file and seeing the folder list
**Board:** Front panel · **Status:** Fixed

- **Symptom:** Every USB copy operation paused for close to five seconds before the destination list — or the "done" screen — appeared.
- **Cause:** `usbRefreshMainboardFolderList()` reset its own timeout on every byte received, so it was really waiting for five full seconds of *silence* after the main board's list finished sending — which takes well under a second.
- **Fix:** Treat 300 ms of silence as "list complete," with a 3-second cap as a safety net if the main board never replies. (`ESP32_S3_FRONT.ino`)

### JAC-05 — Copying a single file never asks which folder it goes into
**Board:** Front panel · **Status:** Fixed

- **Symptom:** A file always landed in a folder matching the USB source folder's name, with no way to choose otherwise.
- **Fix:** Added a destination picker reusing the existing folder-selection screen: refresh the main board's folder list, let the operator choose (or cancel with `*`), then send `ABf1` for the chosen folder — not the source folder's name. (`ESP32_S3_FRONT.ino`)

### JAC-06 — The last folder in any list was unreachable
**Board:** Front panel · **Status:** Fixed

- **Symptom:** Folder pickers never showed the final entry in the list, and wrapped one folder short.
- **Cause:** `folder_selection(int folderCount)` treats its argument as the true folder count internally, but every call site passed `totalfolder_m - 1` — a pre-existing off-by-one that predated this work.
- **Fix:** Pass the true count at both call sites; the picker now also defaults its cursor to the active folder and marks it with `@`. (`ESP32_S3_FRONT.ino`)

### JAC-07 — No proof the two boards agree on ribbon & card count
**Board:** Both · **Status:** Fixed

- **Symptom:** Changing ribbon or card count in Settings updated the front panel instantly, with no way to tell whether the main board actually received and applied it.
- **Fix:** Main board now replies `ABDy*{nr}*{nc}#&` with the values it applied after every change; front panel waits up to 2 seconds and shows **Synced**, **Mismatch** (both values, side by side), or **No reply**. (`Main_Board.ino`, `ESP32_S3_FRONT.ino`)

### JAC-08 — A file could pass format-check and still be wrong for this loom
**Board:** Both · **Status:** Fixed

- **Symptom:** A design file's per-card byte count was checked only against the front panel's own ribbon & card count — never against the main board's, which is the one that actually matters at run time.
- **Fix:** Main board now checks the declared per-card size against its own `nr×nc+4` during the `ABct` handshake and rejects a mismatch with `D3*{got}*{expected}#`, instead of accepting anything up to 1028 bytes. Front panel surfaces the exact byte counts on rejection instead of retrying blind. (`Main_Board.ino`, `ESP32_S3_FRONT.ino`)

### JAC-09 — Delete was already built on the main board, but never wired up
**Board:** Main board · **Status:** Fixed

- **Context:** The main board already had working `ABd1`/`ABd2` handlers for folder and file delete — nothing on the front panel ever called them.
- **Fix:** File Manager gained **Folder Delete** and **File Delete**, following the confirm-then-request pattern from a sibling project: pick a target, confirm (`E`=yes, `*`=no), send the delete, show the result. File delete first switches the main board's active folder, since its delete-by-index only ever operates on whichever folder is currently loaded. (`ESP32_S3_FRONT.ino`)

### JAC-10 — Keypad columns bleeding into each other
**Board:** Front panel · **Status:** Fixed

- **Symptom:** Pressing **2** behaved like **1**; pressing **6** displayed **5**; pressing **R** behaved like **B** — always the second key in a row collapsing into the first, and only after copying a file.
- **Ruled out:** Column wiring (pin assignments cross-checked and confirmed correct), heap exhaustion (free heap held rock-steady at 285,060 bytes across every test), and unsafe USB Host re-initialization (the library already guards `begin()` against being called while running).
- **Cause:** `serialinput()` wrote every incoming byte into a fixed 50-byte `data3[]` array with no bounds check. A long or malformed frame — far more likely during the fast, back-to-back folder/file list burst that follows a large copy — could walk `nc1` (an unsigned byte) well past 50, silently overwriting whatever global sat next in memory before eventually wrapping at 256. The corruption only existed until the next reset, which is why reflashing identical code appeared to "fix" it.
- **Fix:** Added the same bounds-check guard the main board's parser already had, at both byte-write sites: if the frame runs past `sizeof(data3)` before a terminator appears, reset and bail instead of overwriting adjacent memory. Also matched the main board's enlarged Serial2 receive buffer (256 B default → 2048 B) to cut the odds of buffered bytes going missing under load. (`ESP32_S3_FRONT.ino`)

### JAC-11 — File-change screen shows one extra empty slot
**Board:** Front panel · **Status:** Fixed

- **Symptom:** A folder with 3 files showed 4 selectable slots when cycling through the Running Screen's F1/F2 file-change control — the last one blank.
- **Cause:** `fileselection()` (used for F1/F2 file change in Running Mode) let its cursor reach index `totalfile_m` — one past the last real file — before wrapping. The same class of off-by-one already fixed in `folder_selection()` (JAC-06), just in a different function.
- **Fix:** Wrap boundaries corrected to stop at `totalfile_m - 1` in both directions. (`ESP32_S3_FRONT.ino`)

### JAC-12 — Active folder/file tracking goes stale after a list rebuild
**Board:** Main board · **Status:** Fixed

- **Symptom:** After copying a folder over USB, the previously-active folder sometimes displayed as the wrong entry. Separately, deleting the currently active folder could leave stale filenames sitting in the newly-loaded fallback folder's file list.
- **Cause:** Two related gaps in how the main board rebuilds its lists. The `ABf3` (refresh) handler re-scans the SD card's folder order — which can shift once a new folder is added — without re-resolving `foldernum` against the new order, and never told the front panel the corrected index. Separately, the delete-current-folder fallback rebuilt the new active folder's file list without resetting `totalfile` first, so counting continued from the just-deleted folder's leftover count instead of starting at zero.
- **Fix:** `ABf3` now re-finds the active folder by name in the rebuilt list, corrects `foldernum`, and pushes it back to the front panel via `currentfoldernumber()`. The delete-current-folder fallback now resets `totalfile` to 0 before rebuilding. (`Main_Board.ino`)

---

## The wire protocol

Every message starts with the two bytes `A` `B`, followed by a command byte (and sometimes a sub-command byte), a `*`-separated payload, then a terminator. Which terminator is used depends on who's talking.

| Direction | Framing | Example |
|---|---|---|
| Front panel → main board | literal bytes `{` then `}` | `ABf1*HARI - 2{}` |
| Main board → front panel | literal bytes `#` then `&` | `ABLK3*HARI 1#&` |

### Short acknowledgements

Copy, delete, and folder-create replies skip full framing and send a compact two-byte code instead — the receiver watches the stream for the pair `D` + digit.

| Code | Meaning |
|---|---|
| `D0` | reject / retry (bad CRC, format mismatch) |
| `D1` | success / already exists |
| `D2` | accepted / newly created |
| `D3` | failed (mkdir failed, or size mismatch) |

### Command reference — running mode & settings

| Command | Direction | Payload | Meaning |
|---|---|---|---|
| `ABI1` | front → main | — | Master init on boot |
| `ABC0` | front → main | — | Comms check — replies `EJ` |
| `ABR1` | front → main | — | Poll running status (pick, height, repeat) |
| `ABF` | front → main | folder index | Switch the main board's active folder |
| `ABE2` / `ABE6` | front → main | pick number | Jump F1 / F2 pick number mid-run |
| `ABE3` / `ABE7` | front → main | repeat count | Set F1 / F2 repeat count |
| `ABEK` / `ABEL` | front → main | file index | Change the F1 / F2 file mid-run |
| `ABn` / `ABN` | front → main | raw byte, card / ribbon count | Set card count / ribbon count |
| `ABDy` | main → front | `*nr*nc` | Confirms the ribbon/card count actually applied |
| `ABb` / `ABi` | front → main | mode byte | Invert-design toggle / Jacquard profile |
| `ABTK` / `ABTL` | main → front | count | Total folder count / total file count |
| `ABLK` / `ABLM` | main → front | `*index*name` | One folder / file list entry |

### Command reference — USB copy & delete

| Command | Direction | Payload | Reply | Meaning |
|---|---|---|---|---|
| `ABf1` | front → main | folder name | D1/D2/D3 | Create or select destination folder |
| `ABf2` | front → main | file name | D0/D1/D2 | Open destination file (never overwrites) |
| `ABf3` | front → main | — | ABTK/ABLK… | Refresh the whole folder & file list |
| `ABf4` | front → main | file name | D1/D2 | Does this file already exist? |
| `ABf5` | front → main | file name | `ABDz*size#&` | Received file size, for a post-copy check |
| `ABf6` | front → main | source size | — | Announces the incoming file's exact size |
| `ABf7` | front → main | — | D1 | Abort copy & delete the incomplete file |
| `ABDo` | front → main | 62-byte header | D1 | Bitmap header (height field at offset 0x16) |
| `ABct` | front → main | `*pick*cardBytes` | D2 / D3 | Announce the next card — D3 if `cardBytes` ≠ the main board's own `nr×nc+4` |
| `ABDe` / `ABDs` | front → main | payload + CRC16 | D0/D1 | Card data — `e` mid-file, `s` final card |
| `ABd1` | front → main | folder index | D1/D2 | Delete a folder (recursive) |
| `ABd2` | front → main | file index | D1/D2 | Delete a file from the active folder |

### Card data integrity

Every card's payload is followed by a 2-byte CRC16-CCITT (polynomial `0x1021`, initial value `0xFFFF`), high byte first. The main board recomputes it on receipt and only accepts the card if the CRC matches *and* its pick number is exactly one more than the last card it wrote — either failure gets a `D0`, and the front panel retries up to five times before giving up on the file.

```cpp
uint16_t crc16_ccitt(const uint8_t *data, size_t len) {
  uint16_t crc = 0xFFFF;
  for (size_t i = 0; i < len; i++) {
    crc ^= (uint16_t)data[i] << 8;
    for (uint8_t b = 0; b < 8; b++)
      crc = (crc & 0x8000) ? (crc << 1) ^ 0x1021 : (crc << 1);
  }
  return crc;
}
```

### Format compatibility

A design file is only valid for a given loom configuration if its per-card byte count equals `nr × nc + 4`. Both boards now check this independently — the front panel before it ever opens the file, the main board again during `ABct` — so a mismatch between the two boards' settings is caught at transfer time instead of surfacing later as a garbled run.

---

## Code map

### Front panel — `ESP32_S3_FRONT.ino`
- TFT menu system & 4×4 keypad input
- Running-mode display and live pick/repeat editing
- Settings: password, ribbon/card count, invert, profile
- USB Host (flash-drive) browsing, copy, and delete
- Owns no storage — every write goes to the main board

### Main board — `mainboard/Main_Board.ino`
- SD card: design storage, folder & file list
- RTC + external EEPROM: settings and the time-lock feature
- Reads the active design row-by-row per loom pick
- Drives the solenoid shift registers off two pulse sensors
- Sole authority for create / delete / write on the SD card

The split is deliberate: the main board can never be blocked waiting on a screen redraw or a keypress, and the front panel can never leave the SD card in a half-written state just because a menu got exited early. Everything that crosses the wire is either a request the main board can safely ignore if malformed, or a short status push the front panel can safely miss and re-ask for.
