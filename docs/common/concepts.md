# Common Concepts

## Coordinate System
Quake uses a **Z-Up**, Right-Handed coordinate system:
- **X**: Forward / East
- **Y**: Left / North
- **Z**: Up

## Units
- **1 Unit** ≈ 1 Inch (approx 2.54cm).
- **Player Height**: 56 units.
- **Player Width**: 32x32 units.

## Scale Factor
In `libquake`, we typically apply an `inverseScale` (often 1/24.0f or similar) when rendering in engines that expect 1 unit = 1 meter, as Quake units are quite large.

## Reference
- [Quake Units](https://quakewiki.org/wiki/Quake_Standards)
