# Audio waveform report

Retail source: installed PS3 game in RPCS3.
Retail WAV files stay under ignored `tests/_results/audio/waveforms/`.

Alignment allows up to 30 ms; RMS ratio is OpenEgg divided by retail.

| Case | Shift ms | Correlation | RMS ratio | Left RMS ratio | Right RMS ratio | Result |
| --- | ---: | ---: | ---: | ---: | ---: | --- |
| immediate_2x | 0.7 | 0.986 | 1.011 | 1.011 | 1.011 | FAIL |
| delay_1_tick | 0.4 | 0.844 | 0.994 | 0.994 | 0.994 | FAIL |
| delay_2_ticks | 0.4 | 0.982 | 0.989 | 0.989 | 0.989 | FAIL |
| delay_3_ticks | 0.4 | 0.991 | 0.991 | 0.991 | 0.991 | PASS |

Matched: **1/4**. Thresholds: correlation ≥0.99, RMS within 2%, alignment within 30 ms. This checks a few short sounds, not full audio parity.
