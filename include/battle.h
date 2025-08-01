#ifndef GUARD_BATTLE_H
#define GUARD_BATTLE_H

// should they be included here or included individually by every file?
#include "constants/battle.h"
#include "constants/form_change_types.h"
#include "battle_main.h"
#include "battle_message.h"
#include "battle_util.h"
#include "battle_script_commands.h"
#include "battle_ai_switch_items.h"
#include "battle_gfx_sfx_util.h"
#include "battle_util2.h"
#include "battle_bg.h"
#include "pokeball.h"
#include "battle_debug.h"
#include "battle_dynamax.h"
#include "battle_terastal.h"
#include "battle_gimmick.h"
#include "generational_changes.h"
#include "move.h"
#include "random.h" // for rng_value_t
#include "trainer_slide.h"

// Helper for accessing command arguments and advancing gBattlescriptCurrInstr.
//
// For example accuracycheck is defined as:
//
//     .macro accuracycheck failInstr:req, move:req
//     .byte 0x1
//     .4byte \failInstr
//     .2byte \move
//     .endm
//
// Which corresponds to:
//
//     CMD_ARGS(const u8 *failInstr, u16 move);
//
// The arguments can be accessed as cmd->failInstr and cmd->move.
// gBattlescriptCurrInstr = cmd->nextInstr; advances to the next instruction.
#define CMD_ARGS(...) const struct __attribute__((packed)) { u8 opcode; RECURSIVELY(R_FOR_EACH(APPEND_SEMICOLON, __VA_ARGS__)) const u8 nextInstr[0]; } *const cmd UNUSED = (const void *)gBattlescriptCurrInstr
#define VARIOUS_ARGS(...) CMD_ARGS(u8 battler, u8 id, ##__VA_ARGS__)
#define NATIVE_ARGS(...) CMD_ARGS(void (*func)(void), ##__VA_ARGS__)

// Used to exclude moves learned temporarily by Transform or Mimic
#define MOVE_IS_PERMANENT(battler, moveSlot)                        \
   (!(gBattleMons[battler].status2 & STATUS2_TRANSFORMED)           \
 && !(gDisableStructs[battler].mimickedMoves & (1u << moveSlot)))

// Battle Actions
// These determine what each battler will do in a turn
#define B_ACTION_USE_MOVE               0
#define B_ACTION_USE_ITEM               1
#define B_ACTION_SWITCH                 2
#define B_ACTION_RUN                    3
#define B_ACTION_SAFARI_WATCH_CAREFULLY 4
#define B_ACTION_SAFARI_BALL            5
#define B_ACTION_SAFARI_POKEBLOCK       6
#define B_ACTION_SAFARI_GO_NEAR         7
#define B_ACTION_SAFARI_RUN             8
#define B_ACTION_WALLY_THROW            9
#define B_ACTION_EXEC_SCRIPT            10
#define B_ACTION_TRY_FINISH             11
#define B_ACTION_FINISHED               12
#define B_ACTION_CANCEL_PARTNER         12 // when choosing an action
#define B_ACTION_NOTHING_FAINTED        13 // when choosing an action
#define B_ACTION_DEBUG                  20
#define B_ACTION_THROW_BALL             21 // R to throw last used ball
#define B_ACTION_NONE                   0xFF

#define BATTLE_BUFFER_LINK_SIZE 0x1000

// Cleared each time a mon leaves the field, either by switching out or fainting
struct DisableStruct
{
    u32 transformedMonPersonality;
    bool8 transformedMonShininess;
    u16 disabledMove;
    u16 encoredMove;
    u8 protectUses:4;
    u8 stockpileCounter:4;
    s8 stockpileDef;
    s8 stockpileSpDef;
    s8 stockpileBeforeDef;
    s8 stockpileBeforeSpDef;
    u8 substituteHP;
    u8 encoredMovePos;
    u16 disableTimer;
    u16 encoreTimer;
    u16 perishSongTimer;
    u16 rolloutTimer;
    u16 rolloutTimerStartValue;
    u16 tauntTimer;
    u8 furyCutterCounter;
    u8 battlerPreventingEscape;
    u8 battlerWithSureHit;
    u8 isFirstTurn;
    u8 mimickedMoves:4;
    u8 chargeTimer:4;
    u8 rechargeTimer;
    u8 autotomizeCount;
    u16 slowStartTimer;
    u16 embargoTimer;
    u16 magnetRiseTimer;
    u16 telekinesisTimer;
    u16 healBlockTimer;
    u16 laserFocusTimer;
    u16 throatChopTimer;
    u8 wrapTurns;
    u16 syrupBombTimer;
    u16 tormentTimer; // used for G-Max Meltdown
    u8 usedMoves:4;
    u8 truantCounter:1;
    u8 truantSwitchInHack:1;
    u8 noRetreat:1;
    u8 tarShot:1;
    u8 octolock:1;
    u8 cudChew:1;
    u8 spikesDone:1;
    u8 toxicSpikesDone:1;
    u8 stickyWebDone:1;
    u8 stealthRockDone:1;
    u8 weatherAbilityDone:1;
    u8 terrainAbilityDone:1;
    u8 syrupBombIsShiny:1;
    u8 steelSurgeDone:1;
    u8 usedProteanLibero:1;
    u8 flashFireBoosted:1;
    u16 overwrittenAbility;   // abilities overwritten during battle (keep separate from battle history in case of switching)
    u8 boosterEnergyActivates:1;
    u8 roostActive:1;
    u8 unburdenActive:1;
    u8 neutralizingGas:1;
    u8 iceFaceActivationPrevention:1; // fixes hit escape move edge case
    u8 padding:3;
};

// Fully Cleared each turn after end turn effects are done. A few things are cleared before end turn effects
struct ProtectStruct
{
    u32 protected:7; // 126 protect options
    u32 endured:1;
    u32 noValidMoves:1;
    u32 helpingHand:1;
    u32 bounceMove:1;
    u32 stealMove:1;
    u32 nonVolatileStatusImmobility:1;
    u32 confusionSelfDmg:1;
    u32 chargingTurn:1;
    u32 fleeType:2; // 0: Normal, 1: FLEE_ITEM, 2: FLEE_ABILITY
    u32 unableToUseMove:1; // Not to be confused with HITMARKER_UNABLE_TO_USE_MOVE (It is questionable though if there is a difference. Needs further research)
    u32 notFirstStrike:1;
    u32 palaceUnableToUseMove:1;
    u32 powderSelfDmg:1;
    u32 statRaised:1;
    u32 usedCustapBerry:1;    // also quick claw
    u32 touchedProtectLike:1;
    u32 unused:8;
    // End of 32-bit bitfield
    u16 disableEjectPack:1;
    u16 statFell:1;
    u16 pranksterElevated:1;
    u16 quickDraw:1;
    u16 beakBlastCharge:1;
    u16 quash:1;
    u16 shellTrap:1;
    u16 eatMirrorHerb:1;
    u16 activateOpportunist:2; // 2 - to copy stats. 1 - stats copied (do not repeat). 0 - no stats to copy
    u16 usedAllySwitch:1;
    u16 lashOutAffected:1;
    u16 padding:4;
    // End of 16-bit bitfield
    u16 physicalDmg;
    u16 specialDmg;
    u8 physicalBattlerId;
    u8 specialBattlerId;
};

// Cleared at the start of HandleAction_ActionFinished
struct SpecialStatus
{
    s32 physicalDmg;
    s32 specialDmg;
    u8 changedStatsBattlerId; // Battler that was responsible for the latest stat change. Can be self.
    u8 statLowered:1;
    u8 lightningRodRedirected:1;
    u8 restoredBattlerSprite: 1;
    u8 faintedHasReplacement:1;
    u8 preventLifeOrbDamage:1; // So that Life Orb doesn't activate various effects.
    u8 afterYou:1;
    u8 enduredDamage:1;
    u8 stormDrainRedirected:1;
    // End of byte
    u8 switchInAbilityDone:1;
    u8 switchInItemDone:1;
    u8 instructedChosenTarget:3;
    u8 berryReduced:1;
    u8 announceNeutralizingGas:1;   // See Cmd_switchineffects
    u8 neutralizingGasRemoved:1;    // See VARIOUS_TRY_END_NEUTRALIZING_GAS
    // End of byte
    u8 gemParam;
    // End of byte
    u8 gemBoost:1;
    u8 rototillerAffected:1;  // to be affected by rototiller
    u8 parentalBondState:2;
    u8 multiHitOn:1;
    u8 distortedTypeMatchups:1;
    u8 teraShellAbilityDone:1;
    u8 criticalHit:1;
    // End of byte
    u8 dancerUsedMove:1;
    u8 dancerOriginalTarget:3;
    u8 unused:4;
    // End of byte
};

struct SideTimer
{
    u16 reflectTimer;
    u8 reflectBattlerId;
    u16 lightscreenTimer;
    u8 lightscreenBattlerId;
    u16 mistTimer;
    u8 mistBattlerId;
    u16 safeguardTimer;
    u8 safeguardBattlerId;
    u16 spikesAmount; // debug menu complains. might be better to solve there instead if possible
    u16 toxicSpikesAmount;
    u16 stealthRockAmount;
    u16 stickyWebAmount;
    u8 stickyWebBattlerId;
    u8 stickyWebBattlerSide; // Used for Court Change
    u16 auroraVeilTimer;
    u8 auroraVeilBattlerId;
    u16 tailwindTimer;
    u8 tailwindBattlerId;
    u16 luckyChantTimer;
    u8 luckyChantBattlerId;
    u16 steelsurgeAmount;
    // Timers below this point are not swapped by Court Change
    u16 followmeTimer;
    u8 followmeTarget:3;
    u8 followmePowder:1; // Rage powder, does not affect grass type pokemon.
    u16 retaliateTimer;
    u16 damageNonTypesTimer;
    u8 damageNonTypesType;
    u16 rainbowTimer;
    u16 seaOfFireTimer;
    u16 swampTimer;
};

struct FieldTimer
{
    u16 mudSportTimer;
    u16 waterSportTimer;
    u16 wonderRoomTimer;
    u16 magicRoomTimer;
    u16 trickRoomTimer;
    u16 terrainTimer;
    u16 gravityTimer;
    u16 fairyLockTimer;
};

struct WishFutureKnock
{
    u16 futureSightCounter[MAX_BATTLERS_COUNT];
    u8 futureSightBattlerIndex[MAX_BATTLERS_COUNT];
    u8 futureSightPartyIndex[MAX_BATTLERS_COUNT];
    u16 futureSightMove[MAX_BATTLERS_COUNT];
    u16 wishCounter[MAX_BATTLERS_COUNT];
    u8 wishPartyId[MAX_BATTLERS_COUNT];
    u8 weatherDuration;
    u8 knockedOffMons[NUM_BATTLE_SIDES]; // Each battler is represented by a bit.
};

struct AI_SavedBattleMon
{
    u16 ability;
    u16 moves[MAX_MON_MOVES];
    u16 heldItem;
    u16 species:15;
    u16 saved:1;
    u8 types[3];
};

struct AiPartyMon
{
    u16 species;
    u16 item;
    u16 heldEffect;
    u16 ability;
    u16 gender;
    u16 level;
    u16 moves[MAX_MON_MOVES];
    u32 status;
    bool8 isFainted;
    bool8 wasSentInBattle;
    u8 switchInCount; // Counts how many times this Pokemon has been sent out or switched into in a battle.
};

struct AIPartyData // Opposing battlers - party mons.
{
    struct AiPartyMon mons[NUM_BATTLE_SIDES][PARTY_SIZE]; // 2 parties(player, opponent). Used to save information on opposing party.
    u8 count[NUM_BATTLE_SIDES];
};

struct SwitchinCandidate
{
    struct BattlePokemon battleMon;
    bool8 hypotheticalStatus;
};

struct SimulatedDamage
{
    u16 minimum;
    u16 median;
    u16 maximum;
};

// Ai Data used when deciding which move to use, computed only once before each turn's start.
struct AiLogicData
{
    u16 abilities[MAX_BATTLERS_COUNT];
    u16 items[MAX_BATTLERS_COUNT];
    u16 holdEffects[MAX_BATTLERS_COUNT];
    u8 holdEffectParams[MAX_BATTLERS_COUNT];
    u16 lastUsedMove[MAX_BATTLERS_COUNT];
    u8 hpPercents[MAX_BATTLERS_COUNT];
    u16 partnerMove;
    u16 speedStats[MAX_BATTLERS_COUNT]; // Speed stats for all battles, calculated only once, same way as damages
    struct SimulatedDamage simulatedDmg[MAX_BATTLERS_COUNT][MAX_BATTLERS_COUNT][MAX_MON_MOVES]; // attacker, target, moveIndex
    uq4_12_t effectiveness[MAX_BATTLERS_COUNT][MAX_BATTLERS_COUNT][MAX_MON_MOVES]; // attacker, target, moveIndex
    u8 moveAccuracy[MAX_BATTLERS_COUNT][MAX_BATTLERS_COUNT][MAX_MON_MOVES]; // attacker, target, moveIndex
    u8 moveLimitations[MAX_BATTLERS_COUNT];
    u8 monToSwitchInId[MAX_BATTLERS_COUNT]; // ID of the mon to switch in.
    u8 mostSuitableMonId[MAX_BATTLERS_COUNT]; // Stores result of GetMostSuitableMonToSwitchInto, which decides which generic mon the AI would switch into if they decide to switch. This can be overruled by specific mons found in ShouldSwitch; the final resulting mon is stored in AI_monToSwitchIntoId.
    struct SwitchinCandidate switchinCandidate; // Struct used for deciding which mon to switch to in battle_ai_switch_items.c
    u8 weatherHasEffect:1; // The same as HasWeatherEffect(). Stored here, so it's called only once.
    u8 ejectButtonSwitch:1; // Tracks whether current switch out was from Eject Button
    u8 ejectPackSwitch:1; // Tracks whether current switch out was from Eject Pack
    u8 predictingSwitch:1; // Determines whether AI will use predictions this turn or not
    u8 aiSwitchPredictionInProgress:1; // Tracks whether the AI is in the middle of running prediction calculations
    u8 padding:3;
    u8 shouldSwitch; // Stores result of ShouldSwitch, which decides whether a mon should be switched out
    u8 aiCalcInProgress:1;
    u8 battlerDoingPrediction; // Stores which battler is currently running its prediction calcs
};

struct AI_ThinkingStruct
{
    u8 aiState;
    u8 movesetIndex;
    u16 moveConsidered;
    s32 score[MAX_MON_MOVES];
    u32 funcResult;
    u32 aiFlags[MAX_BATTLERS_COUNT];
    u8 aiAction;
    u8 aiLogicId;
    struct AI_SavedBattleMon saved[MAX_BATTLERS_COUNT];
};

#define AI_MOVE_HISTORY_COUNT 3

struct BattleHistory
{
    u16 abilities[MAX_BATTLERS_COUNT];
    u8 itemEffects[MAX_BATTLERS_COUNT];
    u16 usedMoves[MAX_BATTLERS_COUNT][MAX_MON_MOVES];
    u16 moveHistory[MAX_BATTLERS_COUNT][AI_MOVE_HISTORY_COUNT]; // 3 last used moves for each battler
    u8 moveHistoryIndex[MAX_BATTLERS_COUNT];
    u16 trainerItems[MAX_BATTLERS_COUNT];
    u8 itemsNo;
    u16 heldItems[MAX_BATTLERS_COUNT];
};

struct BattleScriptsStack
{
    const u8 *ptr[8];
    u8 size;
};

struct BattleCallbacksStack
{
    void (*function[8])(void);
    u8 size;
};

struct StatsArray
{
    u16 stats[NUM_STATS];
    u16 level;
};

struct BattleResources
{
    struct SecretBase* secretBase;
    struct BattleScriptsStack* battleScriptsStack;
    struct BattleCallbacksStack* battleCallbackStack;
    struct StatsArray* beforeLvlUp;
    struct AI_ThinkingStruct *ai;
    struct AiLogicData *aiData;
    struct AIPartyData *aiParty;
    struct BattleHistory *battleHistory;
    u8 bufferA[MAX_BATTLERS_COUNT][0x200];
    u8 bufferB[MAX_BATTLERS_COUNT][0x200];
    u8 transferBuffer[0x100];
};

#define AI_THINKING_STRUCT ((struct AI_ThinkingStruct *)(gBattleResources->ai))
#define AI_DATA ((struct AiLogicData *)(gBattleResources->aiData))
#define AI_PARTY ((struct AIPartyData *)(gBattleResources->aiParty))
#define BATTLE_HISTORY ((struct BattleHistory *)(gBattleResources->battleHistory))

struct BattleResults
{
    u8 playerFaintCounter;    // 0x0
    u8 opponentFaintCounter;  // 0x1
    u8 playerSwitchesCounter; // 0x2
    u8 numHealingItemsUsed;   // 0x3
    u8 numRevivesUsed;        // 0x4
    u8 playerMonWasDamaged:1; // 0x5
    u8 caughtMonBall:4;       // 0x5
    u8 shinyWildMon:1;        // 0x5
    u16 playerMon1Species;    // 0x6
    u8 playerMon1Name[POKEMON_NAME_LENGTH + 1];    // 0x8
    u8 battleTurnCounter;     // 0x13
    u8 playerMon2Name[POKEMON_NAME_LENGTH + 1];    // 0x14
    u8 pokeblockThrows;       // 0x1F
    u16 lastOpponentSpecies;  // 0x20
    u16 lastUsedMovePlayer;   // 0x22
    u16 lastUsedMoveOpponent; // 0x24
    u16 playerMon2Species;    // 0x26
    u16 caughtMonSpecies;     // 0x28
    u8 caughtMonNick[POKEMON_NAME_LENGTH + 1];     // 0x2A
    u8 filler35;           // 0x35
    u8 catchAttempts[POKEBALL_COUNT];     // 0x36
};

struct BattleTv_Side
{
    u32 spikesMonId:3;
    u32 reflectMonId:3;
    u32 lightScreenMonId:3;
    u32 safeguardMonId:3;
    u32 mistMonId:3;
    u32 futureSightMonId:3;
    u32 doomDesireMonId:3;
    u32 perishSongMonId:3;
    u32 wishMonId:3;
    u32 grudgeMonId:3;
    u32 usedMoveSlot:2;
    u32 spikesMoveSlot:2;
    u32 reflectMoveSlot:2;
    u32 lightScreenMoveSlot:2;
    u32 safeguardMoveSlot:2;
    u32 mistMoveSlot:2;
    u32 futureSightMoveSlot:2;
    u32 doomDesireMoveSlot:2;
    u32 perishSongMoveSlot:2;
    u32 wishMoveSlot:2;
    u32 grudgeMoveSlot:2;
    u32 destinyBondMonId:3;
    u32 destinyBondMoveSlot:2;
    u32 faintCause:4;
    u32 faintCauseMonId:3;
    u32 explosion:1;
    u32 explosionMoveSlot:2;
    u32 explosionMonId:3;
    u32 perishSong:1;
};

struct BattleTv_Position
{
    u32 curseMonId:3;
    u32 leechSeedMonId:3;
    u32 nightmareMonId:3;
    u32 wrapMonId:3;
    u32 attractMonId:3;
    u32 confusionMonId:3;
    u32 curseMoveSlot:2;
    u32 leechSeedMoveSlot:2;
    u32 nightmareMoveSlot:2;
    u32 wrapMoveSlot:2;
    u32 attractMoveSlot:2;
    u32 confusionMoveSlot:2;
    u32 waterSportMoveSlot:2;
    u32 waterSportMonId:3;
    u32 mudSportMonId:3;
    u32 mudSportMoveSlot:2;
    u32 ingrainMonId:3;
    u32 ingrainMoveSlot:2;
    u32 attackedByMonId:3;
    u32 attackedByMoveSlot:2;
};

struct BattleTv_Mon
{
    u32 psnMonId:3;
    u32 badPsnMonId:3;
    u32 brnMonId:3;
    u32 prlzMonId:3;
    u32 slpMonId:3;
    u32 frzMonId:3;
    u32 psnMoveSlot:2;
    u32 badPsnMoveSlot:2;
    u32 brnMoveSlot:2;
    u32 prlzMoveSlot:2;
    u32 slpMoveSlot:2;
    u32 frzMoveSlot:2;
};

struct BattleTv
{
    struct BattleTv_Mon mon[NUM_BATTLE_SIDES][PARTY_SIZE];
    struct BattleTv_Position pos[NUM_BATTLE_SIDES][2]; // [side][flank]
    struct BattleTv_Side side[NUM_BATTLE_SIDES];
};

struct BattleTvMovePoints
{
    s16 points[2][PARTY_SIZE * 4];
};

struct LinkBattlerHeader
{
    u8 versionSignatureLo;
    u8 versionSignatureHi;
    u8 vsScreenHealthFlagsLo;
    u8 vsScreenHealthFlagsHi;
    struct BattleEnigmaBerry battleEnigmaBerry;
};

struct Illusion
{
    u8 on;
    u8 set;
    u8 broken;
    u8 partyId;
    struct Pokemon *mon;
};

struct ZMoveData
{
    u8 viable:1;   // current move can become a z move
    u8 viewing:1;  // if player is viewing the z move name instead of regular moves
    u8 healReplacement:6;
    u8 possibleZMoves[MAX_BATTLERS_COUNT];
    u16 baseMoves[MAX_BATTLERS_COUNT];
};

struct DynamaxData
{
    u16 dynamaxTurns[MAX_BATTLERS_COUNT];
    u16 baseMoves[MAX_BATTLERS_COUNT]; // base move of Max Move
    u16 lastUsedBaseMove;
};

struct BattleGimmickData
{
    u8 usableGimmick[MAX_BATTLERS_COUNT];                // first usable gimmick that can be selected for each battler
    bool8 playerSelect;                                  // used to toggle trigger and update battle UI
    u8 triggerSpriteId;
    u8 indicatorSpriteId[MAX_BATTLERS_COUNT];
    u8 toActivate;                                       // stores whether a battler should transform at start of turn as bitfield
    u8 activeGimmick[NUM_BATTLE_SIDES][PARTY_SIZE];      // stores the active gimmick for each party member
    bool8 activated[MAX_BATTLERS_COUNT][GIMMICKS_COUNT]; // stores whether a trainer has used gimmick
};

struct LostItem
{
    u16 originalItem:15;
    u16 stolen:1;
};

struct BattleVideo {
    u32 battleTypeFlags;
    rng_value_t rngSeed;
};

enum BattleIntroStates
{
    BATTLE_INTRO_STATE_GET_MON_DATA,
    BATTLE_INTRO_STATE_LOOP_BATTLER_DATA,
    BATTLE_INTRO_STATE_PREPARE_BG_SLIDE,
    BATTLE_INTRO_STATE_WAIT_FOR_BG_SLIDE,
    BATTLE_INTRO_STATE_DRAW_SPRITES,
    BATTLE_INTRO_STATE_DRAW_PARTY_SUMMARY,
    BATTLE_INTRO_STATE_WAIT_FOR_PARTY_SUMMARY,
    BATTLE_INTRO_STATE_INTRO_TEXT,
    BATTLE_INTRO_STATE_WAIT_FOR_INTRO_TEXT,
    BATTLE_INTRO_STATE_TRAINER_SEND_OUT_TEXT,
    BATTLE_INTRO_STATE_WAIT_FOR_TRAINER_SEND_OUT_TEXT,
    BATTLE_INTRO_STATE_TRAINER_1_SEND_OUT_ANIM,
    BATTLE_INTRO_STATE_TRAINER_2_SEND_OUT_ANIM,
    BATTLE_INTRO_STATE_WAIT_FOR_TRAINER_2_SEND_OUT_ANIM,
    BATTLE_INTRO_STATE_WAIT_FOR_WILD_BATTLE_TEXT,
    BATTLE_INTRO_STATE_PRINT_PLAYER_SEND_OUT_TEXT,
    BATTLE_INTRO_STATE_WAIT_FOR_PLAYER_SEND_OUT_TEXT,
    BATTLE_INTRO_STATE_PRINT_PLAYER_1_SEND_OUT_TEXT,
    BATTLE_INTRO_STATE_PRINT_PLAYER_2_SEND_OUT_TEXT,
    BATTLE_INTRO_STATE_SET_DEX_AND_BATTLE_VARS
};

struct BattlerState
{
    u8 targetsDone[MAX_BATTLERS_COUNT];

    u32 commandingDondozo:1;
    u32 absent:1;
    u32 focusPunchBattlers:1;
    u32 multipleSwitchInBattlers:1;
    u32 alreadyStatusedMoveAttempt:1; // For example when using Thunder Wave on an already paralyzed Pokémon.
    u32 activeAbilityPopUps:1;
    u32 lastMoveFailed:1; // For Stomping Tantrum
    u32 forcedSwitch:1;
    u32 storedHealingWish:1;
    u32 storedLunarDance:1;
    u32 usedEjectItem:1;
    u32 sleepClauseEffectExempt:1; // Stores whether effect should be exempt from triggering Sleep Clause (Effect Spore)
    u32 usedMicleBerry:1;
    u32 pursuitTarget:1;
    u32 padding:17;
    // End of Word
};

// Cleared at the beginning of the battle. Fields need to be cleared when needed manually otherwise.
struct BattleStruct
{
    struct BattlerState battlerState[MAX_BATTLERS_COUNT];
    u8 eventBlockCounter;
    u8 turnEffectsBattlerId;
    u8 endTurnEventsCounter;
    u16 wrappedMove[MAX_BATTLERS_COUNT];
    u16 moveTarget[MAX_BATTLERS_COUNT];
    u32 expShareExpValue;
    u32 expValue;
    u8 expGettersOrder[PARTY_SIZE]; // First battlers which were sent out, then via exp-share
    u8 expGetterMonId;
    u8 expOrderId:3;
    u8 expGetterBattlerId:2;
    u8 teamGotExpMsgPrinted:1; // The 'Rest of your team got msg' has been printed.
    u8 givenExpMons; // Bits for enemy party's pokemon that gave exp to player's party.
    u8 expSentInMons; // As bits for player party mons - not including exp share mons.
    u8 wildVictorySong;
    u8 dynamicMoveType;
    u8 wrappedBy[MAX_BATTLERS_COUNT];
    u8 battlerPreventingSwitchout;
    u8 moneyMultiplier:6;
    u8 moneyMultiplierItem:1;
    u8 moneyMultiplierMove:1;
    u8 savedTurnActionNumber;
    u8 eventsBeforeFirstTurnState;
    u8 faintedActionsState;
    u8 faintedActionsBattlerId;
    u8 scriptPartyIdx; // for printing the nickname
    bool8 selectionScriptFinished[MAX_BATTLERS_COUNT];
    u8 battlerPartyIndexes[MAX_BATTLERS_COUNT];
    u8 monToSwitchIntoId[MAX_BATTLERS_COUNT];
    u8 battlerPartyOrders[MAX_BATTLERS_COUNT][PARTY_SIZE / 2];
    u8 runTries;
    u8 caughtMonNick[POKEMON_NAME_LENGTH + 1];
    u8 safariGoNearCounter;
    u8 safariPkblThrowCounter;
    u8 safariEscapeFactor;
    u8 safariCatchFactor;
    u8 linkBattleVsSpriteId_V; // The letter "V"
    u8 linkBattleVsSpriteId_S; // The letter "S"
    u8 chosenMovePositions[MAX_BATTLERS_COUNT];
    u8 stateIdAfterSelScript[MAX_BATTLERS_COUNT];
    u8 prevSelectedPartySlot;
    u8 stringMoveType;
    u8 palaceFlags; // First 4 bits are "is <= 50% HP and not asleep" for each battler, last 4 bits are selected moves to pass to AI
    u8 field_93; // related to choosing pokemon?
    u8 wallyBattleState;
    u8 wallyMovesState;
    u8 wallyWaitFrames;
    u8 wallyMoveFrames;
    u16 lastTakenMove[MAX_BATTLERS_COUNT]; // Last move that a battler was hit with.
    u16 hpOnSwitchout[NUM_BATTLE_SIDES];
    u32 savedBattleTypeFlags;
    u16 abilityPreventingSwitchout;
    u8 hpScale;
    u16 synchronizeMoveEffect;
    u8 anyMonHasTransformed:1; // Only used in battle_tv.c
    u8 multipleSwitchInState:2;
    u8 multipleSwitchInCursor:3;
    u8 padding1:2;
    u8 multipleSwitchInSortedBattlers[MAX_BATTLERS_COUNT];
    void (*savedCallback)(void);
    u16 usedHeldItems[PARTY_SIZE][NUM_BATTLE_SIDES]; // For each party member and side. For harvest, recycle
    u16 chosenItem[MAX_BATTLERS_COUNT];
    u16 choicedMove[MAX_BATTLERS_COUNT];
    u16 changedItems[MAX_BATTLERS_COUNT];
    u8 canPickupItem;
    u8 switchInBattlerCounter;
    u8 arenaTurnCounter;
    u8 turnSideTracker;
    u16 lastTakenMoveFrom[MAX_BATTLERS_COUNT][MAX_BATTLERS_COUNT]; // a 2-D array [target][attacker]
    union {
        struct LinkBattlerHeader linkBattlerHeader;
        struct BattleVideo battleVideo;
    } multiBuffer;
    u8 startingStatus:6; // status to apply at battle start. defined in constants/battle.h
    u8 startingStatusDone:1;
    u8 terrainDone:1;
    u8 overworldWeatherDone:1;
    u8 unused:3;
    u8 isAtkCancelerForCalledMove:1; // Certain cases in atk canceler should only be checked once, when the original move is called, however others need to be checked the twice.
    u8 friskedAbility:1; // If identifies two mons, show the ability pop-up only once.
    u8 fickleBeamBoosted:1;
    u8 poisonPuppeteerConfusion:1;
    u16 startingStatusTimer;
    u8 atkCancellerTracker;
    struct BattleTvMovePoints tvMovePoints;
    struct BattleTv tv;
    u8 AI_monToSwitchIntoId[MAX_BATTLERS_COUNT];
    s8 arenaMindPoints[2];
    s8 arenaSkillPoints[2];
    u16 arenaStartHp[2];
    u8 arenaLostPlayerMons; // Bits for party member, lost as in referee's decision, not by fainting.
    u8 arenaLostOpponentMons;
    u8 debugBattler;
    u8 magnitudeBasePower;
    u8 presentBasePower;
    u8 roostTypes[MAX_BATTLERS_COUNT][2];
    u8 savedBattlerTarget[5];
    u8 savedBattlerAttacker[5];
    u8 savedTargetCount:4;
    u8 savedAttackerCount:4;
    bool8 ateBoost[MAX_BATTLERS_COUNT];
    u8 abilityPopUpSpriteIds[MAX_BATTLERS_COUNT][2];    // two per battler
    struct ZMoveData zmove;
    struct DynamaxData dynamax;
    struct BattleGimmickData gimmick;
    const u8 *trainerSlideMsg;
    enum BattleIntroStates introState:8;
    u8 ateBerry[2]; // array id determined by side, each party pokemon as bit
    u8 stolenStats[NUM_BATTLE_STATS]; // hp byte is used for which stats to raise, other inform about by how many stages
    u8 lastMoveTarget[MAX_BATTLERS_COUNT]; // The last target on which each mon used a move, for the sake of Instruct
    u16 tracedAbility[MAX_BATTLERS_COUNT];
    u16 hpBefore[MAX_BATTLERS_COUNT]; // Hp of battlers before using a move. For Berserk and Anger Shell.
    struct Illusion illusion[MAX_BATTLERS_COUNT];
    u8 soulheartBattlerId;
    u8 friskedBattler; // Frisk needs to identify 2 battlers in double battles.
    u8 sameMoveTurns[MAX_BATTLERS_COUNT]; // For Metronome, number of times the same moves has been SUCCESFULLY used.
    u16 moveEffect2; // For Knock Off
    u16 changedSpecies[NUM_BATTLE_SIDES][PARTY_SIZE]; // For forms when multiple mons can change into the same pokemon.
    u8 quickClawBattlerId;
    struct LostItem itemLost[NUM_BATTLE_SIDES][PARTY_SIZE];  // Pokemon that had items consumed or stolen (two bytes per party member per side)
    u8 blunderPolicy:1; // should blunder policy activate
    u8 swapDamageCategory:1; // Photon Geyser, Shell Side Arm, Light That Burns the Sky
    u8 bouncedMoveIsUsed:1;
    u8 snatchedMoveIsUsed:1;
    u8 descriptionSubmenu:1; // For Move Description window in move selection screen
    u8 ackBallUseBtn:1; // Used for the last used ball feature
    u8 ballSwapped:1; // Used for the last used ball feature
    u8 throwingPokeBall:1;
    u8 ballSpriteIds[2];    // item gfx, window gfx
    u8 moveInfoSpriteId; // move info, window gfx
    u8 appearedInBattle; // Bitfield to track which Pokemon appeared in battle. Used for Burmy's form change
    u8 skyDropTargets[MAX_BATTLERS_COUNT]; // For Sky Drop, to account for if multiple Pokemon use Sky Drop in a double battle.
    // When using a move which hits multiple opponents which is then bounced by a target, we need to make sure, the move hits both opponents, the one with bounce, and the one without.
    u8 attackerBeforeBounce:2;
    u8 beatUpSlot:3;
    u8 pledgeMove:1;
    u8 effectsBeforeUsingMoveDone:1; // Mega Evo and Focus Punch/Shell Trap effects.
    u8 spriteIgnore0Hp:1;
    u8 battleBondBoost[NUM_BATTLE_SIDES]; // Bitfield for each party.
    u8 bonusCritStages[MAX_BATTLERS_COUNT]; // G-Max Chi Strike boosts crit stages of allies.
    u8 itemPartyIndex[MAX_BATTLERS_COUNT];
    u8 itemMoveIndex[MAX_BATTLERS_COUNT];
    u8 isSkyBattle:1;
    s32 aiDelayTimer; // Counts number of frames AI takes to choose an action.
    s32 aiDelayFrames; // Number of frames it took to choose an action.
    s32 aiDelayCycles; // Number of cycles it took to choose an action.
    u8 timesGotHit[NUM_BATTLE_SIDES][PARTY_SIZE];
    u8 transformZeroToHero[NUM_BATTLE_SIDES];
    u8 stickySyrupdBy[MAX_BATTLERS_COUNT];
    u8 intrepidSwordBoost[NUM_BATTLE_SIDES];
    u8 dauntlessShieldBoost[NUM_BATTLE_SIDES];
    u8 supersweetSyrup[NUM_BATTLE_SIDES];
    u8 supremeOverlordCounter[MAX_BATTLERS_COUNT];
    u8 shellSideArmCategory[MAX_BATTLERS_COUNT][MAX_BATTLERS_COUNT];
    u8 speedTieBreaks; // MAX_BATTLERS_COUNT! values.
    u8 categoryOverride; // for Z-Moves and Max Moves
    u16 commanderActive[MAX_BATTLERS_COUNT];
    u32 stellarBoostFlags[NUM_BATTLE_SIDES]; // stored as a bitfield of flags for all types for each side
    u8 monCausingSleepClause[NUM_BATTLE_SIDES]; // Stores which pokemon on a given side is causing Sleep Clause to be active as the mon's index in the party
    u8 additionalEffectsCounter:4; // A counter for the additionalEffects applied by the current move in Cmd_setadditionaleffects
    u8 cheekPouchActivated:1;
    u8 padding2:3;
    u8 pursuitStoredSwitch; // Stored id for the Pursuit target's switch
    s32 battlerExpReward;
    u16 prevTurnSpecies[MAX_BATTLERS_COUNT]; // Stores species the AI has in play at start of turn
    s32 moveDamage[MAX_BATTLERS_COUNT];
    s32 critChance[MAX_BATTLERS_COUNT];
    u16 moveResultFlags[MAX_BATTLERS_COUNT];
    u8 missStringId[MAX_BATTLERS_COUNT];
    u8 noResultString[MAX_BATTLERS_COUNT];
    u8 doneDoublesSpreadHit:1;
    u8 calculatedDamageDone:1;
    u8 calculatedSpreadMoveAccuracy:1;
    u8 printedStrongWindsWeakenedAttack:1;
    u8 numSpreadTargets:2;
    u8 bypassMoldBreakerChecks:1; // for ABILITYEFFECT_IMMUNITY
    u8 noTargetPresent:1;
    u8 usedEjectItem;
    u8 usedMicleBerry;
    struct MessageStatus slideMessageStatus;
    u8 trainerSlideSpriteIds[MAX_BATTLERS_COUNT];
    u16 opponentMonCanTera:6;
    u16 opponentMonCanDynamax:6;
    u16 padding:4;
};

struct AiBattleData
{
    s32 finalScore[MAX_BATTLERS_COUNT][MAX_BATTLERS_COUNT][MAX_MON_MOVES]; // AI, target, moves to make debugging easier
    u8 chosenMoveIndex[MAX_BATTLERS_COUNT];
    u8 chosenTarget[MAX_BATTLERS_COUNT];
    u8 actionFlee:1;
    u8 choiceWatch:1;
    u8 padding:6;
};

// The palaceFlags member of struct BattleStruct contains 1 flag per move to indicate which moves the AI should consider,
// and 1 flag per battler to indicate whether the battler is awake and at <= 50% HP (which affects move choice).
// The assert below is to ensure palaceFlags is large enough to store these flags without overlap.
STATIC_ASSERT(sizeof(((struct BattleStruct *)0)->palaceFlags) * 8 >= MAX_BATTLERS_COUNT + MAX_MON_MOVES, PalaceFlagsTooSmall)

#define DYNAMIC_TYPE_MASK                 ((1 << 6) - 1)
#define F_DYNAMIC_TYPE_IGNORE_PHYSICALITY  (1 << 6) // If set, the dynamic type's physicality won't be used for certain move effects.
#define F_DYNAMIC_TYPE_SET                 (1 << 7) // Set for all dynamic types to distinguish a dynamic type of Normal (0) from no dynamic type.

static inline bool32 IsBattleMovePhysical(u32 move)
{
    return GetBattleMoveCategory(move) == DAMAGE_CATEGORY_PHYSICAL;
}

static inline bool32 IsBattleMoveSpecial(u32 move)
{
    return GetBattleMoveCategory(move) == DAMAGE_CATEGORY_SPECIAL;
}

static inline bool32 IsBattleMoveStatus(u32 move)
{
    return GetMoveCategory(move) == DAMAGE_CATEGORY_STATUS;
}

static inline bool32 IsBattleMoveRecoil(u32 move)
{
    return GetMoveRecoil(move) > 0 || GetMoveEffect(move) == EFFECT_RECOIL_IF_MISS;
}

/* Checks if 'battlerId' is any of the types.
 * Passing multiple types is more efficient than calling this multiple
 * times with one type because it shares the 'GetBattlerTypes' result. */
#define _IS_BATTLER_ANY_TYPE(battlerId, ignoreTera, ...) \
    ({ \
        u32 types[3]; \
        GetBattlerTypes(battlerId, ignoreTera, types); \
        RECURSIVELY(R_FOR_EACH(_IS_BATTLER_ANY_TYPE_HELPER, __VA_ARGS__)) FALSE; \
    })

#define _IS_BATTLER_ANY_TYPE_HELPER(type) (types[0] == type) || (types[1] == type) || (types[2] == type) ||

#define IS_BATTLER_ANY_TYPE(battlerId, ...) _IS_BATTLER_ANY_TYPE(battlerId, FALSE, __VA_ARGS__)
#define IS_BATTLER_OF_TYPE IS_BATTLER_ANY_TYPE
#define IS_BATTLER_ANY_BASE_TYPE(battlerId, ...) _IS_BATTLER_ANY_TYPE(battlerId, TRUE, __VA_ARGS__)
#define IS_BATTLER_OF_BASE_TYPE IS_BATTLER_ANY_BASE_TYPE

#define IS_BATTLER_TYPELESS(battlerId) \
    ({ \
        u32 types[3]; \
        GetBattlerTypes(battlerId, FALSE, types); \
        types[0] == TYPE_MYSTERY && types[1] == TYPE_MYSTERY && types[2] == TYPE_MYSTERY; \
    })

#define SET_BATTLER_TYPE(battlerId, type)              \
{                                                      \
    gBattleMons[battlerId].types[0] = type;            \
    gBattleMons[battlerId].types[1] = type;            \
    gBattleMons[battlerId].types[2] = TYPE_MYSTERY;    \
}

#define RESTORE_BATTLER_TYPE(battlerId)                                                        \
{                                                                                              \
    gBattleMons[battlerId].types[0] = gSpeciesInfo[gBattleMons[battlerId].species].types[0];   \
    gBattleMons[battlerId].types[1] = gSpeciesInfo[gBattleMons[battlerId].species].types[1];   \
    gBattleMons[battlerId].types[2] = TYPE_MYSTERY;                                            \
}

#define GET_STAT_BUFF_ID(n) ((n & 7))              // first three bits 0x1, 0x2, 0x4
#define GET_STAT_BUFF_VALUE_WITH_SIGN(n) ((n & 0xF8))
#define GET_STAT_BUFF_VALUE(n) (((n >> 3) & 0xF))      // 0x8, 0x10, 0x20, 0x40
#define STAT_BUFF_NEGATIVE 0x80                     // 0x80, the sign bit

#define SET_STAT_BUFF_VALUE(n) ((((n) << 3) & 0xF8))

#define SET_STATCHANGER(statId, stage, goesDown) (gBattleScripting.statChanger = (statId) + ((stage) << 3) + (goesDown << 7))
#define SET_STATCHANGER2(dst, statId, stage, goesDown)(dst = (statId) + ((stage) << 3) + (goesDown << 7))

#define DO_ACCURACY_CHECK 2 // Don't skip the accuracy check before the move might be absorbed

// NOTE: The members of this struct have hard-coded offsets
//       in include/constants/battle_script_commands.h
struct BattleScripting
{
    s32 unused1;
    s32 bideDmg;
    u8 multihitString[6];
    bool8 expOnCatch;
    u8 unused2;
    u8 animArg1;
    u8 animArg2;
    u16 savedStringId;
    u8 moveendState;
    u8 savedStatChanger; // For further use, if attempting to change stat two times(ex. Moody)
    u8 shiftSwitched; // When the game tells you the next enemy's pokemon and you switch. Option for noobs but oh well.
    u8 battler;
    u8 animTurn;
    u8 animTargetsHit;
    u8 statChanger;
    bool8 statAnimPlayed;
    u8 getexpState;
    u8 battleStyle;
    u8 drawlvlupboxState;
    u8 learnMoveState;
    u8 savedBattler;
    u8 reshowMainState;
    u8 reshowHelperState;
    u8 levelUpHP;
    u8 windowsType; // B_WIN_TYPE_*
    u8 multiplayerId;
    u8 specialTrainerBattleType;
    bool8 monCaught;
    s32 savedDmg;
    u16 savedMoveEffect; // For moves hitting multiple targets.
    u16 moveEffect;
    u16 multihitMoveEffect;
    u8 illusionNickHack; // To properly display nick in STRINGID_ENEMYABOUTTOSWITCHPKMN.
    bool8 fixedPopup;   // Force ability popup to stick until manually called back
    u16 abilityPopupOverwrite;
    u8 switchCase;  // Special switching conditions, eg. red card
    u8 overrideBerryRequirements;
    u8 stickyWebStatDrop; // To prevent Defiant activating on a Court Change'd Sticky Web
};

struct BattleSpriteInfo
{
    u16 invisible:1; // 0x1
    u16 lowHpSong:1; // 0x2
    u16 behindSubstitute:1; // 0x4
    u16 flag_x8:1; // 0x8
    u16 hpNumbersNoBars:1; // 0x10
    u16 transformSpecies;
};

struct BattleAnimationInfo
{
    u16 animArg; // to fill up later
    u8 field_2;
    u8 field_3;
    u8 field_4;
    u8 field_5;
    u8 field_6;
    u8 field_7;
    u8 ballThrowCaseId:6;
    u8 isCriticalCapture:1;
    u8 criticalCaptureSuccess:1;
    u8 introAnimActive:1;
    u8 wildMonInvisible:1;
    u8 field_9_x1C:3;
    u8 field_9_x20:1;
    u8 field_9_x40:1;
    u8 field_9_x80:1;
    u8 numBallParticles;
    u8 field_B;
    s16 ballSubpx;
    u8 field_E;
    u8 field_F;
};

struct BattleHealthboxInfo
{
    u8 partyStatusSummaryShown:1;
    u8 healthboxIsBouncing:1;
    u8 battlerIsBouncing:1;
    u8 ballAnimActive:1; // 0x8
    u8 statusAnimActive:1; // x10
    u8 animFromTableActive:1; // x20
    u8 specialAnimActive:1; // x40
    u8 triedShinyMonAnim:1;
    u8 finishedShinyMonAnim:1;
    u8 opponentDrawPartyStatusSummaryDelay:4;
    u8 bgmRestored:1;
    u8 waitForCry:1;
    u8 healthboxSlideInStarted:1;
    u8 healthboxBounceSpriteId;
    u8 battlerBounceSpriteId;
    u8 animationState;
    u8 partyStatusDelayTimer;
    u8 matrixNum;

    u8 shadowSpriteIdPrimary;
    u8 shadowSpriteIdSecondary;

    u8 soundTimer;
    u8 introEndDelay;
    u8 field_A;
    u8 field_B;
};

struct BattleBarInfo
{
    u8 healthboxSpriteId;
    s32 maxValue;
    s32 oldValue;
    s32 receivedValue;
    s32 currValue;
};

struct BattleSpriteData
{
    struct BattleSpriteInfo *battlerData;
    struct BattleHealthboxInfo *healthBoxesData;
    struct BattleAnimationInfo *animationData;
    struct BattleBarInfo *battleBars;
};

#include "sprite.h"

struct MonSpritesGfx
{
    void *firstDecompressed; // ptr to the decompressed sprite of the first Pokémon
    u8 *spritesGfx[MAX_BATTLERS_COUNT];
    struct SpriteTemplate templates[MAX_BATTLERS_COUNT];
    struct SpriteFrameImage frameImages[MAX_BATTLERS_COUNT][MAX_MON_PIC_FRAMES];
    u8 *barFontGfx;
    u16 *buffer;
};

struct QueuedStatBoost
{
    u8 stats;   // bitfield for each battle stat that is set if the stat changes
    s8 statChanges[NUM_BATTLE_STATS - 1];    // highest bit being set decreases the stat
}; /* size = 8 */

// All battle variables are declared in battle_main.c
extern u16 gBattle_BG0_X;
extern u16 gBattle_BG0_Y;
extern u16 gBattle_BG1_X;
extern u16 gBattle_BG1_Y;
extern u16 gBattle_BG2_X;
extern u16 gBattle_BG2_Y;
extern u16 gBattle_BG3_X;
extern u16 gBattle_BG3_Y;
extern u16 gBattle_WIN0H;
extern u16 gBattle_WIN0V;
extern u16 gBattle_WIN1H;
extern u16 gBattle_WIN1V;
extern u8 gDisplayedStringBattle[425];
extern u8 gBattleTextBuff1[TEXT_BUFF_ARRAY_COUNT];
extern u8 gBattleTextBuff2[TEXT_BUFF_ARRAY_COUNT];
extern u8 gBattleTextBuff3[TEXT_BUFF_ARRAY_COUNT + 13]; //to handle stupidly large z move names
extern u32 gBattleTypeFlags;
extern u8 gBattleEnvironment;
extern u8 *gBattleAnimBgTileBuffer;
extern u8 *gBattleAnimBgTilemapBuffer;
extern u32 gBattleControllerExecFlags;
extern u8 gBattlersCount;
extern u16 gBattlerPartyIndexes[MAX_BATTLERS_COUNT];
extern u8 gBattlerPositions[MAX_BATTLERS_COUNT];
extern u8 gActionsByTurnOrder[MAX_BATTLERS_COUNT];
extern u8 gBattlerByTurnOrder[MAX_BATTLERS_COUNT];
extern u8 gCurrentTurnActionNumber;
extern u8 gCurrentActionFuncId;
extern struct BattlePokemon gBattleMons[MAX_BATTLERS_COUNT];
extern u8 gBattlerSpriteIds[MAX_BATTLERS_COUNT];
extern u8 gCurrMovePos;
extern u8 gChosenMovePos;
extern u16 gCurrentMove;
extern u16 gChosenMove;
extern u16 gCalledMove;
extern s32 gBideDmg[MAX_BATTLERS_COUNT];
extern u16 gLastUsedItem;
extern u16 gLastUsedAbility;
extern u8 gBattlerAttacker;
extern u8 gBattlerTarget;
extern u8 gBattlerFainted;
extern u8 gEffectBattler;
extern u8 gPotentialItemEffectBattler;
extern u8 gAbsentBattlerFlags;
extern u8 gMultiHitCounter;
extern const u8 *gBattlescriptCurrInstr;
extern u8 gChosenActionByBattler[MAX_BATTLERS_COUNT];
extern const u8 *gSelectionBattleScripts[MAX_BATTLERS_COUNT];
extern const u8 *gPalaceSelectionBattleScripts[MAX_BATTLERS_COUNT];
extern u16 gLastPrintedMoves[MAX_BATTLERS_COUNT];
extern u16 gLastMoves[MAX_BATTLERS_COUNT];
extern u16 gLastLandedMoves[MAX_BATTLERS_COUNT];
extern u16 gLastHitByType[MAX_BATTLERS_COUNT];
extern u16 gLastUsedMoveType[MAX_BATTLERS_COUNT];
extern u16 gLastResultingMoves[MAX_BATTLERS_COUNT];
extern u16 gLockedMoves[MAX_BATTLERS_COUNT];
extern u16 gLastUsedMove;
extern u8 gLastHitBy[MAX_BATTLERS_COUNT];
extern u16 gChosenMoveByBattler[MAX_BATTLERS_COUNT];
extern u32 gHitMarker;
extern u8 gBideTarget[MAX_BATTLERS_COUNT];
extern u32 gSideStatuses[NUM_BATTLE_SIDES];
extern struct SideTimer gSideTimers[NUM_BATTLE_SIDES];
extern u32 gStatuses3[MAX_BATTLERS_COUNT];
extern u32 gStatuses4[MAX_BATTLERS_COUNT];
extern struct DisableStruct gDisableStructs[MAX_BATTLERS_COUNT];
extern u16 gPauseCounterBattle;
extern u16 gPaydayMoney;
extern u8 gBattleCommunication[BATTLE_COMMUNICATION_ENTRIES_COUNT];
extern u8 gBattleOutcome;
extern struct ProtectStruct gProtectStructs[MAX_BATTLERS_COUNT];
extern struct SpecialStatus gSpecialStatuses[MAX_BATTLERS_COUNT];
extern u16 gBattleWeather;
extern struct WishFutureKnock gWishFutureKnock;
extern u16 gIntroSlideFlags;
extern u8 gSentPokesToOpponent[2];
extern struct BattleEnigmaBerry gEnigmaBerries[MAX_BATTLERS_COUNT];
extern struct BattleScripting gBattleScripting;
extern struct BattleStruct *gBattleStruct;
extern struct AiBattleData *gAiBattleData;
extern u8 *gLinkBattleSendBuffer;
extern u8 *gLinkBattleRecvBuffer;
extern struct BattleResources *gBattleResources;
extern u8 gActionSelectionCursor[MAX_BATTLERS_COUNT];
extern u8 gMoveSelectionCursor[MAX_BATTLERS_COUNT];
extern u8 gBattlerStatusSummaryTaskId[MAX_BATTLERS_COUNT];
extern u8 gBattlerInMenuId;
extern bool8 gDoingBattleAnim;
extern u32 gTransformedPersonalities[MAX_BATTLERS_COUNT];
extern bool8 gTransformedShininess[MAX_BATTLERS_COUNT];
extern u8 gPlayerDpadHoldFrames;
extern struct BattleSpriteData *gBattleSpritesDataPtr;
extern struct MonSpritesGfx *gMonSpritesGfxPtr;
extern u16 gBattleMovePower;
extern u16 gMoveToLearn;
extern u32 gFieldStatuses;
extern struct FieldTimer gFieldTimers;
extern u16 gBattleTurnCounter;
extern u8 gBattlerAbility;
extern struct QueuedStatBoost gQueuedStatBoosts[MAX_BATTLERS_COUNT];

extern void (*gPreBattleCallback1)(void);
extern void (*gBattleMainFunc)(void);
extern struct BattleResults gBattleResults;
extern u8 gLeveledUpInBattle;
extern u8 gHealthboxSpriteIds[MAX_BATTLERS_COUNT];
extern u8 gMultiUsePlayerCursor;
extern u8 gNumberOfMovesToChoose;
extern bool8 gHasFetchedBall;
extern u8 gLastUsedBall;
extern u16 gLastThrownBall;
extern u16 gBallToDisplay;
extern bool8 gLastUsedBallMenuPresent;
extern u8 gPartyCriticalHits[PARTY_SIZE];
extern u8 gCategoryIconSpriteId;

static inline bool32 IsBattlerAlive(u32 battler)
{
    if (gBattleMons[battler].hp == 0)
        return FALSE;
    else if (battler >= gBattlersCount)
        return FALSE;
    else if (gAbsentBattlerFlags & (1u << battler))
        return FALSE;
    else
        return TRUE;
}

static inline bool32 IsBattlerTurnDamaged(u32 battler)
{
    return gSpecialStatuses[battler].physicalDmg != 0
        || gSpecialStatuses[battler].specialDmg != 0
        || gSpecialStatuses[battler].enduredDamage;
}

static inline bool32 IsBattlerAtMaxHp(u32 battler)
{
    return gBattleMons[battler].hp == gBattleMons[battler].maxHP;
}

static inline u32 GetBattlerPosition(u32 battler)
{
    return gBattlerPositions[battler];
}

static inline u32 GetBattlerAtPosition(u32 position)
{
    u32 battler;
    for (battler = 0; battler < gBattlersCount; battler++)
    {
        if (GetBattlerPosition(battler) == position)
            break;
    }
    return battler;
}

static inline u32 GetPartnerBattler(u32 battler)
{
    return GetBattlerAtPosition(BATTLE_PARTNER(GetBattlerPosition(battler)));
}

static inline u32 GetOppositeBattler(u32 battler)
{
    return GetBattlerAtPosition(BATTLE_OPPOSITE(GetBattlerPosition(battler)));
}

static inline u32 GetBattlerSide(u32 battler)
{
    return GetBattlerPosition(battler) & BIT_SIDE;
}

static inline bool32 IsBattlerAlly(u32 battlerAtk, u32 battlerDef)
{
    return (GetBattlerSide(battlerAtk) == GetBattlerSide(battlerDef));
}

static inline u32 GetOpposingSideBattler(u32 battler)
{
    return GetBattlerAtPosition(BATTLE_OPPOSITE(GetBattlerSide(battler)));
}

static inline struct Pokemon* GetPartyBattlerData(u32 battler)
{
    u32 index = gBattlerPartyIndexes[battler];
    return (GetBattlerSide(battler) == B_SIDE_OPPONENT) ? &gEnemyParty[index] : &gPlayerParty[index];
}

static inline struct Pokemon *GetSideParty(u32 side)
{
    return (side == B_SIDE_PLAYER) ? gPlayerParty : gEnemyParty;
}

static inline struct Pokemon *GetBattlerParty(u32 battler)
{
    return GetSideParty(GetBattlerSide(battler));
}

static inline bool32 IsDoubleBattle(void)
{
    return gBattleTypeFlags & BATTLE_TYPE_DOUBLE;
}

static inline bool32 IsSpreadMove(u32 moveTarget)
{
    return IsDoubleBattle() && (moveTarget == MOVE_TARGET_BOTH || moveTarget == MOVE_TARGET_FOES_AND_ALLY);
}

static inline bool32 IsDoubleSpreadMove(void)
{
    return gBattleStruct->numSpreadTargets > 1
        && !(gHitMarker & (HITMARKER_IGNORE_SUBSTITUTE | HITMARKER_PASSIVE_DAMAGE | HITMARKER_UNABLE_TO_USE_MOVE))
        && IsSpreadMove(GetBattlerMoveTargetType(gBattlerAttacker, gCurrentMove));
}

static inline bool32 IsBattlerInvalidForSpreadMove(u32 battlerAtk, u32 battlerDef, u32 moveTarget)
{
    return battlerDef == battlerAtk
        || !IsBattlerAlive(battlerDef)
        || (battlerDef == BATTLE_PARTNER(battlerAtk) && (moveTarget == MOVE_TARGET_BOTH));
}

#endif // GUARD_BATTLE_H
