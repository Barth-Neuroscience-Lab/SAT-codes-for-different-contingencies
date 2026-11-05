# pseudo_80_V2

One of the eight air-puff / water-reward task sketches for the Arduino Yún (independent-draw design). The task, timing, wiring, data format and upload steps are shared by the sketches and documented in the [Arduino README](../README.md).

## Trial probabilities

Water and air puff are drawn independently on every trial:

| Variable | Value | Meaning |
|---|---|---|
| `waterprob` | 50 % | Real water on 50 % of trials, sham pump otherwise |
| `puffprob` | 80 % | Real air puff on 80 % of trials, sham puff otherwise |

Expected trial mix:

| Probability | Code | Trial type |
|---|---|---|
| 40 % | **SR** | air puff + water |
| 40 % | **S** | air puff only (sham pump) |
| 10 % | **R** | water only (sham puff) |
| 10 % | **B** | blank (sham puff + sham pump) |

Phase coding follows the water, as in the ProbSAT sketches.

## Data file

Written to `/mnt/sda1/arduino/www/` on the Yún as

```
pseudo2.txt<MM_DD_YY>~T~<hh_mm_ss>.txt
```

For example: `pseudo2.txt11_03_26~T~12_00_00.txt`. Note the `~T~` separator.

