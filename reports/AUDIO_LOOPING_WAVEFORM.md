# Audio waveform report

Retail source: installed PS3 game in RPCS3.
Retail WAV files stay under ignored `tests/_results/audio/waveforms/`.

Alignment allows up to 30 ms; RMS ratio is OpenEgg divided by retail.

| Case | Shift ms | Correlation | RMS ratio | Left RMS ratio | Right RMS ratio | Result |
| --- | ---: | ---: | ---: | ---: | ---: | --- |
| no_loop | 0.4 | 0.993 | 0.990 | 0.990 | 0.990 | PASS |
| loop_count_1 | -0.0 | 0.989 | 1.014 | 1.014 | 1.014 | FAIL |
| loop_count_2 | -0.0 | 0.993 | 1.006 | 1.006 | 1.006 | PASS |
| loop_count_3 | -0.0 | 0.995 | 1.002 | 1.002 | 1.002 | PASS |
| loop_forever_timed_stop | -0.0 | 0.996 | 0.999 | 0.999 | 0.999 | PASS |
| offset_loop_1 | -0.1 | 0.999 | 0.992 | 0.992 | 0.992 | PASS |

Matched: **5/6**. Thresholds: correlation ≥0.99, RMS within 2%, alignment within 30 ms. This checks a few short sounds, not full audio parity.
