# Audio waveform report

Retail source: installed PS3 game in RPCS3.
Retail WAV files stay under ignored `tests/_results/audio/waveforms/`.

Alignment allows up to 30 ms; RMS ratio is OpenEgg divided by retail.

| Case | Shift ms | Correlation | RMS ratio | Left RMS ratio | Right RMS ratio | Result |
| --- | ---: | ---: | ---: | ---: | ---: | --- |
| speed_half | 0.9 | 0.994 | 0.998 | 0.998 | 0.998 | PASS |
| speed_double | 0.7 | 0.986 | 0.998 | 0.998 | 0.998 | FAIL |
| fade_40 | 0.4 | 0.886 | 1.324 | 1.324 | 1.324 | FAIL |
| loop_one | -0.0 | 0.989 | 0.966 | 0.966 | 0.966 | FAIL |
| pause_resume | 0.4 | 0.993 | 0.990 | 0.990 | 0.990 | PASS |
| pan_left | 0.4 | 0.993 | 0.990 | 0.990 | silent | PASS |
| pan_right | 0.4 | 0.993 | 0.990 | silent | 0.990 | PASS |
| gate_100_ms | -0.1 | 0.947 | 0.956 | 0.956 | 0.956 | FAIL |

Matched: **4/8**. Thresholds: correlation ≥0.99, RMS within 2%, alignment within 30 ms. This checks a few short sounds, not full audio parity.
