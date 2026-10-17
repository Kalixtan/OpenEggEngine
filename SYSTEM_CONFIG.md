# Runtime system settings

The executable reads `OpenEgg.ini` from its own directory at startup. This
is independent of the shell's current directory. `OpenEgg.ini.example` is the
tracked template; copy it beside the executable and name it `OpenEgg.ini`.

```ini
[system]
os_id = ps3
language = en_us
retail_build = true
letterbox = true
```

`os_id` accepts `windows`/`win32`, `osx`, `ps3`, `ps4`, `ngp`/`vita`,
`vitatv`, or numeric IDs 0 through 5. It is passed to the script-facing
`SYSTEM_GetOSID()` and selects platform-specific game scripts. The executable
prints the selected OS and language code on startup.

`language` accepts the names and numeric codes listed in the INI template.
`retail_build` controls `SYSTEM_IsRetailBuild()`. `SYSTEM_IsBeta()` remains
the retail constant false, matching its native implementation.

`letterbox` controls desktop viewport presentation. `true` preserves the
configured game aspect ratio and draws black bars where the host window has
extra space. Set it to `false` to use the full host window, as the earlier
desktop renderer did. In that mode, viewport and script-facing aspect-ratio
queries follow the current window dimensions, so UI and world framing adapt
to different window shapes. The default is `true`.

The `SYSTEM_GetOSDescription()` names `Win32`, `OS X`, `PS3`, and `NGP` are
grounded in script call sites. The PS4 and Vita TV labels are currently
provisional; verify those against the native platform string table before
using those modes for parity work.

Invalid values are reported and leave that setting at its default. Missing
files use the retail defaults: PS3, English US, retail build enabled.
