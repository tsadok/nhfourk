/* vim:set cin ft=c sw=4 sts=4 ts=8 et ai cino=Ls\:0t0(0 : -*- mode:c;fill-column:80;tab-width:8;c-basic-offset:4;indent-tabs-mode:nil;c-file-style:"k&r" -*-*/
/* Last modified by Fredrik Ljungdahl, 2017-10-08 */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "alignrec.h" /* for mood rings */

/* KMH -- Copied from pray.c; this really belongs in a header file */
#define DEVOUT 14
#define STRIDENT 4

#define Your_Own_Role(mndx) \
        ((mndx) == urole.malenum || \
         (urole.femalenum != NON_PM && (mndx) == urole.femalenum))
#define Your_Own_Race(mndx) \
        ((mndx) == urace.malenum || \
         (urace.femalenum != NON_PM && (mndx) == urace.femalenum))

static const char readable[] = { ALL_CLASSES, 0 };
static const char all_count[] = { ALLOW_COUNT, ALL_CLASSES, 0 };

static void wand_explode(struct obj *);
static void do_class_genocide(void);
static void stripspe(struct obj *);
static void p_glow1(enum msg_channel, struct obj *);
static void p_glow2(enum msg_channel, struct obj *, const char *);
static void do_scroll_water(int, int, int, schar);
static void maybe_tame(struct monst *, struct obj *);

static void set_lit(int, int, void *);

int
doread(const struct nh_cmd_arg *arg)
{
    boolean confused, known;
    struct obj *scroll;
    const char *descr;

    known = FALSE;
    if (check_capacity(NULL))
        return 0;

    scroll = getargobj(arg, readable, "read");
    if (!scroll)
        return 0;

    /* outrumor has its own blindness check */
    if (scroll->otyp == FORTUNE_COOKIE) {
        pline(msgc_occstart,
              "You break up the cookie and throw away the pieces.");
        outrumor(bcsign(scroll), BY_COOKIE);
        if (!Blind)
            break_conduct(conduct_illiterate);
        useup(scroll);
        return 1;
    } else if (scroll->otyp == T_SHIRT) {
        static const char *const shirt_msgs[] = {       /* Scott Bigham */
            "I explored the Dungeons of Doom and all I got was this lousy "
                "T-shirt!",
            "Is that Mjollnir in your pocket or are you just happy to see me?",
            "It's not the size of your sword, it's how #enhance'd you are "
                "with it.",
            "Madame Elvira's House O' Succubi Lifetime Customer",
            "Madame Elvira's House O' Succubi Employee of the Month",
            "Ludios Vault Guards Do It In Small, Dark Rooms",
            "Yendor Military Soldiers Do It In Large Groups",
            "I survived Yendor Military Boot Camp",
            "Ludios Accounting School Intra-Mural Lacrosse Team",
            "Oracle(TM) Fountains 10th Annual Wet T-Shirt Contest",
            "Hey, black dragon!  Disintegrate THIS!",
            "I'm With Stupid -->",
            "Don't blame me, I voted for Izchak!",
            "Yendor Academy of Alchemy, Class of '47",
            "Zombies aren't so tough. I'm more scared of an archon apocalypse.",
            "Owlbears:  All they want is a hug.  Is that so bad?",
            "Lycanthropy Gives Me the Right To Bear Arms.", /* werebear arms */
            "I am the Very Model of a Pious Individual", /* Gilbert & Sullivan
                                                            via Yakko Warner */
            "I'm a Gnome Lord.  Wanna Touch My Stone?",
            "Frobozz Magic Shirt Company", /* Quendor */
            "It's All About the Flatheads", /* i.e., the zorkmids */
            "Don't Panic",      /* HHGTTG */
            "Furinkan High School Athletic Dept.",      /* Ranma 1/2 */
            "Hel-LOOO, Nurse!", /* Animaniacs */
            "=^.^=", /* Japanese-style (non-sideways) emoticon. */
            "100% goblin hair - do not wash",
            "Aberzombie and Fitch",
            "Don't ask me, I only adventure here",
            "d, your dog or a killer?",
            "FREE PUG AND NEWT!",
            "Go team ant!",
            "Got newt?",
            "Hello, my darlings!", /* Charlie Drake */
            "Hey! Nymphs! Steal This T-Shirt!",
            "I <3 Dungeon of Doom",
            "I <3 Maud",
            "I am a Barbarian. If you see me running, try to keep up.",
            "I am not a pack rat - I am a collector",
            "I bounced off a rubber tree",  /* Monkey Island */
            "Plunder Island Brimstone Beach Club", /* Monkey Island */
            "If you can read this, I can hit you with my polearm",
            "I'm confused!",
            "I scored with the princess",
            "I want to live forever or die in the attempt.",
            "Lichen Park",
            "LOST IN THOUGHT - please send search party",
            "Meat is Mordor",
            "Minetown Better Business Bureau",
            "Minetown Watch",
            "Protection Racketeer",
            "Real men love Crom",
            "Somebody stole my Mojo!",
            "The Hellhound Gang",
            "The Werewolves",
            "They Might Be Storm Giants",
            "Weapons don't kill people, I kill people",
            "White Zombie",
            "You're killing me!",
            "Anhur State University - Home of the Fighting Fire Ants!",
            "FREE HUGS",
            "Serial Ascender",
            "Real men are valkyries",
            "Real men play with the Evil Patch applied",
            "Young Men's Cavedigging Association",
            "Occupy Fort Ludios",
            "I couldn't afford this T-shirt so I stole it!",
            "Mind flayers suck",
            "I'm not wearing any pants",
            "Down with the living!",
            "Vegetarian",
            "Vegan and Proud",
            "Hello, I'm War",
            "Sredni Vashtar went forth.  " /* Saki short story */
                "His thoughts were red and his teeth were white.",
            "Keep calm and explore the dungeon.",
            "One does not simply kill the Wizard of Yendor.",
            "I killed the Wizard of Yendor.  (He got over it.)",
            "Giants are Tossers!", /* -- elenmirie */
        };
        const char *buf;
        int erosion;

        if (Blind) {
            pline(msgc_cancelled, "You can't feel any Braille writing.");
            return 0;
        }
        break_conduct(conduct_illiterate);
        pline_implied(msgc_info, "It reads:");
        buf = shirt_msgs[scroll->o_id % SIZE(shirt_msgs)];
        erosion = greatest_erosion(scroll);
        if (erosion)
            buf = eroded_text(buf,
                              (int)(strlen(buf) * erosion / (2 * MAX_ERODE)),
                              scroll->o_id ^ (unsigned)u.ubirthday);
        pline(msgc_info, "\"%s\"", buf);
        return 1;
    } else if (scroll->oclass == RING_CLASS) {
        const char *dscr = OBJ_DESCR(objects[scroll->otyp]);
        if (Blind) {
            pline(msgc_cancelled, "Being blind, you cannot read the %s.",
                  ((!strcmp(dscr, "mood")) ? "stone's color" :
                   (!strcmp(dscr, "class")) ? "year" : "engraving"));
            return 0;
        }
        if (!strcmp(dscr, "mood")) {
            if (scroll != EQUIP(os_ringl) && scroll != EQUIP(os_ringr)) {
                pline(msgc_actionboring, "The stone is %s.", hcolor("black"));
            } else {
                pline(msgc_info, "The stone is %s.", hcolor(
                          scroll->cursed ? "purple" :
                          u.ualign.record >= PIOUS     ? "blue"   :
                          u.ualign.record >= DEVOUT    ? "green"  :
                          u.ualign.record >= STRIDENT  ? "yellow" :
                          u.ualign.record >= HALTINGLY ? "orange" :
                          u.ualign.record >= NOMINALLY ? "red"    : "black"));
            }
            return 1;
        } else if (!strcmp(dscr, "class")) {
            break_conduct(conduct_illiterate);
            pline(msgc_actionok, "Yendor Academy, Class of '%02d",
                  (scroll->o_id % 100));
            return 1;
        } else if ((!strcmp(dscr, "intaglio")) ||
                   (!strcmp(dscr, "signet"))) {
            const char *const ring_engr[] = {
                "the symbol of House Dyne", /* Jackson's Whole */
                "the griffin of House Harkonnen", /* Dune */
                "the personal sigil of Croesus", /* Fort Ludios */
                "the Frobozz Magic Ring Company logo", /* Quendor / GUE */
                "a heptagram", /* AceHack, dNetHack */
                "the elemental symbol of earth",
                "the elemental symbol of water",
                "the elemental symbol of air",
                "the elemental symbol of fire",
                "the elemental symbol of aether",
                "a likeness of the original owner",
                "a likeness of the original owner's spouse",
                "a likeness of the original owner's father",
                "a likeness of the original owner's mother",
                "the ring maker's mark",
                "the ring makers' guild's seal of quality",
                "a ward against nightmares",
                "a ward against boils and bedsores",
                "a ward against the destruction of the universe",
                "a ward against bad morale",
                msgprintf("an alchemical formula: %s + %s => %s",
                          OBJ_DESCR(objects[POT_HEALING]),
                          OBJ_DESCR(objects[POT_SPEED]),
                          OBJ_DESCR(objects[POT_FULL_HEALING])),
                msgprintf("an alchemical formula: %s + %s => %s",
                          OBJ_DESCR(objects[POT_HEALING]),
                          OBJ_DESCR(objects[POT_GAIN_LEVEL]),
                          OBJ_DESCR(objects[POT_EXTRA_HEALING])),
                msgprintf("an alchemical formula: %s + %s => %s",
                          OBJ_DESCR(objects[POT_FULL_HEALING]),
                          OBJ_DESCR(objects[POT_GAIN_ENERGY]),
                          OBJ_DESCR(objects[POT_GAIN_ABILITY])),
            };
            break_conduct(conduct_illiterate);
            pline(msgc_actionok, "The %s is engraved with %s.",
                  (!strcmp(dscr, "intaglio") ?
                   "underside of the stone" : "face of the ring"),
                  ring_engr[scroll->o_id % SIZE(ring_engr)]);
            return 1;
        } else {
            pline(msgc_cancelled, "This ring bears no inscription.");
            return 0;
        }
    } else if (scroll->orune) {
        break_conduct(conduct_illiterate);
        if (Hallucination) {
            pline(msgc_actionok, "That is one %s rune, %s.",
                  ((!((moves + 0) % 7)) ? "totally gnarly" :
                   (!((moves + 1) % 7)) ? "way groovy" :
                   (!((moves + 2) % 7)) ? "psychedelic" :
                   (!((moves + 3) % 7)) ? "mind-blowing" :
                   (!((moves + 4) % 7)) ? msgprintf("seriously %s",
                                                    hcolor(NULL)) :
                   (!((moves + 5) % 7)) ? "majorly rad" : "trippy"),
                  ((u.ufemale && !(moves % 3)) ? "dudette" : "dude"));
        } else if (u.rune_known[scroll->orune]) {
            if (Blind) {
                pline(msgc_info, "You can feel a Rune of %s on the %s.",
                      rune_name[scroll->orune],
                      xname_sansrunic(scroll));
            } else {
                pline(msgc_info, "There is a Rune of %s on the %s.",
                      rune_name[scroll->orune],
                      xname_sansrunic(scroll));
            }
        } else {
            if (Blind) {
                pline(msgc_info, "You can feel a rune on the %s, "
                      "but you cannot guess its meaning.",
                      xname_sansrunic(scroll));
            } else {
                pline(msgc_info, "You do not know the meaning of the %s.",
                      rune_appearance_pool[u.rune_appearance[scroll->orune]]);
            }
        }
        return 1;
    } else if ((descr = OBJ_DESCR(objects[scroll->otyp])) &&
               !strncmpi("runed", descr, 5)) {
        /* TODO: if (Hallucination) { ... ? */
        if (Blind) {
            pline(msgc_cancelled, "You cannot see the runes.");
            return 0;
        } else if (Race_if(PM_ELF)) {
            /* Elvish runes, if you can read them, tell who made the item,
               where, and when.  For practical reasons, we use the object ID.
               This creates minor weirdness if anyone is paying very close
               attention, particularly when joining and splitting stacks; this
               is better than making items refuse to stack just because of
               non-matching Elvish runes, which have no gameplay importance.
               Note that the number of cities, the number of monarchs, the
               number of makers, and the number of possible years numbers are
               all relatively prime, so all combinations are possible.
               Assigning specific monarchs and makers to specific cities
               and years is the kind of world building Dwarf Fortress would
               do, but that's too much hassle for our purposes here.  However,
               some of these names do have actual meanings in Quenya. */
            static const char *const elvish_city[] = { /* 11 of these */
                "Aelanolir", "Arlembrond", "Brellin", "Cielerond",
                "Citimeokal", "Erebrast", "Kalopele", "Mardiesse i Salque",
                "Melcataure", "Pelerond", "Taurepeler", "Vanaost" };
            static const char *const elvish_monarch[] = { /* 17 */
                "Alasseheru", "King Linneanahrive", "Blemonir Maraingole",
                "King Maraoma", "Queen Melimakamba", "King Menonaicele",
                "Valief Meryale", "King Nenaranta", "King Nurosenda",
                "King Palimerima", "Finlanir Poicacsa", "Oleros Ranetavari",
                "King Serlonde and Queen Lintefiriel", "Henilwe Sardatolde",
                "Lord Sinyapele", "Prince Sinyekal", "King Turemardo" };
            static const char *const elvish_maker[] = { /* 65 = 5 * 13 */
                "Aerod", "Aeras", "Amhel", "Amlenoth", "Amnir", "Amwe",
                "Anelas", "Anelrian", "Arlebrimbor", "Baranhel", "Belin",
                "Brimeloth", "Celaeros", "Celeuor", "Celeurog", "Celeuromil",
                "Daeleth", "Deneuril", "Duilibrian", "Earuillien", "Earanhel",
                "Ederas", "Eldanekil", "Elmanir", "Eleurin", "Ennaedor",
                "Eolandeor", "Eorelin", "Finleroth", "Finriel", "Galaros",
                "Garlemneth", "Gelrindhe", "Glanarwe", "Glaurin", "Gorodir",
                "Henarwe", "Ilonwe", "Ilimdir", "Laniras", "Lebriul",
                "Lindarion", "Marlinwe", "Naliador", "Nerlena", "Nirdan",
                "Oldrion", "Oroleth", "Osmurien", "Penrelhas", "Penthuel",
                "Quenrelwe", "Romil", "Ruidhir", "Ruilnias", "Saelen",
                "Saunelos", "Tabreth", "Tabrion", "Tenriel", "Tenrilos",
                "Thargilranos", "Thelimnir", "Tuillen", "Valrion" };
            int nseed = ((unsigned) u.ubirthday / 257U);
            int year  = 7 + ((nseed + scroll->o_id + scroll->otyp) % 478);
            const char *itemword = ((scroll->oclass == WAND_CLASS) ? "wand" :
                                    (scroll->otyp == ELVEN_ARROW) ? "arrow" :
                                    (scroll->otyp == ELVEN_DAGGER ||
                                     is_sword(scroll)) ? "blade" :
                                    (scroll->oclass == WEAPON_CLASS) ?
                                    "weapon" : xname_sansrunic(scroll));
            pline(msgc_info, "%s %s %s made in %s,"
                  " in year %d of the reign of %s, by %s the %s.",
                  ((scroll->quan == 1) ? "This" : "These"),
                  ((scroll->quan == 1) ? itemword: makeplural(itemword)),
                  ((scroll->quan == 1) ? "was" : "were"),
                  elvish_city[(nseed + scroll->o_id) % SIZE(elvish_city)], year,
                  elvish_monarch[(nseed + scroll->o_id) % SIZE(elvish_monarch)],
                  elvish_maker[(nseed + scroll->o_id) % SIZE(elvish_maker)],
                  ((scroll->otyp == ELVEN_ARROW) ? "fletcher" :
                   (objects[scroll->otyp].oc_material == WOOD) ? "woodcarver" :
                   (scroll->oclass == WAND_CLASS) ? "artificer" :
                   (scroll->oclass == WEAPON_CLASS ? "weaponsmith" :
                    "artisan")));
        } else {
            /* Our character can't read Elvish; but recognizing them as elvish
               runes is enough to type-identify the item as of Elvish origin. */
            pline(msgc_info, "Elvish runes run %s %s.",
                  (scroll->otyp == ELVEN_BOW ?
                   "up the arc of" : "the length of the"),
                  (scroll->otyp == ELVEN_ARROW ||
                   scroll->otyp == ELVEN_SPEAR) ? "shaft" :
                  (scroll->otyp == ELVEN_DAGGER || is_sword(scroll)) ? "blade" :
                  (objects[scroll->otyp].oc_material == WOOD) ? "wood" :
                  (scroll->oclass == WAND_CLASS) ? "wand" :
                  xname_sansrunic(scroll));
            makeknown(scroll->otyp);
        }
        return 1;
    } else if (scroll->oclass != SCROLL_CLASS &&
               scroll->oclass != SPBOOK_CLASS) {
        pline(msgc_cancelled, "That is a silly thing to read.");
        return 0;
    } else if (Blind) {
        const char *what = 0;

        if (scroll->oclass == SPBOOK_CLASS)
            what = "mystic runes";
        else if (!scroll->dknown)
            what = "formula on the scroll";
        if (what) {
            pline(msgc_cancelled, "Being blind, you cannot read the %s.", what);
            return 0;
        }
    }

    /* TODO: When we add a conduct assistance option, add a condition here.  Or
       better yet, check for all reading of things. */
    if (scroll->otyp == SPE_BOOK_OF_THE_DEAD &&
        !u.uconduct[conduct_illiterate] &&
        yn("You are currently illiterate and could invoke the Book "
           "instead. Read it nonetheless?") != 'y')
        return 0;

    if (scroll->otyp != SPE_BLANK_PAPER && scroll->otyp != SCR_BLANK_PAPER)
        break_conduct(conduct_illiterate);

    confused = (Confusion != 0);
    if (scroll->oclass == SPBOOK_CLASS)
        return study_book(scroll, arg);

    scroll->in_use = TRUE;      /* scroll, not spellbook, now being read */
    if (scroll->otyp != SCR_BLANK_PAPER) {
        if (Blind)
            pline(msgc_occstart,
                  "As you %s the formula, the scroll disappears.",
                  (is_silent(youmonst.data) || Strangled) ? "cogitate on" :
                  "pronounce");
        else
            pline(msgc_occstart, "As you read the scroll, it disappears.");
        if (confused) {
            if (Hallucination)
                pline(msgc_substitute, "Being so trippy, you screw up...");
            else
                pline(msgc_substitute,
                      "Being confused, you mis%s the magic words...",
                      (is_silent(youmonst.data) || Strangled) ? "understand" :
                      "pronounce");
        }
    }
    if (!seffects(scroll, &known)) {
        if (!objects[scroll->otyp].oc_name_known) {
            if (known) {
                makeknown(scroll->otyp);
                more_experienced(0, 10);
            } else if (!objects[scroll->otyp].oc_uname)
                docall(scroll);
        }
        if (scroll->otyp != SCR_BLANK_PAPER)
            useup(scroll);
        else
            scroll->in_use = FALSE;
    }
    return 1;
}

static void
stripspe(struct obj *obj)
{
    if (obj->blessed)
        pline(msgc_noconsequence, "Nothing happens.");
    else {
        if (obj->spe > 0) {
            obj->spe = 0;
            if (obj->otyp == OIL_LAMP || obj->otyp == BRASS_LANTERN)
                obj->age = 0;
            pline(msgc_itemloss, "Your %s %s briefly.", xname(obj),
                  otense(obj, "vibrate"));
        } else
            pline(msgc_noconsequence, "Nothing happens.");
    }
}

static void
p_glow1(enum msg_channel msgc, struct obj *otmp)
{
    pline(msgc, "Your %s %s briefly.", xname(otmp),
          otense(otmp, Blind ? "vibrate" : "glow"));
}

static void
p_glow2(enum msg_channel msgc, struct obj *otmp, const char *color)
{
    pline(msgc, "Your %s %s%s%s for a moment.", xname(otmp),
          otense(otmp, Blind ? "vibrate" : "glow"), Blind ? "" : " ",
          Blind ? "" : hcolor(color));
}

/* Is the object chargeable?  For purposes of inventory display; it is
   possible to be able to charge things for which this returns FALSE. */
boolean
is_chargeable(struct obj *obj)
{
    if (obj->oclass == WAND_CLASS)
        return TRUE;
    /* known && !uname is possible after amnesia/mind flayer */
    if (obj->oclass == RING_CLASS)
        return (boolean) ((objects[obj->otyp].oc_charged &&
                           ((objects[obj->otyp].oc_name_known &&
                             obj->dknown) || obj->known)) ||
                          (obj->dknown &&
                           objects[obj->otyp].oc_uname &&
                           !objects[obj->otyp].oc_name_known));
    if (is_weptool(obj))        /* specific check before general tools */
        return FALSE;

    /* Magic lamps can't reasonably be recharged. But they must be shown in the
       list, to avoid telling them apart from brass lamps with a ?oC. */
    if (obj->oclass == TOOL_CLASS)
        return (boolean) (objects[obj->otyp].oc_charged ||
                          obj->otyp == BRASS_LANTERN || obj->otyp == OIL_LAMP ||
                          obj->otyp == MAGIC_LAMP);

    return FALSE;       /* why are weapons/armor considered charged anyway? */
}

/*
 * recharge an object; curse_bless is -1 if the recharging implement
 * was cursed, +1 if blessed, 0 otherwise.
 */
void
recharge(struct obj *obj, int curse_bless)
{
    int n;
    boolean is_cursed, is_blessed;

    is_cursed = curse_bless < 0;
    is_blessed = curse_bless > 0;

    /* Scrolls of charging now ID charge count, as well as doing the charging,
       unless cursed. */
    if (!is_cursed)
        obj->known = 1;

    if (obj->oclass == WAND_CLASS) {
        /* undo any prior cancellation, even when is_cursed */
        if (obj->spe == -1)
            obj->spe = 0;

        /*
         * Recharging might cause wands to explode.
         *
         *  v = number of previous recharges
         *        v = percentage chance to explode on this attempt
         *                v = cumulative odds for exploding
         *  0 :   0       0
         *  1 :   0.29    0.29
         *  2 :   2.33    2.62
         *  3 :   7.87   10.28
         *  4 :  18.66   27.02
         *  5 :  36.44   53.62
         *  6 :  62.97   82.83
         *  7 : 100     100
         *
         * No custom RNG, because we can't know whether the user will recharge
         * the same wand repeatedly, or recharge a number of different wands
         * separately, or a mixture, and this woud throw off the RNG.
         */
        n = (int)obj->recharged;
        if (obj->otyp == WAN_WISHING) { /* never recharge these */
            if (!Deaf)
                verbalize(msgc_itemloss,
                          "Ixnay on the wishing for more wishes.");
            else
                pline(msgc_itemloss, "Nothing seems to happen.");
            return;
        } else if ((n > 0) && (n * n * n > rn2(7 * 7 * 7))) {
            /* recharge_limit */
            wand_explode(obj);
            return;
        }
        /* didn't explode, so increment the recharge count */
        obj->recharged = (unsigned)(n + 1);

        /* now handle the actual recharging */
        if (is_cursed) {
            stripspe(obj);
        } else {
            int lim = (obj->otyp == WAN_WISHING) ? 3 :
                (objects[obj->otyp].oc_dir != NODIR) ? 8 : 15;

            n = (lim == 3) ? 3 : rn1(5, lim + 1 - 5);
            if (!is_blessed)
                n = rnd(n);

            if (obj->spe < n)
                obj->spe = n;
            else
                obj->spe++;
            if (obj->otyp == WAN_WISHING && obj->spe > 3) {
                wand_explode(obj);
                return;
            }
            if (obj->spe >= lim)
                p_glow2(msgc_itemrepair, obj, "blue");
            else
                p_glow1(msgc_itemrepair, obj);
        }

    } else if (obj->oclass == RING_CLASS && objects[obj->otyp].oc_charged) {
        /* charging does not affect ring's curse/bless status */
        int s = is_blessed ? rnd(3) : is_cursed ? -rnd(2) : 1;
        boolean is_on = (obj == uleft || obj == uright);

        /* destruction depends on current state, not adjustment */
        if (obj->spe > rn2(7) || obj->spe <= -5) {
            pline(msgc_itemloss, "Your %s %s momentarily, then %s!",
                  xname(obj), otense(obj, "pulsate"), otense(obj, "explode"));
            if (is_on)
                setunequip(obj);
            s = rnd(3 * abs(obj->spe)); /* amount of damage */
            useup(obj);
            losehp(s, killer_msg(DIED, "an exploding ring"));
        } else {
            enum objslot slot = obj == uleft ? os_ringl : os_ringr;

            pline(s > 0 ? msgc_itemrepair : msgc_itemloss,
                  "Your %s spins %sclockwise for a moment.", xname(obj),
                  s < 0 ? "counter" : "");
            /* cause attributes and/or properties to be updated */
            if (is_on)
                setunequip(obj);
            obj->spe += s;      /* update the ring while it's off */
            if (is_on)
                setequip(slot, obj, em_silent);
            /* oartifact: if a touch-sensitive artifact ring is ever created
               the above will need to be revised */
        }

    } else if (obj->oclass == TOOL_CLASS) {
        int rechrg = (int)obj->recharged;

        /* tools don't have a limit, but the counter used does */
        if (rechrg < 7) /* recharge_limit */
            obj->recharged++;

        switch (obj->otyp) {
        case BELL_OF_OPENING:
            if (is_cursed)
                stripspe(obj);
            else if (is_blessed)
                obj->spe += rnd(3);
            else
                obj->spe += 1;
            if (obj->spe > 5)
                obj->spe = 5;
            break;
        case MAGIC_MARKER:
        case TINNING_KIT:
        case EXPENSIVE_CAMERA:
            if (is_cursed)
                stripspe(obj);
            else if (rechrg && obj->otyp == MAGIC_MARKER) {
                /* previously recharged */
                obj->recharged = 1;     /* override increment done above */
                if (obj->spe < 3)
                    pline(msgc_failcurse,
                          "Your marker seems permanently dried out.");
                else
                    pline(msgc_failcurse, "Nothing happens.");
            } else if (is_blessed) {
                n = rn1(16, 15);        /* 15..30 */
                if (obj->spe + n <= 50)
                    obj->spe = 50;
                else if (obj->spe + n <= 75)
                    obj->spe = 75;
                else {
                    int chrg = (int)obj->spe;

                    if ((chrg + n) > 127)
                        obj->spe = 127;
                    else
                        obj->spe += n;
                }
                p_glow2(msgc_itemrepair, obj, "blue");
            } else {
                n = rn1(11, 10);        /* 10..20 */
                if (obj->spe + n <= 50)
                    obj->spe = 50;
                else {
                    int chrg = (int)obj->spe;

                    if ((chrg + n) > 127)
                        obj->spe = 127;
                    else
                        obj->spe += n;
                }
                p_glow2(msgc_itemrepair, obj, "white");
            }
            break;
        case OIL_LAMP:
        case BRASS_LANTERN:
            if (is_cursed) {
                stripspe(obj);
                if (obj->lamplit) {
                    if (!Blind)
                        pline(msgc_consequence, "%s out!", Tobjnam(obj, "go"));
                    end_burn(obj, TRUE);
                }
            } else if (is_blessed) {
                obj->spe = 1;
                obj->age = 1500;
                p_glow2(msgc_itemrepair, obj, "blue");
            } else {
                obj->spe = 1;
                obj->age += 750;
                if (obj->age > 1500)
                    obj->age = 1500;
                p_glow1(msgc_itemrepair, obj);
            }
            break;
        case CRYSTAL_BALL:
            if (is_cursed)
                stripspe(obj);
            else if (is_blessed && obj->spe <= 25) {
                obj->spe += 25;
                p_glow2(msgc_itemrepair, obj, "blue");
            } else {
                if (obj->spe <=25) {
                    obj->spe += 10;
                    p_glow1(msgc_itemrepair, obj);
                } else
                    pline(msgc_failcurse, "Nothing happens.");
            }
            break;
        case HORN_OF_PLENTY:
        case BAG_OF_TRICKS:
        case CAN_OF_GREASE:
            if (is_cursed)
                stripspe(obj);
            else if (is_blessed) {
                if (obj->spe <= 10)
                    obj->spe += rn1(10, 6);
                else
                    obj->spe += rn1(5, 6);
                if (obj->spe > 50)
                    obj->spe = 50;
                p_glow2(msgc_itemrepair, obj, "blue");
            } else {
                obj->spe += rnd(5);
                if (obj->spe > 50)
                    obj->spe = 50;
                p_glow1(msgc_itemrepair, obj);
            }
            break;
        case MAGIC_FLUTE:
        case MAGIC_HARP:
        case FROST_HORN:
        case FIRE_HORN:
        case DRUM_OF_EARTHQUAKE:
            if (is_cursed) {
                stripspe(obj);
            } else if (is_blessed) {
                obj->spe += dice(2, 4);
                if (obj->spe > 20)
                    obj->spe = 20;
                p_glow2(msgc_itemrepair, obj, "blue");
            } else {
                obj->spe += rnd(4);
                if (obj->spe > 20)
                    obj->spe = 20;
                p_glow1(msgc_itemrepair, obj);
            }
            break;
        default:
            goto not_chargable;
        }       /* switch */

    } else {
    not_chargable:
        pline(msgc_badidea, "You have a feeling of loss.");
    }
}


/* randomize the given list of numbers  0 <= i < count */
/* This was used by the amnesia code.
static void
randomize(int *indices, int count)
{
    int i, iswap, temp;

    for (i = count - 1; i > 0; i--) {
        if ((iswap = rn2(i + 1)) == i)
            continue;
        temp = indices[i];
        indices[i] = indices[iswap];
        indices[iswap] = temp;
    }
}
*/

/* monster is hit by scroll of taming's effect */
static void
maybe_tame(struct monst *mtmp, struct obj *sobj)
{
    if (sobj->cursed) {
        setmangry(mtmp);
    } else {
        if (mtmp->isshk)
            make_happy_shk(mtmp, FALSE);
        else if (!resist(mtmp, sobj->oclass, 0, NOTELL))
            tamedog(mtmp, NULL);
    }
}

void
do_uncurse_effect(boolean blessedeffect, boolean confusable)
{
    struct obj *obj;

    for (obj = invent; obj; obj = obj->nobj) {
        long wornmask;

        /* gold isn't subject to cursing and blessing */
        if (obj->oclass == COIN_CLASS)
            continue;

        wornmask = obj->owornmask & W_MASKABLE;
        if (wornmask && !blessedeffect) {
            /* handle a couple of special cases; we don't allow
               auxiliary weapon slots to be used to artificially
               increase number of worn items */
            if (obj == uswapwep) {
                if (!u.twoweap)
                    wornmask = 0L;
            } else if (obj == uquiver) {
                if (obj->oclass == WEAPON_CLASS) {
                    /* mergeable weapon test covers ammo, missiles, 
                       spears, daggers & knives */
                    if (!objects[obj->otyp].oc_merge)
                        wornmask = 0L;
                } else if (obj->oclass == GEM_CLASS) {
                    /* possibly ought to check whether alternate
                       weapon is a sling... */
                    if (!uslinging())
                        wornmask = 0L;
                } else {
                    /* weptools don't merge and aren't reasonable
                       quivered weapons */
                    wornmask = 0L;
                }
            }
        }

        if (blessedeffect || wornmask || obj->otyp == LOADSTONE ||
            (obj->otyp == LEASH && obj->leashmon)) {
            if (confusable && Confusion)
                blessorcurse(obj, 2, rng_main);
            else
                uncurse(obj);
        }
    }
    update_inventory();
}

void
do_scroll_water(int cx, int cy, int radius, schar newterrain)
{
    int x = cx, y = cy, tries = 100, i;
    boolean killengulfer = FALSE;
    struct monst *mtmp;
    struct trap  *ttmp;
    const char *liqname = (newterrain == LAVAPOOL) ? "lava" : "water";
    /* Currently no traps require an(), or we'd need an_your() */

    while ((radius > 0) && (tries-- > 0) &&
           ((x == cx && y == cy) ||
            !isok(x,y) ||
            abs(x - cx) + abs(y - cy) > radius)) {
        x = cx - radius + rn2(radius * 2);
        y = cy - radius + rn2(radius * 2);
    }
    if (Engulfed) {
        if (noncorporeal(u.ustuck->data)) {
            if (Hallucination)
                pline(msgc_combatimmune, /* sort of */
                      "Oh, the leaks of Egypt!");
            else if (!Blind)
                pline(msgc_combatimmune, /* sort of */
                      "The %s gushes right out through %s.",
                      liqname, mon_nam(u.ustuck));
            else
                pline(msgc_combatimmune, /* sort of */
                      "It doesn't seem as %s in here as you expected.",
                      ((newterrain == LAVAPOOL) ? "hot" : "wet"));
            killengulfer = FALSE;
        } else if (flaming(u.ustuck->data)) {
            if (newterrain == LAVAPOOL) {
                u.ustuck->mhpmax += 2;
                u.ustuck->mhp = u.ustuck->mhpmax;
                pline(msgc_consequence, "%s heats up!", Monnam(u.ustuck));
                killengulfer = FALSE;
            } else {
                if (Hallucination)
                    pline(msgc_kill,
                          "Billy Joel lyrics run through your mind.");
                else 
                    pline(msgc_kill, "The water extinguishes %s!",
                          mon_nam(u.ustuck));
                killengulfer = TRUE;
            }
        } else if (monsndx(u.ustuck->data) == PM_FOG_CLOUD ||
                   monsndx(u.ustuck->data) == PM_STEAM_VORTEX) {
            /* Why do fog clouds count as whirly?
               Nevermind, they're a special case here anyway. */
            if (newterrain == LAVAPOOL) {
                pline(msgc_consequence, "%s heats up!", Monnam(u.ustuck));
                newcham(u.ustuck, &mons[PM_FIRE_VORTEX], FALSE, FALSE);
            } else {
                if (Hallucination)
                    pline(msgc_consequence, "You feel a rainstorm coming on!");
                else
                    pline(msgc_consequence,
                          "%s feels denser as it absorbs the %s.",
                          Monnam(u.ustuck), liqname);
            }
            u.ustuck->mhpmax += 2;
            u.ustuck->mhp = u.ustuck->mhpmax;
            killengulfer = FALSE;
        } else if (is_whirly(u.ustuck->data)) {
            /* TODO: should ice vortices be special-cased immune? */
            pline(msgc_kill, "%s slows as it takes up the %s, stops whirling,"
                  " and lets you go.", Monnam(u.ustuck), liqname);
            killengulfer = TRUE;
        } else if ((is_swimmer(u.ustuck->data) || amphibious(u.ustuck->data) ||
                    breathless(u.ustuck->data)) && !(newterrain == LAVAPOOL)) {
            /* The engulfer survives, but what happens to YOU in there? */
            pline(msgc_badidea, "The %s fills with water!",
                  mbodypart(u.ustuck, STOMACH));
            (void) drown();
            return;
        } else {
            /* Engulfer drowns (or dies in the lava), you get released. */
            if (Hallucination) {
                pline(msgc_combatgood, "It's not the heat, really.  "
                      "It's the humidity that gets ya.");
            } else {
                pline(msgc_combatgood, "The %s fills the %s.", liqname,
                      mbodypart(u.ustuck, STOMACH));
                pline(msgc_combatgood, "%s %s.", Monnam(u.ustuck),
                      ((newterrain == LAVAPOOL) ?
                       "perishes in the lava" : "drowns"));
            }
            x = u.ux; y = u.uy;
            killengulfer = TRUE;
        }
    }
    ttmp = t_at(level, x, y);
    if (ttmp) {
        ttmp->tseen = 1;
        switch(ttmp->ttyp) {
        case HOLE:
        case TRAPDOOR:
        case TELEP_TRAP:
        case LEVEL_TELEP:
        case MAGIC_PORTAL:
            if (!Blind && Hallucination)
                pline(msgc_yafm, "Pee pee go down the hooole!");
            else if (!Blind)
                pline(msgc_yafm, "The %s disappears into %s.", liqname,
                      (ttmp->madeby_u ?
                       msgprintf("your %s", trapexplain[ttmp->ttyp - 1]) :
                       an(trapexplain[ttmp->ttyp - 1])));
            break;
        case PIT:
        case SPIKED_PIT:
            if (!Blind && Hallucination)
                pline(msgc_yafm,
                      "That's just about the nicest outhouse you've seen!");
            else if (!Blind)
                pline(msgc_yafm, "The %s fills the pit.", liqname);
            deltrap(level, ttmp);
            level->locations[x][y].typ = newterrain;
            break;
        case VIBRATING_SQUARE:
            if (Hallucination && !Blind)
                pline(msgc_yafm, "Woah, dude, look at the %s shake!", liqname);
            else if (!Blind)
                pline(msgc_yafm, "A mysterious force whisks the %s away.",
                      liqname);
            break;
        case POLY_TRAP:
            if (Hallucination && !Blind)
                pline(msgc_consequence,
                      "If you ever drop your keys into molten lava, "
                      "let 'em go, because man, they're gone.");
            else if (!Blind)
                pline(msgc_consequence, "The %s changes into %s!",
                      liqname,
                      ((newterrain == LAVAPOOL) ? "water" : "lava"));
            deltrap(level, ttmp);
            level->locations[x][y].typ =
                (newterrain == LAVAPOOL) ? POOL : LAVAPOOL;
            break;
        default:
            if (!Blind)
                pline(msgc_consequence, "The %s %s away %s.", liqname,
                      ((newterrain == LAVAPOOL) ? "burns" : "washes"),
                      (ttmp->madeby_u ?
                       msgprintf("your %s", trapexplain[ttmp->ttyp - 1]) :
                       an(trapexplain[ttmp->ttyp - 1])));
            deltrap(level, ttmp);
            break;
        }
    } else {
        switch(level->locations[x][y].typ) {
        case SDOOR:
        case SCORR:
            pline(msgc_consequence,
                  "%s flows into a space you didn't see before.",
                  msgupcasefirst(liqname));
            level->locations[x][y].typ = newterrain;
            break;
        case POOL:
        case MOAT:
            if (newterrain == LAVAPOOL) {
                pline(msgc_consequence,
                      "The water boils away, revealing a pool of lava!");
                level->locations[x][y].typ = LAVAPOOL;
            } else
                pline(msgc_yafm, "The water level increases slightly.");
            break;
        case DRAWBRIDGE_UP:
            level->locations[x][y].drawbridgemask &= ~DB_UNDER;
            /* fall through */
        case DBWALL:
            pline(msgc_yafm, "That's %s under the bridge, now.", liqname);
            break;
        case LAVAPOOL:
            if (newterrain == LAVAPOOL)
                pline(msgc_yafm,
                      "The lava gives off noxious fumes and a cloud of ash.");
            else
                pline(msgc_consequence, "Great clouds of steam swirl up "
                      "as the water hits the lava.");
            for (i = 0; i <= rn2(3); i++) {
                makemon(&mons[PM_STEAM_VORTEX], level, u.ux, u.uy, rng_main);
                makemon(&mons[PM_FOG_CLOUD], level, u.ux, u.uy, rng_main);
            }
            if (newterrain != LAVAPOOL) {
                pline(msgc_consequence, "The lava cools and solidifies.");
                level->locations[x][y].typ = ROOM;
            }
            break;
        case IRONBARS:
            pline(msgc_consequence, "The iron bars %s away.",
                  ((newterrain == LAVAPOOL) ? "melt" : "rust"));
            level->locations[x][y].typ = newterrain;
            break;
        case ICE:
            if (newterrain == LAVAPOOL) {
                pline(msgc_consequence, "The ice melts.");
                level->locations[x][y].typ = POOL;
                break;
            }
            pline(msgc_yafm, "The water freezes.");
            break;
        case FOUNTAIN:
            if (newterrain == FOUNTAIN)
                break;
        case DOOR:
        case SINK:
        case BENCH:
        case THRONE:
        case GRAVE:
            pline(msgc_consequence, "The %s is %s away by the %s.",
                  surface(x,y),
                  ((newterrain == LAVAPOOL) ? "burned" : "washed"),
                  ((newterrain == LAVAPOOL) ? "lava" : "torrent"));
            /* fall through */
        case ROOM:
        case CORR:
            level->locations[x][y].typ = newterrain;
            break;
        case ALTAR:
        case MAGIC_CHEST:
        case LADDER:
        case STAIRS:
            pline(msgc_yafm, "A mysterious force whisks the %s away.",
                  liqname);
            break;
        default:
            ; /* do nothing -- don't mess with walls, etc. */
        }
    }
    newsym(x,y);
    if (killengulfer) {
        mtmp = u.ustuck;
        expels(mtmp, mtmp->data, FALSE);
        xkilled(mtmp, 0);
        return;
    }
    mtmp = m_at(level, x, y);
    if (mtmp == &youmonst) {
        spoteffects(FALSE);
    } else if (mtmp) {
        minliquid(mtmp);
    }
}

int
seffects(struct obj *sobj, boolean *known)
{
    int cval;
    boolean confused = (Confusion != 0);
    struct obj *otmp;

    switch (sobj->otyp) {
    case SCR_ENCHANT_ARMOR:
        {
            schar s;
            boolean special_armor;
            boolean same_color;

            makeknown(SCR_ENCHANT_ARMOR);
            otmp = some_armor(&youmonst);
            if (!otmp) {
                strange_feeling(sobj,
                                !Blind ? msgprintf("Your %s glows then fades.",
                                                   body_part(SKIN)) :
                                msgprintf("Your %s feels warm for a moment.",
                                          body_part(SKIN)));
                exercise(A_CON, !sobj->cursed);
                exercise(A_STR, !sobj->cursed);
                return 1;
            }
            if (confused) {
                otmp->oerodeproof = !(sobj->cursed);
                if (Blind) {
                    otmp->rknown = FALSE;
                    pline(msgc_nospoil, "Your %s %s warm for a moment.",
                          xname(otmp), otense(otmp, "feel"));
                } else {
                    otmp->rknown = TRUE;
                    pline(sobj->cursed ? msgc_itemloss : msgc_itemrepair,
                          "Your %s %s covered by a %s %s %s!", xname(otmp),
                          otense(otmp, "are"),
                          sobj->cursed ? "mottled" : "shimmering",
                          hcolor(sobj->cursed ? "black" : "golden"),
                          sobj->cursed ? "glow" :
                          (is_shield(otmp) ? "layer" : "shield"));
                }
                if (otmp->oerodeproof && (otmp->oeroded || otmp->oeroded2)) {
                    otmp->oeroded = otmp->oeroded2 = 0;
                    pline_implied(msgc_itemrepair,
                                  "Your %s %s as good as new!", xname(otmp),
                                  otense(otmp, Blind ? "feel" : "look"));
                }
                break;
            }
            /* elven armor vibrates warningly when enchanted beyond a limit */
            special_armor = is_elven_armor(otmp) || (Role_if(PM_WIZARD) &&
                                                     otmp->otyp == CORNUTHAUM);
            if (sobj->cursed)
                same_color = (otmp->otyp == BLACK_DRAGON_SCALES ||
                              otmp->scalecolor == DRAGONCOLOR_BLACK);
            else
                same_color = (otmp->scalecolor == DRAGONCOLOR_SILVER ||
                              otmp->otyp == SILVER_DRAGON_SCALES ||
                              otmp->otyp == SHIELD_OF_REFLECTION);
            if (Blind)
                same_color = FALSE;

            /* KMH -- catch underflow */
            s = sobj->cursed ? -otmp->spe : otmp->spe;

            /* If you are wearing dragon scales over body armor, the armor
               becomes scaled.  This overrides the random choice of which armor
               to enchant.  The code for this makes some assumptions about the
               order of the dragons, their scales, and the dragoncolor enum. */
            if (uarmc && uarmc->otyp >= FIRST_DRAGON_SCALES &&
                uarmc->otyp <= LAST_DRAGON_SCALES && uarm) {
                int clr = DRAGONCOLOR_FIRST +
                    (uarmc->otyp - FIRST_DRAGON_SCALES);
                struct obj *armor  = uarm;
                struct obj *scales = uarmc;
                enum msg_channel msgc_scaleevap = msgc_consequence;
                if (armor->scalecolor == clr) {
                    msgc_scaleevap = msgc_itemloss;
                    pline(msgc_itemrepair, /* for lack of a better channel */
                          "The scaly sheen on your %s still seems %s.",
                          xname(armor), hcolor(DRAGONCOLOR_NAME(clr)));
                }
                else if (uarm->scalecolor)
                    pline(msgc_itemrepair, /* there's no itemneutral */
                          "The scaly sheen on your %s changes from %s to %s.",
                          xname(armor),
                          hcolor(DRAGONCOLOR_NAME(armor->scalecolor)),
                          hcolor(DRAGONCOLOR_NAME(clr)));
                else
                    pline(msgc_itemrepair,
                          "Your %s acquires an interesting %s scaly sheen!",
                          xname(uarm), hcolor(DRAGONCOLOR_NAME(clr)));
                setworn(NULL, W_MASK(os_arm));
                setworn(NULL, W_MASK(os_armc));
                armor->scalecolor = clr;
                armor->oerodeproof = 1; /* Dragon scales are impervious to fire
                                           and other such forms of damage.
                                           Armor surfaced with scales, too. */
                armor->rknown = 1; /* The scales are quite obvious, both
                                      visually and to tactile inspection. */
                armor->cursed = 0;
                if (sobj->blessed) {
                    armor->oeroded = armor->oeroded2 = 0;
                    armor->blessed = 1;
                }
                setworn(armor, W_MASK(os_arm));
                *known = TRUE;
                if (scales->unpaid && s > 0)
                    adjust_bill_val(scales);
                pline_implied(msgc_scaleevap, "The scales have been absorbed.");
                useup(scales);
                break;
            }

            /* a custom RNG for this would only give marginal benefits:
               intentional overenchantment attemps are rare */
            if (s > (special_armor ? 5 : 3) && rn2(s)) {
                pline(msgc_itemloss,
                      "Your %s violently %s%s%s for a while, then %s.",
                      xname(otmp), otense(otmp, Blind ? "vibrate" : "glow"),
                      (!Blind && !same_color) ? " " : "",
                      (Blind || same_color) ? "" :
                      hcolor(sobj->cursed ? "black" : "silver"),
                      otense(otmp, "evaporate"));
                setunequip(otmp);
                useup(otmp);
                break;
            }

            /* OTOH, a custom RNG for /this/ is useful. The main balance
               implication here is whether armour goes to 5 or stops at 4. As
               such, if we're enchanting from 2 or 3 precisely, and the
               resulting spe is 4 or 5 precisely, we re-randomize which.
               Exception: an uncursed enchant from 3 always goes to 4 (we never
               bump it up to 5). */
            s = sobj->cursed ? -1 :
                otmp->spe >= 9 ? (rn2(otmp->spe) == 0) :
                sobj->blessed ? rnd(3 - otmp->spe / 3) : 1;
            if ((otmp->spe == 2 || otmp->spe == 3) && sobj->blessed &&
                (otmp->spe + s == 4 || otmp->spe + s == 5))
                s = 4 + rn2_on_rng(2, rng_armor_ench_4_5) - otmp->spe;
            if (s < 0 && otmp->unpaid)
                costly_damage_obj(otmp);
            if (s >= 0 && otmp->otyp >= FIRST_DRAGON_SCALES &&
                otmp->otyp <= LAST_DRAGON_SCALES && !uskin()) {
                /* If there were body armor under the scales, we'd have handled
                   things up above.  Ergo, this is the other case, wherein the
                   scales get applied to the wearer as a polymorph. */
                //polyself(FALSE);
                /* If we just call polyself(), polymorph control would let the
                   player become anything; but we're not reading a scroll of
                   polymorph, the only source of polymorphing here is the
                   scales; ergo, the player should only be able to become a
                   dragon of the appropriate color, by this method.  So instead
                   of calling polyself, we handle things at a lower level... */
                dragonscale_polyself();
                break;
            }
            pline((otmp->known && s == 0) ? msgc_failrandom :
                  (!otmp->known && Hallucination) ? msgc_nospoil :
                  sobj->cursed ? msgc_itemloss : msgc_itemrepair,
                  "Your %s %s%s%s%s for a %s.", xname(otmp),
                  s == 0 ? "violently " : "",
                  otense(otmp, Blind ? "vibrate" : "glow"),
                  (!Blind && !same_color) ? " " : "",
                  (Blind || same_color) ? "" :
                  hcolor(sobj->cursed ? "black" : "silver"),
                  (s * s > 1) ? "while" : "moment");
            otmp->cursed = sobj->cursed;
            if (!otmp->blessed || sobj->cursed)
                otmp->blessed = sobj->blessed;
            if (s) {
                otmp->spe += s;
                adj_abon(otmp, s);
                *known = otmp->known;
            }

            if ((otmp->spe > (special_armor ? 5 : 3)) &&
                (special_armor || !rn2(7)))
                pline_implied(msgc_hint, "Your %s suddenly %s %s.", xname(otmp),
                              otense(otmp, "vibrate"),
                              Blind ? "again" : "unexpectedly");
            if (otmp->unpaid && s > 0)
                adjust_bill_val(otmp);
            break;
        }
    case SCR_DESTROY_ARMOR:
        {
            otmp = some_armor(&youmonst);
            if (confused) {
                if (!otmp) {
                    strange_feeling(sobj, "Your bones itch.");
                    exercise(A_STR, FALSE);
                    exercise(A_CON, FALSE);
                    return 1;
                }
                otmp->oerodeproof = sobj->cursed;
                p_glow2(!sobj->bknown ? msgc_nospoil :
                        otmp->oerodeproof ? msgc_itemrepair : msgc_itemloss,
                        otmp, "purple");
                break;
            }
            if (!sobj->cursed || !otmp || !otmp->cursed) {
                if (!destroy_arm(otmp)) {
                    strange_feeling(sobj, msgprintf("Your %s itches.",
                                                    body_part(SKIN)));
                    exercise(A_STR, FALSE);
                    exercise(A_CON, FALSE);
                    return 1;
                } else
                    *known = TRUE;
            } else {    /* armor and scroll both cursed */
                pline(msgc_itemloss, "Your %s %s.", xname(otmp),
                      otense(otmp, "vibrate"));
                if (otmp->spe >= -6) {
                    otmp->spe--;
                    if ((otmp->otyp == HELM_OF_BRILLIANCE) ||
                        (otmp->otyp == CORNUTHAUM)) {
                        ABON(A_INT)--;
                        ABON(A_WIS)--;
                        makeknown(otmp->otyp);
                    }
                    if (otmp->otyp == GAUNTLETS_OF_DEXTERITY) {
                        ABON(A_DEX)--;
                        makeknown(otmp->otyp);
                    }
                }
                make_stunned(HStun + rn1(10, 10), TRUE);
            }
        }
        break;
    case SCR_CONFUSE_MONSTER:
    case SPE_CONFUSE_MONSTER:
        if (youmonst.data->mlet != S_HUMAN || sobj->cursed) {
            if (!HConfusion)
                pline(msgc_statusbad, "You feel confused.");
            make_confused(HConfusion + rnd(100), FALSE);
        } else if (confused) {
            if (!sobj->blessed) {
                pline(msgc_statusbad, "Your %s begin to %s%s.",
                      makeplural(body_part(HAND)), Blind ? "tingle" : "glow ",
                      Blind ? "" : hcolor("purple"));
                make_confused(HConfusion + rnd(100), FALSE);
            } else {
                pline(msgc_statusheal, "A %s%s surrounds your %s.",
                      Blind ? "" : hcolor("red"),
                      Blind ? "faint buzz" : " glow", body_part(HEAD));
                make_confused(0L, TRUE);
            }
        } else {
            if (!sobj->blessed) {
                pline(msgc_statusgood, "Your %s%s %s%s.",
                      makeplural(body_part(HAND)),
                      Blind ? "" : " begin to glow",
                      Blind ? (const char *)"tingle" : hcolor("red"),
                      u.umconf ? " even more" : "");
                u.umconf++;
            } else {
                if (Blind)
                    pline(msgc_statusgood, "Your %s tingle %s sharply.",
                          makeplural(body_part(HAND)),
                          u.umconf ? "even more" : "very");
                else
                    pline(msgc_statusgood, "Your %s glow a%s brilliant %s.",
                          makeplural(body_part(HAND)),
                          u.umconf ? "n even more" : "", hcolor("red"));
                /* after a while, repeated uses become less effective */
                if (u.umconf >= 40)
                    u.umconf++;
                else
                    u.umconf += rn1(8, 2);
            }
        }
        break;
    case SCR_SCARE_MONSTER:
    case SPE_CAUSE_FEAR:
        {
            int ct = feareffect((confused || sobj->cursed), sobj->oclass);
            if (!ct)
                You_hear(msgc_levelsound, "%s in the distance.",
                         (confused || sobj->cursed) ?
                         "sad wailing" : "maniacal laughter");
            else if (sobj->otyp == SCR_SCARE_MONSTER)
                You_hear(msgc_levelsound, "%s close by.",
                         (confused || sobj->cursed) ?
                         "sad wailing" : "maniacal laughter");
            break;
        }
    case SCR_BLANK_PAPER:
        if (Blind)
            pline(msgc_yafm, "You don't remember there being any "
                  "magic words on this scroll.");
        else
            pline(msgc_yafm, "This scroll seems to be blank.");
        *known = TRUE;
        break;
    case SCR_REMOVE_CURSE:
        makeknown(SCR_REMOVE_CURSE); /* Fall Through */
    case SPE_REMOVE_CURSE:
        {
            if (confused)
                if (Hallucination)
                    pline(msgc_itemloss,
                          "You feel the power of the Force against you!");
                else
                    pline(msgc_itemloss, "You feel like you need some help.");
            else if (Hallucination)
                pline(msgc_itemrepair,
                      "You feel in touch with the Universal Oneness.");
            else
                pline(msgc_itemrepair, "You feel like someone is helping you.");

            if (sobj->cursed) {
                pline(msgc_failcurse, "The scroll disintegrates.");
            } else {
                do_uncurse_effect(sobj->blessed, TRUE);
            }
            if (Punished && !confused) {
                unpunish();
                update_inventory();
            }
            *known = TRUE;
            break;
        }
    case SCR_CREATE_MONSTER:
    case SPE_CREATE_MONSTER:
        if (create_critters
            (1 + ((confused || sobj->cursed) ? 12 : 0) +
             ((sobj->blessed ||
               rn2(73)) ? 0 : rnd(4)), confused ? &mons[PM_ACID_BLOB] : NULL))
            *known = TRUE;
        /* no need to flush monsters; we ask for identification only if the
           monsters are not visible */
        break;
    case SCR_ENCHANT_WEAPON:
        makeknown(SCR_ENCHANT_WEAPON);
        if (uwep && confused) {
            /* oclass check added 10/25/86 GAN but altered in Fourk:  we now
               allow non-weapons to be made durable, but we only repair existing
               damage on weapons and weapon tools. */
            uwep->oerodeproof = !(sobj->cursed);
            if (Blind) {
                uwep->rknown = FALSE;
                pline(msgc_nospoil, "Your %s feels warm for a moment.",
                      (uwep && (uwep->oclass == WEAPON_CLASS ||
                                is_weptool(uwep))) ? "weapon" :
                      distant_name(uwep, xname));
            } else {
                uwep->rknown = TRUE;
                pline(sobj->cursed ? msgc_itemloss : msgc_itemrepair,
                      "Your %s covered by a %s %s %s!", aobjnam(uwep, "are"),
                      sobj->cursed ? "mottled" : "shimmering",
                      hcolor(sobj->cursed ? "purple" : "golden"),
                      sobj->cursed ? "glow" : "shield");
            }
            if (uwep->oerodeproof && (uwep->oeroded || uwep->oeroded2) &&
                (uwep->oclass == WEAPON_CLASS || is_weptool(uwep))) {
                uwep->oeroded = uwep->oeroded2 = 0;
                pline_implied(msgc_itemrepair, "Your %s as good as new!",
                              aobjnam(uwep, Blind ? "feel" : "look"));
            }
        } else {
            /* don't bother with a custom RNG here, 6/7 on weapons is much less
               balance-affecting than 4/5 on armour */
            return !chwepon(sobj,
                            sobj->cursed ? -1 : !uwep ? 1 :
                            uwep->spe >= 9 ? (rn2(uwep->spe) == 0) :
                            sobj->blessed ? rnd(3 - uwep->spe / 3) : 1);
        }
        break;
    case SCR_TAMING:
    case SPE_CHARM_MONSTER:
        if (Engulfed) {
            maybe_tame(u.ustuck, sobj);
        } else {
            int i, j, bd = confused ? 5 : 1;
            struct monst *mtmp;

            for (i = -bd; i <= bd; i++)
                for (j = -bd; j <= bd; j++) {
                    if (!isok(u.ux + i, u.uy + j))
                        continue;
                    if ((mtmp = m_at(level, u.ux + i, u.uy + j)) != 0)
                        maybe_tame(mtmp, sobj);
                }
        }
        break;
    case SCR_GENOCIDE:
        pline(msgc_intrgain, "You have found a scroll of genocide!");
        *known = TRUE;
        if (sobj->blessed)
            do_class_genocide();
        else
            do_genocide((!sobj->cursed) | (2 * ! !Confusion));
        break;
    case SCR_LIGHT:
        if (!Blind)
            *known = TRUE;
        litroom(!confused && !sobj->cursed, sobj);
        break;
    case SCR_TELEPORTATION:
        if (confused || sobj->cursed)
            level_tele();
        else {
            if (sobj->blessed && !Teleport_control) {
                *known = TRUE;
                if (yn("Do you wish to teleport?") == 'n')
                    break;
            }
            tele();
            if (Teleport_control || !couldsee(u.ux0, u.uy0) ||
                (distu(u.ux0, u.uy0) >= 16))
                *known = TRUE;
        }
        break;
    case SCR_GOLD_DETECTION:
        if (confused || sobj->cursed)
            return trap_detect(sobj);
        else
            return gold_detect(sobj, known);
    case SPE_DETECT_FOOD:
        if (food_detect(sobj, known))
            return 1;   /* nothing detected */
        break;
    case SPE_IDENTIFY:
        cval = rn2_on_rng(25, rng_id_count) / 5;
        goto id;
    case SCR_IDENTIFY:
        /* known = TRUE; */

        cval = rn2_on_rng(25, rng_id_count);
        if (sobj->cursed || (!sobj->blessed && cval % 5))
            cval = 1;  /* cursed 100%, uncursed 80% chance of 1 */
        else if (sobj->blessed && cval / 5 == 1 && Luck > 0)
            cval = 2;  /* with positive luck, interpret 1 as 2 when blessed */
        else
            cval /= 5; /* otherwise, randomize all/1/2/3/4 items IDed */

        if (confused)
            pline(msgc_yafm, "You identify this as an identify scroll.");
        else
            pline_implied((cval == 5 && invent) ?
                          msgc_youdiscover : msgc_uiprompt,
                          "This is an identify scroll.");

        if (!objects[sobj->otyp].oc_name_known)
            more_experienced(0, 10);
        useup(sobj);
        makeknown(SCR_IDENTIFY);
    id:
        if (cval > 1 && challengemode)
            cval--;
        if (invent && !confused) {
            identify_pack(cval);
        }
        return 1;

    case SCR_CHARGING:
        if (confused) {
            pline(msgc_statusheal, "You feel charged up!");
            if (u.uen < u.uenmax)
                u.uen = u.uenmax;
            else
                u.uen = (u.uenmax += dice(5, 4));
            break;
        }
        pline(msgc_uiprompt, "This is a charging scroll.");

        cval = sobj->cursed ? -1 : (sobj->blessed ? 1 : 0);
        if (challengemode && cval > 0 && rn2(3))
            cval--;
        if (!objects[sobj->otyp].oc_name_known)
            more_experienced(0, 10);
        useup(sobj);
        makeknown(SCR_CHARGING);

        otmp = getobj(all_count, "charge", FALSE);
        if (!otmp)
            return 1;
        recharge(otmp, cval);
        return 1;

    case SCR_MAGIC_MAPPING:
        if (level->flags.nommap) {
            pline(msgc_statusbad, "Your mind is filled with crazy lines!");
            if (Hallucination)
                pline_implied(msgc_statusbad, "Wow!  Modern art.");
            else
                pline_implied(msgc_statusbad, "Your %s spins in bewilderment.",
                              body_part(HEAD));
            make_confused(HConfusion + rnd(30), FALSE);
            break;
        }
        if (sobj->blessed) {
            int x, y;

            for (x = 0; x < COLNO; x++)
                for (y = 0; y < ROWNO; y++)
                    if (level->locations[x][y].typ == SDOOR)
                        cvt_sdoor_to_door(&level->locations[x][y], &u.uz);
            /* do_mapping() already reveals secret passages */
        }
        *known = TRUE;
        /* Fall Through */
    case SPE_MAGIC_MAPPING:
        if (level->flags.nommap) {
            pline(msgc_statusbad,
                  "Your %s spins as something blocks the spell!",
                  body_part(HEAD));
            make_confused(HConfusion + rnd(30), FALSE);
            break;
        }
        pline(msgc_youdiscover, "A map coalesces in your mind!");
        cval = (sobj->cursed && !confused);
        if (cval)
            HConfusion = 1;     /* to screw up map */

        do_mapping((sobj->oclass == SPBOOK_CLASS) ?
                   (P_SKILL(objects[sobj->otyp].oc_skill) >= P_SKILLED) :
                   sobj->blessed);

        if (cval) {
            HConfusion = 0;     /* restore */
            pline(msgc_substitute,
                  "Unfortunately, you can't grasp the details.");
        }
        break;
    case SCR_SILENCE:
        /* This scroll isn't actually _implemented_ yet; I just needed
           to get the save-breaking parts done first. */
        pline(msgc_statusbad, "You hear nothing.");
        break;
    case SCR_CONSECRATION:
    {
        int typ = level->locations[u.ux][u.uy].typ;
        int newtype = confused ? MAGIC_CHEST : ALTAR;
        aligntyp aalign  = (sobj->cursed || In_hell(&u.uz)) ? A_NONE :
            (sobj->blessed || In_quest(&u.uz)) ? u.ualignbase[A_ORIGINAL] :
            (u.ualign.type == A_NEUTRAL) ? A_LAWFUL : A_NEUTRAL;
        /* the altar can be made chaotic via same-race sacrifice */
        if (In_endgame(&u.uz) || t_at(level, u.ux, u.uy) ||
            !ACCESSIBLE(typ) || IS_DOOR(typ) || IS_FURNITURE(typ) ||
            (typ >= ICE)) { /* ROOM is intended, but corridor for example is
                               also permitted. */
            pline(msgc_yafm, "Nothing happens.");
            break;
        }
        *known = TRUE;
        if (newtype == ALTAR) {
            level->locations[u.ux][u.uy].altarmask = Align2amask(aalign);
            if (!Blind || !(Levitation || Flying))
                pline(msgc_actionok, "The %s beneath your %s rises.",
                      surface(u.ux, u.uy), makeplural(body_part(FOOT)));
            pline(msgc_actionok, "You feel a surge of the power of %s.",
                  align_gname(aalign));
        } else if (!Blind || !(Levitation || Flying)) {
            pline(msgc_actionok, "A great chest rises from the %s.",
                  surface(u.ux, u.uy));
        }
        level->locations[u.ux][u.uy].typ = newtype;
        break;
    }
    case SCR_WATER:
        *known = TRUE;
        if (Is_airlevel(&u.uz) || Is_earthlevel(&u.uz) || Is_firelevel(&u.uz) ||
            (Is_waterlevel(&u.uz) && confused)) {
            pline(msgc_yafm, "Nothing happens.");
            break;
        }
        if (Hallucination)
            pline(msgc_actionok, "Your %s start peeing all over the place!",
                  makeplural(body_part(HAND)));
        else
            pline(msgc_actionok, "%s gushes out of the scroll!",
                  (confused ? "Lava" : "Water"));
        if (Is_waterlevel(&u.uz)) {
            pline(msgc_yafm, "The water level cannot increase any further.");
            break;
        }
        if (sobj->blessed && ! rn2(6)) {
            do_scroll_water(u.ux, u.uy, 0, FOUNTAIN);
        } else {
            int i, max = rnd(6) + rnd(6);
            int liquid = confused ? LAVAPOOL : POOL;
            for (i = 1; i <= max; i++) {
                do_scroll_water(u.ux, u.uy, 4, liquid);
            }
            if (sobj->cursed) {
                do_scroll_water(u.ux, u.uy, 0, liquid);
            }
        }
        if (!Blind)
            vision_recalc(0);
        break;
    case SCR_FIRE:
        /* Note: Modifications have been made as of 3.0 to allow for
           some damage under all potential cases. */
        cval = bcsign(sobj);
        if (!objects[sobj->otyp].oc_name_known)
            more_experienced(0, 10);
        useup(sobj);
        makeknown(SCR_FIRE);
        if (confused) {
            if (Fire_resistance) {
                shieldeff(u.ux, u.uy);
                if (!Blind)
                    pline(msgc_playerimmune,
                          "Oh, look, what a pretty fire in your %s.",
                          makeplural(body_part(HAND)));
                else
                    pline(msgc_playerimmune,
                          "You feel a pleasant warmth in your %s.",
                          makeplural(body_part(HAND)));
            } else {
                pline(msgc_substitute,
                      "The scroll catches fire and you burn your %s.",
                      makeplural(body_part(HAND)));
                losehp(1, killer_msg(DIED, "a scroll of fire"));
            }
            return 1;
        }
        if (Underwater)
            pline(msgc_actionok, "The water around you vaporizes violently!");
        else {
            pline(msgc_actionok, "The scroll erupts in a tower of flame!");
            burn_away_slime();
        }
        explode(u.ux, u.uy, 11, (2 * (rn1(3, 3) + 2 * cval) + 1) / 3,
                SCROLL_CLASS, EXPL_FIERY, NULL, 0);
        return 1;
    case SCR_EARTH:
        /* TODO: handle steeds */
        if (!Is_rogue_level(&u.uz) &&
            (!In_endgame(&u.uz) || Is_earthlevel(&u.uz))) {
            int x, y;
            if (Hallucination && Confusion) {
                pline(msgc_playerimmune, "You are already stoned.");
                break;
            }

            /* Identify the scroll */
            pline(msgc_actionok, "The %s rumbles %s you!",
                  ceiling(u.ux, u.uy), sobj->blessed ? "around" : "above");
            *known = TRUE;
            sokoban_guilt();

            /* Loop through the surrounding squares */
            if (!sobj->cursed)
                for (x = u.ux - 1; x <= u.ux + 1; x++) {
                    for (y = u.uy - 1; y <= u.uy + 1; y++) {
                        /* Is this a suitable spot? */
                        if (isok(x, y) && !closed_door(level, x, y) &&
                            !IS_ROCK(level->locations[x][y].typ) &&
                            !IS_AIR(level->locations[x][y].typ) &&
                            (x != u.ux || y != u.uy)) {
                            struct obj *otmp2;
                            struct monst *mtmp;

                            /* Make the object(s) */
                            otmp2 = mksobj(level, confused ? ROCK : BOULDER,
                                           FALSE, FALSE, rng_main);
                            if (!otmp2)
                                continue;       /* Shouldn't happen */
                            otmp2->quan = confused ? rn1(5, 2) : 1;
                            otmp2->owt = weight(otmp2);

                            /* Find the monster here (won't be player) */
                            mtmp = m_at(level, x, y);
                            if (mtmp && !amorphous(mtmp->data) &&
                                !passes_walls(mtmp->data) &&
                                !noncorporeal(mtmp->data) &&
                                !unsolid(mtmp->data)) {
                                struct obj *helmet = which_armor(mtmp, os_armh);
                                int mdmg;

                                if (cansee(mtmp->mx, mtmp->my)) {
                                    if (!helmet || !is_metallic(helmet))
                                        pline(combat_msgc(&youmonst, mtmp,
                                                          cr_hit),
                                              "%s is hit by %s!",
                                              Monnam(mtmp), doname(otmp2));
                                    if (!canspotmon(mtmp))
                                        map_invisible(mtmp->mx, mtmp->my);
                                }
                                mdmg = dmgval(otmp2, mtmp) * otmp2->quan;
                                if (helmet) {
                                    if (is_metallic(helmet)) {
                                        if (canseemon(mtmp))
                                            pline(combat_msgc(&youmonst, mtmp,
                                                              cr_resist),
                                                  "A %s bounces off "
                                                  "%s hard %s.",
                                                  doname(otmp2),
                                                  s_suffix(mon_nam(mtmp)),
                                                  helmet_name(helmet));
                                        else
                                            You_hear(msgc_levelsound,
                                                     "a clanging sound.");
                                        if (mdmg > 2)
                                            mdmg = 2;
                                    } else {
                                        if (canseemon(mtmp))
                                            pline_implied(
                                                combat_msgc(&youmonst,
                                                            mtmp, cr_hit),
                                                "%s's %s does not protect %s.",
                                                Monnam(mtmp), xname(helmet),
                                                mhim(mtmp));
                                    }
                                }
                                mtmp->mhp -= mdmg;
                                if (mtmp->mhp <= 0)
                                    xkilled(mtmp, 1);
                            }
                            /* Drop the rock/boulder to the floor */
                            if (!flooreffects(otmp2, x, y, "fall")) {
                                place_object(otmp2, level, x, y);
                                stackobj(otmp2);
                                newsym(x, y);   /* map the rock */
                            }
                        }
                    }
                }
            /* Attack the player */
            if (!sobj->blessed) {
                int dmg;
                struct obj *otmp2;

                /* Okay, _you_ write this without repeating the code */
                otmp2 = mksobj(level, confused ? ROCK : BOULDER,
                               FALSE, FALSE, rng_main);
                if (!otmp2)
                    break;
                otmp2->quan = confused ? rn1(5, 2) : 1;
                otmp2->owt = weight(otmp2);
                if (!amorphous(youmonst.data) && !Passes_walls &&
                    !noncorporeal(youmonst.data) && !unsolid(youmonst.data)) {
                    pline(msgc_badidea, "You are hit by %s!", doname(otmp2));
                    dmg = dmgval(otmp2, &youmonst) * otmp2->quan;
                    if (uarmh && !sobj->cursed) {
                        if (is_metallic(uarmh)) {
                            pline(msgc_playerimmune,
                                  "Fortunately, you are wearing a hard %s.",
                                  helmet_name(uarmh));
                            if (dmg > 2)
                                dmg = 2;
                        } else if (flags.verbose) {
                            pline(msgc_notresisted,
                                  "Your %s does not protect you.",
                                  xname(uarmh));
                        }
                    }
                } else
                    dmg = 0;
                /* Must be before the losehp(), for bones files */
                if (!flooreffects(otmp2, u.ux, u.uy, "fall")) {
                    place_object(otmp2, level, u.ux, u.uy);
                    stackobj(otmp2);
                    newsym(u.ux, u.uy);
                }
                if (dmg)
                    losehp(dmg, killer_msg(DIED, "a scroll of earth"));
            }
        }
        break;
    case SCR_PUNISHMENT:
        *known = TRUE;
        if (confused || sobj->blessed) {
            pline(msgc_failcurse, "You feel guilty.");
            break;
        }
        punish(sobj);
        break;
    case SCR_STINKING_CLOUD:{
            coord cc;
            int range = sobj->cursed ? 37 : sobj->blessed ? 70 : 50;

            pline(msgc_hint, "You have found a scroll of stinking cloud!");
            *known = TRUE;
            pline(msgc_uiprompt, "Where do you want to center the cloud?");
            cc.x = u.ux;
            cc.y = u.uy;
            if (getpos(&cc, TRUE, "the desired position", FALSE) ==
                NHCR_CLIENT_CANCEL) {
                pline(msgc_cancelled, "Never mind.");
                return 0;
            }
            if (!cansee(cc.x, cc.y) || distu(cc.x, cc.y) > range) {
                pline(msgc_yafm, "You smell %s.", Hallucination ? "breakfast" :
                      "rotten eggs");
                return 0;
            }
            create_gas_cloud(level, cc.x, cc.y, 3 + bcsign(sobj),
                             8 + 4 * bcsign(sobj));
            break;
        }
    case SCR_WISHING:
        *known = TRUE;
        if (Luck + (challengemode ? 0 : rn2(5)) < 0) {
            pline(msgc_itemloss, "Unfortunately, nothing happens.");
            break;
        } else if (sobj->cursed) {
            pline(msgc_itemloss,
                  "You finish reading the %s, but nothing happens.",
                  (Hallucination ? "horse" : "scroll"));
            break;
        }
        makewish((sobj->blessed) ? 3 : 2);
        break;
    default:
        impossible("What weird effect is this? (%u)", sobj->otyp);
    }
    return 0;
}

int
feareffect(boolean reverse, char itemclass)
{
    int count = 0;
    struct monst *mtmp;
    for (mtmp = level->monlist; mtmp; mtmp = mtmp->nmon) {
        if (DEADMONSTER(mtmp))
            continue;
        if (cansee(mtmp->mx, mtmp->my)) {
            if (reverse) {
                mtmp->mflee = mtmp->mfrozen = mtmp->msleeping = 0;
                mtmp->mcanmove = 1;
            } else if (!resist(mtmp, itemclass, 0, NOTELL))
                monflee(mtmp, 0, FALSE, FALSE);
            if (!mtmp->mtame)
                count++;   /* pets don't laugh at you */
        }
    }
    return count;
}

static void
wand_explode(struct obj *obj)
{
    obj->in_use = TRUE; /* in case losehp() is fatal */
    pline(msgc_consequence, "Your %s vibrates violently, and explodes!",
          xname(obj));
    losehp(rnd(2 * (u.uhpmax + 1) / 3), killer_msg(DIED, "an exploding wand"));
    useup(obj);
    exercise(A_STR, FALSE);
}

/*
 * Low-level lit-field update routine.
 */
static void
set_lit(int x, int y, void *val)
{
    if (val)
        level->locations[x][y].lit = 1;
    else {
        level->locations[x][y].lit = 0;
        snuff_light_source(x, y);
    }
}

void
litroom(boolean on, struct obj *obj)
{
    char is_lit;        /* value is irrelevant; we use its address as a `not
                           null' flag for set_lit() */
    int wandlevel = 0;
    if (obj && obj->oclass == WAND_CLASS)
        wandlevel = getwandlevel(&youmonst, obj);
    /* In case monsters ever uses a light creation spell, wandlevel
       check must be fixed -- perform a deliberate crash */
    if (flags.mon_moving)
        impossible("monster tries to create light?");
    int lightradius = 5;
    if (obj && obj->oclass == SCROLL_CLASS && obj->blessed)
        lightradius = 9;
    else if (wandlevel)
        lightradius = (wandlevel == P_UNSKILLED ? 3  :
                       wandlevel == P_BASIC     ? 5  :
                       wandlevel == P_SKILLED   ? 9  :
                       wandlevel == P_EXPERT    ? 15 :
                       wandlevel == P_MASTER    ? -1 :
                       1);

    /* first produce the text (provided you're not blind) */
    if (!on) {
        struct obj *otmp;

        if (!Blind) {
            if (Engulfed) {
                pline(msgc_yafm, "It seems even darker in here than before.");
                return;
            }
            if (uwep && artifact_light(uwep) && uwep->lamplit)
                pline(msgc_substitute,
                      "Suddenly, the only light left comes from %s!",
                      the(xname(uwep)));
            else
                pline(msgc_substitute, "You are surrounded by darkness!");
        }

        /* the magic douses lamps, et al, too */
        for (otmp = invent; otmp; otmp = otmp->nobj)
            if (otmp->lamplit)
                snuff_lit(otmp);
        if (Blind)
            goto do_it;
    } else {
        if (Blind)
            goto do_it;
        if (Engulfed) {
            if (is_animal(u.ustuck->data))
                pline(msgc_yafm, "%s %s is lit.", s_suffix(Monnam(u.ustuck)),
                      mbodypart(u.ustuck, STOMACH));
            else if (is_whirly(u.ustuck->data))
                pline(msgc_actionok, "%s shines briefly.", Monnam(u.ustuck));
            else
                pline(msgc_actionok, "%s glistens.", Monnam(u.ustuck));
            return;
        }
        pline(msgc_actionok, "A lit field surrounds you!");
    }

do_it:
    /* No-op in water - can only see the adjacent squares and that's it! */
    if (Underwater || Is_waterlevel(&u.uz))
        return;
    /*
     *  If we are darkening the room and the hero is punished but not
     *  blind, then we have to pick up and replace the ball and chain so
     *  that we don't remember them if they are out of sight.
     */
    if (Punished && !on && !Blind)
        move_bc(1, 0, uball->ox, uball->oy, uchain->ox, uchain->oy);

    if (Is_rogue_level(&u.uz)) {
        /* Can't use do_clear_area because MAX_RADIUS is too small */
        /* rogue lighting must light the entire room */
        int rnum = level->locations[u.ux][u.uy].roomno - ROOMOFFSET;
        int rx, ry;

        if (rnum >= 0) {
            for (rx = level->rooms[rnum].lx - 1;
                 rx <= level->rooms[rnum].hx + 1; rx++)
                for (ry = level->rooms[rnum].ly - 1;
                     ry <= level->rooms[rnum].hy + 1; ry++)
                    set_lit(rx, ry, (on ? &is_lit : NULL));
            level->rooms[rnum].rlit = on;
        }
        /* hallways remain dark on the rogue level */
    } else {
        if (lightradius == -1) { /* entire floor */
            int x, y;

            for (x = 0; x < COLNO; x++)
                for (y = 0; y < ROWNO; y++)
                    set_lit(x, y, (on ? &is_lit : NULL));
        } else
            do_clear_area(u.ux, u.uy, lightradius, set_lit, (on ? &is_lit : NULL));
    }

    /*
     *  If we are not blind, then force a redraw on all positions in sight
     *  by temporarily blinding the hero.  The vision recalculation will
     *  correctly update all previously seen positions *and* correctly
     *  set the waslit bit [could be messed up from above].
     */
    if (!Blind) {
        vision_recalc(2);

        /* replace ball&chain */
        if (Punished && !on)
            move_bc(0, 0, uball->ox, uball->oy, uchain->ox, uchain->oy);
    }

    turnstate.vision_full_recalc = TRUE; /* delayed vision recalculation */
}


static void
do_class_genocide(void)
{
    int i, j, immunecnt, gonecnt, goodcnt, class, feel_dead = 0;
    const char *buf;
    const char mimic_buf[] = {def_monsyms[S_MIMIC], '\0'};
    boolean gameover = FALSE;   /* true iff killed self */

    for (j = 0;; j++) {
        if (j >= 5) {
            pline(msgc_cancelled, "That's enough tries!");
            return;
        }

        buf = getlin("What class of monsters do you wish to genocide?",
                     FALSE); /* not meaningfully repeatable... */
        buf = msgmungspaces(buf);

        /* choosing "none" preserves genocideless conduct */
        if (!strcmpi(buf, "none") || !strcmpi(buf, "nothing"))
            return;

        if (strlen(buf) == 1) {
            if (buf[0] == ILLOBJ_SYM)
                buf = mimic_buf;
            class = def_char_to_monclass(buf[0]);
        } else {
            class = 0;
            buf = makesingular(buf);
        }

        immunecnt = gonecnt = goodcnt = 0;
        for (i = LOW_PM; i < NUMMONS; i++) {
            if (class == 0 && strstri(monexplain[(int)mons[i].mlet], buf) != 0)
                class = mons[i].mlet;
            if (mons[i].mlet == class) {
                if (!(mons[i].geno & G_GENO))
                    immunecnt++;
                else if (mvitals[i].mvflags & G_GENOD)
                    gonecnt++;
                else
                    goodcnt++;
            }
        }
        /*
         * TODO[?]: If user's input doesn't match any class
         *          description, check individual species names.
         */
        if (!goodcnt && class != mons[urole.malenum].mlet &&
            class != mons[urace.malenum].mlet) {
            if (gonecnt)
                pline(msgc_cancelled,
                      "All such monsters are already nonexistent.");
            else if (immunecnt || (buf[0] == DEF_INVISIBLE && buf[1] == '\0'))
                pline(msgc_cancelled,
                      "You aren't permitted to genocide such monsters.");
            else if (wizard && buf[0] == '*') {
                pline(msgc_debug,
                      "Blessed genocide of '*' is deprecated. Use #levelcide "
                      "for the same result.\n");
                do_level_genocide();
                return;
            } else
                pline(msgc_cancelled,
                      "That symbol does not represent any monster.");
            continue;
        }

        for (i = LOW_PM; i < NUMMONS; i++) {
            if (mons[i].mlet == class) {
                const char *nam = makeplural(mons[i].mname);

                /* Although "genus" is Latin for race, the hero benefits from
                   both race and role; thus genocide affects either. */
                if (Your_Own_Role(i) || Your_Own_Race(i) ||
                    ((mons[i].geno & G_GENO)
                     && !(mvitals[i].mvflags & G_GENOD))) {
                    /* This check must be first since player monsters might
                       have G_GENOD or !G_GENO. */
                    mvitals[i].mvflags |= (G_GENOD | G_NOCORPSE);
                    reset_rndmonst(i);
                    kill_genocided_monsters();
                    update_inventory(); /* eggs & tins */
                    /* While endgame messages track whether you genocided
                     * by means other than looking at u.uconduct, call
                     * break_conduct anyway to correctly note the first turn
                     * in which it happened. */
                    break_conduct(conduct_genocide);
                    pline(msgc_intrgain, "Wiped out all %s.", nam);
                    if (Upolyd && i == u.umonnum) {
                        u.mh = -1;
                        if (Unchanging) {
                            if (!feel_dead++)
                                pline(msgc_fatal_predone, "You die.");
                            /* finish genociding this class of monsters before
                               ultimately dying */
                            gameover = TRUE;
                        } else
                            /* Cannot cause a "stuck in monster form" death
                               (!Unchanging); but cannot have a null reason
                               (u.mh just got damaged). Thus we use a
                               searchable string for the death reason that
                               looks out of place, because it should never
                               show up. */
                            rehumanize(GENOCIDED, "arbitrary death reason");
                    }
                    /* Self-genocide if it matches either your race or role.
                       Assumption: male and female forms share same monster
                       class. */
                    if (i == urole.malenum || i == urace.malenum) {
                        u.uhp = -1;
                        if (Upolyd) {
                            if (!feel_dead++)
                                pline(msgc_fatal, "You feel dead inside.");
                        } else {
                            if (!feel_dead++)
                                pline(msgc_fatal_predone, "You die.");
                            gameover = TRUE;
                        }
                    }
                } else if (mvitals[i].mvflags & G_GENOD) {
                    if (!gameover)
                        pline(msgc_yafm, "All %s are already nonexistent.",
                              nam);
                } else if (!gameover) {
                    /* suppress feedback about quest beings except for those
                       applicable to our own role */
                    if ((mons[i].msound != MS_LEADER ||
                         quest_info(MS_LEADER) == i)
                        && (mons[i].msound != MS_NEMESIS ||
                            quest_info(MS_NEMESIS) == i)
                        && (mons[i].msound != MS_GUARDIAN ||
                            quest_info(MS_GUARDIAN) == i)
                        /* non-leader/nemesis/guardian role-specific monster */
                        && (i != PM_NINJA ||    /* nuisance */
                            Role_if(PM_SAMURAI))) {
                        boolean named, uniq;

                        named = type_is_pname(&mons[i]) ? TRUE : FALSE;
                        uniq = (mons[i].geno & G_UNIQ) ? TRUE : FALSE;
                        /* one special case */
                        if (i == PM_HIGH_PRIEST)
                            uniq = FALSE;

                        pline(msgc_yafm,
                              "You aren't permitted to genocide %s%s.",
                              (uniq && !named) ? "the " : "",
                              (uniq || named) ? mons[i].mname : nam);
                    }
                }
            }
        }
        if (gameover || u.uhp == -1) {
            (gameover ? done : set_delayed_killer)(
                GENOCIDED, killer_msg(GENOCIDED,
                                      "a blessed scroll of genocide"));
        }
        return;
    }
}

void
do_level_genocide(void)
{
    /* to aid in topology testing; remove pesky monsters */
    struct monst *mtmp, *mtmp2;

    int gonecnt = 0;

    for (mtmp = level->monlist; mtmp; mtmp = mtmp2) {
        mtmp2 = mtmp->nmon;
        if (DEADMONSTER(mtmp))
            continue;
        mongone(mtmp);
        gonecnt++;
    }
    pline(msgc_debug, "Eliminated %d monster%s.", gonecnt, plur(gonecnt));
}

#define REALLY 1
#define PLAYER 2
#define ONTHRONE 4
/* how: 0 = no genocide; create monsters (cursed scroll) */
/*      1 = normal genocide */
/*      3 = forced genocide of player */
/*      5 (4 | 1) = normal genocide from throne */
void
do_genocide(int how)
{
    const char *buf;
    int i, killplayer = 0;
    int mndx;
    const struct permonst *ptr;
    const char *which;

    if (how & PLAYER) {
        mndx = u.umonster;      /* non-polymorphed mon num */
        ptr = &mons[mndx];
        buf = msg_from_string(ptr->mname);
        killplayer++;
    } else {
        for (i = 0;; i++) {
            if (i >= 5) {
                pline(msgc_cancelled, "That's enough tries!");
                return;
            }
            buf = getlin("What monster do you want to genocide? [type the name]",
                         FALSE); /* not meaningfully repeatable... */
            buf = msgmungspaces(buf);
            /* choosing "none" preserves genocideless conduct */
            if (!strcmpi(buf, "none") || !strcmpi(buf, "nothing")) {
                /* ... but no free pass if cursed */
                if (!(how & REALLY)) {
                    ptr = rndmonst(&u.uz, rng_main);
                    if (!ptr)
                        return; /* no message, like normal case */
                    mndx = monsndx(ptr);
                    break;      /* remaining checks don't apply */
                } else
                    return;
            }

            mndx = name_to_mon(buf);
            if (mndx == NON_PM || (mvitals[mndx].mvflags & G_GENOD)) {
                pline(msgc_cancelled, "Such creatures %s exist in this world.",
                      (mndx == NON_PM) ? "do not" : "no longer");
                continue;
            }
            ptr = &mons[mndx];
            /* Although "genus" is Latin for race, the hero benefits from both
               race and role; thus genocide affects either. */
            if (Your_Own_Role(mndx) || Your_Own_Race(mndx)) {
                killplayer++;
                break;
            }

            if (!(ptr->geno & G_GENO)) {
                if (canhear())
                    pline(msgc_npcvoice,
                          (In_endgame(&u.uz)) ?
                          "A thunderous voice booms through the plane:" :
                          (In_quest(&u.uz) &&
                           (Role_if(PM_ROGUE) || Role_if(PM_TOURIST))) ?
                          "A thunderous voice booms through the city:" :
                          Is_outdoors(&u.uz) ?
                          "A thunderous voice booms throughout the land:" :
                          Underwater ?
                          "A thunderous voice booms through the depths:" :
                          "A thunderous voice booms through the caverns:");
                /* This part you can hear in your mind even if deaf: */
                verbalize(msgc_hint, "No, %s!  That will not be done.",
                          mortal_or_creature(youmonst.data, TRUE));
                continue;
            }
            if (is_human(ptr))
                adjalign(-sgn(u.ualign.type));
            if (is_demon(ptr))
                adjalign(sgn(u.ualign.type));

            /* KMH -- Unchanging prevents rehumanization */
            if (Unchanging && ptr == youmonst.data)
                killplayer++;
            break;
        }
    }

    which = "all ";
    if (Hallucination) {
        if (Upolyd)
            buf = youmonst.data->mname;
        else {
            buf = (u.ufemale && urole.name.f) ? urole.name.f : urole.name.m;
            buf = msglowercase(msg_from_string(buf));
        }
    } else {
        buf = ptr->mname;       /* make sure we have standard singular */
        if ((ptr->geno & G_UNIQ) && ptr != &mons[PM_HIGH_PRIEST])
            which = !type_is_pname(ptr) ? "the " : "";
    }
    if (how & REALLY) {
        /* setting no-corpse affects wishing and random tin generation */
        mvitals[mndx].mvflags |= (G_GENOD | G_NOCORPSE);
        pline(msgc_intrgain, "Wiped out %s%s.", which,
              (*which != 'a') ? buf : makeplural(buf));

        if (killplayer) {
            /* might need to wipe out dual role */
            if (urole.femalenum != NON_PM && mndx == urole.malenum)
                mvitals[urole.femalenum].mvflags |= (G_GENOD | G_NOCORPSE);
            if (urole.femalenum != NON_PM && mndx == urole.femalenum)
                mvitals[urole.malenum].mvflags |= (G_GENOD | G_NOCORPSE);
            if (urace.femalenum != NON_PM && mndx == urace.malenum)
                mvitals[urace.femalenum].mvflags |= (G_GENOD | G_NOCORPSE);
            if (urace.femalenum != NON_PM && mndx == urace.femalenum)
                mvitals[urace.malenum].mvflags |= (G_GENOD | G_NOCORPSE);

            u.uhp = -1;

            const char *killer;
            if (how & PLAYER) {
                killer = killer_msg(GENOCIDED, "genocidal confusion");
            } else if (how & ONTHRONE) {
                /* player selected while on a throne */
                killer = killer_msg(GENOCIDED, "an imperious order");
            } else {    /* selected player deliberately, not confused */
                killer = killer_msg(GENOCIDED, "a scroll of genocide");
            }

            /* Polymorphed characters will die as soon as they're rehumanized. */
            /* KMH -- Unchanging prevents rehumanization */
            if (Upolyd && ptr != youmonst.data) {
                pline(msgc_fatal, "You feel dead inside.");
                set_delayed_killer(GENOCIDED, killer);
            } else
                done(GENOCIDED, killer);
        } else if (ptr == youmonst.data) {
            /* As above: the death reason should never be relevant. */
            rehumanize(GENOCIDED, "arbitrary death reason");
        }
        reset_rndmonst(mndx);
        /* While endgame messages track whether you genocided
         * by means other than looking at u.uconduct, call
         * break_conduct anyway to correctly note the first turn
         * in which it happened. */
        break_conduct(conduct_genocide);
        kill_genocided_monsters();
        update_inventory();     /* in case identified eggs were affected */
    } else {
        int cnt = 0;

        if (!(mons[mndx].geno & G_UNIQ) &&
            !(mvitals[mndx].mvflags & (G_GENOD | G_EXTINCT)))
            for (i = rn1(3, 4); i > 0; i--) {
                if (!makemon(ptr, level, u.ux, u.uy, NO_MINVENT))
                    break;      /* couldn't make one */
                ++cnt;
                if (mvitals[mndx].mvflags & G_EXTINCT)
                    break;      /* just made last one */
            }
        if (cnt)
            pline(msgc_substitute, "Sent in some %s.", makeplural(buf));
        else
            pline(msgc_noconsequence, "Nothing happens.");
    }
}

void
punish(struct obj *sobj)
{
    /* KMH -- Punishment is still okay when you are riding */
    pline(msgc_statusbad, "You are being punished for your misbehavior!");
    if (Punished) {
        pline_implied(msgc_statusbad, "Your iron ball gets heavier.");
        uball->owt += 160 * (1 + sobj->cursed);
        return;
    }
    if (amorphous(youmonst.data) || is_whirly(youmonst.data) ||
        unsolid(youmonst.data)) {
        pline_implied(msgc_playerimmune,
                      "A ball and chain appears, then falls away.");
        dropy(mkobj(level, BALL_CLASS, TRUE, rng_main));
        return;
    }
    uchain = mkobj(level, CHAIN_CLASS, TRUE, rng_main);
    uball = mkobj(level, BALL_CLASS, TRUE, rng_main);
    uball->spe = 1;     /* special ball (see save) */

    /* Place ball & chain. We can do this while swallowed now, too (and must do,
       because otherwise they'll be floating objects). */
    placebc();
    if (Blind)
        set_bc(1);  /* set up ball and chain variables */
    newsym(u.ux, u.uy);     /* see ball&chain if can't see self */
}

void
unpunish(void)
{       /* remove the ball and chain */
    struct obj *savechain = uchain;

    obj_extract_self(uchain);
    newsym(uchain->ox, uchain->oy);
    dealloc_obj(savechain);
    uball->spe = 0;
    uball = uchain = NULL;
}

/* some creatures have special data structures that only make sense in their
 * normal locations -- if the player tries to create one elsewhere, or to revive
 * one, the disoriented creature becomes a zombie
 */
boolean
cant_create(int *mtype, boolean revival)
{

    /* SHOPKEEPERS can be revived now */
    if (*mtype == PM_GUARD || (*mtype == PM_SHOPKEEPER && !revival)
        || *mtype == PM_ALIGNED_PRIEST /* || *mtype==PM_ANGEL */ ) {
        *mtype = PM_HUMAN_ZOMBIE;
        return TRUE;
    } else if (*mtype == PM_LONG_WORM_TAIL) {   /* for create_particular() */
        *mtype = PM_LONG_WORM;
        return TRUE;
    }
    return FALSE;
}


/*
 * Make a new monster with the type controlled by the user.
 *
 * Note:  when creating a monster by class letter, specifying the
 * "strange object" (']') symbol produces a random monster rather
 * than a mimic; this behavior quirk is useful so don't "fix" it...
 */
boolean
create_particular(const struct nh_cmd_arg *arg)
{
    char monclass = MAXMCLASSES;
    const char *buf, *bufp;
    int which, tries, i;
    const struct permonst *whichpm;
    struct monst *mtmp;
    boolean madeany = FALSE;
    boolean parseadjective = TRUE;
    boolean maketame, makepeaceful, makehostile;
    boolean cancelled, fast, slow, revived;
    boolean fleeing, blind, paralyzed, sleeping;
    boolean stunned, confused, suspicious, mavenge;
    int quan = 1;

    /* This is explicitly a debug mode operation only. (In addition to the
       rather unflavoured messages, it prints some additional debug info that
       would leak info in a non-debug-mode game.) */
    if (!wizard)
        impossible("calling create_particular outside debug mode");

    if ((arg->argtype & CMD_ARG_LIMIT) && arg->limit > 1)
        quan = arg->limit;

    tries = 0;
    do {
        which = urole.malenum;  /* an arbitrary index into mons[] */
        maketame = makepeaceful = makehostile = FALSE;
        cancelled = fast = slow = revived = fleeing = blind = mavenge = FALSE;
        paralyzed = sleeping = stunned = confused = suspicious = FALSE;
        buf = getarglin(
            arg, "Create what kind of monster? [type the name or symbol]");
        bufp = msgmungspaces(buf);
        if (*bufp == '\033')
            return FALSE;

        /* get quantity */
        if (digit(*bufp) && strcmp(bufp, "0")) {
            quan = atoi(bufp);
            while (digit(*bufp))
                bufp++;
            while (*bufp == ' ')
                bufp++;
        }

        while (parseadjective) {
            parseadjective = FALSE;
            /* allow the initial disposition to be specified */
            if (!strncmpi(bufp, "tame ", 5)) {
                bufp += 5;
                parseadjective = maketame = TRUE;
            } else if (!strncmpi(bufp, "peaceful ", 9)) {
                bufp += 9;
                parseadjective = makepeaceful = TRUE;
            } else if (!strncmpi(bufp, "hostile ", 8)) {
                bufp += 8;
                parseadjective = makehostile = TRUE;
            }
            /* allow status effects to be specified */
            if (!strncmpi(bufp, "cancelled ", 10)) {
                bufp += 10;
                parseadjective = cancelled = TRUE;
            } else if (!strncmpi(bufp, "canceled ", 9)) {
                bufp += 9;
                parseadjective = cancelled = TRUE;
            }
            if (!strncmpi(bufp, "fast ", 5)) {
                bufp += 5;
                parseadjective = fast = TRUE;
            } else if (!strncmpi(bufp, "slow ", 5)) {
                bufp += 5;
                parseadjective = slow = TRUE;
            }
            if (!strncmpi(bufp, "revived ", 8)) {
                bufp += 9;
                parseadjective = revived = TRUE;
            }
            if (!strncmpi(bufp, "fleeing ", 8)) {
                bufp += 8;
                parseadjective = fleeing = TRUE;
            }
            if (!strncmpi(bufp, "blind ", 6)) {
                bufp += 6;
                parseadjective = blind = TRUE;
            }
            if (!strncmpi(bufp, "paralyzed ", 10)) {
                bufp += 10;
                parseadjective = paralyzed = TRUE;
            }
            if (!strncmpi(bufp, "sleeping ", 9)) {
                bufp += 9;
                parseadjective = sleeping = TRUE;
            }
            if (!strncmpi(bufp, "stunned ", 8)) {
                bufp += 8;
                parseadjective = stunned = TRUE;
            }
            if (!strncmpi(bufp, "confused ", 9)) {
                bufp += 9;
                parseadjective = confused = TRUE;
            }
            if (!strncmpi(bufp, "suspicious ", 11)) {
                bufp += 11;
                parseadjective = suspicious = TRUE;
            }
            if (!strncmpi(bufp, "mavenge ", 8)) {
                bufp += 8;
                parseadjective = mavenge = TRUE;
            }
        }
            
        /* decide whether a valid monster was chosen */
        if (strlen(bufp) == 1) {
            monclass = def_char_to_monclass(*bufp);
            if (monclass != MAXMCLASSES)
                break;  /* got one */
        } else {
            which = name_to_mon(bufp);
            if (which >= LOW_PM)
                break;  /* got one */
        }
        /* no good; try again... */
        pline(msgc_cancelled, "I've never heard of such monsters.");
    } while (++tries < 5);

    mtmp = NULL;

    if (tries == 5) {
        pline(msgc_cancelled, "That's enough tries!");
    } else {
        cant_create(&which, FALSE);
        whichpm = &mons[which];
        for (i = 0; i < quan; i++) {
            /* no reason to use a custom RNG for wizmode commands */
            if (monclass != MAXMCLASSES)
                whichpm = mkclass(&u.uz, monclass, 0, rng_main);
            if (maketame) {
                mtmp = makemon(whichpm, level, u.ux, u.uy, MM_EDOG);
                if (mtmp)
                    initedog(mtmp);
            } else {
                mtmp = makemon(whichpm, level, u.ux, u.uy, NO_MM_FLAGS);
                if ((makepeaceful || makehostile) && mtmp)
                    msethostility(mtmp, !makepeaceful, TRUE);
            }
            if (mtmp) {
                madeany = TRUE;
                if (cancelled)
                    mtmp->mcan = 1;
                if (fast)
                    mtmp->mspeed = MFAST;
                else if (slow)
                    mtmp->mspeed = MSLOW;
                if (revived)
                    mtmp->mrevived = 1;
                if (fleeing)
                    mtmp->mflee = 1;
                if (blind)
                    mtmp->mcansee = 0;
                if (paralyzed)
                    mtmp->mcanmove = 0;
                if (sleeping && !resists_sleep(mtmp))
                    mtmp->msleeping = 1;
                if (stunned)
                    mtmp->mstun = 1;
                if (confused)
                    mtmp->mconf = 1;
                if (suspicious)
                    mtmp->msuspicious = 1;
                if (mavenge)
                    mtmp->mavenge = 1;
            }
        }
    }

    /* Output the coordinates of one created monster. This is used by the
       testbench, so that it can #genesis up a monster and then start aiming at
       it. (Note that #genesis is zero-time, meaning that the monster will have
       no chance to move out of position until after the player's next turn.
       Arguably, it would ease debugging further to give the monster summoning
       sickness too, but we don't do that yet, because other sorts of debugging
       might want the monster to be as "normal" as possible.) */
    if (mtmp) {
        int dx = mtmp->mx - u.ux;
        int dy = mtmp->my - u.uy;

        enum nh_direction dir = DIR_NONE;

        if (dx == 0)
            dir = dy < 0 ? DIR_N : DIR_S;
        else if (dy == 0)
            dir = dx < 0 ? DIR_W : DIR_E;
        else if (dx == dy)
            dir = dx < 0 ? DIR_NW : DIR_SE;
        else if (dx == -dy)
            dir = dx < 0 ? DIR_SW : DIR_NE;

        pline(msgc_debug, "Created a monster at (%d, %d), direction %d",
              mtmp->mx, mtmp->my, dir);
    }

    return madeany;
}

/*read.c*/
