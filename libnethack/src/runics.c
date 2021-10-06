/* vim:set cin ft=c sw=4 sts=4 ts=8 et ai cino=Ls\:0t0(0 : -*- mode:c;fill-column:80;tab-width:8;c-basic-offset:4;indent-tabs-mode:nil;c-file-style:"k&r" -*-*/
/* This file (runics.c) is in the public domain. */

#include "extern.h"
#include "global.h"
#include "runes.h"
#include "runics.h"


/* These MUST be in the same order as enum rune in runes.h: */
const char *const rune_name[] = {
    0, /* no rune at all, or Elvish runes on "runed" things */
    "Nothing", /* An actual rune, symbolizing the concept of Nothing */
    "Fire", "Cold", "Sleep", "Poison", "Lightning",
    "Acid", "Magic", "Storm", "Blessing", "Curse",
    "Action", "Damage", "Cancellation", "Awareness", "Polymorph",
    "Teleportation", "Draining", "Warning", "Accuracy", "Peace",
    /* If any TBA runes are generated, it's an error. */
    "Error 1", "Error 2", "Error 3", "Error 4", "Error 5",
    "Error 6", "Error 7", "Error 8", "Error 9", "Error 10",
    "Error 11", "Error 12", "Error 13", "Error 14", "Error 15",
    "Error 16", "Error 17", "Error 18", "Error 19", "Error 20",
};

const char *const rune_appearance_pool[] = {
    "ancient rune",
    "angled rune",
    "animated rune",
    "baffling rune",
    "baroque rune",
    "beautiful rune",
    "bifurcated rune",
    "bourgeoisie rune",
    "captivating rune",
    "celtic rune",
    "changing rune",
    "circumscribed rune",
    "complex rune",
    "convoluted rune",
    "crooked rune",
    "cryptic rune",
    "curved rune",
    "dark rune",
    "decorative rune",
    "demibold rune",
    "dwarvish rune",
    "elaborate rune",
    "elegant rune",
    "embossed rune",
    "engraved rune",
    "entrancing rune",
    "esoteric rune",
    "familiar rune",
    "fancy rune",
    "fascinating rune",
    "florid rune",
    "gaudy rune",
    "gilded rune",
    "glowing rune",
    "gnomic rune",
    "gothic rune",
    "graphic rune",
    "grotesque rune",
    "hackneyed rune",
    "hallowed rune",
    "heathen rune",
    "horrific rune",
    "intricate rune",
    "impressive rune",
    "limned rune",
    "logographic rune",
    "looping rune",
    "magnificent rune",
    "medieval rune",
    "memorable rune",
    "milled rune",
    "morbid rune",
    "multidimensional rune",
    "mysterious rune",
    "narrow rune",
    "neat rune",
    "oblique rune",
    "obscure rune",
    "occidental rune",
    "occult rune",
    "old-fashioned rune",
    "ominous rune",
    "ornamented rune",
    "ornate rune",
    "ostentatious rune",
    "perplexing rune",
    "plain rune",
    "portentous rune",
    "raised rune",
    "random emoji",
    "recondite rune",
    "resplendent rune",
    "rotating rune",
    "rounded rune",
    "seriffed rune",
    "shifting rune",
    "simple rune",
    "square rune",
    "static rune",
    "straight rune",
    "strange rune",
    "stylized rune",
    "subtle rune",
    "symetrical rune",
    "tawdry rune",
    "torpid rune",
    "traditional rune",
    "ugly rune",
    "unchanging rune",
    "weird rune",
    "whimsical rune",
    0
};

xchar
rune_damage_type(enum rune r)
{
    switch (r) {
    case RUNE_FIRE:
        return AD_FIRE;
    case RUNE_COLD:
        return AD_COLD;
    case RUNE_SLEEP:
        return AD_SLEE;
    case RUNE_POISON:
        return AD_DRST;
    case RUNE_LIGHTNING:
        return AD_ELEC;
    case RUNE_ACID:
        return AD_ACID;
    case RUNE_MAGIC:
        return AD_MAGM;
    case RUNE_DRAIN:
        return AD_DRLI;
    case RUNE_PEACE:
        return AD_HEAL;
    case RUNE_CANCELLATION:  /* fall through */
    case RUNE_DAMAGE:        /* fall through */
    case RUNE_ACCURACY:      /* fall through */
    default:
        return AD_PHYS;
    }
}

/* Some runes can occur on any (non-artifact, non-magical, non-dragon)
   armor or weapon; some can occur on (non-artifact, non-magical,
   non-dragon) body armor and weapons; and some can only occur on
   (non-artifact) weapons. */
boolean
rune_can_occur(enum rune thisrune, enum objslot slot)
{
    switch (thisrune) {
    case RUNE_NONE:
    case RUNE_FIRE:
    case RUNE_COLD:
    case RUNE_SLEEP:
    case RUNE_POISON:
    case RUNE_LIGHTNING:
        return TRUE;
    case RUNE_ACID:
    case RUNE_MAGIC:
    case RUNE_STORM:
    case RUNE_BLESSING:
    case RUNE_CURSE:
        if ((slot == os_arm) || (slot == os_wep) ||
            (slot == os_quiver))
            return TRUE;
        else return FALSE;
    default:
        if ((slot == os_wep) || (slot == os_quiver))
            return TRUE;
        else return FALSE;
    }
}

enum objslot
runeslot(struct obj *otmp)
{
    if (!otmp) {
        impossible("runeslot(): object is invalid.");
        return FALSE;
    }
    return (is_launcher(otmp)) ? os_wep :
        (is_ammo(otmp)) ? os_quiver :
        (otmp->oclass == WEAPON_CLASS) ? os_wep :
        (is_weptool(otmp)) ? os_wep :
        (otmp->oclass == ARMOR_CLASS) ? objects[(otmp)->otyp].oc_armcat :
        os_invalid;
}

