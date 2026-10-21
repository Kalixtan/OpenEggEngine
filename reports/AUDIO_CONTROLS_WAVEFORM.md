# Audio waveform report

Retail source: installed PS3 game in RPCS3.
Retail WAV files stay under ignored `tests/_results/audio/waveforms/`.

Correlation uses aligned samples; RMS ratio is OpenEgg divided by retail.

| Case | Shift ms | Correlation | RMS ratio | Left RMS ratio | Right RMS ratio | Result |
| --- | ---: | ---: | ---: | ---: | ---: | --- |
| speed_half | 0.9 | 0.994 | 0.998 | 0.998 | 0.998 | PASS |
| speed_double | -2.0 | 0.986 | 0.998 | 0.998 | 0.998 | FAIL |
| fade_40 | 0.4 | 0.886 | 1.324 | 1.324 | 1.324 | FAIL |
| loop_one | -13.4 | 1.000 | 0.943 | 0.943 | 0.943 | FAIL |
| pause_resume | -24.2 | 0.000 | 0.990 | 0.990 | 0.990 | FAIL |
| pan_left | -7.7 | 0.993 | 1.841 | 1.841 | silent | FAIL |
| pan_right | -7.5 | 0.999 | 1.145 | silent | 1.145 | FAIL |
| gate_100_ms | -8.1 | 0.957 | 1.011 | 1.011 | 1.011 | FAIL |

Matched: **1/8**. Thresholds: correlation ≥0.99 and RMS within 2%. This checks a few short sounds, not full audio parity.
