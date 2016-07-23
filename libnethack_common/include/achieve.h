/* vim:set cin ft=c sw=4 sts=4 ts=8 et ai cino=Ls\:0t0(0 : -*- mode:c;fill-column:80;tab-width:8;c-basic-offset:4;indent-tabs-mode:nil;c-file-style:"k&r" -*-*/
/* Last modified by Nathan Eady, 2015-12-22 */
/* Copyright (c) Nathan Eady, 2015. */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef ACHIEVE_H
#define ACHIEVE_H

/* Unlocked-feature fields. */
#define UNLOCK_FIELD_ROLE      1
#define UNLOCK_FIELD_RACE      2
#define UNLOCK_FIELD_OPT       3
#define UNLOCK_FIELD_ASCROLES  4
#define UNLOCK_FIELD_ASCRACES  5
#define UNLOCK_FIELD_MAX   UNLOCK_FIELD_ASCRACES

/* Unlockable features.  By performing various achievements, the
   player can unlock access to various features:  races, roles,
   and options (which includes some alternate modes of play). */

#define UNLOCKFEAT_ALWAYSUNLOCKED    0x00000001UL
/* The first bit of each field is special, always unlocked.
   This is used to ensure that the player has at least one
   valid unlocked race/role combination that can be used to
   play initially, until others can be unlocked.  Races and
   roles that are always unlocked still need bits allocated,
   for the all-roles/all-races ascension calculations. */

/* First unlock field: player character role */
/* Note that always-unlocked roles must have an ALLROLES constant for the
   second field in role.c, to ensure uniqueness for all-roles tracking. */
#define UNLOCKFEAT_ALLROLES_BARB     0x00000002UL
#define UNLOCKFEAT_ROLE_BARBARIAN    UNLOCKFEAT_ALWAYSUNLOCKED
#define UNLOCKFEAT_ALLROLES_WIZARD   0x00000004UL
#define UNLOCKFEAT_ROLE_WIZARD       UNLOCKFEAT_ALWAYSUNLOCKED
#define UNLOCKFEAT_ROLE_RANGER       0x00000008UL
#define UNLOCKFEAT_ROLE_MONK         0x00000010UL
#define UNLOCKFEAT_ROLE_ROGUE        0x00000020UL
#define UNLOCKFEAT_ROLE_KNIGHT       0x00000040UL
#define UNLOCKFEAT_ROLE_PRIEST       0x00000080UL
#define UNLOCKFEAT_ROLE_CAVEMAN      0x00000100UL
#define UNLOCKFEAT_ROLE_HEALER       0x00000200UL
#define UNLOCKFEAT_ROLE_VALKYRIE     0x00000400UL
#define UNLOCKFEAT_ROLE_ARCHEOLOGIST 0x00000800UL
#define UNLOCKFEAT_ROLE_SAMURAI      0x00001000UL
#define UNLOCKFEAT_ROLE_TOURIST      0x00002000UL
#define UNLOCKFEAT_ROLE_DRUID        0x00004000UL
#define UNLOCKFEAT_ROLE_CONVICT      0x00008000UL
/* When adding to this list:
 * 1. implement the role
 * 2. update UNLOCKFEAT_ROLE_MAX, below.
 */
#define UNLOCKFEAT_ROLE_MAX          UNLOCKFEAT_ROLE_CONVICT

/* Second unlock field: playable race */
#define UNLOCKFEAT_RACE_HUMAN        UNLOCKFEAT_ALWAYSUNLOCKED
#define UNLOCKFEAT_ALLRACES_HUMAN    0x00000002UL
#define UNLOCKFEAT_RACE_DWARF        0x00000004UL
#define UNLOCKFEAT_RACE_ELF          0x00000008UL
#define UNLOCKFEAT_RACE_ORC          0x00000010UL
#define UNLOCKFEAT_RACE_GNOME        0x00000020UL
#define UNLOCKFEAT_RACE_SYLPH        0x00000040UL
#define UNLOCKFEAT_RACE_GIANT        0x00000080UL
#define UNLOCKFEAT_RACE_SCURRIER     0x00000100UL
/* When adding to this list:
 * 1. implement the playable race
 * 2. update UNLOCKFEAT_RACE_MAX, below.
 */
#define UNLOCKFEAT_RACE_MAX          UNLOCKFEAT_RACE_SCURRIER

/* Third unlock field: options (and modes) */
#define UNLOCKFEAT_OPT_PERMABLIND    0x00000002UL
#define UNLOCKFEAT_OPT_PERMAHALLU    0x00000004UL
#define UNLOCKFEAT_OPT_PERMACONF     0x00000008UL
#define UNLOCKFEAT_OPT_PERMASTUN     0x00000010UL
#define UNLOCKFEAT_OPT_PERMAGLIB     0x00000020UL
#define UNLOCKFEAT_OPT_PERMAFUMBLE   0x00000040UL
#define UNLOCKFEAT_OPT_PERMALAME     0x00000080UL
#define UNLOCKFEAT_OPT_PERMABADLUCK  0x00000100UL
#define UNLOCKFEAT_OPT_CHALLENGE     0x00001000UL
#define UNLOCKFEAT_OPT_POLYINIT      0x00002000UL
#define UNLOCKFEAT_OPT_ONEHP         0x00004000UL
#define UNLOCKFEAT_OPT_FRUITNAME     0x00010000UL
#define UNLOCKFEAT_OPT_RACE          0x00020000UL
#define UNLOCKFEAT_OPT_SHOWRACE      0x00040000UL
#define UNLOCKFEAT_OPT_HORSENAME     0x00080000UL
#define UNLOCKFEAT_OPT_AUTOWEAR      0x00100000UL
/* When adding to this list:
 * 1. update explain_unlockable_option
 * 2. update is_unlocked_option
 * 3. update UNLOCKFEAT_OPT_MAX, below.
 * 4. remember to include a way to unlock it.
 */
#define UNLOCKFEAT_OPT_MAX           UNLOCKFEAT_OPT_AUTOWEAR


/* Achievements:  feats that can be performed in order to unlock the
   above features.  The relationship between unlockable features and
   the achievements used to unlock them is NOT one-to-one or onto. 
   That is, some achievements may unlock more than one feature, and
   some features can be unlocked via more than one achievement. */

enum achievement {
    /* When adding to this list, update explain_achievement()
       and add the calls to achievement() where appropriate. */
    achieve_stairs_normal,   /* use normal stairs to go up or down */
    achieve_stairs_branch,   /* use branch stairs to enter/leave */
    achieve_eat_slimemold,   /* eat a customizable fruit */
    achieve_mines_temple,    /* enter the Minetown temple */
    achieve_mines_end,       /* reach the bottom of the Mines */
    achieve_altar_buctest,   /* identify an item's beatitude on an altar */
    achieve_altar_sacrifice, /* offer a corpse on an altar */
    achieve_altar_convert,   /* convert an altar to your alignment */
    achieve_soko_stairs,     /* use normal stairs _in Sokoban_ */
    achieve_soko_complete,   /* enter the Sokoban zoo, without cheating */
    achieve_dig,             /* dig, either horizontally or vertically */
    achieve_vault_gold,      /* take gold from a vault */
    achieve_sproom_zoo,      /* enter a treasure zoo (in the main dungeon) */
    achieve_sproom_throne,   /* enter a throne room */
    achieve_sproom_lephall,  /* enter a leprechaun hall */
    achieve_sproom_dragons,  /* enter a dragon hall */
    achieve_portal,          /* be transported by a magic portal */
    achieve_kill_medusa,     /* kill Medusa */
    achieve_kill_nemesis,    /* kill your quest nemesis */
    achieve_kill_rodney,     /* kill the Wizard of Yendor */
    achieve_passtune,        /* play the passtune */
    achieve_valley_stairs,   /* use the down stairs in the Valley */
    achieve_holy_water,      /* receive holy water from prayer */
    achieve_invocation,      /* perform the Invocation Ritual */
    achieve_ascend,          /* win the game */
    achieve_ascend_conduct,     /* ascend with a tracked conduct */
    achieve_ascend_conduct_med, /* ascend with a moderately difficult conduct*/
    achieve_ascend_challenge,   /* ascend in challenge mode */
    achieve_ascend_impaired,    /* ascend with a perma-impairment option */
    achieve_ascend_conduct_hard,/* ascend with a difficult conduct combo */
    achieve_ascend_speedrun,    /* ascend in under ten thousand turns */
    achieve_ascend_allroles,    /* ascend all roles */
    achieve_ascend_allraces,    /* ascend all races */

    /* receive permission to go on the quest... */
    achieve_quest_barb,         /* as a Barbarian */
    achieve_quest_wizard,       /* as a Wizard */
    achieve_quest_ranger,       /* as a Ranger */
    achieve_quest_monk,         /* as a Monk */
    achieve_quest_rogue,        /* as a Rogue */
    achieve_quest_knight,       /* as a Knight */
    achieve_quest_priest,       /* as a Priest */
    achieve_quest_caveman,      /* as a Caveman */
    achieve_quest_healer,       /* as a Healer */
    achieve_quest_valkyrie,     /* as a Valkyrie */
    achieve_quest_samurai,      /* as a Samurai */
    achieve_quest_archeologist, /*as an Archeologist */
    achieve_quest_tourist,      /* as a Tourist */
    achieve_quest_convict,      /* as a Convict */
    achieve_quest_druid,        /* as a Druid */
    
};

#endif /* ACHIEVE_H */
