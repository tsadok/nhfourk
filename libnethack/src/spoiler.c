/* vim:set cin ft=c sw=4 sts=4 ts=8 et ai cino=Ls\:0t0(0 : -*- mode:c;fill-column:80;tab-width:8;c-basic-offset:4;indent-tabs-mode:nil;c-file-style:"k&r" -*-*/
/* Last modified by Nathan Eady, 2015-05-19 */
/* File opening code based on the xlogfile code. */
/* Object-looping code based on makedefs.c */
/* Concept based on Autospoil, by Cristan Szmajda
   (but I have only seen the output of autospoil, not the source). */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "extern.h"
#include "artifact.h"
#include "prop.h"
#include <fcntl.h>

#define SPOILPREFIX SCOREPREFIX
#define VARIANTNAME "NetHack 4"
#define VERSION msgprintf("%d.%d.%d", VERSION_MAJOR, VERSION_MINOR, PATCHLEVEL)

static const char * semicolonjoin(const char *a, const char *b);
/* poor man's join function */

static const char * htmlheader(const char *spoilername);
static const char * spoiloname(int i);
static const char * spoilweapskill(int i);
static const char * spoilschool(int i);
static const char * spoildamage(int i, boolean large, struct artifact *);
#define SDAM FALSE
#define LDAM TRUE
static const char * spoiltohit(int i, struct artifact *);
static const char * oslotname(enum objslot os);
static const char * spoilmaligntyp(int i);
static const char * spoilaligntyp(aligntyp aln);
static const char * spoiloneattack(const struct attack *attk);
static const char * spoilattacks(int i);
static const char * spoilresistances(uchar res, boolean convey, int i);
static const char * spoilmonsize(int i);
static const char * spoilmonflags(int i);
static void spoilobjclass(FILE *file, const char *hrname, const char *aname,
                          int classone, int classtwo);
static const char * spoilrolename(short rolepm);
static const char * spoilracename(short racepm);
static const char * spoilartalign(struct artifact *art);
static const char * spoilarteffects(struct artifact *art,
                                    unsigned long spfx, struct attack attk);
static const char * spoilartinvoke(struct artifact *art);
static const char * spoilartotherinfo(struct artifact *art);

static const char *at[17] =
        {"passive", "claw", "bite", "kick", "butt", "touch",
         "sting", "hug", "AT_8", "AT_9", "spit", "engulf", "breath",
         "actively explode", "passively explode", "gaze", "tentacles"};
static const char *ad[43] =
        {"physical", "magic missile", "fire", "cold", "sleep", "disint",
         "shock", "strength drain", "acid", "special1", "special2",
         "blinding", "stun", "slow", "paralysis", "level drain",
         "energy drain", "leg wound", "petrification", "stick-to-you",
         "gold theft", "item theft", "charming", "teleportation",
         "rust", "confusion", "digestion", "healing", "wrap-around",
         "lycanthropy", "dexterity drain", "constitution drain",
         "intelligence drain", "disease", "rotting", "seduction",
         "hallucination", "death", "pestilence", "famine", "sliming",
         "disenchantment", "corrosion"};

/* NOTE: the order of these words exactly corresponds to the
   order of oc_material values #define'd in objclass.h.  I
   initially copy/pasted it from foodwords[] in eat.c */
static const char *const material[] = {
    "meal", "liquid", "wax", "vegetable", "meat",
    "paper", "cloth", "leather", "wood", "bone", "scale",
    "iron or steel", "metal", "copper", "silver", "gold",
    "platinum", "mithril", "plastic", "glass", "gemstone", "mineral"
};

const char *
htmlheader(const char * spoilername)
{
    const char *variantname = VARIANTNAME;
    const char *version     = VERSION;
    const char *createdby   = "<!-- Generated Automatically by the NH4 Spoiler Creator -->";
    const char *copyright   = "<!-- HTML Markup by Nathan Eady is public domain or CC0 at your option -->";
    const char *csslink     = "<link rel=\"stylesheet\" type=\"text/css\" href=\"spoilers.css\" />";
    return msgprintf("<html><head><title>%s %s Spoiler</title>\n%s\n%s\n%s\n</head><body>"
                     "<p>This spoiler pertains to %s version %s.</p>",
                     variantname, spoilername, createdby, copyright, csslink, variantname, version);
}

const char *
spoiloname(int i)
{
    const char *name = obj_descr[objects[i].oc_name_idx].oc_name;
    const char *desc = obj_descr[objects[i].oc_descr_idx].oc_descr;
    if (name && desc)
        return msgprintf("%s <span class=\"appearance\">(%s)</span>",
                         name, desc);
    else if (name)
        return name;
    else if (desc)
        return msgprintf("<span class=\"appearance\">(%s)</span>", desc);
    else return "[Unknown Object]";
}

const char *
spoilweapskill(int i)
{
    return skill_name(abs(objects[i].oc_skill));
}

const char *
spoilschool(int i)
{
    return skill_name(objects[i].oc_skill);
}

const char *
spoildamage(int i, boolean large, struct artifact *art)
{
    int dmg = large ? objects[i].oc_wldam : objects[i].oc_wsdam;
    const char *bonus = (art && art->attk.damd) ?
        msgprintf("<span class=\"dbon\">+d%d</span>", art->attk.damd) : "";
    return msgprintf("d%d%s", dmg, bonus);
}

const char *
spoiltohit(int i, struct artifact *art)
{
    const char *bonus = (art && art->attk.damn) ?
        msgprintf("<span class=\"abon\">+%d</span>", art->attk.damn) : "";
    if (objects[i].oc_hitbon > 0)
        return msgprintf("+%d%s", objects[i].oc_hitbon, bonus);
    else if (objects[i].oc_hitbon == 0)
        return bonus;
    else
        return msgprintf("%d%s", objects[i].oc_hitbon, bonus);
}

static const char *
oslotname(enum objslot os)
{
    if (os == ARM_CLOAK)
        return "cloak";
    if (os == ARM_HELM)
        return "helm";
    if (os == ARM_GLOVES)
        return "gloves";
    if (os == ARM_SHIELD)
        return "off hand";
    if (os == ARM_BOOTS)
        return "boots";
    if (os == ARM_SHIRT)
        return "shirt";
    if (os == ARM_SUIT)
        return "body armor";

    if (os == os_amul)
        return "amulet";
    if (os == os_ringl)
        return "left ring finger";
    if (os == os_ringr)
        return "right ring finger";
    if (os == os_tool)
        return "eyewear";
    
    if (os == os_saddle)
        return "saddle";
    if (os == os_carried)
        return "carried artifact";
    if (os == os_invoked)
        return "invoked artifact";

    return "";
}

static const char *
semicolonjoin(const char *a, const char *b)
{
    if (b[0])
        return msgprintf("%s; %s", a, b);
    return a;
}

/* The way monster alignments are specified is horrible.  It is
   the way it is because of the way monster alignment interacts
   with player alignment record, which is even more horrible. */
static const char *
spoilmaligntyp(int i)
{
    aligntyp aln = mons[i].maligntyp;
    if (aln == A_NONE) return spoilaligntyp(A_NONE);
    if (aln >  0) return spoilaligntyp(A_LAWFUL);
    if (aln == 0) return spoilaligntyp(A_NEUTRAL);
    if (aln <  0) return spoilaligntyp(A_CHAOTIC);
    return spoilaligntyp(aln);
}

static const char *
spoilaligntyp(aligntyp aln)
{
    if (aln == A_NONE)
        return "<span class=\"alnmoloch\">una</span>";
    if (aln == A_LAWFUL)
        return "<span class=\"alnlaw\">law</span>";
    if (aln == A_NEUTRAL)
        return "<span class=\"alnneu\">neu</span>";
    if (aln == A_CHAOTIC)
        return "<span class=\"alncha\">cha</span>";
    return "<span class=\"error alnunknown\">ERR</span>";    
}

static const char *
spoiloneattack(const struct attack *attk)
{
    if (!attk->aatyp && !attk->adtyp && !attk->damn && !attk->damd)
        return "";
    return msgprintf("<span class=\"attack\">%dd%d <span class=\"aatyp\">%s</span> <span type=\"adtype\">%s</span></span>",
                     attk->damn, attk->damd,
                     ((attk->aatyp == AT_WEAP) ? "weapon" :
                      (attk->aatyp == AT_MAGC) ? "spellcasting" :
                      (attk->aatyp < 17 /* && attk->aatyp >= 0 */) ?
                      at[attk->aatyp] : "mysterious"),
                     ((attk->adtyp == AD_CLRC) ? "clerical spellcasting" :
                      (attk->adtyp == AD_SPEL) ? "arcane spellcasting" :
                      (attk->adtyp == AD_RBRE) ? "random breath weapon" :
                      (attk->adtyp == AD_SAMU) ? "amulet stealing" :
                      (attk->adtyp == AD_CURS) ? "intrinsic stealing" :
                      (attk->adtyp < 43 /* && attk->adtyp >= 0 */) ?
                      ad[attk->adtyp] : "unknown damage"));
}

/* This relies on NATTK being small and known at code-writing time.
   Otherwise we'd have to get clever with buffers or something. */
static const char *
spoilattacks(int i)
{
    return semicolonjoin(spoiloneattack(&mons[i].mattk[0]),
                         semicolonjoin(spoiloneattack(&mons[i].mattk[1]),
           semicolonjoin(spoiloneattack(&mons[i].mattk[2]),
                         semicolonjoin(spoiloneattack(&mons[i].mattk[3]),
           semicolonjoin(spoiloneattack(&mons[i].mattk[4]),
                         spoiloneattack(&mons[i].mattk[5]))))));
}

static const char *
spoilresistances(uchar res, boolean convey, int i)
{
    return msgprintf("%s%s%s%s%s%s%s%s%s%s%s",
                     /* First, the easy ones that come from res: */
                     ((res & MR_FIRE)   ? "<span class=\"resf\" title=\"Fire\">F</span>" : ""),
                     ((res & MR_COLD)   ? "<span class=\"resc\" title=\"Cold\">C</span>" : ""),
                     ((res & MR_SLEEP)  ? "<span class=\"resz\" title=\"ZZZ = sleep\">Z</span>" : ""),
                     ((res & MR_DISINT) ? "<span class=\"resd\" title=\"Disint\">D</span>" : ""),
                     ((res & MR_ELEC)   ? "<span class=\"ress\" title=\"Shock\">S</span>" : ""),
                     ((res & MR_POISON) ? "<span class=\"resp\" title=\"Poison\">P</span>" : ""),
                     ((res & MR_ACID)   ? "<span class=\"resa\" title=\"Acid\">A</span>" : ""),
                     ((res & MR_STONE)  ? "<span class=\"resn\" title=\"stoNe\">N</span>" : ""),
                     /* corpses can also convey teleportitis, teleport control, telepathy */
                     ((convey && ((mons[i].mflags1 & M1_TPORT) != 0)) ?
                      "<span class=\"tport\" title=\"Jumpy (teleportitis)\">J</span>" : ""),
                     ((convey && ((mons[i].mflags1 & M1_TPORT_CNTRL) != 0)) ?
                      "<span class=\"tctrl\" title=\"Teleport control\">T</span>" : ""),
                     ((convey && (i == PM_FLOATING_EYE || i == PM_MIND_FLAYER ||
                                  i == PM_MASTER_MIND_FLAYER)) ?
                      "<span class=\"telepathy\" title=\"telepathy (mnemonic: ESP)\">E</span>" : ""));
    /* TODO: support MR, sickness resistance, reflection */
}

static const char *
spoilmonsize(int i)
{
    uchar s = mons[i].msize;
    const char * size[8] =
        { "<span class=\"sizetiny\">tiny</span>",
          "<span class=\"sizesmall\">small</span>",
          "<span class=\"sizemedium\">medium</span>",
          "<span class=\"sizelarge\">large</span>",
          "<span class=\"error unknownsize\">size 5</span>",
          "<span class=\"error unknownsize\">size 6</span>",
          "<span class=\"sizegigantic\">gigantic</span>"};
    if (s < 8)
        return size[s];
    return "<span class=\"error unknownsize\">unknown</span>";
}

static const char *
spoilmonflags(int i)
{
    return msgprintf("%s%s%s%s%s%s%s%s" "%s%s%s%s%s%s%s%s"   /* M1 */
                     "%s%s%s%s%s%s%s%s" "%s%s%s%s%s%s%s%s%s" /* M1 */
                     "%s%s%s%s%s%s%s%s" "%s%s%s%s%s%s"       /* M2 */
                     "%s%s%s%s%s%s%s%s" "%s%s%s%s%s%s%s%s"   /* M2 */
                     "%s%s%s%s%s%s%s%s" "%s%s",              /* M3 */
                     /* M1 least significant byte */
                     ((mons[i].mflags1 & M1_FLY)       ? "<span class=\"flgfly\">Fly</span> " : ""),
                     ((mons[i].mflags1 & M1_SWIM)      ? "<span class=\"flgswim\">Swim</span> " : ""),
                     ((mons[i].mflags1 & M1_AMORPHOUS) ? "<span class=\"flgamorph\">Amorph</span> " : ""),
                     ((mons[i].mflags1 & M1_WALLWALK)  ? "<span class=\"flgwwalk\">Wallwalk</span> " : ""),
                     ((mons[i].mflags1 & M1_CLING)     ? "<span class=\"flgcling\">Cling</span> " : ""),
                     (((mons[i].mflags1 & M1_TUNNEL) && !(mons[i].mflags1 & M1_NEEDPICK)) ?
                                                         "<span class=\"flgtunnel\">Tunnel</span> " : ""),
                     ((mons[i].mflags1 & M1_NEEDPICK)  ? "<span class=\"flgpick\">Pick</span> " : ""),
                     ((mons[i].mflags1 & M1_CONCEAL)   ? "<span class=\"flgconceal\">Conceal/span> " : ""),
                     /* M1 second least byte */
                     ((mons[i].mflags1 & M1_HIDE)       ? "<span class=\"flghide\">Hide</span> " : ""),
                     ((mons[i].mflags1 & M1_AMPHIBIOUS) ? "<span class=\"flgamphib\">Amphib</span> " : ""),
                     ((mons[i].mflags1 & M1_BREATHLESS) ? "<span class=\"flgbreathless\">Breathless</span> " : ""),
                     ((mons[i].mflags1 & M1_NOTAKE)     ? "<span class=\"flgnotake\">NoTake</span> " : ""),
                     ((mons[i].mflags1 & M1_NOEYES)     ? "<span class=\"flgnoeyes\">NoEyes</span> " : ""),
                     ((mons[i].mflags1 & M1_NOHANDS)    ? "<span class=\"flgfly\">NoHands</span> " : ""),
                     ((mons[i].mflags1 & M1_NOLIMBS)    ? "<span class=\"flgfly\">NoLimbs</span> " : ""),
                     ((mons[i].mflags1 & M1_NOHEAD)     ? "<span class=\"flgnohead\">NoHead</span> " : ""),
                     /* M1 third byte */
                     ((mons[i].mflags1 & M1_MINDLESS)   ? "<span class=\"flgmindless\">Mindless</span> " : ""),
                     ((mons[i].mflags1 & M1_HUMANOID)   ? "<span class=\"flghumanoid\">Humanoid</span> " : ""),
                     ((mons[i].mflags1 & M1_ANIMAL)     ? "<span class=\"flganimal\">Animal</span> " : ""),
                     ((mons[i].mflags1 & M1_SLITHY)     ? "<span class=\"flgslithy\">Slithy</span> " : ""),
                     ((mons[i].mflags1 & M1_UNSOLID)    ? "<span class=\"flgunsolid\">Unsolid</span> " : ""),
                     ((mons[i].mflags1 & M1_THICK_HIDE) ? "<span class=\"flgthickhide\">ThickHide</span> " : ""),
                     ((mons[i].mflags1 & M1_OVIPAROUS)  ? "<span class=\"flgoviparous\">Oviparous</span> " : ""),
                     ((mons[i].mflags1 & M1_REGEN)      ? "<span class=\"flgregen\">Regen</span> " : ""),
                     /* M1 most significant byte */
                     ((mons[i].mflags1 & M1_SEE_INVIS)  ? "<span class=\"flgseeinvis\">SeeInvis</span> " : ""),
                     ((mons[i].mflags1 & M1_TPORT)      ? "<span class=\"flgtport\">Tport</span> " : ""),
                     ((mons[i].mflags1 & M1_TPORT_CNTRL)? "<span class=\"flgtportcntrl\">TeleCtrl</span> " : ""),
                     ((mons[i].mflags1 & M1_ACID)       ? "<span class=\"flgacid\">Acidic</span> " : ""),
                     ((mons[i].mflags1 & M1_POIS)       ? "<span class=\"flgpois\">Poisonous</span> " : ""),
                     (((mons[i].mflags1 & M1_CARNIVORE) && !(mons[i].mflags1 & M1_HERBIVORE)) ?
                                                          "<span class=\"flgcarnivore\">Carnivore</span> " : ""),
                     (((mons[i].mflags1 & M1_HERBIVORE) && !(mons[i].mflags1 & M1_CARNIVORE))  ?
                                                          "<span class=\"flgherbivore\">Herbivore</span> " : ""),
                     ((mons[i].mflags1 & M1_OMNIVORE)   ? "<span class=\"flgomnivore\">Omnivore</span> " : ""),
                     ((mons[i].mflags1 & M1_METALLIVORE)? "<span class=\"flgmetallivore\">Metallivore</span> " : ""),
                     /* M2 least significant byte */
                     ((mons[i].mflags2 & M2_NOPOLY)     ? "<span class=\"flgnopoly\">NoPoly</span> " : ""),
                     ((mons[i].mflags2 & M2_UNDEAD)     ? "<span class=\"flgundead\">Undead</span> " : ""),
                     ((mons[i].mflags2 & M2_WERE)       ? "<span class=\"flgwere\">Lycanthrope</span> " : ""),
                     ((mons[i].mflags2 & M2_HUMAN)      ? "<span class=\"flghuman\">Human</span> " : ""),
                     ((mons[i].mflags2 & M2_ELF)        ? "<span class=\"flgelf\">Elf</span> " : ""),
                     ((mons[i].mflags2 & M2_DWARF)      ? "<span class=\"flgdwarf\">Dwarf</span> " : ""),
                     ((mons[i].mflags2 & M2_GNOME)      ? "<span class=\"flggnome\">Gnome</span> " : ""),
                     ((mons[i].mflags2 & M2_ORC)        ? "<span class=\"flgorc\">Orc</span> " : ""),
                     /* M2 second least byte */
                     ((mons[i].mflags2 & M2_DEMON)      ? "<span class=\"flgdemon\">Demon</span> " : ""),
                     ((mons[i].mflags2 & M2_MERC)       ? "<span class=\"flgmerc\">Mercinary</span> " : ""),
                     ((mons[i].mflags2 & M2_LORD)       ? "<span class=\"flglord\">Lord</span> " : ""),
                     ((mons[i].mflags2 & M2_PRINCE)     ? "<span class=\"flgprince\">Prince</span> " : ""),
                     ((mons[i].mflags2 & M2_MINION)     ? "<span class=\"flgminion\">Minion</span> " : ""),
                     ((mons[i].mflags2 & M2_GIANT)      ? "<span class=\"flggiant\">Giant</span> " : ""),
                      /* There are two open bits here */
                      /* M2 third byte */
                     ((mons[i].mflags2 & M2_MALE)       ? "<span class=\"flgmale\">Male</span> " : ""),
                     ((mons[i].mflags2 & M2_FEMALE)     ? "<span class=\"flgfemale\">Female</span> " : ""),
                     ((mons[i].mflags2 & M2_NEUTER)     ? "<span class=\"flgneuter\">Neuter</span> " : ""),
                     ((mons[i].mflags2 & M2_PNAME)      ? "<span class=\"flgpname\">ProperName</span> " : ""),
                     ((mons[i].mflags2 & M2_HOSTILE)    ? "<span class=\"flghostile\">Hostile</span> " : ""),
                     ((mons[i].mflags2 & M2_PEACEFUL)   ? "<span class=\"flgpeaceful\">Peaceful</span> " : ""),
                     ((mons[i].mflags2 & M2_DOMESTIC)   ? "<span class=\"flgdomestic\">Domestic</span> " : ""),
                     ((mons[i].mflags2 & M2_WANDER)     ? "<span class=\"flgwander\">Wander</span> " : ""),
                     /* M2 most significant byte */
                     ((mons[i].mflags2 & M2_STALK)      ? "<span class=\"flgstalk\">Stalk</span> " : ""),
                     ((mons[i].mflags2 & M2_NASTY)      ? "<span class=\"flgnasty\">M2_NASTY</span> " : ""),
                     ((mons[i].mflags2 & M2_STRONG)     ? "<span class=\"flgstrong\">Strong</span> " : ""),
                     ((mons[i].mflags2 & M2_ROCKTHROW)  ? "<span class=\"flgrockthrow\">Boulders</span> " : ""),
                     ((mons[i].mflags2 & M2_GREEDY)     ? "<span class=\"flggreedy\">Greedy</span> " : ""),
                     ((mons[i].mflags2 & M2_JEWELS)     ? "<span class=\"flgjewels\">Jewels</span> " : ""),
                     ((mons[i].mflags2 & M2_COLLECT)    ? "<span class=\"flgcollect\">Collects</span> " : ""),
                     ((mons[i].mflags2 & M2_MAGIC)      ? "<span class=\"flgmagic\">MagicItems</span> " : ""),
                     /* M3 least significant byte */
                     (((mons[i].mflags3 & M3_WANTSAMUL) && !(mons[i].mflags1 & M3_COVETOUS)) ?
                                                          "<span class=\"flgwantsamul\">Amulet</span> " : ""),
                     (((mons[i].mflags3 & M3_WANTSBELL) && !(mons[i].mflags1 & M3_COVETOUS)) ?
                                                          "<span class=\"flgwantsbell\">Bell</span> " : ""),
                     (((mons[i].mflags3 & M3_WANTSBOOK) && !(mons[i].mflags1 & M3_COVETOUS)) ?
                                                          "<span class=\"flgwantsbook\">Book</span> " : ""),
                     (((mons[i].mflags3 & M3_WANTSCAND) && !(mons[i].mflags1 & M3_COVETOUS)) ?
                                                          "<span class=\"flgwantscand\">Candellabrum</span> " : ""),
                     (((mons[i].mflags3 & M3_WANTSARTI) && !(mons[i].mflags1 & M3_COVETOUS)) ?
                                                          "<span class=\"flgwantsarti\">Artifact</span> " : ""),
                     ((mons[i].mflags3 & M3_COVETOUS)   ? "<span class=\"flgcovetous\">Covetous</span> " : ""),
                     /* There's an open bit here */
                     ((mons[i].mflags3 & M3_WAITFORU)   ? "<span class=\"flgwaitforu\">WaitForU</span> " : ""),
                     ((mons[i].mflags3 & M3_CLOSE)      ? "<span class=\"flgclose\">Close</span> " : ""),
                     /* M3 second byte */
                     ((mons[i].mflags3 & M3_INFRAVISION)  ? "<span class=\"flginfravision\">InfraVision</span> " : ""),
                     ((mons[i].mflags3 & M3_INFRAVISIBLE) ? "<span class=\"flginfravisible\">InfraVisible</span> " : ""));
}

static void
spoilobjclass(FILE *file, const char * hrname, const char * aname,
                 int classone, int classtwo)
{
    int i;
    const char * extrafield = (classone == FOOD_CLASS) ?
        "<th class=\"numeric extrafield nutrition\">Nutr</th>" :
        (classone == SPBOOK_CLASS) ?
        msgprintf("<th class=\"spschool\">school</th>"
                  "<th class=\"numeric extrafield splev\">%s</th>",
                  "splev") : "";
    fprintf(file, "\n<h1><a name=\"%s\">%s</a></h1>\n"
            "<table id=\"%s\"><thead>\n  "
            "<tr><th id=\"object\">object</th>"
            "<th class=\"material\">mat</th>%s"
            "<th class=\"numeric weight\">wt</th>"
            "<th class=\"numeric price\">zm</th>"
            "</tr>\n</thead><tbody>\n",
            aname, hrname, aname, extrafield);
    for (i = 0; !i || objects[i].oc_class != ILLOBJ_CLASS; i++) {
        if (objects[i].oc_class != classone &&
            objects[i].oc_class != classtwo)
            continue;
        const char * extravalue = (classone == FOOD_CLASS) ?
            msgprintf("<td class=\"numeric extrafield nutrition\">%d</td>",
                      objects[i].oc_nutrition) :
            (classone == SPBOOK_CLASS) ?
            msgprintf("<td class=\"spschool\">%s</td>"
                      "<td class=\"numeric extrafield splev\">%d</td>",
                      spoilschool(i), objects[i].oc_level) : "";
        fprintf(file, "<tr><td class=\"object\">%s</td>"
                "<td class=\"material\">%s</td>%s"
                "<td class=\"numeric weight\">%d</td>"
                "<td class=\"numeric price\">%d</td>"
                "</tr>\n",
                spoiloname(i), material[objects[i].oc_material],
                extravalue, objects[i].oc_weight, objects[i].oc_cost);
    }

    fprintf(file, "</tbody></table>\n");
}

const char *
spoilrolename(short rolepm)
{
    if (rolepm == NON_PM)
        return "";
    return msgprintf("<span class=\"role\">%s</span> ", mons[rolepm].mname);
}

const char *
spoilracename(short racepm)
{
    if (racepm == NON_PM)
        return "";
    return msgprintf("<span class=\"race\">%s</span> ", mons[racepm].mname);
}

const char *
spoilartalign(struct artifact *art)
{
    return msgprintf("%s%s%s%s",
                     ((art->spfx & SPFX_INTEL) ?
                      "<span class=\"artint\">Int</span> " : ""),
                     msgprintf("<span class=\"artaln\">%s</span> ",
                               spoilaligntyp(art->alignment)),
                     spoilrolename(art->role),
                     spoilracename(art->race));
}

static const char *
spoilarteffects(struct artifact *art, unsigned long spfx, struct attack attk)
{
    return msgprintf("%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s %s",
                     ((spfx & SPFX_SEEK) ?
                      /* TODO: currently, the code checks SPFX_SEARCH for both
                         auto-searching and the +n search bonus and never
                         checks SPFX_SEEK at all.  Fix that. */
                      "<span class=\"spfx spfxseek\">+n search</span> " : ""),
                     ((spfx & SPFX_WARN) ?
                      "<span class=\"spfx spfxwarn\">warn</span> " : ""),
                     ((spfx & SPFX_DRLI) ?
                      "<span class=\"spfx spfxdrli\">drain</span>" : ""),
                     ((spfx & SPFX_SEARCH) ?
                      "<span class=\"spfx spfxsearch\">autosrch</span> " : ""),
                     ((spfx & SPFX_BEHEAD) ?
                      "<span class=\"spfx spfxbehead\">vorpal</span> " : ""),
                     ((spfx & SPFX_HALRES) ?
                      msgprintf("<span class=\"spfx spfxhalres\">%s%s</span> ",
                                "<abbr title=\"hallucination resistance\">",
                                "halres</abbr>"): ""),
                     ((spfx & SPFX_ESP) ?
                      "<span class=\"spfx spfxesp\">ESP</span> " : ""),
                     ((spfx & SPFX_STLTH) ?
                      "<span class=\"spfx spfxstealth\">stealth</span> " : ""),
                     ((spfx & SPFX_REGEN) ?
                      "<span class=\"spfx spfxregen\">regen</span> " : ""),
                     ((spfx & SPFX_EREGEN) ?
                      "<span class=\"spfx spfxeregen\">eregen</span> " : ""),
                     ((spfx & SPFX_HSPDAM) ?
                      "<span class=\"spfx spfxhspdam\">half-spell</span> " :
                      ""),
                     ((spfx & SPFX_HPHDAM) ?
                      "<span class=\"spfx spfxhphdam\">half-phys</span> " :
                      ""),
                     ((spfx & SPFX_TCTRL) ?
                      "<span class=\"spfx spfxtctrl\">T-ctrl</span> " : ""),
                     ((spfx & SPFX_LUCK) ?
                      "<span class=\"spfx spfxluck\">luck</span> " : ""),
                     ((spfx & SPFX_XRAY) ?
                      "<span class=\"spfx spfxxray\">xray</span> " : ""),
                     ((spfx & SPFX_REFLECT) ?
                      "<span class=\"spfx spfxreflect\">reflect</span> " : ""),
                     msgprintf("<span class=\"artattk\">%s</span>",
                               /* TODO: handle attk.damn and attk.damd */
                               (attk.adtyp && attk.adtyp == AD_MAGM) ? "MR" :
                               (attk.adtyp && attk.adtyp < 43) ?
                               ad[attk.adtyp] : "")
        );
}

const char *
spoilartinvoke(struct artifact *art)
{
    uchar i = art->inv_prop;
    switch(i) {
    case 0:
        return "";
    case TAMING:
        return "<span class=\"invoke invoketaming\">taming</span>";
    case HEALING:
        return "<span class=\"invoke invokehealing\">healing</span>";
    case ENERGY_BOOST:
        return "<span class=\"invoke invokeenergyboost\">power</span>";
    case UNTRAP:
        return "<span class=\"invoke invokeuntrap\">untrap</span>";
    case CHARGE_OBJ:
        return "<span class=\"invoke invokecharging\">charging</span>";
    case LEV_TELE:
        return "<span class=\"invoke invokelevport\">levelport</span>";
    case CREATE_PORTAL:
        return "<span class=\"invoke invokebranchport\">branchport</span>";
    case ENLIGHTENING:
        return "<span class=\"invoke invokeenlight\">enlightenment</span>";
    case CREATE_AMMO:
        return "<span class=\"invoke invokeammo\">create ammo</span>";
    /* TODO: handle cases FIRE_RES through LAST_PROP */
    /* For now I'm just doing the ones used by existing artifacts. */
    case INVIS:
        return "<span class=\"invoke invokeinvis\">invisibility</span>";
    case LEVITATION:
        return "<span class=\"invoke invokelev\">levitation</span>";
    case CONFLICT:
        return "<span class=\"invoke invokeconflict\">conflict</span>";

    default:
        return msgprintf("<!-- unknown invoke property -->%d", (int) i);
    }
}

const char *
spoilartotherinfo(struct artifact *art)
{
    return msgprintf("%s%s%s",
                     ((art->spfx & SPFX_NOGEN) ?
                      "<span class=\"artother artnogen\">nogen</span> " : ""),
                     (!(art->spfx & SPFX_RESTR) ?
                      "<span class=\"artother artcanname\">#name</span> " : ""),
                     ((art->spfx & SPFX_SPEAK) ?
                      "<span class=\"artother artspeak\">speaks</span> " : ""));
}

void
makespoilers(void)
{
    FILE *outfile;
    int fd = open_datafile("weapon-spoiler.html",
                           O_CREAT | O_WRONLY, SPOILPREFIX);
    int i;
    struct artifact *art;
    
    /* ######################## Weapons ######################## */

    if (fd < 0) {
        pline(msgc_debug, "Failed to write weapon spoiler.  Is it writable?");
        return;
    }

    if (change_fd_lock(fd, FALSE, LT_WRITE, 10)) {
        outfile = fdopen(fd, "w");
        fprintf(outfile, htmlheader("Weapons"));
        fprintf(outfile, "\n<table id=\"weapons\"><thead>\n  "
                "<tr><th class=\"object\">weapon</th>"
                "<th class=\"artifact\">artifact</th>"
                "<th class=\"skill\">skill</th>"
                "<th class=\"material\">mat</th>"
                "<th class=\"numeric tohit\">hit</th>"
                "<th class=\"damage sdam\">sdam</th>"
                "<th class=\"damage ldam\">ldam</th>"
                "<th class=\"numeric weight\">wt</th>"
                "<th class=\"numeric price\">zm</th>"
                "</tr>\n</thead><tbody>\n");
        for (i = 0; !i || objects[i].oc_class != ILLOBJ_CLASS; i++) {
            if (objects[i].oc_class != WEAPON_CLASS &&
                (objects[i].oc_class != TOOL_CLASS ||
                 objects[i].oc_skill == P_NONE))
                continue;
            fprintf(outfile, "  <tr><td colspan=\"2\" class=\"object\">%s</td>"
                    "<td class=\"skill\">%s</td>"
                    "<td class=\"material\">%s</td>"
                    "<td class=\"numeric tohit\">%s</td>"
                    "<td class=\"damage sdam\">%s</td>"
                    "<td class=\"damage ldam\">%s</td>"
                    "<td class=\"numeric weight\">%d</td>"
                    "<td class=\"numeric price\">%d</td>"
                    "</tr>\n",
                    spoiloname(i), spoilweapskill(i),
                    material[objects[i].oc_material], spoiltohit(i, NULL),
                    spoildamage(i, SDAM, NULL), spoildamage(i, LDAM, NULL),
                    objects[i].oc_weight, objects[i].oc_cost);
            /* Now check for artifacts with this base item */
            for (art = artilist + 1; art->otyp; art++) {
                if (art->otyp == i) {
                    /* TODO: add info about which kinds of monsters
                             the damage bonus applies against. */
                    fprintf(outfile, "<tr><td class=\"object\"></td>"
                            "<td class=\"artifact\">%s</td>"
                            "<td class=\"skill\">%s</td>"
                            "<td class=\"material\">%s</td>"
                            "<td class=\"numeric tohit\">%s</td>"
                            "<td class=\"damage sdam\">%s</td>"
                            "<td class=\"damage ldam\">%s</td>"
                            "<td class=\"numeric weight\">%d</td>"
                            "<td class=\"numeric price\">%d</td>"
                            "</tr>", art->name, spoilweapskill(i),
                            material[objects[i].oc_material],
                            spoiltohit(i, art), spoildamage(i, SDAM, art),
                            spoildamage(i, LDAM, art), objects[i].oc_weight,
                            objects[i].oc_cost);
                }
            }
        }
        fprintf(outfile, "\n</tbody></table>\n");
        change_fd_lock(fd, FALSE, LT_NONE, 0);
        fclose(outfile);
    }

    /* ######################## Armor ######################## */
    fd = open_datafile("armor-spoiler.html",
                       O_CREAT | O_WRONLY, SPOILPREFIX);
    
    if (fd < 0) {
        pline(msgc_debug, "Failed to write armor spoiler.  Is it writable?");
        return;
    }
    
    if (change_fd_lock(fd, FALSE, LT_WRITE, 10)) {
        outfile = fdopen(fd, "w");
        fprintf(outfile, htmlheader("Armor"));
        fprintf(outfile, "\n<table id=\"armor\"><thead>\n  "
                "<tr><th class=\"slot\">slot</th>"
                "<th class=\"object\">armor</th>"
                "<th class=\"numeric mc\">MC</th>"
                "<th class=\"numeric ac\">def</th>"
                "<th class=\"material\">mat</th>"
                "<th class=\"numeric weight\">wt</th>"
                "<th class=\"numeric price\">zm</th>"
                "</tr>\n</thead><tbody>\n");

        for (i = 0; !i || objects[i].oc_class != ILLOBJ_CLASS; i++) {
            if (objects[i].oc_class != ARMOR_CLASS)
                continue;
            fprintf(outfile, "<tr><td class=\"slot\">%s</td>"
                    "<td class=\"object\">%s</td>"
                    "<td class=\"numeric mc\">%s</td>"
                    "<td class=\"numeric ac\">%d</td>"
                    "<td class=\"material\">%s</td>"
                    "<td class=\"numeric weight\">%d</td>"
                    "<td class=\"numeric price\">%d</td>"
                    "</tr>\n",
                    oslotname(objects[i].oc_armcat), spoiloname(i),
                    (objects[i].a_can ?
                     msgprintf("MC%d", objects[i].a_can) : ""),
                    objects[i].a_ac, material[objects[i].oc_material],
                    objects[i].oc_weight, objects[i].oc_cost);
        }


        fprintf(outfile, "\n</tbody></table>\n");
        change_fd_lock(fd, FALSE, LT_NONE, 0);
        fclose(outfile);
    }

    /* ######################### Artifacts ######################### */
    fd = open_datafile("artifact-spoiler.html",
                       O_CREAT | O_WRONLY, SPOILPREFIX);
    if (fd < 0) {
        pline(msgc_debug,
              "Failed to write artifact spoiler.  Is it writable?");
        return;
    }
    if (change_fd_lock(fd, FALSE, LT_WRITE, 10)) {
        struct artifact *art;
        outfile = fdopen(fd, "w");
        fprintf(outfile, htmlheader("Artifacts"));
        fprintf(outfile, "\n<p>For weapon artifacts, see also the "
                "<a href=\"weapon-spoiler.html\">weapon spoiler</a></p>\n");
        fprintf(outfile, "\n<table id=\"artifacts\"><thead>\n  <tr>"
                "<th>name</th>"
                "<th>base object</th>"
                "<th>alignments</th>"
                /* "<th>attack</th>" */ /* Meh, see weapons spoiler. */
                "<th>when equipped</th>"
                "<th>when carried</th>"
                "<th>when invoked</th>"
                "<th>other info</th>"
                "<th class=\"numeric price\">cost</th>"
                "</tr>\n</thead><tbody>\n");

        for (art = artilist + 1; art->otyp; art++) {
            fprintf(outfile, "<tr><td class=\"artifact\">%s</td>"
                    "<td class=\"object\">%s</td>"
                    "<td class=\"artalign\">%s</td>"
                    /* "<td class=\"artattack\">%s</td>" */
                    "<td class=\"artequipped\">%s</td>"
                    "<td class=\"artcarried\">%s</td>"
                    "<td class=\"artinvoked\">%s</td>"
                    "<td class=\"artotherinfo\">%s</td>"
                    "<td class=\"numeric price\">%ld</td>"
                    "</tr>",
                    art->name, simple_typename(art->otyp),
                    spoilartalign(art),
                    /* spoilarteffects(art, 0, art->attk) */
                    spoilarteffects(art, art->spfx, art->defn),
                    spoilarteffects(art, art->cspfx, art->cary),
                    spoilartinvoke(art),
                    spoilartotherinfo(art),
                    art->cost);
        }

        fprintf(outfile, "\n</tbody></table>\n</html>\n");

        change_fd_lock(fd, FALSE, LT_NONE, 0);
        fclose(outfile);
    }    

    /* ####################### Other Objects ####################### */
    fd = open_datafile("objects-spoiler.html",
                       O_CREAT | O_WRONLY, SPOILPREFIX);
    if (fd < 0) {
        pline(msgc_debug, "Failed to write object spoiler.  Is it writable?");
        return;
    }
    if (change_fd_lock(fd, FALSE, LT_WRITE, 10)) {
        outfile = fdopen(fd, "w");
        fprintf(outfile, htmlheader("Objects"));

        fprintf(outfile, "<p>Note: for random-appearance objects, "
                "material follows appearance rather than function.</p>");

        fprintf(outfile, "<p>See also: <a href=\"weapon-spoiler.html\">"
                "Weapons Spoiler</a></p>");
        fprintf(outfile, "<p>See also: <a href=\"armor-spoiler.html\">"
                "Armor Spoiler</a></p>");
        fprintf(outfile, "<p>See also: <a href=\"artifact-spoiler.html\">"
                "Artifact Spoiler</a></p>");
        spoilobjclass(outfile, "Jewelry", "jewelry", RING_CLASS, AMULET_CLASS);
        /* TODO: alchemy spoiler */
        spoilobjclass(outfile, "Potions", "potions", POTION_CLASS, POTION_CLASS);
        spoilobjclass(outfile, "Scrolls", "scrolls", SCROLL_CLASS, SCROLL_CLASS);
        spoilobjclass(outfile, "Books", "books", SPBOOK_CLASS, SPBOOK_CLASS);
        spoilobjclass(outfile, "Wands", "wands", WAND_CLASS, WAND_CLASS);
        spoilobjclass(outfile, "Tools", "tools", TOOL_CLASS, TOOL_CLASS);
        spoilobjclass(outfile, "Comestibles", "food", FOOD_CLASS, FOOD_CLASS);
        fprintf(outfile, "<p>The nutritional properties of various corpses, and "
                "what resistances they can grant, are included in the "
                "<a href=\"monster-spoiler.html\">monster spoiler</a>.</p>");
        spoilobjclass(outfile, "Rocks and Gems", "rocks", GEM_CLASS, ROCK_CLASS);

        change_fd_lock(fd, FALSE, LT_NONE, 0);
        fclose(outfile);
    }

    /* ######################## Monsters ######################## */
    fd = open_datafile("monster-spoiler.html",
                       O_CREAT | O_WRONLY, SPOILPREFIX);
    
    if (fd < 0) {
        pline(msgc_debug, "Failed to write monster spoiler.  Is it writable?");
        return;
    }
    if (change_fd_lock(fd, FALSE, LT_WRITE, 10)) {
        outfile = fdopen(fd, "w");
        fprintf(outfile, htmlheader("Monsters"));
        fprintf(outfile, "\n<table id=\"monsters\"><thead>\n  "
                "<tr><th class=\"mlet\"></th>"
                "<th class=\"monster\">monster</th>"
                "<th class=\"numeric level\">lvl</th>"
                "<th class=\"numeric monstr\">mon<br />str</th>"
                "<th class=\"numeric speed\">mov</th>"
                "<th class=\"numeric ac\">def</th>"
                "<th class=\"numeric monmr\">mr</th>"
                "<th class=\"align\">aln</th>"
                "<th class=\"attacks\">attacks</th>"
                "<th class=\"resistances\">resists</th>"
                "<th class=\"resgranted\">grants</th>"
                "<th class=\"numeric nutrition\">nutr</th>"
                "<th class=\"numeric weight\">wt</th>"
                "<th class=\"size\">sz</th>"
                "<th class=\"flags\">flags</th>"
                "</tr>\n</thead><tbody>\n");

        for (i = 0; mons[i].mlet; i++) {
            const boolean ul = mons[i].mcolor & HI_ULINE ? TRUE : FALSE;
            const uchar  clr = ul ? (mons[i].mcolor - HI_ULINE) :
                mons[i].mcolor;
/*
            const uchar clr  = (mons[i].mcolor > CLR_MAX) ?
                (mons[i].mcolor & CLR_MAX) : mons[i].mcolor;
*/
            const char *mlet = msgprintf("<span class=\"color%d\">"
                                         "%s%c%s</span>", clr,
                                         (ul ? "<u>" : ""),
                                         (def_monsyms[(int)mons[i].mlet]),
                                         (ul ? "</u>" : ""));
            fprintf(outfile, "<tr><td class=\"mlet\">%s</td>"
                    "<td class=\"monster\">%s</td>"
                    "<td class=\"numeric level\">%d</td>"
                    "<td class=\"numeric monstr\">%d</td>"
                    "<td class=\"numeric speed\">%d</td>"
                    "<td class=\"numeric ac\">%d</td>"
                    "<td class=\"numeric monmr\">%d</td>"
                    "<td class=\"align\">%s</td>"
                    "<td class=\"attacks\">%s</td>"
                    "<td class=\"resistances\">%s</td>"
                    "<td class=\"resgranted\">%s</td>"
                    "<td class=\"numeric nutrition\">%d</td>"
                    "<td class=\"numeric weight\">%d</td>"
                    "<td class=\"size\">%s</td>"
                    "<td class=\"flags\">%s</td>"
                    "</tr>\n", mlet, mons[i].mname, mons[i].mlevel,
                    monstr[i], mons[i].mmove, (10 - mons[i].ac),
                    mons[i].mr, spoilmaligntyp(i), spoilattacks(i),
                    spoilresistances(mons[i].mresists, FALSE, i),
                    spoilresistances(mons[i].mconveys, TRUE, i),
                    mons[i].cnutrit, mons[i].cwt, spoilmonsize(i),
                    spoilmonflags(i));
        }
        fprintf(outfile, "\n</tbody></table>\n</html>\n");

        change_fd_lock(fd, FALSE, LT_NONE, 0);
        fclose(outfile);     
    }
    /* ####################### Role / Race ####################### */
    // TODO

    /* ######################### Index ########################### */
    fd = open_datafile("index.html",
                       O_CREAT | O_WRONLY, SPOILPREFIX);
    
    if (fd < 0) {
        pline(msgc_debug, "Failed to write spoiler index.  Is it writable?");
        return;
    }
    if (change_fd_lock(fd, FALSE, LT_WRITE, 10)) {
        outfile = fdopen(fd, "w");
        fprintf(outfile, htmlheader(""));

        fprintf(outfile, "<ul>\n"
                "   <li><a href=\"objects-spoiler.html\">Objects</a><ul>"
                "       <li><a href=\"weapon-spoiler.html\">Weapons</a></li>"
                "       <li><a href=\"armor-spoiler.html\">Armor</a></li>"
                "       <li><a href=\"objects-spoiler.html#jewelry\">Jewelry</a></li>"
                "       <li><a href=\"objects-spoiler.html#potions\">Potions</a></li>"
                "       <li><a href=\"objects-spoiler.html#scrolls\">Scrolls</a></li>"
                "       <li><a href=\"objects-spoiler.html#books\">Books &amp; Spells</a></li>"
                "       <li><a href=\"objects-spoiler.html#wands\">Wands</a></li>"
                "       <li><a href=\"objects-spoiler.html#tools\">Tools</a></li>"
                "       <li><a href=\"objects-spoiler.html#food\">Comestibles</a></li>"
                "       <li><a href=\"objects-spoiler.html#rocks\">Rocks &amp; Gems</a></li>"
                "   </ul></li>"
                "   <li><a href=\"artifact-spoiler.html\">Artifacts</a></li>"
                "   <li><a href=\"monster-spoiler.html\">Monsters</a></li>"
                "</ul>\n");

        fprintf(outfile, "\n</html>\n");
        change_fd_lock(fd, FALSE, LT_NONE, 0);
        fclose(outfile);             
    }
    
    pline(msgc_debug, "Spoiler files generated.");
}
