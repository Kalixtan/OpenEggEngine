# CHIP-8

A small CHIP-8 interpreter written in GameMonkeyScript. It has a menu of bundled CC0 games and demos; use Up/Down and Enter to choose one. The `games/` folder is also scanned for `.hex` ROMs.

The display is 64×32. Press Escape to return to the menu. Keyboard mapping is:

```text
1234    -> 123C
QWER    -> 456D
ASDF    -> 789E
ZXCV    -> A0BF
```

Add a ROM by converting a legally obtained `.ch8` file:

```sh
python3 convert_rom.py game.ch8 games/25_My_Game.hex
```

The interpreter targets classic CHIP-8. Super-CHIP instructions such as `DXY0` are not supported. Sound timer state is implemented, but there is no audio output yet. Bundled ROM titles, authors, source revision, and license are listed in [`games/SOURCES.md`](games/SOURCES.md).

Run from the repository root with a game data root containing `projects/chip8`:

```sh
build/OpenEgg projects/chip8 Main.gm Main
```

The script uses retail GameMonkeyScript bindings and does not modify OpenEgg engine code.
