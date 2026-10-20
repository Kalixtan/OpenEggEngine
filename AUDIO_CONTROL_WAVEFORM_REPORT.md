# Audio waveform report

Retail source: installed PS3 game in RPCS3.
Retail WAV files stay under ignored `tests/_results/audio/waveforms/`.

Correlation uses aligned samples; RMS ratio is OpenEgg divided by retail.

| Case | Shift ms | Correlation | RMS ratio | Left RMS ratio | Right RMS ratio | Result |
| --- | ---: | ---: | ---: | ---: | ---: | --- |
| speed_half | 0.9 | 0.994 | 0.964 | 0.964 | 0.964 | FAIL |
| speed_double | -2.0 | 0.986 | 1.044 | 1.044 | 1.044 | FAIL |
| fade_40 | -4.9 | 0.976 | 1.497 | 1.497 | 1.497 | FAIL |
| loop_one | -2.2 | 0.776 | 0.964 | 0.964 | 0.964 | FAIL |
| pause_resume | -2.9 | 0.993 | 0.990 | 0.990 | 0.990 | PASS |

Matched: **1/5**. Thresholds: correlation ≥0.99 and RMS within 2%. This checks a few short sounds, not full audio parity.
