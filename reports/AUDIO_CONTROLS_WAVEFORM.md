# Audio waveform report

Retail source: installed PS3 game in RPCS3.
Retail WAV files stay under ignored `tests/_results/audio/waveforms/`.

Correlation uses aligned samples; RMS ratio is OpenEgg divided by retail.

| Case | Shift ms | Correlation | RMS ratio | Left RMS ratio | Right RMS ratio | Result |
| --- | ---: | ---: | ---: | ---: | ---: | --- |
| speed_half | 0.9 | 0.994 | 1.001 | 1.001 | 1.001 | PASS |
| speed_double | -2.0 | 0.986 | 1.011 | 1.011 | 1.011 | FAIL |
| fade_40 | -4.9 | 0.991 | 0.999 | 0.999 | 0.999 | PASS |
| loop_one | -13.4 | 1.000 | 0.943 | 0.943 | 0.943 | FAIL |
| pause_resume | -2.9 | 0.993 | 0.990 | 0.990 | 0.990 | PASS |
| pan_left | -7.1 | 0.993 | 0.994 | 0.994 | silent | PASS |
| pan_right | -7.5 | 0.999 | 1.145 | silent | 1.145 | FAIL |
| gate_one_ms | 155.0 | 0.000 | 0.282 | 0.282 | 0.282 | FAIL |

Matched: **4/8**. Thresholds: correlation ≥0.99 and RMS within 2%. This checks a few short sounds, not full audio parity.
