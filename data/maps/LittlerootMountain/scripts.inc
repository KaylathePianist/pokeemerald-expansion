LittlerootMountain_MapScripts::
	.byte 0


LittlerootMountain_RockSign::
# 4 "data/maps/LittlerootMountain/scripts.pory"
	lockall
# 5 "data/maps/LittlerootMountain/scripts.pory"
	msgbox LittlerootMountain_RockSign_Text_0
# 6 "data/maps/LittlerootMountain/scripts.pory"
	releaseall
	return


LittlerootMountain_CaveSign::
# 10 "data/maps/LittlerootMountain/scripts.pory"
	lockall
# 11 "data/maps/LittlerootMountain/scripts.pory"
	msgbox LittlerootMountain_CaveSign_Text_0
# 12 "data/maps/LittlerootMountain/scripts.pory"
	releaseall
	return


LittlerootMountain_RockSign_Text_0:
# 5 "data/maps/LittlerootMountain/scripts.pory"
	.string "WARNING: Sharp rocks ahead!$"

LittlerootMountain_CaveSign_Text_0:
# 11 "data/maps/LittlerootMountain/scripts.pory"
	.string "LITTLEROOT MOUNTAIN\nWe aren't sure why we call it little.$"
