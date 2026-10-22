# Audio waveform report

Retail source: installed PS3 game in RPCS3.
Retail WAV files stay under ignored `tests/_results/audio/waveforms/`.

Alignment allows up to 30 ms; RMS ratio is OpenEgg divided by retail.

| Case | Shift ms | Correlation | RMS ratio | Left RMS ratio | Right RMS ratio | Result |
| --- | ---: | ---: | ---: | ---: | ---: | --- |
| speed_half | 0.9 | 0.994 | 0.998 | 0.998 | 0.998 | PASS |
| speed_double | 0.7 | 0.986 | 0.998 | 0.998 | 0.998 | FAIL |
| speed_double_deferred | 0.4 | 0.844 | 0.994 | 0.994 | 0.994 | FAIL |
| fade_40 | 0.4 | 0.991 | 0.999 | 0.999 | 0.999 | PASS |
| loop_one | -0.0 | 0.989 | 1.014 | 1.014 | 1.014 | FAIL |
| pause_resume | 0.4 | 0.993 | 0.990 | 0.990 | 0.990 | PASS |
| pan_left | 0.4 | 0.993 | 0.990 | 0.990 | silent | PASS |
| pan_right | 0.4 | 0.993 | 0.990 | silent | 0.990 | PASS |
| gate_100_ms | 0.0 | 0.997 | 1.014 | 1.014 | 1.014 | PASS |

Matched: **6/9**. Thresholds: correlation ≥0.99, RMS within 2%, alignment within 30 ms. This checks a few short sounds, not full audio parity.
