# minimp3

Vendored `minimp3.h`, `minimp3_ex.h`, and the upstream CC0 license from
[lieff/minimp3](https://github.com/lieff/minimp3) at commit
`ea99364f61c14656440e8d77e9c233ccf3124633`.

The engine uses the MP3-only high-level file decoder for the MP3 assets accepted
by retail FMOD's default resource path. This avoids an additional platform
library dependency. VAG and FSB continue to use their separate retail resource
kinds; FSB decoding is not implemented yet.
