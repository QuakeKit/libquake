#pragma once

const char *entbuff = R"""(
{
"sounds" "4"
"classname" "worldspawn"
"wad" "/home/tino/engines/quakeED/wads/QUAKE101.WAD"
"message" "Introduction"
"worldtype" "0"
}
{
"classname" "info_player_start"
"origin" "544 288 32"
"angle" "90"
}
{
"sounds" "3"
"speed" "50"
"angle" "270"
"classname" "func_door"
"wait" "-1"
"model" "*6"
}
{
"targetname" "t2"
"speed" "50"
"angle" "90"
"classname" "func_door"
"wait" "-1"
"model" "*7"
}
{
"light" "150"
"origin" "-168 2392 152"
"classname" "light"
}
{
"wait" "-1"
"target" "t2"
"message" "You haven't registered Quake!\n\nCall 1-800-idgames to unlock\nthe full game from CD-ROM\nor for mail delivery."
"classname" "trigger_onlyregistered"
"model" "*8"
}
{
"message" "0"
"classname" "trigger_setskill"
"model" "*9"
}
{
"target" "t11"
"classname" "trigger_teleport"
"model" "*54"
}
{
"targetname" "t11"
"origin" "-124 1748 -668"
"classname" "info_null"
}
)""";
