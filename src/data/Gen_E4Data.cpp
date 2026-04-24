// DO NOT EDIT -- auto-generated

#include "data/Gen_E4Data.h"

#include "game/ObjectSlots.h"

#include <cstddef>

namespace gen_e4
{

const ScriptInstr SCRIPT[SCRIPT_COUNT] = {

	// Intro Script
	{OP_SET_E4_WORD, 0x00, SVAR_TEXT_SPEED, 0x007D, 0, 0, 0, 0, -1,
	 NULL}, // [0] $0110
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1,
	 "  NOVADRIVE COUNTDOWN  "}, // [1] $0116
	{OP_SET_E4_WORD, 0x00, SVAR_TEXT_SPEED, 0x0032, 0, 0, 0, 0, -1,
	 NULL},												   // [2] $0130
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1, "          3 "}, // [3] $0136
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1, "          2 "}, // [4] $0146
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1, "          1 "}, // [5] $0156
	{OP_SET_E4_WORD, 0x00, SVAR_TEXT_SPEED, 0x0019, 0, 0, 0, 0, -1,
	 NULL},												   // [6] $0166
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1, "          0 "}, // [7] $016C
	{OP_SET_E4_WORD, 0x00, SVAR_SCRIPT_RUNNING, 0x0001, 0, 0, 0, 0, -1,
	 NULL},											   // [8] $017C
	{OP_CALL, 0x00, 0x06CE, 0, 0, 0, 0, 0, 130, NULL}, // [9] $0182 -> Delay 100
	{OP_SET_E4_WORD, 0x00, SVAR_TEXT_SPEED, 0x007D, 0, 0, 0, 0, -1,
	 NULL},														   // [10] $0186
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1, "   NOVADRIVE ENGAGED"}, // [11] $018C
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1,
	 " PRESTINIUM ON COURSE"}, // [12] $01A4
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1,
	 "DESTINATION-GAMMA FIVE"},							 // [13] $01BC
	{OP_DISPLAY_MSG, 0x00, 0, 0, 0, 50, 0, 0, -1, NULL}, // [14] $01D6
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1,
	 " DAMAGE CONTROL REPORT"}, // [15] $01DA
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1,
	 " SOME CONFLICT DAMAGE"},								  // [16] $01F4
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1, "       CHECKING"}, // [17] $020C
	{OP_DISPLAY_MSG, 0x00, 0, 0, 0, 50, 0, 0, -1, NULL},	  // [18] $021E
	{OP_CALL, 0x00, 0x06CE, 0, 0, 0, 0, 0, 130,
	 NULL}, // [19] $0222 -> Delay 100
	{OP_SET_VAR_L, 0x00, 0, 0, 0x00009000, 0x00F8, 0, 0, -1,
	 NULL},													   // [20] $0226
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1, "      EMERGENCY!"}, // [21] $022E
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1,
	 " GUIDANCE SYSTEM FAULT"},									  // [22] $0242
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1, "   COLLISION COURSE"}, // [23] $025C
	{OP_SET_E4_WORD, 0x00, SVAR_SCRIPT_RUNNING, 0x8000, 0, 0, 0, 0, -1,
	 NULL}, // [24] $0272
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1,
	 " DISENGAGING NOVADRIVE"}, // [25] $0278
	{OP_CALL, 0x00, 0x06CE, 0, 0, 0, 0, 0, 130,
	 NULL}, // [26] $0292 -> Delay 100
	{OP_CALL, 0x00, 0x06CE, 0, 0, 0, 0, 0, 130,
	 NULL}, // [27] $0296 -> Delay 100
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1, "   UNABLE TO CORRECT"}, // [28] $029A
	{OP_SET_E4_WORD, 0x00, SVAR_SCRIPT_RUNNING, 0xFFFF, 0, 0, 0, 0, -1,
	 NULL}, // [29] $02B2
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1,
	 "MAXIMUM REVERSE THRUST"}, // [30] $02B8

	// Intro Spin
	{OP_GOTO, 0x00, 0x02D2, 0, 0, 0, 0, 0, 31,
	 NULL}, // [31] $02D2 -> Intro Spin

	// Crash Message
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1, "     CRASH"},   // [32] $02D6
	{OP_CLR_FLAGS, 0x00, 0, 0, 0, 0, 0x0040, 0, -1, NULL}, // [33] $02E4
	{OP_RETURN, 0x00, 0, 0, 0, 0, 0, 0, -1, NULL},		   // [34] $02E8

	// Landing Script (Event 0)
	{OP_SET_VAR_W, 0x00, 0, OBJ_DOMINION_DART_B, 0, 0x00FE, 0, 0, -1,
	 NULL},														 // [35] $02EA
	{OP_SET_ENTITY_FLAG, 0x00, 0, 0x0004, 0, 0, 0, 0, -1, NULL}, // [36] $02F0
	{OP_SET_VAR_W, 0x00, 0, OBJ_INTERSTELLAR_CRAFT, 0, 0x00FE, 0, 0, -1,
	 NULL},														 // [37] $02F4
	{OP_SET_ENTITY_FLAG, 0x00, 0, 0x0004, 0, 0, 0, 0, -1, NULL}, // [38] $02FA
	{OP_SET_VAR_W, 0x00, 0, OBJ_MECHANOID, 0, 0x00FE, 0, 0, -1,
	 NULL},														 // [39] $02FE
	{OP_SET_ENTITY_FLAG, 0x00, 0, 0x0004, 0, 0, 0, 0, -1, NULL}, // [40] $0304
	{OP_DISPLAY_MSG, 0x00, 0, 0, 0, 50, 0, 0, -1, NULL},		 // [41] $0308
	{OP_CALL, 0x00, 0x06E2, 0, 0, 0, 0, 0, 133, NULL}, // [42] $030C -> Delay 50
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1, "    CRASH IMMINENT!"}, // [43] $0310
	{OP_CALL, 0x00, 0x06E2, 0, 0, 0, 0, 0, 133, NULL}, // [44] $0326 -> Delay 50
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1, " RETURNING TO - - -"}, // [45] $032A
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1, "    MANUAL CONTROL"},  // [46] $0340
	{OP_CALL, 0x00, 0x06CE, 0, 0, 0, 0, 0, 130,
	 NULL}, // [47] $0356 -> Delay 100
	{OP_RANDOM_MSG, 0x00, 0, 0, 0, 0, 0x0003, 60, -1, NULL}, // [48] $035A
	{OP_DISPLAY_MSG, 0x00, 0, 0, 0, 50, 0, 0, -1, NULL},	 // [49] $0360

	// Status Report
	{OP_CLR_FLAGS, 0x00, 0, 0, 0, 0, 0x0080, 0, -1, NULL},		 // [50] $0364
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1, "     STATUS REPORT"}, // [51] $0368
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1,
	 " CRASH LANDED ON TARG"}, // [52] $037E
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1,
	 " PRESTINIUM DESTROYED"}, // [53] $0396
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1,
	 " STATE OF WAR BETWEEN"}, // [54] $03AE
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1,
	 "PALYARS AND MECHANOIDS"}, // [55] $03C6
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1,
	 " LOCATION NEAR AIRBASE"},									   // [56] $03E0
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1, "    AIRCRAFT ON PAD"},  // [57] $03FA
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1, " TYPE  DOMINION DART"}, // [58] $0410
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1, "   MESSAGE RECEIVED"},  // [59] $0428
	{OP_SET_E4_WORD, 0x00, SVAR_TEXT_SPEED, 0x007D, 0, 0, 0, 0, -1,
	 NULL},														   // [60] $043E
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1, "    CRAFT FOR SALE"},   // [61] $0444
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1, "  PRICE 5000 CREDITS"}, // [62] $045A
	{OP_DISPLAY_MSG, 0x00, 0, 0, 0, 21, 0, 0, -1, NULL},		   // [63] $0472
	{OP_CALL, 0x00, 0x06F6, 0, 0, 0, 0, 0, 139,
	 NULL}, // [64] $0476 -> Y/N Prompt
	{OP_IF_FLAGS, 0x00, 0, 0, 0, 0, 0x0001, 0, 102,
	 NULL}, // [65] $047A -> Purchase Handler
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1,
	 "I THINK YOU SHOULD BUY"}, // [66] $0480
	{OP_CALL, 0x00, 0x06F6, 0, 0, 0, 0, 0, 139,
	 NULL}, // [67] $049A -> Y/N Prompt
	{OP_IF_FLAGS, 0x00, 0, 0, 0, 0, 0x0001, 0, 102,
	 NULL}, // [68] $049E -> Purchase Handler
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1, "   IS THIS SENSIBLE"}, // [69] $04A4
	{OP_CALL, 0x00, 0x06F6, 0, 0, 0, 0, 0, 139,
	 NULL}, // [70] $04BA -> Y/N Prompt
	{OP_IF_FLAGS, 0x00, 0, 0, 0, 0, 0x0001, 0, 102,
	 NULL}, // [71] $04BE -> Purchase Handler
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1, "   YOU PLAN TO WALK"}, // [72] $04C4
	{OP_CALL, 0x00, 0x06F6, 0, 0, 0, 0, 0, 139,
	 NULL}, // [73] $04DA -> Y/N Prompt
	{OP_IF_FLAGS, 0x00, 0, 0, 0, 0, 0x0001, 0, 102,
	 NULL}, // [74] $04DE -> Purchase Handler
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1, "   YOU MUST BE CRAZY"}, // [75] $04E4
	{OP_IF_FLAGS, 0x00, 0, 0, 0, 0, 0x0080, 0, 82,
	 NULL}, // [76] $04FC -> Long Walk
	{OP_DISPLAY_MSG, 0x00, 0, 0, 0, 50, 0, 0, -1, NULL}, // [77] $0502
	{OP_CALL, 0x00, 0x0568, 0, 0, 0, 0, 0, 86,
	 NULL}, // [78] $0506 -> Is Anybody There
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1,
	 "NOW YOU ARE HERE - - -"},									   // [79] $050A
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1, "  I WILL START AGAIN"}, // [80] $0524
	{OP_GOTO, 0x00, 0x0364, 0, 0, 0, 0, 0, 50,
	 NULL}, // [81] $053C -> Status Report

	// Long Walk
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1,
	 "IT WILL BE A LONG WALK"},							 // [82] $0540
	{OP_DISPLAY_MSG, 0x00, 0, 0, 0, 50, 0, 0, -1, NULL}, // [83] $055A

	// Wait For Craft
	{OP_IF_HAS_ITEM, 0x00, 0, OBJ_DOMINION_DART_B, 0, 0, 0, 0, 107,
	 NULL}, // [84] $055E -> Stolen Craft (Event 1)
	{OP_GOTO, 0x00, 0x055E, 0, 0, 0, 0, 0, 84,
	 NULL}, // [85] $0564 -> Wait For Craft

	// Is Anybody There
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1, "   IS ANYBODY THERE"}, // [86] $0568
	{OP_CALL, 0x00, 0x06BE, 0, 0, 0, 0, 0, 126,
	 NULL}, // [87] $057E -> Delay Or Input
	{OP_IF_FLAGS, 0x00, 0, 0, 0, 0, 0x0080, 0, 99,
	 NULL}, // [88] $0582 -> You Are Back
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1,
	 "AM I TALKING TO MYSELF"}, // [89] $0588
	{OP_CALL, 0x00, 0x06BE, 0, 0, 0, 0, 0, 126,
	 NULL}, // [90] $05A2 -> Delay Or Input
	{OP_IF_FLAGS, 0x00, 0, 0, 0, 0, 0x0080, 0, 99,
	 NULL}, // [91] $05A6 -> You Are Back
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1,
	 "  ARE YOU STILL ALIVE"}, // [92] $05AC
	{OP_CALL, 0x00, 0x06BE, 0, 0, 0, 0, 0, 126,
	 NULL}, // [93] $05C4 -> Delay Or Input
	{OP_IF_FLAGS, 0x00, 0, 0, 0, 0, 0x0080, 0, 99,
	 NULL}, // [94] $05C8 -> You Are Back
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1, "     WHERE ARE YOU"}, // [95] $05CE

	// Buy Retry
	{OP_CALL, 0x00, 0x06BE, 0, 0, 0, 0, 0, 126,
	 NULL}, // [96] $05E4 -> Delay Or Input
	{OP_IF_FLAGS, 0x00, 0, 0, 0, 0, 0x0080, 0, 99,
	 NULL}, // [97] $05E8 -> You Are Back
	{OP_GOTO, 0x00, 0x05E4, 0, 0, 0, 0, 0, 96, NULL}, // [98] $05EE -> Buy Retry

	// You Are Back
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1, "   AH! YOU ARE BACK"}, // [99] $05F2
	{OP_CALL, 0x00, 0x06CE, 0, 0, 0, 0, 0, 130,
	 NULL},										   // [100] $0608 -> Delay 100
	{OP_RETURN, 0x00, 0, 0, 0, 0, 0, 0, -1, NULL}, // [101] $060C

	// Purchase Handler
	{OP_ADD_VAR_BCD, 0x00, 0, 0, 0x99995000, 0x00F8, 0, 0, -1,
	 NULL}, // [102] $060E
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1,
	 " TRANSACTION COMPLETED"},							 // [103] $0616
	{OP_DISPLAY_MSG, 0x00, 0, 0, 0, 21, 0, 0, -1, NULL}, // [104] $0630
	{OP_DISPLAY_MSG, 0x00, 0, 0, 0, 50, 0, 0, -1, NULL}, // [105] $0634
	{OP_GOTO, 0x00, 0x055E, 0, 0, 0, 0, 0, 84,
	 NULL}, // [106] $0638 -> Wait For Craft

	// Stolen Craft (Event 1)
	{OP_SET_VAR_W, 0x00, 0, OBJ_DOMINION_DART_B, 0, 0x00FE, 0, 0, -1,
	 NULL},														 // [107] $063C
	{OP_SET_ENTITY_FLAG, 0x00, 0, 0x00F3, 0, 0, 0, 0, -1, NULL}, // [108] $0642
	{OP_CLR_FLAGS, 0x00, 0, 0, 0, 0, 0x0040, 0, -1, NULL},		 // [109] $0646
	{OP_IF_FLAGS, 0x00, 0, 0, 0, 0, 0x0001, 0, 120,
	 NULL}, // [110] $064A -> NPC Encounter
	{OP_SET_E4_WORD, 0x00, SVAR_TEXT_SPEED, 0x007D, 0, 0, 0, 0, -1,
	 NULL}, // [111] $0650
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1,
	 "THIS CRAFT BELONGED TO"},							 // [112] $0656
	{OP_DISPLAY_MSG, 0x00, 0, 0, 0, 17, 0, 0, -1, NULL}, // [113] $0670
	{OP_DISPLAY_MSG, 0x00, 0, 0, 0, 18, 0, 0, -1, NULL}, // [114] $0674
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1,
	 " HE IS NOT TOO PLEASED"},							   // [115] $0678
	{OP_SET_FLAGS, 0x00, 0, 0, 0, 0, 0x0020, 0, -1, NULL}, // [116] $0692
	{OP_CALL, 0x00, 0x1420, 0, 0, 0, 0, 0, 492,
	 NULL}, // [117] $0696 -> Palyar Ship Attack
	{OP_DISPLAY_MSG, 0x00, 0, 0, 0, 50, 0, 0, -1, NULL}, // [118] $069A
	{OP_GOTO, 0x00, 0x06BA, 0, 0, 0, 0, 0, 125,
	 NULL}, // [119] $069E -> NPC Goto Wrapper

	// NPC Encounter
	{OP_CALL, 0x00, 0x06EC, 0, 0, 0, 0, 0, 136,
	 NULL}, // [120] $06A2 -> Delay 750
	{OP_IF_RANDOM_GE, 0x80, 0, 0x5000, 0, 0, 0, 0, 124,
	 NULL}, // [121] $06A6 -> NPC Call Wrapper
	{OP_SET_VAR_W, 0x00, 0, 0x0000, 0, 0x00FF, 0, 0, -1, NULL}, // [122] $06AC
	{OP_IF_IN_BLDG, 0x00, 0, 0, 0, 0, 0, 0, 120,
	 NULL}, // [123] $06B2 -> NPC Encounter

	// NPC Call Wrapper
	{OP_CALL, 0x00, 0x072A, 0, 0, 0, 0, 0, 148,
	 NULL}, // [124] $06B6 -> Briefing Info

	// NPC Goto Wrapper
	{OP_GOTO, 0x00, 0x06BA, 0, 0, 0, 0, 0, 125,
	 NULL}, // [125] $06BA -> NPC Goto Wrapper

	// Delay Or Input
	{OP_CLR_COUNTER, 0x00, 0, 0, 0, 0, 0, 0, -1, NULL}, // [126] $06BE
	{OP_IF_FLAGS, 0x00, 0, 0, 0, 0, 0x0080, 0, 129,
	 NULL}, // [127] $06C0 -> Return Only
	{OP_IF_COUNTER_GE, 0x00, 0, 0x02EE, 0, 0, 0, 0, 127,
	 NULL}, // [128] $06C6 -> Delay Or Input+1

	// Return Only
	{OP_RETURN, 0x00, 0, 0, 0, 0, 0, 0, -1, NULL}, // [129] $06CC

	// Delay 100
	{OP_CLR_COUNTER, 0x00, 0, 0, 0, 0, 0, 0, -1, NULL}, // [130] $06CE
	{OP_IF_COUNTER_GE, 0x00, 0, 0x0064, 0, 0, 0, 0, 131,
	 NULL},										   // [131] $06D0 -> Delay 100+1
	{OP_RETURN, 0x00, 0, 0, 0, 0, 0, 0, -1, NULL}, // [132] $06D6

	// Delay 50
	{OP_CLR_COUNTER, 0x00, 0, 0, 0, 0, 0, 0, -1, NULL}, // [133] $06E2
	{OP_IF_COUNTER_GE, 0x00, 0, 0x0032, 0, 0, 0, 0, 134,
	 NULL},										   // [134] $06E4 -> Delay 50+1
	{OP_RETURN, 0x00, 0, 0, 0, 0, 0, 0, -1, NULL}, // [135] $06EA

	// Delay 750
	{OP_CLR_COUNTER, 0x00, 0, 0, 0, 0, 0, 0, -1, NULL}, // [136] $06EC
	{OP_IF_COUNTER_GE, 0x00, 0, 0x02EE, 0, 0, 0, 0, 137,
	 NULL},										   // [137] $06EE -> Delay 750+1
	{OP_RETURN, 0x00, 0, 0, 0, 0, 0, 0, -1, NULL}, // [138] $06F4

	// Y/N Prompt
	{OP_ENABLE_INPUT, 0x00, 0, 0, 0, 0, 0, 0, -1, NULL}, // [139] $06F6
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1,
	 "  DO YOU WANT TO BUY?"},							// [140] $06F8
	{OP_CLR_COUNTER, 0x00, 0, 0, 0, 0, 0, 0, -1, NULL}, // [141] $0710
	{OP_IF_VEHICLE, 0x00, 0, 0x0015, 0, 0, 0, 0, 146,
	 NULL}, // [142] $0712 -> Y/N Prompt+7
	{OP_IF_COUNTER_GE, 0x00, 0, 0x00FA, 0, 0, 0, 0, 142,
	 NULL}, // [143] $0718 -> Y/N Prompt+3
	{OP_CLR_FLAGS, 0x00, 0, 0, 0, 0, 0x0001, 0, -1, NULL}, // [144] $071E
	{OP_RETURN, 0x00, 0, 0, 0, 0, 0, 0, -1, NULL},		   // [145] $0722
	{OP_SET_FLAGS, 0x00, 0, 0, 0, 0, 0x0001, 0, -1, NULL}, // [146] $0724
	{OP_RETURN, 0x00, 0, 0, 0, 0, 0, 0, -1, NULL},		   // [147] $0728

	// Briefing Info
	{OP_DISPLAY_MSG, 0x00, 0, 0, 0, 3, 0, 0, -1, NULL}, // [148] $072A
	{OP_DISPLAY_MSG, 0x00, 0, 0, 0, 5, 0, 0, -1, NULL}, // [149] $072E
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1,
	 " FOR MORE INFORMATION"}, // [150] $0732
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1,
	 "GO TO THE BRIEFING ROOM"}, // [151] $074A
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1,
	 "IN COMPLEX AT LOC 09-06"},						 // [152] $0764
	{OP_DISPLAY_MSG, 0x00, 0, 0, 0, 7, 0, 0, -1, NULL},	 // [153] $077E
	{OP_DISPLAY_MSG, 0x00, 0, 0, 0, 50, 0, 0, -1, NULL}, // [154] $0782
	{OP_RETURN, 0x00, 0, 0, 0, 0, 0, 0, -1, NULL},		 // [155] $0786

	// Mechanoid Demands (Event 12)
	{OP_CALL, 0x00, 0x06E2, 0, 0, 0, 0, 0, 133,
	 NULL}, // [156] $0788 -> Delay 50
	{OP_DISPLAY_MSG, 0x00, 0, 0, 0, 12, 0, 0, -1, NULL}, // [157] $078C
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1,
	 " THE MECHANOID DEMANDS"}, // [158] $0790
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1,
	 " THAT YOU PUT HIM DOWN"},							   // [159] $07AA
	{OP_DISPLAY_MSG, 0x00, 0, 0, 0, 50, 0, 0, -1, NULL},   // [160] $07C4
	{OP_CLR_FLAGS, 0x00, 0, 0, 0, 0, 0x0040, 0, -1, NULL}, // [161] $07C8
	{OP_RETURN, 0x00, 0, 0, 0, 0, 0, 0, -1, NULL},		   // [162] $07CC

	// Bank (Event 8)
	{OP_CALL, 0x00, 0x06E2, 0, 0, 0, 0, 0, 133,
	 NULL}, // [163] $07D0 -> Delay 50
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1, "         BANK"}, // [164] $07D4
	{OP_DISPLAY_MSG, 0x00, 0, 0, 0, 21, 0, 0, -1, NULL},	// [165] $07E4
	{OP_DISPLAY_MSG, 0x00, 0, 0, 0, 50, 0, 0, -1, NULL},	// [166] $07E8
	{OP_CLR_FLAGS, 0x00, 0, 0, 0, 0, 0x0040, 0, -1, NULL},	// [167] $07EC
	{OP_RETURN, 0x00, 0, 0, 0, 0, 0, 0, -1, NULL},			// [168] $07F0

	// Novabill (Event 36)
	{OP_CALL, 0x00, 0x0A60, 0, 0, 0, 0, 0, 233,
	 NULL}, // [169] $07F2 -> Good Show (Event 49)+5
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1,
	 " NOVABILL WELL DONE!"},							   // [170] $07F6
	{OP_DISPLAY_MSG, 0x00, 0, 0, 0, 50, 0, 0, -1, NULL},   // [171] $080E
	{OP_CLR_FLAGS, 0x00, 0, 0, 0, 0, 0x0040, 0, -1, NULL}, // [172] $0812
	{OP_RETURN, 0x00, 0, 0, 0, 0, 0, 0, -1, NULL},		   // [173] $0816

	// Authors Advert (Event 37)
	{OP_CALL, 0x00, 0x0A60, 0, 0, 0, 0, 0, 233,
	 NULL}, // [174] $0818 -> Good Show (Event 49)+5
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1,
	 "  THE AUTHORS ADVERT"}, // [175] $081C
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1,
	 "THINGS ARE GOING TO BE"}, // [176] $0834
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1,
	 "   TOUGH FROM NOW ON"},							   // [177] $084E
	{OP_SET_FLAGS, 0x00, 0, 0, 0, 0, 0x0008, 0, -1, NULL}, // [178] $0866
	{OP_GOTO, 0x00, 0x0A66, 0, 0, 0, 0, 0, 235,
	 NULL}, // [179] $086A -> Good Show (Event 49)+7

	// Sabins Cube (Event 38)
	{OP_CALL, 0x00, 0x0A60, 0, 0, 0, 0, 0, 233,
	 NULL}, // [180] $086E -> Good Show (Event 49)+5
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1, "      SABINS CUBE"}, // [181] $0872
	{OP_GOTO, 0x00, 0x0A74, 0, 0, 0, 0, 0, 239,
	 NULL}, // [182] $0886 -> Good Show (Event 49)+11

	// Walton Monument (Event 39)
	{OP_CALL, 0x00, 0x0A60, 0, 0, 0, 0, 0, 233,
	 NULL}, // [183] $088A -> Good Show (Event 49)+5
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1,
	 "  THE WALTON MONUMENT"}, // [184] $088E
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1,
	 "TO THE FAMOUS ARCHITECT"}, // [185] $08A6
	{OP_GOTO, 0x00, 0x0A66, 0, 0, 0, 0, 0, 235,
	 NULL}, // [186] $08C0 -> Good Show (Event 49)+7

	// St Stallards (Event 40)
	{OP_CALL, 0x00, 0x0A60, 0, 0, 0, 0, 0, 233,
	 NULL}, // [187] $08C4 -> Good Show (Event 49)+5
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1, "     ST STALLARDS"}, // [188] $08C8
	{OP_GOTO, 0x00, 0x0A66, 0, 0, 0, 0, 0, 235,
	 NULL}, // [189] $08DE -> Good Show (Event 49)+7

	// Moorby Arch (Event 41)
	{OP_CALL, 0x00, 0x0A60, 0, 0, 0, 0, 0, 233,
	 NULL}, // [190] $08E2 -> Good Show (Event 49)+5
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1, "    THE MOORBY ARCH"}, // [191] $08E6
	{OP_GOTO, 0x00, 0x0A74, 0, 0, 0, 0, 0, 239,
	 NULL}, // [192] $08FC -> Good Show (Event 49)+11

	// Tyler Point (Event 42)
	{OP_CALL, 0x00, 0x0A60, 0, 0, 0, 0, 0, 233,
	 NULL}, // [193] $0900 -> Good Show (Event 49)+5
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1, "      TYLER POINT"}, // [194] $0904
	{OP_GOTO, 0x00, 0x0A74, 0, 0, 0, 0, 0, 239,
	 NULL}, // [195] $0918 -> Good Show (Event 49)+11

	// Bosher Stadium (Event 43)
	{OP_CALL, 0x00, 0x0A60, 0, 0, 0, 0, 0, 233,
	 NULL}, // [196] $091C -> Good Show (Event 49)+5
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1, "    BOSHER STADIUM"}, // [197] $0920
	{OP_GOTO, 0x00, 0x0A66, 0, 0, 0, 0, 0, 235,
	 NULL}, // [198] $0936 -> Good Show (Event 49)+7

	// Jordan Airport (Event 44)
	{OP_CALL, 0x00, 0x0A60, 0, 0, 0, 0, 0, 233,
	 NULL}, // [199] $093A -> Good Show (Event 49)+5
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1, "    JORDAN AIRPORT"}, // [200] $093E
	{OP_GOTO, 0x00, 0x0A66, 0, 0, 0, 0, 0, 235,
	 NULL}, // [201] $0954 -> Good Show (Event 49)+7

	// Vector Henge (Event 45)
	{OP_CALL, 0x00, 0x0A60, 0, 0, 0, 0, 0, 233,
	 NULL}, // [202] $0958 -> Good Show (Event 49)+5
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1, "     VECTOR HENGE"}, // [203] $095C
	{OP_GOTO, 0x00, 0x0A74, 0, 0, 0, 0, 0, 239,
	 NULL}, // [204] $0970 -> Good Show (Event 49)+11

	// Brother In Laws House (Event 46)
	{OP_CALL, 0x00, 0x0A60, 0, 0, 0, 0, 0, 233,
	 NULL}, // [205] $0974 -> Good Show (Event 49)+5
	{OP_DISPLAY_MSG, 0x00, 0, 0, 0, 17, 0, 0, -1, NULL}, // [206] $0978
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1,
	 " BROTHER-IN-LAWS HOUSE"},							   // [207] $097C
	{OP_SET_FLAGS, 0x00, 0, 0, 0, 0, 0x0020, 0, -1, NULL}, // [208] $0996
	{OP_GOTO, 0x00, 0x0A66, 0, 0, 0, 0, 0, 235,
	 NULL}, // [209] $099A -> Good Show (Event 49)+7

	// Mechanoid Fort (Event 47)
	{OP_CALL, 0x00, 0x0A60, 0, 0, 0, 0, 0, 233,
	 NULL}, // [210] $099E -> Good Show (Event 49)+5
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1,
	 "  THE MECHANOID FORT"}, // [211] $09A2
	{OP_GOTO, 0x00, 0x0A74, 0, 0, 0, 0, 0, 239,
	 NULL}, // [212] $09BA -> Good Show (Event 49)+11

	// Coach And Horses (Event 48)
	{OP_CALL, 0x00, 0x0A60, 0, 0, 0, 0, 0, 233,
	 NULL}, // [213] $09BE -> Good Show (Event 49)+5
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1,
	 " THE COACH AND HORSES"}, // [214] $09C2
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1,
	 "YOU WILL NOT BEPOPULAR"}, // [215] $09DA
	{OP_GOTO, 0x00, 0x0A66, 0, 0, 0, 0, 0, 235,
	 NULL}, // [216] $09F4 -> Good Show (Event 49)+7

	// New Ship (Event 27)
	{OP_CALL, 0x00, 0x0A60, 0, 0, 0, 0, 0, 233,
	 NULL}, // [217] $09F8 -> Good Show (Event 49)+5
	{OP_DISPLAY_MSG, 0x00, 0, 0, 0, 17, 0, 0, -1, NULL},	  // [218] $09FC
	{OP_DISPLAY_MSG, 0x00, 0, 0, 0, 19, 0, 0, -1, NULL},	  // [219] $0A00
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1, "       NEW SHIP"}, // [220] $0A04
	{OP_SET_FLAGS, 0x00, 0, 0, 0, 0, 0x0020, 0, -1, NULL},	  // [221] $0A16
	{OP_GOTO, 0x00, 0x0A66, 0, 0, 0, 0, 0, 235,
	 NULL}, // [222] $0A1A -> Good Show (Event 49)+7

	// Traitor (Event 50)
	{OP_CALL, 0x00, 0x06E2, 0, 0, 0, 0, 0, 133,
	 NULL}, // [223] $0A1E -> Delay 50
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1, "       TRAITOR!"}, // [224] $0A22
	{OP_DISPLAY_MSG, 0x00, 0, 0, 0, 50, 0, 0, -1, NULL},	  // [225] $0A34
	{OP_CLR_FLAGS, 0x00, 0, 0, 0, 0, 0x0040, 0, -1, NULL},	  // [226] $0A38
	{OP_RETURN, 0x00, 0, 0, 0, 0, 0, 0, -1, NULL},			  // [227] $0A3C

	// Good Show (Event 49)
	{OP_CALL, 0x00, 0x06E2, 0, 0, 0, 0, 0, 133,
	 NULL}, // [228] $0A3E -> Delay 50
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1, "      GOOD SHOW!"}, // [229] $0A42
	{OP_DISPLAY_MSG, 0x00, 0, 0, 0, 50, 0, 0, -1, NULL},	   // [230] $0A56
	{OP_CLR_FLAGS, 0x00, 0, 0, 0, 0, 0x0040, 0, -1, NULL},	   // [231] $0A5A
	{OP_RETURN, 0x00, 0, 0, 0, 0, 0, 0, -1, NULL},			   // [232] $0A5E
	{OP_DISPLAY_MSG, 0x00, 0, 0, 0, 23, 0, 0, -1, NULL},	   // [233] $0A60
	{OP_RETURN, 0x00, 0, 0, 0, 0, 0, 0, -1, NULL},			   // [234] $0A64
	{OP_CALL, 0x00, 0x1420, 0, 0, 0, 0, 0, 492,
	 NULL}, // [235] $0A66 -> Palyar Ship Attack
	{OP_DISPLAY_MSG, 0x00, 0, 0, 0, 50, 0, 0, -1, NULL},   // [236] $0A6A
	{OP_CLR_FLAGS, 0x00, 0, 0, 0, 0, 0x0040, 0, -1, NULL}, // [237] $0A6E
	{OP_RETURN, 0x00, 0, 0, 0, 0, 0, 0, -1, NULL},		   // [238] $0A72
	{OP_CALL, 0x00, 0x1442, 0, 0, 0, 0, 0, 495,
	 NULL}, // [239] $0A74 -> Mechanoid Ship Attack
	{OP_DISPLAY_MSG, 0x00, 0, 0, 0, 50, 0, 0, -1, NULL},   // [240] $0A78
	{OP_CLR_FLAGS, 0x00, 0, 0, 0, 0, 0x0040, 0, -1, NULL}, // [241] $0A7C
	{OP_RETURN, 0x00, 0, 0, 0, 0, 0, 0, -1, NULL},		   // [242] $0A80

	// Random Attack 1 (Event 34)
	{OP_IF_RANDOM_GE, 0x80, 0, 0x8000, 0, 0, 0, 0, 235,
	 NULL}, // [243] $0A82 -> Good Show (Event 49)+7
	{OP_CLR_FLAGS, 0x00, 0, 0, 0, 0, 0x0040, 0, -1, NULL}, // [244] $0A88
	{OP_RETURN, 0x00, 0, 0, 0, 0, 0, 0, -1, NULL},		   // [245] $0A8C

	// Random Attack 2 (Event 35)
	{OP_IF_RANDOM_GE, 0x80, 0, 0x8000, 0, 0, 0, 0, 239,
	 NULL}, // [246] $0A8E -> Good Show (Event 49)+11
	{OP_CLR_FLAGS, 0x00, 0, 0, 0, 0, 0x0040, 0, -1, NULL}, // [247] $0A94
	{OP_RETURN, 0x00, 0, 0, 0, 0, 0, 0, -1, NULL},		   // [248] $0A98

	// Palyar Briefing (Event 28)
	{OP_CALL, 0x00, 0x06E2, 0, 0, 0, 0, 0, 133,
	 NULL}, // [249] $0A9A -> Delay 50
	{OP_SET_VAR_W, 0x00, 0, 0x0029, 0, 0x00FF, 0, 0, -1, NULL}, // [250] $0A9E
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1,
	 " PALYAR BRIEFING ROOM"},							 // [251] $0AA4
	{OP_DISPLAY_MSG, 0x00, 0, 0, 0, 50, 0, 0, -1, NULL}, // [252] $0ABC
	{OP_IF_IN_BLDG, 0x80, 0, 0, 0, 0, 0, 0, 272,
	 NULL}, // [253] $0AC0 -> Palyar Briefing (Event 28)+23
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1, "    WE WILL REWARD"}, // [254] $0AC4
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1,
	 " DELIVERY OF OUR NEEDS"}, // [255] $0ADA
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1,
	 "TO APPROPRIATE ROOMS IN"}, // [256] $0AF4
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1,
	 "THE PALYAR COLONY CRAFT"},						 // [257] $0B0E
	{OP_DISPLAY_MSG, 0x00, 0, 0, 0, 50, 0, 0, -1, NULL}, // [258] $0B28
	{OP_IF_IN_BLDG, 0x80, 0, 0, 0, 0, 0, 0, 272,
	 NULL}, // [259] $0B2C -> Palyar Briefing (Event 28)+23
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1,
	 "THERE IS A BIG FEE FOR"}, // [260] $0B30
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1,
	 " A CAPTURED MECHANOID"},							 // [261] $0B4A
	{OP_DISPLAY_MSG, 0x00, 0, 0, 0, 50, 0, 0, -1, NULL}, // [262] $0B62
	{OP_IF_IN_BLDG, 0x80, 0, 0, 0, 0, 0, 0, 272,
	 NULL}, // [263] $0B66 -> Palyar Briefing (Event 28)+23
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1,
	 "ONLY WHEN YOU KNOW HOW"}, // [264] $0B6A
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1,
	 "TO CHOOSE THEM FROM US"}, // [265] $0B84
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1,
	 " DESTROY ALL MECHANOID"}, // [266] $0B9E
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1,
	 "OCCUPIED LOCATIONS FOR"}, // [267] $0BB8
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1,
	 "VERY SPECIAL GRATITUDE"},									 // [268] $0BD2
	{OP_DISPLAY_MSG, 0x00, 0, 0, 0, 50, 0, 0, -1, NULL},		 // [269] $0BEC
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1, "     BRIEFING ENDS"}, // [270] $0BF0
	{OP_DISPLAY_MSG, 0x00, 0, 0, 0, 50, 0, 0, -1, NULL},		 // [271] $0C06
	{OP_CLR_FLAGS, 0x00, 0, 0, 0, 0, 0x0040, 0, -1, NULL},		 // [272] $0C0A
	{OP_RETURN, 0x00, 0, 0, 0, 0, 0, 0, -1, NULL},				 // [273] $0C0E

	// Mechanoid Briefing (Event 13)
	{OP_CALL, 0x00, 0x06E2, 0, 0, 0, 0, 0, 133,
	 NULL}, // [274] $0C10 -> Delay 50
	{OP_SET_VAR_W, 0x00, 0, 0x0066, 0, 0x00FF, 0, 0, -1, NULL}, // [275] $0C14
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1,
	 "MECHANOID BRIEFING ROOM"},								// [276] $0C1A
	{OP_DISPLAY_MSG, 0x00, 0, 0, 0, 50, 0, 0, -1, NULL},		// [277] $0C34
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1, "      WE WILL PAY"}, // [278] $0C38
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1,
	 "FOR PALYAR REQUIREMENTS"}, // [279] $0C4C
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1,
	 " WHEN DELIVERED TO OUR"}, // [280] $0C66
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1,
	 "CONTROL IN THIS COMPLEX"},						 // [281] $0C80
	{OP_DISPLAY_MSG, 0x00, 0, 0, 0, 50, 0, 0, -1, NULL}, // [282] $0C9A
	{OP_IF_IN_BLDG, 0x80, 0, 0, 0, 0, 0, 0, 290,
	 NULL}, // [283] $0C9E -> Mechanoid Briefing (Event 13)+16
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1,
	 "  WE MAY ALSO PAY FOR"}, // [284] $0CA2
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1,
	 "DESTRUCTION OF SELECTED"}, // [285] $0CBA
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1,
	 " PALYAR INSTALLATIONS"},									 // [286] $0CD4
	{OP_DISPLAY_MSG, 0x00, 0, 0, 0, 50, 0, 0, -1, NULL},		 // [287] $0CEC
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1, "     BRIEFING ENDS"}, // [288] $0CF0
	{OP_DISPLAY_MSG, 0x00, 0, 0, 0, 50, 0, 0, -1, NULL},		 // [289] $0D06
	{OP_CLR_FLAGS, 0x00, 0, 0, 0, 0, 0x0040, 0, -1, NULL},		 // [290] $0D0A
	{OP_RETURN, 0x00, 0, 0, 0, 0, 0, 0, -1, NULL},				 // [291] $0D0E

	// Comm Room (Event 2)
	{OP_CALL, 0x00, 0x06E2, 0, 0, 0, 0, 0, 133,
	 NULL}, // [292] $0D10 -> Delay 50
	{OP_DISPLAY_MSG, 0x00, 0, 0, 0, 8, 0, 0, -1, NULL}, // [293] $0D14
	{OP_SET_VAR_W, 0x00, 0, OBJ_ANTENNA, 0, 0x00FE, 0, 0, -1,
	 NULL},														// [294] $0D18
	{OP_SET_VAR_W, 0x00, 0, 0x0010, 0, 0x00FF, 0, 0, -1, NULL}, // [295] $0D1E
	{OP_IF_ENTITY_OWNER, 0x00, 0, 0, 0, 0, 0, 0, 302,
	 NULL}, // [296] $0D24 -> Hertz Rental
	{OP_IF_ENTITY_ALIVE, 0x00, 0, 0, 0, 0, 0, 0, 302,
	 NULL}, // [297] $0D28 -> Hertz Rental
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1, "      NOT WORKING"}, // [298] $0D2C
	{OP_DISPLAY_MSG, 0x00, 0, 0, 0, 50, 0, 0, -1, NULL},		// [299] $0D40
	{OP_CLR_FLAGS, 0x00, 0, 0, 0, 0, 0x0040, 0, -1, NULL},		// [300] $0D44
	{OP_RETURN, 0x00, 0, 0, 0, 0, 0, 0, -1, NULL},				// [301] $0D48

	// Hertz Rental
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1, "     MESSAGE FROM"}, // [302] $0D4A
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1,
	 "HERTZ SPACESHIP RENTAL"},									// [303] $0D5E
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1, "    SHIP FOR HIRE"}, // [304] $0D78
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1, "      999999 CR"},	// [305] $0D8C
	{OP_ENABLE_INPUT, 0x00, 0, 0, 0, 0, 0, 0, -1, NULL},		// [306] $0D9E
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1,
	 " DO YOU WANT TO HIRE?"}, // [307] $0DA0
	{OP_CALL, 0x00, 0x0710, 0, 0, 0, 0, 0, 141,
	 NULL}, // [308] $0DB8 -> Y/N Prompt+2
	{OP_IF_FLAGS, 0x00, 0, 0, 0, 0, 0x0001, 0, 314,
	 NULL}, // [309] $0DBC -> Hertz Rental+12
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1,
	 " OK PLEASE CALL AGAIN"},							   // [310] $0DC2
	{OP_DISPLAY_MSG, 0x00, 0, 0, 0, 7, 0, 0, -1, NULL},	   // [311] $0DDA
	{OP_CLR_FLAGS, 0x00, 0, 0, 0, 0, 0x0040, 0, -1, NULL}, // [312] $0DDE
	{OP_RETURN, 0x00, 0, 0, 0, 0, 0, 0, -1, NULL},		   // [313] $0DE2
	{OP_IF_VAR_GE, 0x80, 0, 0, 0x01000000, 0x00F8, 0, 0, 320,
	 NULL}, // [314] $0DE4 -> Hertz Rental+18
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1,
	 "  INSUFFICIENT FUNDS"},							   // [315] $0DEE
	{OP_DISPLAY_MSG, 0x00, 0, 0, 0, 21, 0, 0, -1, NULL},   // [316] $0E06
	{OP_DISPLAY_MSG, 0x00, 0, 0, 0, 7, 0, 0, -1, NULL},	   // [317] $0E0A
	{OP_CLR_FLAGS, 0x00, 0, 0, 0, 0, 0x0040, 0, -1, NULL}, // [318] $0E0E
	{OP_RETURN, 0x00, 0, 0, 0, 0, 0, 0, -1, NULL},		   // [319] $0E12
	{OP_ADD_VAR_BCD, 0x00, 0, 0, 0x99000001, 0x00F8, 0, 0, -1,
	 NULL},														  // [320] $0E14
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1, "    SHIP DESPATCHED"}, // [321] $0E1C
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1, "     IT WILL LAND"},	  // [322] $0E32
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1,
	 "AT LOC 08-08 IN 2 MINS"}, // [323] $0E46
	{OP_SET_E4_WORD, 0x00, SVAR_SHIP_HIRE_FLAG, 0xFFFF, 0, 0, 0, 0, -1,
	 NULL},												   // [324] $0E60
	{OP_SET_FLAGS, 0x00, 0, 0, 0, 0, 0x0004, 0, -1, NULL}, // [325] $0E66
	{OP_CLR_FLAGS, 0x00, 0, 0, 0, 0, 0x0040, 0, -1, NULL}, // [326] $0E6A
	{OP_RETURN, 0x00, 0, 0, 0, 0, 0, 0, -1, NULL},		   // [327] $0E6E

	// Room Power (Event 14)
	{OP_SET_VAR_W, 0x00, 0, OBJ_ENERGY_CRYSTAL, 0, 0x00FE, 0, 0, -1,
	 NULL},														// [328] $0E70
	{OP_SET_VAR_W, 0x00, 0, 0x00AD, 0, 0x00FF, 0, 0, -1, NULL}, // [329] $0E76
	{OP_SET_VAR_L, 0x00, 0, 0, 0x00100000, 0x00F9, 0, 0, -1,
	 NULL},													   // [330] $0E7C
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1, "      POWER ROOM"}, // [331] $0E84
	{OP_GOTO, 0x00, 0x1162, 0, 0, 0, 0, 0, 413,
	 NULL}, // [332] $0E98 -> Room Reward (Event 11)+5

	// Room Stores (Event 15)
	{OP_SET_VAR_W, 0x00, 0, OBJ_LARGE_BOX, 0, 0x00FE, 0, 0, -1,
	 NULL},														// [333] $0E9C
	{OP_SET_VAR_W, 0x00, 0, 0x0093, 0, 0x00FF, 0, 0, -1, NULL}, // [334] $0EA2
	{OP_SET_VAR_L, 0x00, 0, 0, 0x00035000, 0x00F9, 0, 0, -1,
	 NULL},													 // [335] $0EA8
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1, "        STORES"}, // [336] $0EB0
	{OP_GOTO, 0x00, 0x1162, 0, 0, 0, 0, 0, 413,
	 NULL}, // [337] $0EC2 -> Room Reward (Event 11)+5

	// Room Armoury (Event 16)
	{OP_SET_VAR_W, 0x00, 0, OBJ_USEFUL_ARMAMENT, 0, 0x00FE, 0, 0, -1,
	 NULL},														// [338] $0EC6
	{OP_SET_VAR_W, 0x00, 0, 0x0092, 0, 0x00FF, 0, 0, -1, NULL}, // [339] $0ECC
	{OP_SET_VAR_L, 0x00, 0, 0, 0x00065000, 0x00F9, 0, 0, -1,
	 NULL},													  // [340] $0ED2
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1, "        ARMOURY"}, // [341] $0EDA
	{OP_GOTO, 0x00, 0x1162, 0, 0, 0, 0, 0, 413,
	 NULL}, // [342] $0EEC -> Room Reward (Event 11)+5

	// Room Laboratory (Event 17)
	{OP_SET_VAR_W, 0x00, 0, OBJ_WINCHESTER, 0, 0x00FE, 0, 0, -1,
	 NULL},														// [343] $0EF0
	{OP_SET_VAR_W, 0x00, 0, 0x0090, 0, 0x00FF, 0, 0, -1, NULL}, // [344] $0EF6
	{OP_SET_VAR_L, 0x00, 0, 0, 0x00100000, 0x00F9, 0, 0, -1,
	 NULL},													   // [345] $0EFC
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1, "      LABORATORY"}, // [346] $0F04
	{OP_GOTO, 0x00, 0x1162, 0, 0, 0, 0, 0, 413,
	 NULL}, // [347] $0F18 -> Room Reward (Event 11)+5

	// Room Control (Event 18)
	{OP_SET_VAR_W, 0x00, 0, OBJ_DATABANK, 0, 0x00FE, 0, 0, -1,
	 NULL},														// [348] $0F1C
	{OP_SET_VAR_W, 0x00, 0, 0x00AA, 0, 0x00FF, 0, 0, -1, NULL}, // [349] $0F22
	{OP_SET_VAR_L, 0x00, 0, 0, 0x00100000, 0x00F9, 0, 0, -1,
	 NULL},														// [350] $0F28
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1, "     CONTROL ROOM"}, // [351] $0F30
	{OP_GOTO, 0x00, 0x1162, 0, 0, 0, 0, 0, 413,
	 NULL}, // [352] $0F44 -> Room Reward (Event 11)+5

	// Room Interview (Event 19)
	{OP_SET_VAR_W, 0x00, 0, OBJ_MECHANOID, 0, 0x00FE, 0, 0, -1,
	 NULL},														// [353] $0F48
	{OP_SET_VAR_W, 0x00, 0, 0x009A, 0, 0x00FF, 0, 0, -1, NULL}, // [354] $0F4E
	{OP_SET_VAR_L, 0x00, 0, 0, 0x00250000, 0x00F9, 0, 0, -1,
	 NULL},														 // [355] $0F54
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1, "    INTERVIEW ROOM"}, // [356] $0F5C
	{OP_GOTO, 0x00, 0x1162, 0, 0, 0, 0, 0, 413,
	 NULL}, // [357] $0F72 -> Room Reward (Event 11)+5

	// Room Mech Fuel (Event 20)
	{OP_SET_VAR_W, 0x00, 0, OBJ_NEUTRON_FUEL, 0, 0x00FE, 0, 0, -1,
	 NULL},														// [358] $0F76
	{OP_SET_VAR_W, 0x00, 0, 0x0046, 0, 0x00FF, 0, 0, -1, NULL}, // [359] $0F7C
	{OP_SET_VAR_L, 0x00, 0, 0, 0x00250000, 0x00F9, 0, 0, -1,
	 NULL}, // [360] $0F82
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1,
	 " MECHANOID FUEL STORES"}, // [361] $0F8A
	{OP_GOTO, 0x00, 0x11DC, 0, 0, 0, 0, 0, 430,
	 NULL}, // [362] $0FA4 -> Room Reward (Event 11)+22

	// Room Mech Power (Event 22)
	{OP_SET_VAR_W, 0x00, 0, OBJ_ENERGY_CRYSTAL, 0, 0x00FE, 0, 0, -1,
	 NULL},														// [363] $0FA8
	{OP_SET_VAR_W, 0x00, 0, 0x0032, 0, 0x00FF, 0, 0, -1, NULL}, // [364] $0FAE
	{OP_SET_VAR_L, 0x00, 0, 0, 0x00100000, 0x00F9, 0, 0, -1,
	 NULL}, // [365] $0FB4
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1,
	 " MECHANOID POWER ROOM"}, // [366] $0FBC
	{OP_GOTO, 0x00, 0x11DC, 0, 0, 0, 0, 0, 430,
	 NULL}, // [367] $0FD4 -> Room Reward (Event 11)+22

	// Room Mech Stores (Event 23)
	{OP_SET_VAR_W, 0x00, 0, OBJ_LARGE_BOX, 0, 0x00FE, 0, 0, -1,
	 NULL},														// [368] $0FD8
	{OP_SET_VAR_W, 0x00, 0, 0x0030, 0, 0x00FF, 0, 0, -1, NULL}, // [369] $0FDE
	{OP_SET_VAR_L, 0x00, 0, 0, 0x00080000, 0x00F9, 0, 0, -1,
	 NULL},														  // [370] $0FE4
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1, "   MECHANOID STORES"}, // [371] $0FEC
	{OP_GOTO, 0x00, 0x11DC, 0, 0, 0, 0, 0, 430,
	 NULL}, // [372] $1002 -> Room Reward (Event 11)+22

	// Room Mech Armoury (Event 24)
	{OP_SET_VAR_W, 0x00, 0, OBJ_USEFUL_ARMAMENT, 0, 0x00FE, 0, 0, -1,
	 NULL},														// [373] $1006
	{OP_SET_VAR_W, 0x00, 0, 0x0022, 0, 0x00FF, 0, 0, -1, NULL}, // [374] $100C
	{OP_SET_VAR_L, 0x00, 0, 0, 0x00120000, 0x00F9, 0, 0, -1,
	 NULL}, // [375] $1012
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1,
	 "   MECHANOID ARMOURY"}, // [376] $101A
	{OP_GOTO, 0x00, 0x11DC, 0, 0, 0, 0, 0, 430,
	 NULL}, // [377] $1032 -> Room Reward (Event 11)+22

	// Room Mech Lab (Event 25)
	{OP_SET_VAR_W, 0x00, 0, OBJ_WINCHESTER, 0, 0x00FE, 0, 0, -1,
	 NULL},														// [378] $1036
	{OP_SET_VAR_W, 0x00, 0, 0x0012, 0, 0x00FF, 0, 0, -1, NULL}, // [379] $103C
	{OP_SET_VAR_L, 0x00, 0, 0, 0x00250000, 0x00F9, 0, 0, -1,
	 NULL}, // [380] $1042
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1,
	 " MECHANOID LABORATORY"}, // [381] $104A
	{OP_GOTO, 0x00, 0x11DC, 0, 0, 0, 0, 0, 430,
	 NULL}, // [382] $1062 -> Room Reward (Event 11)+22

	// Room Engine (Event 3)
	{OP_SET_VAR_W, 0x00, 0, OBJ_NEUTRON_FUEL, 0, 0x00FE, 0, 0, -1,
	 NULL},														// [383] $1066
	{OP_SET_VAR_W, 0x00, 0, 0x0088, 0, 0x00FF, 0, 0, -1, NULL}, // [384] $106C
	{OP_SET_VAR_L, 0x00, 0, 0, 0x00200000, 0x00F9, 0, 0, -1,
	 NULL},														// [385] $1072
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1, "      ENGINE ROOM"}, // [386] $107A
	{OP_GOTO, 0x00, 0x1162, 0, 0, 0, 0, 0, 413,
	 NULL}, // [387] $108E -> Room Reward (Event 11)+5

	// Room Conference (Event 4)
	{OP_SET_VAR_W, 0x00, 0, OBJ_ESSENTIAL_SUPPLY, 0, 0x00FE, 0, 0, -1,
	 NULL},														// [388] $1092
	{OP_SET_VAR_W, 0x00, 0, 0x009B, 0, 0x00FF, 0, 0, -1, NULL}, // [389] $1098
	{OP_SET_VAR_L, 0x00, 0, 0, 0x00050000, 0x00F9, 0, 0, -1,
	 NULL},														  // [390] $109E
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1, "    CONFERENCE ROOM"}, // [391] $10A6
	{OP_GOTO, 0x00, 0x1162, 0, 0, 0, 0, 0, 413,
	 NULL}, // [392] $10BC -> Room Reward (Event 11)+5

	// Room Exchequer (Event 5)
	{OP_SET_VAR_W, 0x00, 0, OBJ_GOLD, 0, 0x00FE, 0, 0, -1, NULL}, // [393] $10C0
	{OP_SET_VAR_W, 0x00, 0, 0x00A2, 0, 0x00FF, 0, 0, -1, NULL},	  // [394] $10C6
	{OP_SET_VAR_L, 0x00, 0, 0, 0x00100000, 0x00F9, 0, 0, -1,
	 NULL},													   // [395] $10CC
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1, "       EXCHEQUER"}, // [396] $10D4
	{OP_GOTO, 0x00, 0x1162, 0, 0, 0, 0, 0, 413,
	 NULL}, // [397] $10E8 -> Room Reward (Event 11)+5

	// Room Kitchen (Event 6)
	{OP_SET_VAR_W, 0x00, 0, OBJ_CATERING, 0, 0x00FE, 0, 0, -1,
	 NULL},														// [398] $10EC
	{OP_SET_VAR_W, 0x00, 0, 0x00A3, 0, 0x00FF, 0, 0, -1, NULL}, // [399] $10F2
	{OP_SET_VAR_L, 0x00, 0, 0, 0x00060000, 0x00F9, 0, 0, -1,
	 NULL},													  // [400] $10F8
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1, "        KITCHEN"}, // [401] $1100
	{OP_GOTO, 0x00, 0x1162, 0, 0, 0, 0, 0, 413,
	 NULL}, // [402] $1112 -> Room Reward (Event 11)+5

	// Room Infirmary (Event 9)
	{OP_SET_VAR_W, 0x00, 0, OBJ_MEDICAL_SUPPLIES, 0, 0x00FE, 0, 0, -1,
	 NULL},														// [403] $1116
	{OP_SET_VAR_W, 0x00, 0, 0x00A5, 0, 0x00FF, 0, 0, -1, NULL}, // [404] $111C
	{OP_SET_VAR_L, 0x00, 0, 0, 0x00040000, 0x00F9, 0, 0, -1,
	 NULL},													   // [405] $1122
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1, "       INFIRMARY"}, // [406] $112A
	{OP_GOTO, 0x00, 0x1162, 0, 0, 0, 0, 0, 413,
	 NULL}, // [407] $113E -> Room Reward (Event 11)+5

	// Room Reward (Event 11)
	{OP_CALL, 0x00, 0x06E2, 0, 0, 0, 0, 0, 133,
	 NULL}, // [408] $1142 -> Delay 50
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1, "        PRISON"}, // [409] $1146
	{OP_DISPLAY_MSG, 0x00, 0, 0, 0, 50, 0, 0, -1, NULL},	 // [410] $1158
	{OP_CLR_FLAGS, 0x00, 0, 0, 0, 0, 0x0040, 0, -1, NULL},	 // [411] $115C
	{OP_RETURN, 0x00, 0, 0, 0, 0, 0, 0, -1, NULL},			 // [412] $1160
	{OP_IF_ENTITY_FLAG, 0x00, 0, 0x0001, 0, 0, 0, 0, 427,
	 NULL}, // [413] $1162 -> Room Reward (Event 11)+19
	{OP_IF_ENTITY_ALIVE, 0x00, 0, 0, 0, 0, 0, 0, 417,
	 NULL}, // [414] $1168 -> Room Reward (Event 11)+9
	{OP_IF_ENTITY_OWNER, 0x00, 0, 0, 0, 0, 0, 0, 417,
	 NULL}, // [415] $116C -> Room Reward (Event 11)+9
	{OP_GOTO, 0x00, 0x11D2, 0, 0, 0, 0, 0, 427,
	 NULL}, // [416] $1170 -> Room Reward (Event 11)+19
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1, "     PLEASE LEAVE"},	 // [417] $1174
	{OP_DISPLAY_MSG, 0x00, 0, 0, 0, 254, 0, 0, -1, NULL},		 // [418] $1188
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1, " HERE FOR b REWARD"}, // [419] $118C
	{OP_IF_ENTITY_OWNER, 0x00, 0, 0, 0, 0, 0, 0, 423,
	 NULL}, // [420] $11A2 -> Room Reward (Event 11)+15
	{OP_IF_IN_BLDG, 0x00, 0, 0, 0, 0, 0, 0, 420,
	 NULL}, // [421] $11A6 -> Room Reward (Event 11)+12
	{OP_GOTO, 0x00, 0x11D2, 0, 0, 0, 0, 0, 427,
	 NULL}, // [422] $11AA -> Room Reward (Event 11)+19
	{OP_SET_ENTITY_FLAG, 0x00, 0, 0x0011, 0, 0, 0, 0, -1, NULL}, // [423] $11AE
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1, "  b CREDITS PAID"},	 // [424] $11B2
	{OP_BCD_OP3, 0x00, 0, 0x00F8, 0, 0x00F8, 0x00F9, 0, -1,
	 NULL},												   // [425] $11C6
	{OP_DISPLAY_MSG, 0x00, 0, 0, 0, 21, 0, 0, -1, NULL},   // [426] $11CE
	{OP_DISPLAY_MSG, 0x00, 0, 0, 0, 50, 0, 0, -1, NULL},   // [427] $11D2
	{OP_CLR_FLAGS, 0x00, 0, 0, 0, 0, 0x0040, 0, -1, NULL}, // [428] $11D6
	{OP_RETURN, 0x00, 0, 0, 0, 0, 0, 0, -1, NULL},		   // [429] $11DA
	{OP_COPY_WORD_VAR, 0x00, 0, 0x00FE, 0, 0x00F8, 0, 0, -1,
	 NULL}, // [430] $11DC
	{OP_COPY_WORD_VAR, 0x00, 0, 0x00FF, 0, 0x00F9, 0, 0, -1,
	 NULL}, // [431] $11E2
	{OP_SET_VAR_W, 0x00, 0, OBJ_MECHANOID, 0, 0x00FE, 0, 0, -1,
	 NULL},														// [432] $11E8
	{OP_SET_VAR_W, 0x00, 0, 0x0066, 0, 0x00FF, 0, 0, -1, NULL}, // [433] $11EE
	{OP_IF_ENTITY_OWNER, 0x80, 0, 0, 0, 0, 0, 0, 438,
	 NULL}, // [434] $11F4 -> Room Reward (Event 11)+30
	{OP_COPY_WORD_VAR, 0x00, 0, 0x00F8, 0, 0x00FE, 0, 0, -1,
	 NULL}, // [435] $11F8
	{OP_COPY_WORD_VAR, 0x00, 0, 0x00F9, 0, 0x00FF, 0, 0, -1,
	 NULL}, // [436] $11FE
	{OP_GOTO, 0x00, 0x1162, 0, 0, 0, 0, 0, 413,
	 NULL}, // [437] $1204 -> Room Reward (Event 11)+5
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1,
	 "  THE MECHANOIDS SAY"}, // [438] $1208
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1,
	 "  PUT BACK OUR LEADER"},							   // [439] $1220
	{OP_DISPLAY_MSG, 0x00, 0, 0, 0, 50, 0, 0, -1, NULL},   // [440] $1238
	{OP_CLR_FLAGS, 0x00, 0, 0, 0, 0, 0x0040, 0, -1, NULL}, // [441] $123C
	{OP_RETURN, 0x00, 0, 0, 0, 0, 0, 0, -1, NULL},		   // [442] $1240

	// Interstellar Ship (Event 7)
	{OP_SET_VAR_W, 0x00, 0, 0x0000, 0, 0x00FF, 0, 0, -1, NULL}, // [443] $1242
	{OP_SET_VAR_W, 0x00, 0, OBJ_NOVADRIVE, 0, 0x00FE, 0, 0, -1,
	 NULL}, // [444] $1248
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1,
	 "   INTERSTELLAR SHIP"}, // [445] $124E
	{OP_IF_HAS_ITEM, 0x80, 0, OBJ_INTERSTELLAR_CRAFT, 0, 0, 0, 0, 459,
	 NULL}, // [446] $1266 -> Launch Prompt+4
	{OP_IF_IN_BLDG, 0x80, 0, 0, 0, 0, 0, 0, 446,
	 NULL}, // [447] $126C -> Interstellar Ship (Event 7)+3
	{OP_CALL, 0x00, 0x06E2, 0, 0, 0, 0, 0, 133,
	 NULL}, // [448] $1270 -> Delay 50
	{OP_IF_FLAGS, 0x00, 0, 0, 0, 0, 0x0004, 0, 455,
	 NULL}, // [449] $1274 -> Launch Prompt
	{OP_IF_ENTITY_ALIVE, 0x00, 0, 0, 0, 0, 0, 0, 455,
	 NULL}, // [450] $127A -> Launch Prompt
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1,
	 "  NOVADRIVE REQUIRED"},							   // [451] $127E
	{OP_DISPLAY_MSG, 0x00, 0, 0, 0, 50, 0, 0, -1, NULL},   // [452] $1296
	{OP_CLR_FLAGS, 0x00, 0, 0, 0, 0, 0x0040, 0, -1, NULL}, // [453] $129A
	{OP_RETURN, 0x00, 0, 0, 0, 0, 0, 0, -1, NULL},		   // [454] $129E

	// Launch Prompt
	{OP_ENABLE_INPUT, 0x00, 0, 0, 0, 0, 0, 0, -1, NULL}, // [455] $12A0
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1,
	 "   PRESS Y TO LAUNCH"}, // [456] $12A2
	{OP_IF_VEHICLE, 0x00, 0, 0x0015, 0, 0, 0, 0, 462,
	 NULL}, // [457] $12BA -> Launch Prompt+7
	{OP_IF_HAS_ITEM, 0x00, 0, OBJ_INTERSTELLAR_CRAFT, 0, 0, 0, 0, 457,
	 NULL}, // [458] $12C0 -> Launch Prompt+2
	{OP_DISPLAY_MSG, 0x00, 0, 0, 0, 50, 0, 0, -1, NULL},   // [459] $12C6
	{OP_CLR_FLAGS, 0x00, 0, 0, 0, 0, 0x0040, 0, -1, NULL}, // [460] $12CA
	{OP_RETURN, 0x00, 0, 0, 0, 0, 0, 0, -1, NULL},		   // [461] $12CE
	{OP_IF_IN_BLDG, 0x80, 0, 0, 0, 0, 0, 0, 446,
	 NULL}, // [462] $12D0 -> Interstellar Ship (Event 7)+3
	{OP_IF_FLAGS, 0x00, 0, 0, 0, 0, 0x0004, 0, 465,
	 NULL}, // [463] $12D4 -> Launch Prompt+10
	{OP_IF_ENTITY_ALIVE, 0x80, 0, 0, 0, 0, 0, 0, 446,
	 NULL}, // [464] $12DA -> Interstellar Ship (Event 7)+3
	{OP_SET_VAR_W, 0x00, 0, OBJ_UNKNOWN, 0, 0x00FD, 0, 0, -1,
	 NULL}, // [465] $12DE
	{OP_IF_ENTITY2_ALIVE, 0x80, 0, 0, 0, 0, 0, 0, 473,
	 NULL}, // [466] $12E4 -> Congratulations
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1,
	 "  THE AUTHOR WON'T LET"}, // [467] $12E8
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1,
	 "  YOU LEAVE UNTIL YOU"},									 // [468] $1302
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1, "    FIX HIS ADVERT"}, // [469] $131A
	{OP_DISPLAY_MSG, 0x00, 0, 0, 0, 50, 0, 0, -1, NULL},		 // [470] $1330
	{OP_CLR_FLAGS, 0x00, 0, 0, 0, 0, 0x0040, 0, -1, NULL},		 // [471] $1334
	{OP_RETURN, 0x00, 0, 0, 0, 0, 0, 0, -1, NULL},				 // [472] $1338

	// Congratulations
	{OP_SET_E4_WORD, 0x00, SVAR_ENDGAME_FLAG, 0x0001, 0, 0, 0, 0, -1,
	 NULL}, // [473] $133A
	{OP_CALL, 0x00, 0x06CE, 0, 0, 0, 0, 0, 130,
	 NULL}, // [474] $1340 -> Delay 100
	{OP_SET_E4_WORD, 0x00, SVAR_TEXT_SPEED, 0x0096, 0, 0, 0, 0, -1,
	 NULL}, // [475] $1344
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1,
	 "  THE AUTHOR SENDS YOU"}, // [476] $134A
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1,
	 "CONGRATULATIONS ON YOUR"}, // [477] $1364
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1,
	 "    ESCAPE FROM TARG"},							 // [478] $137E
	{OP_DISPLAY_MSG, 0x00, 0, 0, 0, 50, 0, 0, -1, NULL}, // [479] $1396
	{OP_IF_FLAGS, 0x80, 0, 0, 0, 0, 0x0020, 0, 485,
	 NULL}, // [480] $139A -> Congratulations+12
	{OP_DISPLAY_MSG, 0x00, 0, 0, 0, 17, 0, 0, -1, NULL}, // [481] $13A0
	{OP_DISPLAY_MSG, 0x00, 0, 0, 0, 18, 0, 0, -1, NULL}, // [482] $13A4
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1,
	 "IS PLEASED YOU'VE GONE"},							 // [483] $13A8
	{OP_DISPLAY_MSG, 0x00, 0, 0, 0, 50, 0, 0, -1, NULL}, // [484] $13C2
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1,
	 "SAVE OUT THE STATUS AND"}, // [485] $13C6
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1,
	 "SEE YOU IN MERCENARY II"}, // [486] $13E0
	{OP_CALL, 0x00, 0x06EC, 0, 0, 0, 0, 0, 136,
	 NULL}, // [487] $13FA -> Delay 750

	// Game Over
	{OP_CALL, 0x00, 0x06EC, 0, 0, 0, 0, 0, 136,
	 NULL}, // [488] $13FE -> Delay 750
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1, "       GAME OVER"}, // [489] $1402
	{OP_SET_E4_WORD, 0x00, SVAR_ENDGAME_FLAG, 0xFFFF, 0, 0, 0, 0, -1,
	 NULL}, // [490] $1416
	{OP_GOTO, 0x00, 0x13FE, 0, 0, 0, 0, 0, 488,
	 NULL}, // [491] $141C -> Game Over

	// Palyar Ship Attack
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1,
	 " PALYAR SHIP ATTACKING"}, // [492] $1420
	{OP_SET_E4_WORD, 0x00, SVAR_ATTACK_FLAG, 0x0001, 0, 0, 0, 0, -1,
	 NULL},										   // [493] $143A
	{OP_RETURN, 0x00, 0, 0, 0, 0, 0, 0, -1, NULL}, // [494] $1440

	// Mechanoid Ship Attack
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1,
	 " MECHANOID SHIP ATTACK"}, // [495] $1442
	{OP_SET_E4_WORD, 0x00, SVAR_ATTACK_FLAG, 0x0001, 0, 0, 0, 0, -1,
	 NULL},										   // [496] $145C
	{OP_RETURN, 0x00, 0, 0, 0, 0, 0, 0, -1, NULL}, // [497] $1462
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1,
	 "  WE ARE MOST PLEASED"}, // [498] $1464
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1,
	 "YOU HAVE DESTROYED ALL"}, // [499] $147C
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1,
	 " KEY OPPOSITION SITES"},							   // [500] $1496
	{OP_SET_FLAGS, 0x00, 0, 0, 0, 0, 0x0002, 0, -1, NULL}, // [501] $14AE
	{OP_RETURN, 0x00, 0, 0, 0, 0, 0, 0, -1, NULL},		   // [502] $14B2

	// Event Handler 26
	{OP_CALL, 0x00, 0x06E2, 0, 0, 0, 0, 0, 133,
	 NULL}, // [503] $14B4 -> Delay 50
	{OP_IF_FLAGS, 0x00, 0, 0, 0, 0, 0x0002, 0, 509,
	 NULL}, // [504] $14B8 -> Event Handler 26+6
	{OP_DISPLAY_MSG, 0x00, 0, 0, 0, 4, 0, 0, -1, NULL}, // [505] $14BE
	{OP_CALL, 0x00, 0x1464, 0, 0, 0, 0, 0, 498,
	 NULL}, // [506] $14C2 -> Mechanoid Ship Attack+3
	{OP_DISPLAY_MSG, 0x00, 0, 0, 0, 7, 0, 0, -1, NULL},	   // [507] $14C6
	{OP_DISPLAY_MSG, 0x00, 0, 0, 0, 50, 0, 0, -1, NULL},   // [508] $14CA
	{OP_CLR_FLAGS, 0x00, 0, 0, 0, 0, 0x0040, 0, -1, NULL}, // [509] $14CE
	{OP_RETURN, 0x00, 0, 0, 0, 0, 0, 0, -1, NULL},		   // [510] $14D2

	// Event Handler 10
	{OP_CALL, 0x00, 0x06E2, 0, 0, 0, 0, 0, 133,
	 NULL}, // [511] $14D4 -> Delay 50
	{OP_IF_FLAGS, 0x00, 0, 0, 0, 0, 0x0002, 0, 523,
	 NULL}, // [512] $14D8 -> Event Handler 10+12
	{OP_DISPLAY_MSG, 0x00, 0, 0, 0, 3, 0, 0, -1, NULL}, // [513] $14DE
	{OP_CALL, 0x00, 0x1464, 0, 0, 0, 0, 0, 498,
	 NULL}, // [514] $14E2 -> Mechanoid Ship Attack+3
	{OP_SET_VAR_L, 0x00, 0, 0, 0x00500000, 0x00F9, 0, 0, -1,
	 NULL},														  // [515] $14E6
	{OP_SET_VAR_W, 0x00, 0, OBJ_PASS, 0, 0x00FE, 0, 0, -1, NULL}, // [516] $14EE
	{OP_SET_VAR_W, 0x00, 0, 0x0050, 0, 0x00FF, 0, 0, -1, NULL},	  // [517] $14F4
	{OP_IF_ENTITY_OWNER, 0x80, 0, 0, 0, 0, 0, 0, 424,
	 NULL}, // [518] $14FA -> Room Reward (Event 11)+16
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1,
	 "YOU WILL FIND A REWARD"}, // [519] $14FE
	{OP_TEXT, 0x00, 0, 0, 0, 0, 0, 0, -1,
	 " INSIDE THE RED HANGAR"}, // [520] $1518
	{OP_SET_ENTITY_LOC, 0x00, 0, 0x0001, 0, OBJ_PASS, 0, 0, -1,
	 NULL},												   // [521] $1532
	{OP_DISPLAY_MSG, 0x00, 0, 0, 0, 7, 0, 0, -1, NULL},	   // [522] $1538
	{OP_CLR_FLAGS, 0x00, 0, 0, 0, 0, 0x0040, 0, -1, NULL}, // [523] $153C
	{OP_RETURN, 0x00, 0, 0, 0, 0, 0, 0, -1, NULL},		   // [524] $1540
};

const int EVENT_ENTRY[EVENT_COUNT] = {
	35,	 107, 292, 383, 388, 393, 398, 443, 163, 403, 511, 408, 156,
	274, 328, 333, 338, 343, 348, 353, 358, 32,	 363, 368, 373, 378,
	503, 217, 249, 163, 32,	 32,  32,  32,	243, 246, 169, 174, 180,
	183, 187, 190, 193, 196, 199, 202, 205, 210, 213, 228, 223,
};

const char *MESSAGES[MESSAGE_COUNT] = {
	"        LOCKED          ",
	" IT'S VERY DARK IN HERE ",
	"       TOO HEAVY        ",
	"  MESSAGE FROM PALYARS  ",
	" MESSAGE FROM MECHANOIDS",
	"  - - - JOB OFFER - - - ",
	"        CHEESE          ",
	"      MESSAGE ENDS      ",
	"   COMMUNICATION ROOM   ",
	"        HANGAR          ",
	"         KITCHEN        ",
	"       ENGINE ROOM      ",
	"        MECHANOID       ",
	"  PALYAR BRIEFING ROOM  ",
	"     CONFERENCE ROOM    ",
	"  THE PALYAR COMMANDERS  BROTHER-IN-LAWS ROOM  ",
	"    PHOTON EMITTER      ",
	"  THE PALYAR COMMANDERS ",
	"     BROTHER-IN-LAW     ",
	"    BROTHER-IN-LAW'S    ",
	" YOU HAVE a CREDITS",
	" YOU HAVE a CREDITS",
	"        EXCHEQUER       ",
	" YOU HAVE JUST DESTROYED",
	"    ANTI TIME BOMB      ",
	"       NOVADRIVE        ",
	"    METAL DETECTOR      ",
	"       ANTIGRAV         ",
	"       POWERAMP         ",
	"     NEUTRON FUEL       ",
	"        ANTENNA         ",
	"    ENERGY CRYSTAL      ",
	"        COFFIN          ",
	"       LARGE BOX        ",
	"       LARGE BOX        ",
	"     USEFUL ARMAMENT    ",
	"        INFIRMARY       ",
	"         GOLD           ",
	"         GOLD           ",
	"        SIGHTS          ",
	"        SIGHTS          ",
	"   MEDICAL SUPPLIES     ",
	" ESSENTIAL 12939 SUPPLY ",
	"      WINCHESTER        ",
	"  CATERING PROVISIONS   ",
	"       DATABANK         ",
	"         PASS           ",
	"     KITCHEN SINK       ",
	"    SHIP DESTROYED      ",
	" ENEMY SHIP DESTROYED   ",
	"                       ",
	"      DISK ERROR       ",
	"PRESS RETURN WHEN READY",
	"PRESS RETURN WHEN READY",
	"PRESS RETURN WHEN READY",
	"PRESS RETURN WHEN READY",
	"PRESS RETURN WHEN READY",
	"   PASS HOLDERS ONLY    ",
	"    LOAD NUMBER 0-9    ",
	"    SAVE NUMBER 0-9    ",
	"       WHERE AM I       ",
	"      YOU CRASHED       ",
	"      GOOD LANDING!     ",
	"          OUCH!         ",
};

const ScriptStateInit STATE_INIT = {
	0x0000, 0x0096, 0x0000, 0x0000, 0x0000,
};

} // namespace gen_e4
