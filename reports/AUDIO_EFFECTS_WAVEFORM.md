# Audio waveform report

Retail source: installed PS3 game in RPCS3.
Retail WAV files stay under ignored `tests/_results/audio/waveforms/`.

Alignment allows up to 30 ms; RMS ratio is OpenEgg divided by retail.

| Case | Shift ms | Correlation | RMS ratio | Left RMS ratio | Right RMS ratio | Result |
| --- | ---: | ---: | ---: | ---: | ---: | --- |
| normal | 0.4 | 0.993 | 0.990 | 0.990 | 0.990 | PASS |
| lowpass_400 | -1.1 | 0.063 | 54.984 | 54.984 | 54.984 | FAIL |
| lowpass_bypassed | 0.4 | 0.993 | 0.990 | 0.990 | 0.990 | PASS |
| lowpass_4000 | 1.7 | 0.823 | 0.483 | 0.483 | 0.483 | FAIL |

Matched: **2/4**. Thresholds: correlation ≥0.99, RMS within 2%, alignment within 30 ms. This checks a few short sounds, not full audio parity.
