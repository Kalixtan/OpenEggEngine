# Audio waveform report

Retail source: installed PS3 game in RPCS3.
Retail WAV files stay in ignored `tests/_results/audio/waveforms/`.

Correlation uses aligned samples; RMS ratio is OpenEgg divided by retail.

| Case | Correlation | RMS ratio | Left RMS ratio | Right RMS ratio | Result |
| --- | ---: | ---: | ---: | ---: | --- |
| normal | 0.993 | 2.101 | 2.101 | 2.101 | FAIL |
| half_volume | 0.993 | 2.101 | 2.101 | 2.101 | FAIL |
| pan_left | 0.993 | 1.486 | 1.486 | silent | FAIL |
| pan_right | 0.993 | 1.486 | silent | 1.486 | FAIL |

Matched: **0/4**. This checks four short sounds, not full audio parity.
