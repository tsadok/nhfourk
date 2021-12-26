/* vim:set cin ft=c sw=4 sts=4 ts=8 et ai cino=Ls\:0t0(0 : -*- mode:c;fill-column:80;tab-width:8;c-basic-offset:4;indent-tabs-mode:nil;c-file-style:"k&r" -*-*/
/* Last modified by Alex Smith, 2015-11-11 */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "lev.h"
#include "dlb.h"

#define MAX_ORACLE_LENGTH 4096

/*  [note: this comment is fairly old, but still accurate for 3.1]
 * Rumors have been entirely rewritten to speed up the access.  This is
 * essential when working from floppies.  Using fseek() the way that's done
 * here means rumors following longer rumors are output more often than those
 * following shorter rumors.  Also, you may see the same rumor more than once
 * in a particular game (although the odds are highly against it), but
 * this also happens with real fortune cookies.  -dgk
 */

/*  3.1
 * The rumors file consists of a "do not edit" line, a hexadecimal number
 * giving the number of bytes of useful/true rumors, followed by those
 * true rumors (one per line), followed by the useless/false/misleading/cute
 * rumors (also one per line).  Number of bytes of untrue rumors is derived
 * via fseek(EOF)+ftell().
 *
 * The oracles file consists of a "do not edit" comment, a decimal count N
 * and set of N+1 hexadecimal fseek offsets, followed by N multiple-line
 * records, separated by "---" lines.  The first oracle is a special case,
 * and placed there by 'makedefs'.
 */

static void init_rumors(dlb *);
static void init_oracles(void);
static const char * oracletext(int);
static void outoracle(boolean, boolean);

static int true_rumor_start, true_rumor_size, true_rumor_end, false_rumor_start,
    false_rumor_size, false_rumor_end;
static int oracle_flg = 0;      /* -1=>don't use, 0=>need init, 1=>init done */
static int oracle_cnt = 0;
static int *oracle_idx = 0;

static void
init_rumors(dlb * fp)
{
    char line[BUFSZ]; /* for fgets */

    dlb_fgets(line, sizeof line, fp);   /* skip "don't edit" comment */
    dlb_fgets(line, sizeof line, fp);
    if (sscanf(line, "%6x\n", &true_rumor_size) == 1 && true_rumor_size > 0L) {
        dlb_fseek(fp, 0L, SEEK_CUR);
        true_rumor_start = dlb_ftell(fp);
        true_rumor_end = true_rumor_start + true_rumor_size;
        dlb_fseek(fp, 0L, SEEK_END);
        false_rumor_end = dlb_ftell(fp);
        false_rumor_start = true_rumor_end;     /* ok, so it's redundant... */
        false_rumor_size = false_rumor_end - false_rumor_start;
    } else
        true_rumor_size = -1L;  /* init failed */
}

/* exclude_cookie is a hack used because we sometimes want to get rumors in a
 * context where messages such as "You swallowed the fortune!" that refer to
 * cookies should not appear.  This has no effect for true rumors since none
 * of them contain such references anyway.
 */
const char *
getrumor(int truth,     /* 1=true, -1=false, 0=either */
         boolean exclude_cookie, int *truth_out, enum rng rng)
{
    dlb *rumors;
    int tidbit, beginning;
    char *endp;
    int ltruth = 0;
    char line[BUFSZ]; /* for fgets */
    const char *rv = "";

    /* If this happens, we couldn't open the RUMORFILE. So synthesize a
       rumor just for the occasion :-) */
    if (true_rumor_size < 0L)
        return "";

    rumors = dlb_fopen(RUMORFILE, "r");

    if (rumors) {
        int count = 0;
        int adjtruth;

        do {
            if (true_rumor_size == 0L) {        /* if this is 1st outrumor() */
                init_rumors(rumors);
                if (true_rumor_size < 0L)       /* init failed */
                    return msgprintf("Error reading \"%.80s\".", RUMORFILE);
            }
            /* 
             *      input:      1    0   -1
             *       rn2 \ +1  2=T  1=T  0=F
             *       adj./ +0  1=T  0=F -1=F
             */
            switch (adjtruth = truth + rn2_on_rng(2, rng)) {
            case 2:    /* (might let a bogus input arg sneak thru) */
            case 1:
                beginning = true_rumor_start;
                tidbit = rn2_on_rng(true_rumor_size, rng);
                break;
            case 0:    /* once here, 0 => false rather than "either" */
            case -1:
                beginning = false_rumor_start;
                tidbit = rn2_on_rng(false_rumor_size, rng);
                break;
            default:
                impossible("strange truth value for rumor");
                if (truth_out)
                    *truth_out = 0;
                return "Oops...";
            }
            dlb_fseek(rumors, beginning + tidbit, SEEK_SET);
            dlb_fgets(line, sizeof line, rumors);
            if (!dlb_fgets(line, sizeof line, rumors) ||
                (adjtruth > 0 && dlb_ftell(rumors) > true_rumor_end)) {
                /* reached end of rumors -- go back to beginning */
                dlb_fseek(rumors, beginning, SEEK_SET);
                dlb_fgets(line, sizeof line, rumors);
            }
            if ((endp = strchr(line, '\n')) != 0)
                *endp = 0;
            char decrypted_line[strlen(line) + 1];
            xcrypt(line, decrypted_line);
            rv = msg_from_string(decrypted_line);
        } while (count++ < 50 && exclude_cookie &&
                 (strstri(rv, "fortune") || strstri(rv, "pity")));
        dlb_fclose(rumors);
        if (count >= 50)
            impossible("Can't find non-cookie rumor?");
        else
            ltruth = (adjtruth > 0) ? 1 : -1;
    } else {
        pline(msgc_saveload, "Can't open rumors file!");
        true_rumor_size = -1;   /* don't try to open it again */
        if (truth_out)
            *truth_out = 0;
    }
    if (truth_out)
        *truth_out = ltruth;
    return rv;
}

void
outrumor(int truth,     /* 1=true, -1=false, 0=either */
         int mechanism)
{
    static const char fortune_msg[] =
        "This cookie has a scrap of paper inside.";
    const char *line;
    boolean reading = (mechanism == BY_COOKIE || mechanism == BY_PAPER);
    int truth_out;

    if (reading) {
        /* deal with various things that prevent reading */
        if (u_helpless(hm_all) && mechanism == BY_COOKIE)
            return;
        else if (Blind) {
            if (mechanism == BY_COOKIE)
                pline(msgc_rumor, fortune_msg);
            pline(msgc_badidea, "What a pity that you cannot read it!");
            return;
        }
    }
    line = getrumor(truth, reading ? FALSE : TRUE, &truth_out, rng_main);
    if (!*line)
        line = "NetHack rumors file closed for renovation.";
    switch (mechanism) {
    case BY_ORACLE:
        /* Oracle delivers the rumor */
        pline(msgc_npcvoice, "True to her word, the Oracle %ssays: ",
              (!rn2(4) ? "offhandedly "
               : (!rn2(3) ? "casually " : (rn2(2) ? "nonchalantly " : ""))));
        verbalize(msgc_rumor, "%s", line);
        return;
    case BY_COOKIE:
        pline(msgc_rumor, fortune_msg);
        /* FALLTHRU */
    case BY_PAPER:
        pline(msgc_npcvoice, "It reads:");
        break;
    }
    pline(msgc_rumor, "%s", line);
}

const char *
oracletext(int n)
{
    static const char * const otext[] = {
        /* The first one, otext[0], is the special "cheapskate" message. */
        "\"...it is rather disconcerting to be confronted with the\n"
        "following theorem from [Baker, Gill, and Solovay, 1975].\n\n"
        "Theorem 7.18  There exist recursive languages A and B such that\n"
        "  (1)  P(A) == NP(A), and\n"
        "  (2)  P(B) != NP(B)\n\n"
        "This provides impressive evidence that the techniques that are\n"
        "currently available will not suffice for proving that P != NP or\n"
        "that P == NP.\"  [Garey and Johnson, p. 185.]",

        "If thy wand hath run out of charges, thou mayst zap it again and\n"
        "again; though naught may happen at first, verily, thy persistence\n"
        "shall be rewarded, as one last charge may yet be wrested from it!",

        "Though the shopkeepers be wary, thieves have nevertheless stolen much\n"
        "by using their digging wands to hasten exits through the pavement, or\n"
        "by training up an animal to act as an accomplice.",

        "If thou hast had trouble with rust on thine armor or weapons, thou\n"
        "shouldst know that thou canst prevent this by, while in a confused\n"
        "state, reading the magical parchments which normally are used to\n"
        "cause their enchantment.  Unguents of lubrication may provide similar\n"
        "protection, albeit of a transitory nature.",

        "Behold the cockatrice, whose diminutive stature belies its hidden\n"
        "might.  The cockatrice can petrify any ordinary being it contacts--\n"
        "save those wise adventurers who eat a dead lizard or blob of acid\n"
        "when they feel themselves slowly turning to stone.",

        "While some wayfarers rely on scrounging finished armor in the\n"
        "dungeon, the resourceful know the mystical means by which mail may\n"
        "be fashioned out of scales from a dragon's hide.",

        "It is customarily known among travelers that extra-healing draughts\n"
        "may clear thy senses when thou art addled by delusory visions.  But\n"
        "never forget, the lowly potion which makes one sick may be used for\n"
        "the same purpose.",

        "While the consumption of lizard flesh or water beloved of the gods\n"
        "may clear the muddled head, the application of the horn of a creature\n"
        "of utmost purity can alleviate many other afflictions as well.",

        "If thou wouldst travel quickly between distant locations, thou must\n"
        "be able to control thy teleports, and in a confused state misread the\n"
        "scroll which usually teleports thyself locally.  Daring adventurers\n"
        "have also performed the same feat sans need for scrolls or potions by\n"
        "stepping into a particular ambuscade.",

        "Almost all adventurers who come this way hope to pass the dread\n"
        "Medusa.  To do this, the best advice is to keep thine eyes\n"
        "blindfolded and to cause the creature to espy its own reflection in\n"
        "a mirror.",

        "And where it is written \"ad aerarium\", diligent searching will\n"
        "often reveal the way to a trap which sends one to the Magic Memory\n"
        "Vault, where the riches of Croesus are stored; however, escaping from\n"
        "the vault with its gold is much harder than getting in.",

        "It is well known that wily shopkeepers raise their prices whene'er\n"
        "they espy the garish apparel of the approaching tourist or the\n"
        "countenance of a disfavored patron.  They favor the gentle of manner\n"
        "and the fair of face.  The boor may expect unprofitable transactions.",

        "The cliche of the kitchen sink swallowing any unfortunate rings that\n"
        "contact its pernicious surface reflecteth greater truth than many\n"
        "homilies, yet even so, few have developed the skill to identify\n"
        "enchanted rings by the transfigurations effected upon the voracious\n"
        "device's frame.  Fewer still have learned the workman's skills needed\n"
        "to dismantle said plumbing and recover the lost jewelry.",

        "The meat of enchanted creatures ofttimes conveyeth magical properties\n"
        "unto the consumer.  A fresh corpse of floating eye doth fetch a high\n"
        "price among wizards for its utility in conferring Telepathy, by which\n"
        "the sightless may locate surrounding minds.",

        "The detection of blessings and curses is in the domain of the gods.\n"
        "They will make this information available to mortals who request it\n"
        "at their places of worship, or elsewhere for those mortals who devote\n"
        "themselves to the service of the gods.",

        "At times, the gods may favor worthy supplicants with named blades\n"
        "whose powers echo throughout legend.  Learned wayfarers can reproduce\n"
        "blades of elven lineage, hated of the orcs, without the need for such\n"
        "intervention.",

        "There are many stories of a mighty amulet, the origins of which are\n"
        "said to be ancient Yendor.  This amulet doth have awesome power, and\n"
        "the gods desire it greatly.  Mortals mayst tap only portions of its\n"
        "terrible abilities.  The stories tell of mortals seeing what their\n"
        "eyes cannot see and seeking places of magical transportation, while\n"
        "having this amulet in their possession.  Others say a mortal must\n"
        "wear the amulet to obtain these powers.  But verily, such power comes\n"
        "at great cost, to preserve the balance.",

        "It is said that thou mayst gain entry to Moloch's sanctuary, if thou\n"
        "darest, from a place where the ground vibrateth in the deepest depths\n"
        "of Gehennom.  Thou needs must have the aid of three magical items.\n"
        "The pure sound of a silver bell shall announce thee.  The terrible\n"
        "runes, read from Moloch's book, shall cause the earth to tremble\n"
        "mightily.  The light of an enchanted candelabrum shall show thee the\n"
        "way.",

        "In the deepest recesses of the Dungeons of Doom, guarding access to\n"
        "the nether regions, there standeth a castle, wherein lieth a wand of\n"
        "wishes.  If thou wouldst gain entry, bear with thee an instrument of\n"
        "music, for the pontlevis may be charmed down with the proper melody.\n"
        "What notes comprise it only the gods know, but a musical mastermind\n"
        "may yet succeed by witful improvisation.  However, the less\n"
        "perspicacious are not without recourse, should they be prepared to\n"
        "circumambulate the castle to the postern.",

        "The name of Elbereth may strike fear into the hearts of thine\n"
        "enemies, if thou dost write it upon the ground at thy feet.  If thou\n"
        "maintainest the utmost calm, thy safety will be aided greatly, but\n"
        "beware lest thy clumsy feet scuff the inscription in the heat of\n"
        "battle, cancelling its potence.",

        "There are many creatures in these dungeons whose attacks, in addition\n"
        "to causing thee injury, may also subject thee, and sometimes also thy\n"
        "possessions, to various afflictions. Some may even kill thee.  The\n"
        "likelihood of such misfortunes can be greatly reduced by wearing a\n"
        "garment that covers not only thy torso but also a larger portion of\n"
        "thy body, covering thy body and arms fully and extending downward to\n"
        "the tops of thy boots.  Additionally, a magical spell may provide the\n"
        "same sort of protection, or perhaps a ring.  It is said that these\n"
        "protections may even be combined, providing even greater immunity.",

        "It is said that the great hero Jason, when he passed through this\n"
        "very dungeon, was wont to master even the most devastating whims of\n"
        "fate and take from them whatever was necessary for his quest.  On one\n"
        "occasion, having stumbled upon a powerful magical scroll that\n"
        "promised to rid him of some kind of beast forever, he chose to rid\n"
        "himself of dragons, fearing their breath; but lo, upon reading the\n"
        "scroll, he discovered to his horror that there had been a curse upon\n"
        "it, and the hero soon found himself surrounded by dragons.  It is\n"
        "said that he slew them all and fashioned powerful armor from their\n"
        "scales, which he wore ever after.",

        "The greatest of the dwarvish armor smiths have keenly guarded the\n"
        "secret of making a magical metal with remarkable properties.  Since\n"
        "the fall of their great stronghold, coats of armor made of this\n"
        "fantastic metal, never cheap even at the best of times, have become a\n"
        "rare treasure indeed.  Stronger than iron but light enough to be\n"
        "comfortable even when worn over most of the body, these mithril coats\n"
        "are also the equal of even the best enchanted travelers' cloaks in\n"
        "terms of protection against the bites and stings of pests and the\n"
        "ravages of the undead children of the night. It is said that some of\n"
        "the greatest elvish smiths also learned from the dwarves of old the\n"
        "secret of making these vestments.",

        "Often the magical potions found in the dungeon can be combined in a\n"
        "way that alters their magical effects. Learning the secret arts of\n"
        "this process can allow a skilled adventurer to convert large numbers\n"
        "of common potions into less common and more useful ones.  Of\n"
        "particular interest is the ability to make potions of healing more\n"
        "potent.  However, this process is not without risk. Some combinations\n"
        "of potions can explode, and others can have unreliable results.",

        "When the great hero Diodorus sought the forge of Hephaestus, in order\n"
        "to remake his father's sword, the path of his journey passed through\n"
        "the land of the fire dragons.  Fearing that his father's instructions\n"
        "for remaking the sword, being written on a parchment scroll, might be\n"
        "burnt up in the flames of a dragon's breath, he held the scroll aloft\n"
        "in his hand, pretending that it was his sword, and performed a ritual\n"
        "for making a weapon proof against fire and rust.  They say that even\n"
        "the god Hephaestus was confused by this gesture, and found himself\n"
        "unable or unwilling to burn the scroll thereafter.",
    };
    const char *answer;
    if (n == -1)
        return msgprintf("%d", SIZE(otext));
    if (n >= SIZE(otext)) {
        impossible("Invalid oracle number: %d (max %d)", n, SIZE(otext));
        return msgprintf("%s", otext[0]);
    }
    pline(msgc_debug, "oracle_flg=%d; oracle_cnt=%d", oracle_flg, oracle_cnt);
    pline(msgc_debug, "chooising otext[oracle_idx[%d]]", n);
    pline(msgc_debug, "chose otext[%d]", oracle_idx[n]);
    answer = otext[oracle_idx[n]];
    oracle_idx[n] = oracle_idx[--oracle_cnt];
    return answer;
}

static void
init_oracles(void)
{
    const char *count = oracletext(-1);
    int i;
    oracle_cnt = atoi(count);
    oracle_idx = malloc(oracle_cnt * sizeof(int));
    for (i = 0; i < oracle_cnt; i++) {
        oracle_idx[i] = i;
    }
    return;
}

void
save_oracles(struct memfile *mf)
{
    int i;

    mtag(mf, 0, MTAG_ORACLES);
    mwrite32(mf, oracle_cnt);
    if (oracle_cnt)
        for (i = 0; i < oracle_cnt; i++)
            mwrite32(mf, oracle_idx[i]);
}


void
free_oracles(void)
{
    if (oracle_cnt) {
        free(oracle_idx);
        oracle_idx = NULL;
        oracle_cnt = 0;
        oracle_flg = 0;
    }
}


void
restore_oracles(struct memfile *mf)
{
    int i;

    oracle_cnt = mread32(mf);
    if (oracle_cnt) {
        oracle_idx = malloc(oracle_cnt * sizeof(int));
        for (i = 0; i < oracle_cnt; i++)
            oracle_idx[i] = mread32(mf);
        oracle_flg = 1; /* no need to call init_oracles() */
    }
}

void
outoracle(boolean special, boolean delphi)
{
    int idx;
    char chosentext[MAX_ORACLE_LENGTH];
    char *nextline;
    struct nh_menulist menu;

    if (oracle_flg < 0 ||       /* couldn't open ORACLEFILE */
        (oracle_flg > 0 && oracle_cnt == 0))    /* oracles already exhausted */
        return;


    if (oracle_flg == 0) {  /* if this is the first outoracle() */
        init_oracles();
        oracle_flg = 1;
        if (oracle_cnt == 0)
            return;
    }
    /* oracle_idx[0] is the special oracle; */
    /* oracle_idx[1..oracle_cnt-1] are normal ones */
    if (oracle_cnt <= 1 && !special)
        return;     /* (shouldn't happen) */
    idx = special ? 0 : rnd(oracle_cnt - 1);

    init_menulist(&menu);

    if (delphi)
        add_menutext(
            &menu, special ?
            "The Oracle scornfully takes all your money and says:" :
            "The Oracle meditates for a moment and then intones:");
    else
        add_menutext(&menu, "The message reads:");
    add_menutext(&menu, "");

    strncpy(chosentext, oracletext(idx), MAX_ORACLE_LENGTH);
    nextline = strtok(chosentext, "\n");
    while (nextline != NULL) {
        pline(msgc_debug, "nextline: %s", nextline);
        add_menutext(&menu, msgprintf("%s", nextline));
        nextline = strtok(NULL, "\n");
    }
    /*
    while (*chosentext) {
        const char *lineend = chosentext;
        const char *line;
        while (*lineend and !(*lineend == '\n')) {
            lineend++;
        }
        strncpy(line, chosentext, (lineend - chosentext));
        add_menutext(&menu, line);
        chosentext = lineend;
        if (*chosentext == '\n')
            chosentext++;
    }
    */

    display_menu(&menu, NULL, PICK_NONE, PLHINT_ANYWHERE,
                 NULL);
}


int
doconsult(struct monst *oracl)
{
    int umoney = money_cnt(invent);
    int u_pay, minor_cost = 50, major_cost = 200 + 10 * u.ulevel * log(u.ulevel);
    int enlcost = 20 * u.ulevel;
    int add_xpts;
    const char *qbuf;

    /* TODO: Do we want this? The purpose seems to be specifically to prevent
       repeating an Oracle donation. */
    action_completed();

    if (!oracl) {
        pline(msgc_cancelled, "There is no one here to consult.");
        return 0;
    } else if (Stormprone || !oracl->mpeaceful) {
        pline(msgc_cancelled, "%s is in no mood for consultations.",
              Monnam(oracl));
        return 0;
    } else if (!umoney) {
        pline(msgc_cancelled, "You have no money.");
        return 0;
    }

    qbuf = msgprintf("\"Inquirest thou after enlightenment?\" (%d %s)",
                     enlcost, currency(enlcost));
    switch (ynq(qbuf)) {
    case 'q':
        return 0;
    case 'y':
        if (umoney < enlcost) {
            pline(msgc_npcvoice, "\"Thou canst not afford it.\"");
            break;
        }
        money2mon(oracl, enlcost);
        pline(msgc_actionok, "You feel self-knowledgeable...");
        win_pause_output(P_MESSAGE);
        enlightenment(FALSE);
        pline_implied(msgc_actionboring, "The feeling subsides.");
        return 1;
    case 'n':
    default:
        break;
    }

    qbuf = msgprintf("\"Wilt thou settle for a minor consultation?\" (%d %s)",
                     minor_cost, currency(minor_cost));
    switch (ynq(qbuf)) {
    default:
    case 'q':
        return 0;
    case 'y':
        if (umoney < minor_cost) {
            pline(msgc_cancelled,
                  "\"Thou hast not even enough money for that!\"");
            return 0;
        }
        u_pay = minor_cost;
        break;
    case 'n':
        if (umoney <= minor_cost ||     /* don't even ask */
            (oracle_cnt == 1 || oracle_flg < 0))
            return 0;
        qbuf = msgprintf("\"Then dost thou desire a major one?\" (%d %s)",
                         major_cost, currency(major_cost));
        if (yn(qbuf) != 'y')
            return 0;
        u_pay = (umoney < major_cost ? umoney : major_cost);

        break;
    }

    money2mon(oracl, u_pay);
    add_xpts = 0;       /* first oracle of each type gives experience points */
    if (u_pay == minor_cost) {
        outrumor(1, BY_ORACLE);
        if (!u.uevent.minor_oracle) {
            add_xpts = u_pay / (u.uevent.major_oracle ? 25 : 10);
            /* 5 pts if very 1st, or 2 pts if major already done */
            if (!u.uevent.major_oracle)
                livelog_write_event(msgprintf("historic_event=%s",
                                              "heard a rumor from The Oracle"));
        }
        u.uevent.minor_oracle = TRUE;
    } else {
        boolean cheapskate = u_pay < major_cost;
        boolean firsttime  = (u.uevent.minor_oracle || u.uevent.major_oracle)
            ? FALSE : TRUE;

        outoracle(cheapskate, TRUE);
        if (!cheapskate && !u.uevent.major_oracle)
            add_xpts = u_pay / (u.uevent.minor_oracle ? 25 : 10);
        /* ~100 pts if very 1st, ~40 pts if minor already done */
        u.uevent.major_oracle = TRUE;
        historic_event(FALSE, firsttime, "received advice from The Oracle.");
        if (firsttime)
            exercise(A_WIS, !cheapskate);
    }
    if (add_xpts) {
        pluslvl(FALSE);
    }
    return 1;
}

/*rumors.c*/

