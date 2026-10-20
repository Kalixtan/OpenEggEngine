# Audio waveform report

Retail source: installed PS3 game in RPCS3.
Retail WAV files stay in ignored `tests/_results/audio/waveforms/`.

Correlation uses aligned samples; RMS ratio is OpenEgg divided by retail.

| Case | Correlation | RMS ratio | Left RMS ratio | Right RMS ratio | Result |
| --- | ---: | ---: | ---: | ---: | --- |
| normal | 0.993 | 0.990 | 0.990 | 0.990 | PASS |
| half_volume | 0.993 | 0.990 | 0.990 | 0.990 | PASS |
| pan_left | 0.993 | 0.990 | 0.990 | silent | PASS |
| pan_right | 0.993 | 0.990 | silent | 0.990 | PASS |

Matched: **4/4**. Thresholds: correlation ≥0.99 and RMS within 2%. This checks four short sounds, not full audio parity.
