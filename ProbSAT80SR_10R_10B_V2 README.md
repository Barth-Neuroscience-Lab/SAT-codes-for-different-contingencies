# ProbSAT80SR_10R_10B_V2

One of the eight air-puff / water-reward task sketches for the Arduino Yún (fixed-mix design). The task, timing, wiring, data format and upload steps are shared by the sketches and documented in the [Arduino README](../README.md).

## Trial-type probabilities

| Probability | Code | Trial type |
|---|---|---|
| 80 % | **SR** | air puff + water |
| 10 % | **R** | water only (sham puff) |
| 10 % | **B** | blank (sham puff + sham pump) |

## Data file

Written to `/mnt/sda1/arduino/www/` on the Yún as

```
PrSAT80sr10r10b.txt<MM_DD_YY>_T_<hh_mm_ss>.txt
```

For example: `PrSAT80sr10r10b.txt11_03_26_T_12_00_00.txt`.
