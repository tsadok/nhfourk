/* vim:set cin ft=c sw=4 sts=4 ts=8 et ai cino=Ls\:0t0(0 : -*- mode:c;fill-column:80;tab-width:8;c-basic-offset:4;indent-tabs-mode:nil;c-file-style:"k&r" -*-*/
/* Last modified by Nathan Eady, 2015-12-23 */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "achieve.h"
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <inttypes.h>

unsigned long achievement_field[UNLOCK_FIELD_MAX + 1];
/* achievement() ensures that the fields are populated before use,
   and achievement_unlock() writes any changes back out. */

static void achievement_unlock(enum achievement ach, int fieldidx,
                               unsigned long fieldbit);
static const char * explain_unlockable(int fieldidx, unsigned long fieldbit);
static const char * explain_unlockable_role(unsigned long fieldbit);
static const char * explain_unlockable_race(unsigned long fieldbit);
static const char * explain_unlockable_option(unsigned long fieldbit);
static const char * explain_achievement(enum achievement ach);
static const char * achievements_filename(void);
static void read_achievements(void);
static void write_achievements(void);

boolean
is_unlocked_feature(int fieldidx, unsigned long fieldbit)
{
    if (fieldidx > UNLOCK_FIELD_MAX)
        panic("Out-of-bounds unlockable-feature field number, %d",
              fieldidx);
    read_achievements();
    if (achievement_field[fieldidx] & fieldbit)
        return TRUE;
    else
        return FALSE;
}

void
achievement(enum achievement ach)
{
    /* First, read the achievements file, so we have an accurate
       picture of what the player has or hasn't already unlocked: */  
    read_achievements();

    /* What does the current achievement unlock? */
    if      (ach == achieve_stairs_normal)
        achievement_unlock(ach, UNLOCK_FIELD_ROLE, UNLOCKFEAT_ROLE_RANGER);
    else if (ach == achieve_stairs_branch)
        achievement_unlock(ach, UNLOCK_FIELD_ROLE, UNLOCKFEAT_ROLE_ROGUE);
    else if (ach == achieve_mines_temple)
        achievement_unlock(ach, UNLOCK_FIELD_ROLE, UNLOCKFEAT_ROLE_MONK);
    else if (ach == achieve_mines_end)
        achievement_unlock(ach, UNLOCK_FIELD_RACE, UNLOCKFEAT_RACE_DWARF);
    else if (ach == achieve_altar_buctest)
        achievement_unlock(ach, UNLOCK_FIELD_ROLE, UNLOCKFEAT_ROLE_PRIEST);
    else if (ach == achieve_altar_sacrifice)
        achievement_unlock(ach, UNLOCK_FIELD_ROLE, UNLOCKFEAT_ROLE_PRIEST);
    else if (ach == achieve_altar_convert)
        achievement_unlock(ach, UNLOCK_FIELD_ROLE, UNLOCKFEAT_ROLE_VALKYRIE);
    else if (ach == achieve_soko_stairs)
        achievement_unlock(ach, UNLOCK_FIELD_RACE, UNLOCKFEAT_RACE_ELF);
    else if (ach == achieve_soko_complete)
        achievement_unlock(ach, UNLOCK_FIELD_ROLE, UNLOCKFEAT_ROLE_KNIGHT);
    else if (ach == achieve_dig)
        achievement_unlock(ach, UNLOCK_FIELD_ROLE, UNLOCKFEAT_ROLE_CAVEMAN);
    else if (ach == achieve_vault_gold)
        achievement_unlock(ach, UNLOCK_FIELD_ROLE, UNLOCKFEAT_ROLE_ROGUE);
    else if (ach == achieve_sproom_zoo)
        achievement_unlock(ach, UNLOCK_FIELD_ROLE, UNLOCKFEAT_ROLE_KNIGHT);
    else if (ach == achieve_sproom_throne)
        achievement_unlock(ach, UNLOCK_FIELD_RACE, UNLOCKFEAT_RACE_ORC);
    else if (ach == achieve_sproom_lephall)
        achievement_unlock(ach, UNLOCK_FIELD_ROLE, UNLOCKFEAT_ROLE_ROGUE);
    else if (ach == achieve_sproom_dragons)
        achievement_unlock(ach, UNLOCK_FIELD_RACE, UNLOCKFEAT_RACE_GNOME);
    else if (ach == achieve_portal)
        achievement_unlock(ach, UNLOCK_FIELD_RACE, UNLOCKFEAT_RACE_GNOME);
    else if (ach == achieve_kill_medusa)
        achievement_unlock(ach, UNLOCK_FIELD_ROLE, UNLOCKFEAT_ROLE_HEALER);
    else if (ach == achieve_kill_nemesis)
        achievement_unlock(ach, UNLOCK_FIELD_RACE, UNLOCKFEAT_RACE_SYLPH);
    else if (ach == achieve_kill_rodney)
        achievement_unlock(ach, UNLOCK_FIELD_RACE, UNLOCKFEAT_RACE_ELF);
    else if (ach == achieve_passtune)
        achievement_unlock(ach, UNLOCK_FIELD_RACE, UNLOCKFEAT_RACE_SYLPH);
    else if (ach == achieve_valley_stairs)
        ;/* Currently this doesn't unlock anything. */
    else if (ach == achieve_holy_water)
        achievement_unlock(ach, UNLOCK_FIELD_ROLE, UNLOCKFEAT_ROLE_HEALER);
    else if (ach == achieve_invocation)
        achievement_unlock(ach, UNLOCK_FIELD_ROLE, UNLOCKFEAT_ROLE_VALKYRIE);
    else if (ach == achieve_ascend) {
        int i;
        boolean allroles = TRUE, allraces = TRUE;
        achievement_unlock(ach, UNLOCK_FIELD_ROLE,
                                UNLOCKFEAT_ROLE_ARCHEOLOGIST);

        /* Do all-roles tracking: */
        achievement_unlock(ach, UNLOCK_FIELD_ASCROLES, urole.unlockconst);
        /* Does that add up to all roles? */
        for (i = 0; roles[i].name.m; i++) {
            if (!is_unlocked_feature(UNLOCK_FIELD_ASCROLES,
                                     roles[i].unlockconst)) {
                allroles = FALSE;
            }
        }
        if (allroles) {
            achievement(achieve_ascend_allroles);
        }

        /* Do all-races tracking: */
        achievement_unlock(ach, UNLOCK_FIELD_ASCRACES, urace.raceunlconst);
        /* Does that add up to all races?  */
        for (i = 0; races[i].adj; i++) {
            if (!is_unlocked_feature(UNLOCK_FIELD_ASCRACES,
                                     races[i].raceunlconst)) {
                allraces = FALSE;
            }
        }
        if (allraces) {
            achievement(achieve_ascend_allraces);
        }
    }
    else if (ach == achieve_ascend_conduct)
        achievement_unlock(ach, UNLOCK_FIELD_RACE, UNLOCKFEAT_RACE_OGRE);
    else if (ach == achieve_ascend_conduct_med)
        achievement_unlock(ach, UNLOCK_FIELD_OPT, UNLOCKFEAT_OPT_POLYINIT);
    else if (ach == achieve_ascend_challenge)
        achievement_unlock(ach, UNLOCK_FIELD_OPT, UNLOCKFEAT_OPT_PERMAFUMBLE);
    else if (ach == achieve_ascend_impaired) {
        achievement_unlock(ach, UNLOCK_FIELD_OPT, UNLOCKFEAT_OPT_PERMALAME);
        achievement_unlock(ach, UNLOCK_FIELD_OPT, UNLOCKFEAT_OPT_PERMABADLUCK);
    }
    else if (ach == achieve_ascend_conduct_hard)
        achievement_unlock(ach, UNLOCK_FIELD_OPT, UNLOCKFEAT_OPT_ONEHP);
    else if (ach == achieve_ascend_speedrun)
        achievement_unlock(ach, UNLOCK_FIELD_OPT, UNLOCKFEAT_OPT_ONEHP);
    else if (ach == achieve_ascend_allroles)
        ;/* TODO */
    else if (ach == achieve_ascend_allraces)
        ;/* TODO */

    else if (ach == achieve_quest_barb)
        achievement_unlock(ach, UNLOCK_FIELD_ROLE, UNLOCKFEAT_ROLE_SAMURAI);
    else if (ach == achieve_quest_wizard)
        achievement_unlock(ach, UNLOCK_FIELD_RACE, UNLOCKFEAT_RACE_SCURRIER);
    else if (ach == achieve_quest_ranger)
        achievement_unlock(ach, UNLOCK_FIELD_ROLE, UNLOCKFEAT_ROLE_DRUID);
    else if (ach == achieve_quest_monk)
        achievement_unlock(ach, UNLOCK_FIELD_ROLE, UNLOCKFEAT_ROLE_TOURIST);
    else if (ach == achieve_quest_rogue)
        achievement_unlock(ach, UNLOCK_FIELD_ROLE, UNLOCKFEAT_ROLE_TOURIST);
    else if (ach == achieve_quest_knight)
        achievement_unlock(ach, UNLOCK_FIELD_ROLE, UNLOCKFEAT_ROLE_DRUID);
    else if (ach == achieve_quest_priest)
        achievement_unlock(ach, UNLOCK_FIELD_ROLE, UNLOCKFEAT_ROLE_CONVICT);
    else if (ach == achieve_quest_caveman)
        achievement_unlock(ach, UNLOCK_FIELD_ROLE, UNLOCKFEAT_ROLE_CONVICT);
    else if (ach == achieve_quest_healer)
        achievement_unlock(ach, UNLOCK_FIELD_OPT, UNLOCKFEAT_OPT_PERMABLIND);
    else if (ach == achieve_quest_valkyrie)
        achievement_unlock(ach, UNLOCK_FIELD_OPT, UNLOCKFEAT_OPT_PERMAHALLU);
    else if (ach == achieve_quest_samurai)
        achievement_unlock(ach, UNLOCK_FIELD_OPT, UNLOCKFEAT_OPT_PERMAGLIB);
    else if (ach == achieve_quest_archeologist)
        achievement_unlock(ach, UNLOCK_FIELD_OPT, UNLOCKFEAT_OPT_PERMACONF);
    else if (ach == achieve_quest_tourist)
        achievement_unlock(ach, UNLOCK_FIELD_OPT, UNLOCKFEAT_OPT_CHALLENGE);
    else if (ach == achieve_quest_convict)
        achievement_unlock(ach, UNLOCK_FIELD_OPT, UNLOCKFEAT_OPT_CHALLENGE);
    else if (ach == achieve_quest_druid)
        achievement_unlock(ach, UNLOCK_FIELD_OPT, UNLOCKFEAT_OPT_POLYINIT);

    else {
        pline(msgc_debug, "Unhandled achievement: %d", ach);
    }
}

static void
achievement_unlock(enum achievement ach, int fieldidx, unsigned long fieldbit)
{
    if (fieldidx > UNLOCK_FIELD_MAX)
        panic("Out-of-bounds unlockable-feature field number, %d",
              fieldidx);

    if (achievement_field[fieldidx] & fieldbit)
        return; /* Already unlocked, nothing to do */

    achievement_field[fieldidx] |= fieldbit;

    /* The ascraces and ascroles fields don't directly represent
       unlockables as such; rather, these fields are for tracking,
       to enable the all-races and all-roles achievements to be
       calculated.  So don't report them as historic events. */
    if (!(fieldidx == UNLOCK_FIELD_ASCROLES ||
          fieldidx == UNLOCK_FIELD_ASCRACES))
        historic_event(FALSE, TRUE,
                       msgprintf("unlocked %s by %s.",
                                 explain_unlockable(fieldidx, fieldbit),
                                 explain_achievement(ach)));
    write_achievements();
}

static const char *
explain_unlockable(int fieldidx, unsigned long fieldbit)
{
    if (fieldidx > UNLOCK_FIELD_MAX)
        panic("Out-of-bounds unlockable-feature field number, %d",
              fieldidx);

    switch (fieldidx) {
    case UNLOCK_FIELD_ROLE:
        return explain_unlockable_role(fieldbit);
    case UNLOCK_FIELD_RACE:
        return explain_unlockable_race(fieldbit);
    case UNLOCK_FIELD_OPT:
        return explain_unlockable_option(fieldbit);
    case UNLOCK_FIELD_ASCROLES:
        impossible("explaining an ascrole");
        return msgprintf("%s ascension", explain_unlockable_role(fieldbit));
    case UNLOCK_FIELD_ASCRACES:
        impossible("explaining an ascrace");
        return msgprintf("%s ascension", explain_unlockable_race(fieldbit));
    default:
        impossible("unknown unlockable field index %d", fieldidx);
        return msgprintf("unknown achievement %lu in field %d",
                         fieldbit, fieldidx);
    }
}

const char *
explain_unlockable_role(unsigned long fieldbit)
{
    int i;
    if (fieldbit & UNLOCKFEAT_ALWAYSUNLOCKED) {
        impossible("explaining always-unlocked role");
        return "always-unlocked role";
    }
    
    for (i = 0; roles[i].name.m; i++) {
        if (fieldbit == roles[i].unlockconst) {
            return msgprintf("the %s role", roles[i].name.m);
        }
    }
    
    impossible("explain_unlockable_role fell through");
    return "a role";
}

const char *
explain_unlockable_race(unsigned long fieldbit)
{
    int i;
    if (fieldbit & UNLOCKFEAT_ALWAYSUNLOCKED) {
        impossible("explaining always-unlocked race");
        return "always-unlocked race";
    }
    
    for (i = 0; races[i].adj; i++) {
        if (fieldbit == races[i].raceunlconst) {
            return msgprintf("the %s race", races[i].adj);
        }
    }
    
    impossible("explain_unlockable_race fell through");
    return "a race";
}

const char *
explain_unlockable_option(unsigned long fieldbit)
{
    if (fieldbit & UNLOCKFEAT_ALWAYSUNLOCKED) {
        impossible("explaining always-unlocked option");
        return "always-unlocked option";
    }

    if      (fieldbit == UNLOCKFEAT_OPT_PERMABLIND)
        return "the permanent-blindness option";
    else if (fieldbit == UNLOCKFEAT_OPT_PERMAHALLU)
        return "the permanent-hallucination option";
    else if (fieldbit == UNLOCKFEAT_OPT_PERMACONF)
        return "the permanent-confusion option";
    else if (fieldbit == UNLOCKFEAT_OPT_PERMAGLIB)
        return "the permanent-greasy-fingers option";
    else if (fieldbit == UNLOCKFEAT_OPT_PERMAFUMBLE)
        return "the permanent-fumbling option";
    else if (fieldbit == UNLOCKFEAT_OPT_PERMALAME)
        return "the permanent-wounded-legs option";
    else if (fieldbit == UNLOCKFEAT_OPT_PERMABADLUCK)
        return "the permanent-bad-luck option";
    else if (fieldbit == UNLOCKFEAT_OPT_CHALLENGE)
        return "challenge mode";
    else if (fieldbit == UNLOCKFEAT_OPT_POLYINIT)
        return "the non-scoring polyinit mode";
    else if (fieldbit == UNLOCKFEAT_OPT_ONEHP)
        return "the single-hitpoint challenge option";
    
    impossible("explain_unlockable_option fell through");
    return "an option";
}

const char *
explain_achievement(enum achievement ach)
{
    if      (ach == achieve_stairs_normal)
        return "using stairs";
    else if (ach == achieve_stairs_branch)
        return "using long stairs";
    else if (ach == achieve_mines_temple)
        return "entering the Minetown temple";
    else if (ach == achieve_mines_end)
        return "reaching the bottom of the Mines";
    else if (ach == achieve_altar_buctest)
        return "identifying an item's beatitude on an altar";
    else if (ach == achieve_altar_sacrifice)
        return "offering a corpse on an altar";
    else if (ach == achieve_altar_convert)
        return "converting an altar's alignment";
    else if (ach == achieve_soko_stairs)
        return "using stairs in Sokoban";
    else if (ach == achieve_soko_complete)
        return "completing Sokoban without cheating";
    else if (ach == achieve_dig)
        return "digging";
    else if (ach == achieve_vault_gold)
        return "taking gold from a vault";
    else if (ach == achieve_sproom_zoo)
        return "entering a treasure zoo";
    else if (ach == achieve_sproom_throne)
        return "entering a throne room";
    else if (ach == achieve_sproom_lephall)
        return "entering a leprechaun hall";
    else if (ach == achieve_sproom_dragons)
        return "entering a dragon hall";
    else if (ach == achieve_portal)
        return "travelling via magic portal";
    else if (ach == achieve_kill_medusa)
        return "defeating Medusa";
    else if (ach == achieve_kill_nemesis)
        return "defeating a quest nemesis";
    else if (ach == achieve_kill_rodney)
        return "defeating the Wizard of Yendor";
    else if (ach == achieve_passtune)
        return "playing the passtune";
    else if (ach == achieve_valley_stairs)
        return "descending the stairs in the Valley of the Dead";
    else if (ach == achieve_holy_water)
        return "receiving holy water as a result of prayer";
    else if (ach == achieve_invocation)
        return "performing an arcane ritual";
    else if (ach == achieve_ascend)
        return "ascending to a higher plane of existence";
    else if (ach == achieve_ascend_conduct)
        return "ascending while keeping a conduct";
    else if (ach == achieve_ascend_conduct_med)
        return "ascending while keeping a moderately difficult conduct";
    else if (ach == achieve_ascend_challenge)
        return "ascending while facing a greater challenge";
    else if (ach == achieve_ascend_impaired)
        return "ascending with a permanent impairment";
    else if (ach == achieve_ascend_conduct_hard)
        return "ascending while facing a difficult combination of challenges";
    else if (ach == achieve_ascend_speedrun)
        return "ascending with a low turn count";
    else if (ach == achieve_ascend_allroles)
        return "ascending as every possible role";
    else if (ach == achieve_ascend_allraces)
        return "ascending as every possible race";
    else if (ach == achieve_quest_barb)
        return "qualifying for the Barbarian quest";
    else if (ach == achieve_quest_wizard)
        return "qualifying for the Wizard quest";
    else if (ach == achieve_quest_ranger)
        return "qualifying for the Ranger quest";
    else if (ach == achieve_quest_monk)
        return "qualifying for the Monk quest";
    else if (ach == achieve_quest_rogue)
        return "qualifying for the Rogue quest";
    else if (ach == achieve_quest_knight)
        return "qualifying for the Knight quest";
    else if (ach == achieve_quest_priest)
        return "qualifying for the Priest quest";
    else if (ach == achieve_quest_caveman)
        return "qualifying for the Caveman quest";
    else if (ach == achieve_quest_healer)
        return "qualifying for the Healer quest";
    else if (ach == achieve_quest_valkyrie)
        return "qualifying for the Valkyrie quest";
    else if (ach == achieve_quest_samurai)
        return "qualifying for the Samurai quest";
    else if (ach == achieve_quest_archeologist)
        return "qualifying for the Archeologist quest";
    else if (ach == achieve_quest_tourist)
        return "qualifying for the Tourist quest";
    else if (ach == achieve_quest_convict)
        return "qualifying for the Convict quest";
    else if (ach == achieve_quest_druid)
        return "qualifying for the Druid quest";

    impossible("explaining unknown achievement");
    return "unknown achievement";
}

const char *
achievements_filename(void)
{
    return msgprintf("%sachievements_%s.dat",
                     fqn_prefix[SCOREPREFIX], u.uplname);
}

void
read_achievements(void)
{
    const char *achfilename = achievements_filename();
    FILE *achfile;
    int i;

    achfile = fopen(achfilename, "r");

    for (i = 0; i <= UNLOCK_FIELD_MAX; i++) {
        if (achfile) {
            achievement_field[i] = (unsigned long)
                fgetc(achfile) +
                (fgetc(achfile) << 8) +
                (fgetc(achfile) << 16) +
                (fgetc(achfile) << 24);
        } else
            achievement_field[i] = 0;
        achievement_field[i] |= UNLOCKFEAT_ALWAYSUNLOCKED;
    }

    if (achfile)
        fclose(achfile);
}

void
write_achievements(void)
{
    const char *achfilename = achievements_filename();
    FILE *achfile;
    int i;

    achfile = fopen(achfilename, "w");

    if (!achfile) {
        impossible("Cannot write achievements file, %s.", achfilename);
        return;
    }

    for (i = 0; i <= UNLOCK_FIELD_MAX; i++) {
        achievement_field[i] |= UNLOCKFEAT_ALWAYSUNLOCKED;
        fputc((achievement_field[i] % 256), achfile);
        fputc(((achievement_field[i] >> 8) % 256), achfile);
        fputc(((achievement_field[i] >> 16) % 256), achfile);
        fputc(((achievement_field[i] >> 24) % 256), achfile);
    }

    if (achfile)
        fclose(achfile);
}

/* End of file, achieve.c */
