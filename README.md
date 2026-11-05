# Arduino firmware — air-puff / water-reward task

This folder contains eight Arduino Yún sketches that run the same beam-break-triggered task. On each trial the subject receives an air puff, a water reward, both or neither. The sketch logs beam, lick and trial-phase data to the Yún's onboard storage.

All eight sketches share the same hardware, pins, trial timeline and data-row format. They differ in **how each trial's outcome is chosen**, and there are two designs.

| Design | Sketches | How a trial is chosen | Proportions within a session |
|---|---|---|---|
| **Fixed mix** | the six `ProbSAT…` sketches | One random draw picks one of three trial types with set probabilities | Vary around the set values |
| **Independent draws** | `EP_Decoupled_80_20_SATboxCODEv3`, `pseudo_80_V2` | Two separate random draws per trial: one for water (`waterprob`), one for the puff (`puffprob`) | Vary around the products of the two probabilities |

## Trial-type codes

The sketch names and the tables below use these codes:

| Code | Air puff | Water | Relays switched |
|---|---|---|---|
| **SR** | Real | Real | puff (D7), pump (D8) |
| **S** | Real | Sham | puff (D7), sham pump (D10) |
| **R** | Sham | Real | sham puff (D2), pump (D8) |
| **B** (blank) | Sham | Sham | sham puff (D2), sham pump (D10) |

Each trial switches one puff-stage relay and one pump-stage relay with identical timing. This means relay clicks and other incidental cues are matched across trial types.

## Sketch comparison

### Fixed-mix design (ProbSAT)

| Sketch | SR  (Stimulus + Reward) | S (Stimulus-only) | R (Reward-only) | B (No-stimulus + No-reward) | Data file prefix | Used for |
|---|---|---|---|---|---|---|
| [`ProbSAT40SR_40S_20B_V1`]([ProbSAT40SR_40S_20B_V1](https://github.com/Barth-Neuroscience-Lab/SAT-codes-for-different-contingencies/blob/main/ProbSAT40SR_40S_20B_V1.ino)/) | 40 % | 40 % | — | 20 % | `PrSAT40sr40s20b.txt` | [FIGURES 4C and 4D] |
| [`ProbSAT50SR_30S_20B_V1`](ProbSAT50SR_30S_20B_V1/) | 50 % | 30 % | — | 20 % | `PrSAT50sr30s20b.txt` | [FIGURES 4B and 4D] |
| [`ProbSAT60SR_20S_20B_V1`](ProbSAT60SR_20S_20B_V1/) | 60 % | 20 % | — | 20 % | `PrSAT60sr20s20b.txt` | [FIGURES 4A and 4D] |
| [`ProbSAT60SR_20R_20B_V1`](ProbSAT60SR_20R_20B_V1/) | 60 % | — | 20 % | 20 % | `PrSAT60sr20r20b.txt` | [FIGURES S5A and S5C] |
| [`ProbSAT80SR_5R_15B_V1`](ProbSAT80SR_5R_15B_V1/) | 80 % | — | 5 % | 15 % | `PrSAT80sr5r15b.txt` | [FIGURES 4E and 4H] |
| [`ProbSAT80SR_10R_10B_V2`](ProbSAT80SR_10R_10B_V2/) | 80 % | — | 10 % | 10 % | `PrSAT80sr10r10b.txt` | [FIGURES 4F and 4H] |

### Independent-draw design

| Sketch | `waterprob` | `puffprob` | Expected SR (Stimulus + Reward) | S (Stimulus-only) | R (Reward-only) | B (No-stimulus + No-reward) | Data file prefix | Used for |
|---|---|---|---|---|---|---|---|---|
| [`EP_Decoupled_80_20_SATboxCODEv3`](EP_Decoupled_80_20_SATboxCODEv3/) | 100 % | 80 % | 80 % | — | 20 % | — | `allreward8020.txt` | [FIGURES 4G—4H, 5, 6, 7, 8, S5B—C, S7—S9] |
| [`pseudo_80_V2`](pseudo_80_V2/) | 50 % | 80 % | 40 % | 40 % | 10 % | 10 % | `pseudo2.txt` | [FIGURES 3, 8, S9] |

In `EP_Decoupled_80_20_SATboxCODEv3`, water is delivered on every trial, so the sham-pump relay (D10) never switches.

Trial outcomes are drawn independently on every trial with `random(100)` in all eight sketches, so the realized proportions within a session will vary around these values.

## Trial timeline (identical in all sketches)

```
beam break ─► WAIT1 ─► PUFF ─► WAIT2 ─► PUMP ─► WAIT3 ─► ready for next trial
              200–800   500    500      75     ≥ 725 ms
              (random)                         and beam clear
```

| Parameter | Value | Variable |
|---|---|---|
| Beam break → puff | Random integer, 200–800 ms (uniform) | `wait1time` |
| Puff duration | 500 ms | `pufftime` |
| Puff offset → pump | 500 ms | `wait2time` |
| Pump pulse | 75 ms | `pumptime` |
| Post-pump interval | `wait3time − 200` = 725 ms minimum; the trial ends only once the beam is also unblocked | `wait3time` |
| Sampling interval | ~100 ms (100 ms delay + loop overhead) | `loopdelay` |

A new trial cannot start until the previous one has ended.

## Hardware

- **Board:** Arduino Yún
- **Storage:** USB stick or micro-SD card mounted at `/mnt/sda1` on the Yún, with an `arduino/www/` folder on it

| Pin | Name in code | Connected to |
|---|---|---|
| D4 | `SENSORPIN` | IR beam-break sensor (internal pull-up; LOW = blocked) |
| D11 | `LICKPIN` | Lick sensor (HIGH = lick) |
| D7 | `relayPinPuff` | Air-puff solenoid valve relay |
| D8 | `relayPinPump` | Water pump relay |
| D2 | `relayPinFake` | Sham-puff relay |
| D10 | `relayPinFakeTwo` | Sham-pump relay |
| A0 | — | Left unconnected; read once to seed the random number generator |

<!-- TODO: add sensor / relay / valve / pump models and a wiring diagram -->

## Software

- Arduino IDE [VERSION] with the **Arduino AVR Boards** core [VERSION]
- **Bridge** library [VERSION] (bundled with the IDE; provides `FileIO.h` and `Process`)

## Uploading

1. Open the `.ino` file of the sketch you need in the Arduino IDE. Each sketch must stay in a folder with the same name.
2. Select **Tools → Board → Arduino Yún** and the correct port.
3. Click **Upload**.
4. Make sure the storage device is mounted at `/mnt/sda1` and that `/mnt/sda1/arduino/www/` exists.

On power-up the sketch waits, writing `initializing` to the progress file every second, until the lick sensor reads LOW. It then reads the Yún's system clock to name the data file and starts waiting for beam breaks.

## Output files

Both files are written to `/mnt/sda1/arduino/www/`, which the Yún's web server also exposes.

### `progressfile.txt`

A one-line status file (`Starting` or `initializing`) for monitoring the board remotely.

### Data file names

Each data file is named with the sketch's prefix followed by the session start time. The separator between date and time differs between the two designs:

| Design | Example (session started 3 Nov 2026, 12:00:00) |
|---|---|
| Fixed mix (ProbSAT) | `PrSAT60sr20r20b.txt11_03_26_T_12_00_00.txt` |
| Independent draws | `pseudo2.txt11_03_26~T~12_00_00.txt` |

The `.txt` in the middle of each name is part of the original filename format. Analysis code that parses dates from filenames must handle both `_T_` and `~T~`.

### Data format

The file has no header row. Each line is one sample, taken about every 100 ms while a trial is running:

```
time_s,beam,lick,phase,wait1_ms,puff
```

| Column | Meaning |
|---|---|
| `time_s` | Seconds since the board powered on (2 decimal places) |
| `beam` | `1` = beam blocked, `0` = clear |
| `lick` | `2` = licking, `0` = not licking |
| `phase` | `3` = WAIT1, `4` = PUFF stage, `5` = WAIT2, `7` = WAIT3, or always `9` on the other trials. **Which trials get coded phases differs by sketch** (see below) |
| `wait1_ms` | This trial's beam-break → puff delay in ms. Shown during WAIT1, then `0` for the rest of the trial |
| `puff` | `1` = real puff, `0` = sham puff |

**Phase coding by sketch:**

| Sketch | Phases 3/4/5/7 are written on… | `9` is written on… |
|---|---|---|
| All six ProbSAT sketches, `pseudo_80_V2` | trials with **real water** (SR, R) | trials without water (S, B) |
| `EP_Decoupled_80_20_SATboxCODEv3` | trials with a **real puff** (SR) | sham-puff trials (R) |

**End-of-trial marker.** When each trial ends, an extra row is written:

```
<time_s>,0,0,7,0,<puff>
```

Here `<time_s>` repeats the timestamp of the trial's last sample, and the phase is always `7`, whatever the trial type.

### Identifying trial types in the data

Use the `phase` and `puff` columns of any regular (non-marker) row of a trial.

**ProbSAT sketches and `pseudo_80_V2`:**

| `phase` | `puff` | Trial type |
|---|---|---|
| 3 / 4 / 5 / 7 | 1 | **SR** — air puff + water |
| 9 | 1 | **S** — air puff, no water |
| 3 / 4 / 5 / 7 | 0 | **R** — water, no puff |
| 9 | 0 | **B** — blank |

**`EP_Decoupled_80_20_SATboxCODEv3`** (water on every trial):

| `phase` | `puff` | Trial type |
|---|---|---|
| 3 / 4 / 5 / 7 | 1 | **SR** — air puff + water |
| 9 | 0 | **R** — water, no puff |

Data rows are buffered in RAM and written to the file every 40 samples and at the end of every trial.

## Notes for reproducibility

- **Code unchanged.** Only comments and leading whitespace were added or edited when these sketches were prepared for publication. Stripping comments and whitespace from each curated file gives the same code as the version used to collect the data.
- **Sampling.** Lick and beam states are sampled at about 10 Hz, and only while a trial is running. Licks shorter than the sampling interval may be missed.
- **Pump pulse.** The 75 ms pump pulse uses a blocking `delay()`, so no samples are logged during it.
- **Probability variables.** In the independent-draw sketches, `waterprob` and `puffprob` set the trial probabilities. In the ProbSAT sketches these variables are declared but unused, and the probabilities come from the `trialtype` thresholds in `loop()`.
- **Unused variables.** `touch`, `lastwrite`, `recording` and `time` are declared but unused in all sketches. They are left in place to keep the code identical to the version used for data collection.
