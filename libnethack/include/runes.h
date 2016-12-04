/* vim:set cin ft=c sw=4 sts=4 ts=8 et ai cino=Ls\:0t0(0 : -*- mode:c;fill-column:80;tab-width:8;c-basic-offset:4;indent-tabs-mode:nil;c-file-style:"k&r" -*-*/
/* This file (runes.h) is in the public domain. */

#ifndef RUNES_H
#define RUNES_H

/* When editing this enum, rune_name in runics.c must be edited to match. */
enum rune {
    RUNE_NONE = 0, /* This one must be false in conditional context */
    RUNE_FIRE, /* weapon does fire damage, provides fire res when equipped,
                  boils liquids when dipped in them */
    RUNE_COLD, /* weapon does cold damage, provides cold res when equipped,
                  freezes liquids when dipped in them */
    RUNE_SLEEP, /* weapon puts targets to sleep; confers restful sleep
                   when equipped */
    RUNE_POISON, /* weapon perma-poisoned, provides poison res when equipped,
                    turns fruit juice to sickness when dipped in it */
    RUNE_LIGHTNING, /* weapon does shock damage,
                       provides shock res when equipped */
    RUNE_ACID, /* weapon does acid damage, provides acid res when equipped */
    RUNE_MAGIC, /* weapon does MAGC damage, provides magic res when equipped */
    RUNE_STORM, /* confers conflict and stormprone when equipped */
    RUNE_BLESSING, /* perma-blessed, blocks inventory cursing effects */
    RUNE_CURSE, /* curses itself and other inventory items when equipped */
    RUNE_ACTION, /* confers free action when equipped */
    RUNE_DAMAGE, /* weapon does 3/2 the damage as its non-runic equivalent */
    RUNE_CANCELLATION, /* weapon cancels monsters when it hits them,
                          cancels (most) potions when dipped in them */
    RUNE_AWARENESS, /* weapon confers auto-searching when equipped,
                       and possibly small-radius see invisible effects */
    RUNE_POLYMORPH, /* confers controlled polymorphitis when equipped */
    RUNE_TELEPORT, /* confers controlled teleportitis when equipped */
    RUNE_DRAIN, /* weapon drains levels, confers drain res when equipped */
    RUNE_WARNING, /* confers warning when equipped */
    RUNE_ACCURACY, /* weapon never misses */
    RUNE_PEACE, /* weapon never hits; improves evasion odds when equipped */
    RUNE_NOTHING, /* no special effect */
    /* If any new runes are added, note that by default they will only ever
       occur on weapons, unless rune_can_occur() says otherwise. */
};
#define RUNE_MAX RUNE_PEACE
/* Some weapons are described by the game as "runed" even if they
   don't have any of the above special runes.  In that case we just
   say that they have "elvish runes" on them that indicate the
   identity of the weapon smith who made them. */
#define RUNE_ELVISH RUNE_NONE

extern const char *const rune_name[];
extern const char *const rune_appearance_pool[];

#endif /* RUNES_H */

