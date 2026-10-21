# Audio waveform report

Retail source: installed PS3 game in RPCS3.
Retail WAV files stay under ignored `tests/_results/audio/waveforms/`.

Correlation uses aligned samples; RMS ratio is OpenEgg divided by retail.

| Case | Shift ms | Correlation | RMS ratio | Left RMS ratio | Right RMS ratio | Result |
| --- | ---: | ---: | ---: | ---: | ---: | --- |
| speed_half | 0.9 | 0.994 | 0.964 | 0.964 | 0.964 | FAIL |
| speed_double | 8.6 | 0.986 | 1.044 | 1.044 | 1.044 | FAIL |
| fade_40 | 5.8 | 0.991 | 0.999 | 0.999 | 0.999 | PASS |
| loop_one | 3.1 | 0.776 | 0.964 | 0.964 | 0.964 | FAIL |
| pause_resume | 2.4 | 0.993 | 0.990 | 0.990 | 0.990 | PASS |
| pan_left | 3.1 | 0.993 | 0.990 | 0.990 | silent | PASS |
| pan_right | 0.4 | 0.993 | 0.990 | silent | 0.990 | PASS |
| gate_one_ms | 8.9 | 0.673 | 1.147 | 1.147 | 1.147 | FAIL |

Matched: **4/8**. Thresholds: correlation ≥0.99 and RMS within 2%. This checks a few short sounds, not full audio parity.
