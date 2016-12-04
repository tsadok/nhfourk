/* vim:set cin ft=c sw=4 sts=4 ts=8 et ai cino=Ls\:0t0(0 : -*- mode:c;fill-column:80;tab-width:8;c-basic-offset:4;indent-tabs-mode:nil;c-file-style:"k&r" -*-*/
/* This file (runics.c) is in the public domain. */

#include "extern.h"
#include "global.h"
#include "runes.h"
#include "runics.h"


/* These MUST be in the same order as enum rune in runes.h: */
const char *const rune_name[] = {
    0,
    "Fire", "Cold", "Sleep", "Poison", "Lightning",
    "Acid", "Magic", "Storm", "Blessing", "Curse",
    "Action", "Damage", "Cancellation", "Awareness", "Polymorph",
    "Teleportation", "Draining", "Warning", "Accuracy", "Peace",
    "Nothing",
};

const char *const rune_appearance_pool[] = {
    "ancient rune",
    "angled rune",
    "beautiful rune",
    "bifurcated rune",
    "circumscribed rune",
    "complex rune",
    "convoluted rune",
    "crooked rune",
    "cryptic rune",
    "curved rune",
    "dark rune",
    "dwarvish rune",
    "elaborate rune",
    "embossed rune",
    "engraved rune",
    "esoteric rune",
    "familiar rune",
    "fancy rune",
    "fascinating rune",
    "florid rune",
    "gaudy rune",
    "gilded rune",
    "glowing rune",
    "gnomic rune",
    "graphic rune",
    "grotesque rune",
    "intricate rune",
    "limned rune",
    "looping rune",
    "magnificent rune",
    "milled rune",
    "morbid rune",
    "narrow rune",
    "obscure rune",
    "occidental rune",
    "occult rune",
    "old-fashioned rune",
    "ominous rune",
    "ornate rune",
    "perplexing rune",
    "plain rune",
    "portentous rune",
    "raised rune",
    "recondite rune",
    "rounded rune",
    "seriffed rune",
    "simple rune",
    "square rune",
    "straight rune",
    "strange rune",
    "stylized rune",
    "symetrical rune",
    "traditional rune",
    "ugly rune",
    "weird rune",
    0
};

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

