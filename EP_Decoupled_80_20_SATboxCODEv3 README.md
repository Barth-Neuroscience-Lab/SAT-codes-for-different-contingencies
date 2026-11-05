# EP_Decoupled_80_20_SATboxCODEv3

One of the eight air-puff / water-reward task sketches for the Arduino Yún (independent-draw design). The task, timing, wiring, data format and upload steps are shared by the sketches and documented in the [Arduino README](../README.md).

## Trial probabilities

Water and air puff are drawn independently on every trial:

| Variable | Value | Meaning |
|---|---|---|
| `waterprob` | 100 % | Water on every trial |
| `puffprob` | 80 % | Real air puff on 80 % of trials, sham puff otherwise |

Expected trial mix:

| Probability | Code | Trial type |
|---|---|---|
| 80 % | **SR** | air puff + water |
| 20 % | **R** | water only (sham puff) |

The sham-pump relay (D10) never switches in this sketch.

## ⚠ Phase coding differs from the other sketches

In this sketch the `phase` column is coded by the **puff**, not the water:

- On real-puff trials, the phase is written as `3` / `4` / `5` / `7`.
- On sham-puff trials, the phase is always `9`.

Since every trial has water here, `phase = 9` means an **R** trial (water, no puff), not a trial without water.

## Data file

Written to `/mnt/sda1/arduino/www/` on the Yún as

```
allreward8020.txt<MM_DD_YY>~T~<hh_mm_ss>.txt
```

For example: `allreward8020.txt11_03_26~T~12_00_00.txt`. Note the `~T~` separator.
