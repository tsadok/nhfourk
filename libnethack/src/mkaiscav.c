/* vim:set cin ft=c sw=4 sts=4 ts=8 et ai cino=Ls\:0t0(0 : -*- mode:c;fill-column:80;tab-width:8;c-basic-offset:4;indent-tabs-mode:nil;c-file-style:"k&r" -*-*/
/* Last modified by Nathan Eady, 2015-July-16 */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */
/* This level generation algorithm is based on Perl code by Alex Smith. */

#include "hack.h"
#include "lev.h"

/* The basic algorithm here is as follows:  each tile in the level
   starts out as "undecided", except for the outside edge, which all
   gets initially marked as wall.

   Then we visit all the other (non-outside-edge) tiles on the level
   in a randomly shuffled order, converting each one either to wall or
   to floor.  A tile becomes floor if it has two walls adjacent to it
   (including diagonally) that don't have a continuous gridbug path of
   wall squares between them.  Otherwise, if there are any adjacent
   walls, it becomes another wall.  If there are no adjacent walls,
   then we choose at random, weighted by taking the level's depth in
   the dungeon into account.

   After the first pass decides every tile, some cleanup is done,
   and after that we decide whether to add some liquid (water or
   lava), then finally place the stairs and run a wallification
   to fix up the visual appearance of the walls.

   The algorithm can be used with levels that have special content
   embedded (by placing the special content on the map after
   initializing everything to undecided but before doing anything
   else, then skipping over the special content areas when making
   changes later), but this is currently not implemented, because
   I wanted to get the basic algorithm working and tested first.
 */

#define AIS_EAST         1
#define AIS_NORTH        2
#define AIS_WEST         4
#define AIS_SOUTH        8
#define AIS_DIRMASK     15
#define AIS_SOLID       16

#define AIS_CORRIDOR    32
#define AIS_FLOOR       64
#define AIS_EDGE       128
#define AIS_WATER      256
#define AIS_LAVA       512
#define AIS_STAIR     1024
#define AIS_UNDECIDED 2048

int map[COLNO + 1][ROWNO + 1];
coord shuffledcoord[(COLNO - 1) * (ROWNO - 1)];
coord upstair, dnstair;
int aisdepth, liqcount;
#define RNGSAFEDEPTH ((aisdepth > 1) ? aisdepth : 1)
int liquid = AIS_WATER;
boolean river;
int rng = rng_main; /* mkaiscav sets this first thing */
#define mrn2(num) (rn2_on_rng(num, rng))

/* Wall direction constants defined above (1=east, 2=north, etc) are
 * combined together as appropriate, so that for example if there are
 * walls to both the east and north, that is represented by 5.
 *                E N   W       S
 *                                  1 1 1 1 1 1
 *              0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5
 * that is to say, reading each column below as a binary number:
 * south:       0 0 0 0 0 0 0 0 1 1 1 1 1 1 1 1
 * west:        0 0 0 0 1 1 1 1 0 0 0 0 1 1 1 1
 * north:       0 0 1 1 0 0 1 1 0 0 1 1 0 0 1 1
 * east:        0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1
 *                ─ │ └ ─ ─ ┘ ┴ │ ┌ │ ├ ┐ ┬ ┤ ┼
 */
int wallglyph[] = { STONE,    HWALL,    VWALL,    BLCORNER,
                    HWALL,    HWALL,    BRCORNER, TUWALL,
                    VWALL,    TLCORNER, VWALL,    TRWALL,
                    TRCORNER, TDWALL,   TLWALL,   CROSSWALL };

struct neighborhood {
    int n[8];
};

static struct neighborhood neighbors(int x, int y);
static void ais_block_pt(int x, int y, boolean check, boolean mark_corridors);
static void markwalldirs(int x, int y, int dirone, int dirtwo);
static void unmarkwalldir(int x, int y, int subtract);
static coord aisstairloc(int baseprob, int edge, int dir, const char *which);
static void placestairs(int trycount);
static boolean xor(boolean conda, boolean condb);
static void dopool(int cx, int cy, int radius, int jitter);
static coord aisplace(void);

struct neighborhood
neighbors(int x, int y)
{
    /* Note that the order here is significant.  It goes around in a circle,
       with even-indexed neighbors being orthogonal, odd diagonal.  The code
       takes advantage of this in a number of places. */
    struct neighborhood answer;
    answer.n[0] = map[x + 1][y];
    answer.n[1] = map[x + 1][y + 1];
    answer.n[2] = map[x][y + 1];
    answer.n[3] = map[x - 1][y + 1];
    answer.n[4] = map[x - 1][y];
    answer.n[5] = map[x - 1][y - 1];
    answer.n[6] = map[x][y - 1];
    answer.n[7] = map[x + 1][y - 1];
    return answer;
}

void
ais_block_pt(int x, int y, boolean check, boolean mark_corridors)
{
    int wallcount;
    if ((x == 0) || (y == 0) || (x == (COLNO - 1)) || (y == (ROWNO - 1)))
        return;
    if (check && (map[x][y] == AIS_SOLID))
        return;
    
    wallcount =
        ((map[x + 1][y] == AIS_SOLID) ? 1 : 0) +
        ((map[x][y + 1] == AIS_SOLID) ? 1 : 0) +
        ((map[x - 1][y] == AIS_SOLID) ? 1 : 0) +
        ((map[x][y - 1] == AIS_SOLID) ? 1 : 0);

    if ((wallcount == 3) || !check) {
        map[x][y] = AIS_SOLID;
        ais_block_pt(x + 1, y, 1, mark_corridors);
        ais_block_pt(x, y + 1, 1, mark_corridors);
        ais_block_pt(x - 1, y, 1, mark_corridors);
        ais_block_pt(x, y - 1, 1, mark_corridors);
        if (mark_corridors) {
            ais_block_pt(x + 1, y + 1, 1, TRUE);
            ais_block_pt(x + 1, y - 1, 1, TRUE);
            ais_block_pt(x - 1, y + 1, 1, TRUE);
            ais_block_pt(x - 1, y - 1, 1, TRUE);
        }
    }
    if (mark_corridors &&
        (map[x][y] != AIS_SOLID) &&
        (map[x][y] != AIS_CORRIDOR)) {
        struct neighborhood neigh = neighbors(x,y);
        int nidx;
        map[x][y] = AIS_CORRIDOR;
        for (nidx = 0; nidx <= 6; nidx += 2) {
            if ((neigh.n[nidx] != AIS_SOLID) &&
                (neigh.n[(nidx + 1) % 8] != AIS_SOLID) &&
                (neigh.n[(nidx + 2) % 8] != AIS_SOLID)) {
                map[x][y] = AIS_FLOOR;
            }
        }
    }
}

void
markwalldirs(int x, int y, int dirone, int dirtwo)
{
    if (map[x][y] != AIS_FLOOR) {
        /* AIS_FLOOR is the only exception, because AIS_CORRIDOR can
           and typically does actually mean it's a door, or will be a
           door after post-processing later. */
        map[x][y] |= dirone;
        map[x][y] |= dirtwo;
    }
}

void
unmarkwalldir(int x, int y, int subtract)
{
    if (map[x][y] != AIS_FLOOR) {
        /* everything except _FLOOR, for the same reason as in markwalldirs */
        map[x][y] &= ~subtract;
    }
}

coord
aisstairloc(int baseprob, int edge, int dir, const char *which)
{
    int stairx, stairy, i;
    int shuffy[ROWNO];                                                                                                                             
    int prob = baseprob;
    int poscount = 0;
    coord sloc;
    /* Initialize list of y coordinates: */
    for (i = 1; i < ROWNO; i++) {
        shuffy[i - 1] = i;
    }
    /* Now shuffle that list (so high base probabilities don't always give us
       stairs in the same corners): */
    for (i = 0; i < ROWNO - 1; i++) {
        int swapi = mrn2(ROWNO - 2);
        int swap  = shuffy[i];
        shuffy[i] = shuffy[swapi];
        shuffy[swapi] = swap;
    }
    sloc.x = 0;
    /* We start with prob set to baseprob and gradually ease it up until
       we successfully place the stairs.  At low prob, the odds of placing
       them on any given tile are low, so if baseprob is low we can get
       pretty far from the edge.  If prob starts high, the stairs almost
       always end up with an x coordinate very near edge. */
    while (prob <= 2000) {
        stairx = edge;
        while ((stairx > 0) && (stairx < COLNO)) {
            for (i = 0; i < ROWNO - 1; i++) {
                stairy = shuffy[i];
                poscount++;
                if ((map[stairx][stairy] == AIS_FLOOR) &&
                    (sloc.x == 0) && (prob > mrn2(1000))) {
                    sloc.x = stairx;
                    sloc.y = stairy;
                    map[stairx][stairy] = AIS_STAIR;
                    if (wizard)
                        pline("Chose %s stair location (%d,%d)"
                              " at prob %d (starting from %d)"
                              " having considered %d positions.",
                              which, upstair.x, upstair.y,
                              prob, baseprob, poscount);
                    return sloc;
                }
            }
            stairx += dir;
        }
        prob++;
    }
    if (wizard)
        pline("Failed to find %s stair location"
              " at prob %d (starting from %d)"
              " having considered %d positions",
              which, prob, baseprob, poscount);
    sloc.x = 0; /* signal retry */
    return sloc;
}

void
placestairs(int trycount)
{
    int stairx = 1;
    int shuffy[ROWNO];
    int i;
    for (i = 1; i < ROWNO; i++) {
        shuffy[i - 1] = i;
    }
    trycount++;
    /* Base stair-placement probability, as each tile is considered:
       high values tend to give you stairs very near the left and
       right edges, so you have to cross the whole map.  Low values
       can give you stairs pretty much anywhere.  We want each of
       these scenarios to happen frequently, hence this formula,
       which has the same maximum as mrn2(26) but gives us really
       low values much more often: */
    int baseprob = mrn2(6) * mrn2(6);
    int prob = baseprob;
    /* Shuffle the y coordinates, so high probabilities don't
       always give us stairs in the same corner. */
    for (i = 0; i < ROWNO - 1; i++) {
        int swapi = mrn2(ROWNO - 2);
        int swap  = shuffy[i];
        shuffy[i] = shuffy[swapi];
        shuffy[swapi] = swap;
    }
    while (prob++ <= 2000 && !(upstair.x)) {
        while (stairx < COLNO && !(upstair.x)) {
            for (i = 0; i < ROWNO - 1; i++) {
                int stairy = shuffy[i];
                if ((map[stairx][stairy] == AIS_FLOOR) &&
                    !(upstair.x) && (prob > mrn2(1000))) {
                    coord ups;
                    ups.x = stairx;
                    ups.y = stairy;
                    upstair = ups;
                    map[stairx][stairy] = AIS_STAIR;
                }
                stairx++;
            }
        }
    }
    /* Reshuffle the stair order for doing the other stairs, so high
       baseprobs don't tend to position the up and down stairs in the
       same row. */
    for (i = 0; i < ROWNO - 1; i++) {
        int swapi = mrn2(ROWNO - 2);
        int swap  = shuffy[i];
        shuffy[i] = shuffy[swapi];
        shuffy[swapi] = swap;
    }
    stairx = COLNO - 1;
    prob   = baseprob;
    while (prob++ <= 2000 && !(dnstair.x)) {
        while (stairx > 0 && !(dnstair.x)) {
            for (i = 0; i < ROWNO - 1; i++) {
                int stairy = shuffy[i];
                if ((map[stairx][stairy] == AIS_FLOOR) &&
                    !(dnstair.x) && (prob > mrn2(1000))) {
                    coord dns;
                    dns.x = stairx;
                    dns.y = stairy;
                    dnstair = dns;
                    map[stairx][stairy] = AIS_STAIR;
                }
                stairx--;
            }
        }
    }
    if ((trycount < 1000) && !(upstair.x && dnstair.x)) {
        /* The prob loop makes this unlikely, but just in case */
        placestairs(trycount);
    }
}

boolean
xor(boolean conda, boolean condb)
{
    /* This function exists purely because I can't bring myself to
       write something like this inline, for code clarity reasons.
       What I really want is to write (conda xor condb), but at
       least with the function call, xor(conda, condb), the intent
       is clear.
    */
    return ((!!conda) != (!!condb));
}

void
dopool(int cx, int cy, int radius, int jitter)
{
    int stretch = 1 + mrn2(50);
    int x, y;
    int xvar = radius * stretch / 100;
    if (jitter == 0)
        jitter = mrn2(50) / 20;
    for (x = cx - xvar; x <= cx + xvar; x++) {
        if ((x > 0) && (x < COLNO - 1)) {
            for (y = cy - radius; y <= cy + radius; y++) {
                int dist = mrn2(jitter || 1)
                    + sqrt(((x - cx) / stretch) * ((x - cx) / stretch) +
                           ((y - cy) * (y - cy)));
                if ((dist <= radius) &&
                    !(map[x][y] & AIS_STAIR) &&
                    (!(map[x][y] & AIS_SOLID) &&
                     !(map[x][y] & AIS_CORRIDOR)))
                    map[x][y] = liquid;
            }
        }
    }
}

void
mkaiscav(struct level *lev)
{
    aisdepth = (depth(&lev->z) - 25) * 2;
    rng = rng_for_level(&lev->z);
    int x, y, i, nidx;
    int num_of_shuffledcoord = ((COLNO - 1) * (ROWNO - 1));
    coord spot;
    struct branch *br = Is_branchlev(&lev->z);
    
    /* Initialize the map to undecided, except the edges: */
    i = 0;
    for (x = 0; x < COLNO; x++) {
        for (y = 0; y < ROWNO; y++) {
            if ((x == 0) || (x == (COLNO - 1)) ||
                (y == 0) || (y == (ROWNO - 1)))
                map[x][y] = AIS_SOLID;
            else {
                coord cc;
                map[x][y] = AIS_UNDECIDED;
                cc.x = x;
                cc.y = y;
                shuffledcoord[i] = cc;
                i++;
            }
        }
    }
    /* Shuffle the coordinates into a random order: */
    for (i = 0; i < num_of_shuffledcoord; i++) {
        int   swapi = mrn2(num_of_shuffledcoord);
        coord swapc = shuffledcoord[i];
        shuffledcoord[i]     = shuffledcoord[swapi];
        shuffledcoord[swapi] = swapc;
    }
    /* TODO: in principle, the algorithm supports embedding
       special-level content.  This is roughly the point in the
       process at which it would ought to be inserted (and marked
       special, so that the adjustment phase, later, would not
       adjust it in undesirable ways).  Implementing the ability
       to embed e.g. the wizard's tower, fake towers, demon
       lairs, or other special features is deferred:  I want
       to implement it eventually, but for now, let's just get
       the basic level generation part working.
    */
    for (i = 0; i < num_of_shuffledcoord; i++) {
        x = shuffledcoord[i].x;
        y = shuffledcoord[i].y;
        if (map[x][y] == AIS_UNDECIDED) {
            struct neighborhood nhood = neighbors(x, y);
            int transitioncount = 0;
            /* To reduce diagonal chokepoints, we treat any diagonally
               adjacent spot "between" two (orthogonally adjacent)
               walls as a floor.  Note that we aren't actually
               converting that tile to floor, just our idea of what
               the neighbor tile is as we consider it. */
            for (nidx = 0; nidx <= 6; nidx += 2) {
                if ((nhood.n[nidx] == AIS_SOLID) &&
                    (nhood.n[(nidx + 2) % 8] == AIS_SOLID) &&
                    (nhood.n[nidx + 1] == AIS_SOLID))
                    nhood.n[nidx + 1] = AIS_FLOOR;
            }
            for (nidx = 0; nidx < 8; nidx++) {
                if (xor((nhood.n[nidx] == AIS_SOLID),
                        (nhood.n[(nidx + 1) % 8] == AIS_SOLID)))
                    transitioncount++;
            }
            map[x][y] = (transitioncount > 2) ? AIS_FLOOR :
                (transitioncount == 2) ? AIS_SOLID :
                (mrn2(100) < ( RNGSAFEDEPTH * RNGSAFEDEPTH * RNGSAFEDEPTH
                              / (100 * 100))) ? AIS_SOLID : AIS_FLOOR;
            if (map[x][y] == AIS_SOLID)
                ais_block_pt(x, y, FALSE, FALSE);
        }
    }

    /* Remove orphaned walls from the map: */
    for (x = 1; x < (COLNO - 1); x++) {
        for (y = 1; y < (ROWNO - 2); y++) {
            if ((map[x + 1][y] != AIS_SOLID) &&
                (map[x - 1][y] != AIS_SOLID) &&
                (map[x][y + 1] != AIS_SOLID) &&
                (map[x][y - 1] != AIS_SOLID) &&
                (map[x][y] == AIS_SOLID))
                map[x][y] = AIS_FLOOR;
        }
    }

    /* Mark corridors on the map */
    for (x = 1; x < (COLNO -1); x++) {
        for (y = 1; y < (ROWNO - 1); y++) {
            ais_block_pt(x, y, TRUE, TRUE);
        }
    }

    if (mrn2(100) > 50) {
        /* To produce longer corridors, block any squares that are
           diagonally adjacent to a corridor but not orthogonally
           adjacent to a corridor or which have both squares a
           knight's move from the corridor open. */
        boolean anychanges = TRUE;
        int sanitycount = 0;
        while (anychanges && (sanitycount++ < 9999)) {
            anychanges = FALSE;
            for (i = 0; i < num_of_shuffledcoord; i++) {
                x = shuffledcoord[i].x;
                y = shuffledcoord[i].y;
                int orthocorridor = 0;
                struct neighborhood hood = neighbors(x, y);
                if ((map[x][y] != AIS_SOLID) &&
                    (map[x][y] != AIS_CORRIDOR)) {
                    for (nidx = 0; nidx <= 6; nidx += 2) {
                        if (hood.n[nidx] == AIS_CORRIDOR)
                            orthocorridor++;
                    }
                    if (orthocorridor == 0) {
                        for (nidx = 1; nidx <= 7; nidx += 2) {
                            if ((hood.n[nidx] == AIS_CORRIDOR) &&
                                ((hood.n[(nidx + 3) % 8] == AIS_SOLID) ||
                                 (hood.n[(nidx + 3) % 8] == AIS_CORRIDOR) ||
                                 (hood.n[(nidx + 5) % 8] == AIS_SOLID) ||
                                 (hood.n[(nidx + 5) % 8] == AIS_CORRIDOR))) {
                                anychanges = TRUE;
                                ais_block_pt(x, y, FALSE, TRUE);
                            }
                        }
                    }
                }
            }
        }
    }

    /* If a corridor has a length of exactly 2, convert it back to
       room squares.  This looks neater than the alternative, although
       the effect is minor. */
    for (x = 1; x < COLNO - 1; x++) {
        for (y = 1; y < ROWNO - 1; y++) {
            if (map[x][y] != AIS_CORRIDOR) {
                int da, db, di;
                for (di = 0; di < 4; di++) {
                    switch (di) {
                    case 1:
                        da = 0; db = 1;
                        break;
                    case 2:
                        da = 0; db = -1;
                        break;
                    case 3:
                        da = 1; db = 0;
                        break;
                    default:
                        da = -1; db = 0;
                    }
                    if (!(((x == (COLNO - 2)) && (da == 1)) ||
                          ((y == (ROWNO - 2)) && (db == 2)) ||
                          ((x == 1) && (da == -1)) ||
                          ((y == 1) && (db == -1))) &&
                        !(map[x + da][y + db] == AIS_CORRIDOR) &&
                        !((map[x + 2 * da][y + 2 * db] == AIS_CORRIDOR) ||
                          (map[x + da + db][y + da + db] == AIS_CORRIDOR) ||
                          (map[x + da - db][y - da + db] == AIS_CORRIDOR) ||
                          (map[x + db][y + da] == AIS_CORRIDOR) ||
                          (map[x - db][y - da] == AIS_CORRIDOR) ||
                          (map[x - da][y - db] == AIS_CORRIDOR))) {
                        if (map[x][y] == AIS_CORRIDOR)
                            map[x][y] = AIS_FLOOR;
                        if (map[x + da][y + db] == AIS_CORRIDOR)
                            map[x + da][y + db] = AIS_FLOOR;
                    }
                }
            }
        }
    }

    /* Work out where walls should be.  We start by imagining a square
       drawn around every open floor space, then imagine removing
       parts of the square that do not connect to other walls, and then
       each piece of each square only gets drawn if it's on a solid tile.

       First, add the wall direction so the floor areas are surrounded: */
    
    for (x = 1; x < (COLNO - 1); x++) {
        for (y = 1; y < (ROWNO - 1); y++) {
            if (map[x][y] == AIS_FLOOR) {
                markwalldirs(x + 1, y, AIS_NORTH, AIS_SOUTH);
                markwalldirs(x - 1, y, AIS_NORTH, AIS_SOUTH);
                markwalldirs(x, y + 1, AIS_WEST, AIS_EAST);
                markwalldirs(x, y - 1, AIS_WEST, AIS_EAST);
                markwalldirs(x + 1, y + 1, AIS_NORTH, AIS_WEST);
                markwalldirs(x - 1, y + 1, AIS_NORTH, AIS_EAST);
                markwalldirs(x + 1, y - 1, AIS_SOUTH, AIS_WEST);
                markwalldirs(x - 1, y - 1, AIS_SOUTH, AIS_EAST);
            }
        }
    }
    /* Now unmark wall directions that would point straight to floor areas: */
    for (x = 0; x < COLNO; x++) {
        for (y = 0; y < ROWNO; y++) {
            if ((x < (COLNO - 1)) &&
                !(map[x + 1][y] & (AIS_SOLID | AIS_CORRIDOR)))
                unmarkwalldir(x, y, AIS_EAST);
            if ((x > 0) &&
                !(map[x - 1][y] & (AIS_SOLID | AIS_CORRIDOR)))
                unmarkwalldir(x, y, AIS_WEST);
            if ((y < ROWNO - 1) &&
                !(map[x][y + 1] & (AIS_SOLID | AIS_CORRIDOR)))
                unmarkwalldir(x, y, AIS_SOUTH);
            if ((y > 0) &&
                !(map[x][y - 1] & (AIS_SOLID | AIS_CORRIDOR)))
                unmarkwalldir(x, y, AIS_NORTH);
        }
    }

    /* Mark corridors on walls secret if it doesn't create an obvious
       dead end (it often does).  The exception is dug-out corner
       tiles, which are converted to diagonal chokepoints instead. */
    for (i = 0; i < num_of_shuffledcoord; i++) {
        x = shuffledcoord[i].x;
        y = shuffledcoord[i].y;
        if (map[x][y] & AIS_CORRIDOR) {
            struct neighborhood hood = neighbors(x, y);
            boolean lonely = TRUE;

            for (nidx = 0; nidx <= 6; nidx += 2) {
                if (hood.n[nidx] & AIS_CORRIDOR)
                    lonely = FALSE;
            }
            if (lonely) {
                map[x][y] |= AIS_SOLID;
                if (((map[x][y] & AIS_NORTH) ||
                     (map[x][y] & AIS_SOUTH) ||
                     (map[x][y] & AIS_WEST)  ||
                     (map[x][y] & AIS_EAST)))
                    /* This almost never happens -- NAE */
                    map[x][y] = AIS_FLOOR;
            }
        }
    }

    /* Go ahead and place the stairs before laying down liquids.  So
       on the off chance the whole level fills up with liquid, we
       won't be without any place to put the stairs. */
    placestairs(0);

    liquid = (aisdepth > (mrn2(1000) / 9)) ? AIS_LAVA : AIS_WATER;
    if (2 > mrn2( RNGSAFEDEPTH )) { /* river */
        int minbreadth = 1 + mrn2(1);
        int cx = (COLNO / 4) + mrn2(COLNO / 2);
        int cy = 0, cydir = 1, cxdir = 0;
        river = TRUE;
        if (50 < mrn2(100)) {
            cy = ROWNO - 1;
            cydir = -1;
        }
        for (i = 0; i < ROWNO; i++) {
            int hflex = minbreadth * 3 / 2;
            if ((100 * cx / COLNO) > 90) {
                cx += hflex - mrn2(hflex || 1);
                cxdir = -2;
            } else if ((100 * cx / COLNO) < 10) {
                cx += mrn2(hflex || 1) - hflex;
                cxdir = 2;
            } else if (abs(cxdir) > (minbreadth * 2)) {
                cxdir = cxdir * 100 / mrn2(100);
                cx += cxdir;
            } else {
                cxdir += hflex - mrn2((hflex * 2) || 1);
                cx += cxdir;
            }
            dopool(cx, cy, minbreadth + mrn2(minbreadth || 1), 0);
            cy += cydir;
        }
    } else { /* No river.  0 or more pools. */
        int pcount = ((mrn2(6) * mrn2( RNGSAFEDEPTH * 8)) - 150) / 100;
        for (i = 0; i < pcount; i++) {
            liqcount++;
            int cx = mrn2(COLNO);
            int cy = mrn2(ROWNO);
            int radius = 2 + (mrn2(mrn2(mrn2(700) || 1) || 1) / 100);
            dopool(cx, cy, radius, 0);
        }
    }

    for (x = 0; x < COLNO; x++) {
        for (y = 0; y < ROWNO; y++) {
            lev->locations[x][y].typ = STONE; /* Default */
            if ((x == upstair.x) && (y == upstair.y)) {
                if (x && y)
                    mkstairs(lev, x, y, 1, NULL);
                else
                    impossible("Badly chosen upstair location, (%d,%d)", upstair.x, upstair.y);
            } else if ((x == dnstair.x) && (y == dnstair.y)) {
                if (!Invocation_lev(&lev->z))
                    if (x && y)
                        mkstairs(lev, x, y, 0, NULL);
                    else
                        impossible("Badly chosen downstair location, (%d,%d)", dnstair.x, dnstair.y);
                else {
                    panic("mkaiscav: not programmed to make the VS.");
                }
            } else if ((map[x][y] & AIS_CORRIDOR) &&
                       (map[x][y] & AIS_SOLID)) {
                lev->locations[x][y].typ = SCORR;
            } else if (map[x][y] & AIS_SOLID) {
                lev->locations[x][y].typ = wallglyph[map[x][y] & AIS_DIRMASK];
            } else if (map[x][y] & AIS_LAVA) {
                lev->locations[x][y].typ = LAVAPOOL;
            } else if (map[x][y] & AIS_WATER) {
                lev->locations[x][y].typ = POOL;
            } else if (map[x][y] & AIS_CORRIDOR) {
                lev->locations[x][y].typ = CORR;
            } else {
                lev->locations[x][y].typ = ROOM;
            }
        }
    }
    /* Ok, that takes care of the terrain. */

    /* Place branch stairs/ladder, if applicable: */
    if (br) {
        coord brloc = aisstairloc(0, 1, 1, "branch");
        place_branch(lev, br, brloc.x, brloc.y);
    }

    /* Place traps: */
    for (i = (5 + mrn2(100 + RNGSAFEDEPTH ) / 12); i > 0; i--) {
        int typ = mrn2(TRAPNUM);
        while ((typ == NO_TRAP) ||
               (typ == MAGIC_PORTAL) || (typ == VIBRATING_SQUARE) ||
               (((typ == HOLE) || (typ == TRAPDOOR)) &&
                !(can_fall_thru(lev)))) {
            typ = mrn2(TRAPNUM);
        }
        spot = aisplace();
        maketrap(lev, spot.x, spot.y, typ, rng);
        
    }

    /* Place boulders: */
    for (i = ((mrn2( RNGSAFEDEPTH )) / 15); i > 0; i--) {
        spot = aisplace();
        mksobj_at(BOULDER, lev, spot.x, spot.y, TRUE, FALSE, rng);
    }

    /* Statuary */
    if (!mrn2(3)) {
        for (i = mrn2(6); i > 0; i--) {
            spot = aisplace();
            const struct permonst *mptr = &mons[rndmonnum(&lev->z, rng)];
            struct obj *statue = mkcorpstat(STATUE, NULL, mptr,
                                            lev, spot.x, spot.y, FALSE, rng);
            struct monst *mtmp = makemon(&mons[statue->corpsenm], lev, COLNO, ROWNO, MM_NOCOUNTBIRTH);
            if (mtmp) {
                while (mtmp->minvent) {
                    struct obj * otmp = mtmp->minvent;
                    otmp->owornmask = 0;
                    obj_extract_self(otmp);
                    add_to_container(statue, otmp);
                }
                statue->owt = weight(statue);
                mongone(mtmp);
            }
        }
    }

    /* Place demons: */
    if (In_hell(&lev->z)) {
        for (i = (3 + mrn2(2 + ( RNGSAFEDEPTH / 10))); i > 0; i--) {
            spot = aisplace();
            makemon(mkclass(&lev->z, S_DEMON, 0, rng), lev, spot.x, spot.y, 0);
        }
    }

    /* Place special monsters: */
    switch(mrn2(13)) {
    case 1:
        spot = aisplace();
        makemon(&mons[PM_TITAN], lev, spot.x, spot.y, 0);
        /* fall through */
    case 2:
        for (i = mrn2(1 + ( RNGSAFEDEPTH / 8)); i > 0; i--) {
            spot = aisplace();
            makemon(((liquid == AIS_LAVA) ?
                     &mons[PM_FIRE_ELEMENTAL] : &mons[PM_GREMLIN]),
                    lev, spot.x, spot.y, 0);
        }
        break;
    case 3:
        for (i = (3 + mrn2(1 + ( RNGSAFEDEPTH / 25))); i > 0; i--) {
            spot = aisplace();
            makemon(mkclass(&lev->z, S_XORN, 0, rng),
                    lev, spot.x, spot.y, 0);
        }
        break;
    case 4:
        for (i = (3 + mrn2(1 + ( RNGSAFEDEPTH / 18))); i > 0; i--) {
            spot = aisplace();
            makemon(mkclass(&lev->z, S_DRAGON, 0, rng),
                    lev, spot.x, spot.y, 0);
        }
        break;
    case 5:
        for (i = (1 + mrn2(1 + ( RNGSAFEDEPTH / 30))); i > 0; i--) {
            spot = aisplace();
            makemon(mkclass(&lev->z, S_JABBERWOCK, 0, rng),
                    lev, spot.x, spot.y, 0);
        }
        break;
    case 6:
        for (i = (1 + mrn2(1 + ( RNGSAFEDEPTH / 40))); i > 0; i--) {
            spot = aisplace();
            makemon(mkclass(&lev->z, S_LICH, 0, rng),
                    lev, spot.x, spot.y, 0);
        }
        break;
    case 7:
        for (i = (4 + mrn2(1 + ( RNGSAFEDEPTH / 15))); i > 0; i--) {
            spot = aisplace();
            makemon(mkclass(&lev->z, S_RUSTMONST, 0, rng),
                    lev, spot.x, spot.y, 0);
        }
        break;
    case 8:
        for (i = (5 + mrn2(1 + ( RNGSAFEDEPTH / 10))); i > 0; i--) {
            spot = aisplace();
            makemon(mkclass(&lev->z, S_TROLL, 0, rng),
                    lev, spot.x, spot.y, 0);
        }
        break;
    case 9:
        lev->flags.graveyard = 1;
        for (i = (10 + mrn2(10 + ( RNGSAFEDEPTH / 10))); i > 0; i--) {
            spot = aisplace();
            makemon(morguemon(&lev->z, rng), lev, spot.x, spot.y, 0);
        }
        break;
    case 10:
        for (i = (5 + mrn2(1 + ( RNGSAFEDEPTH / 15))); i > 0; i--) {
            spot = aisplace();
            makemon(((i % 2) ? &mons[PM_WEREWOLF] :
                     (i % 3) ? &mons[PM_WEREJACKAL] : &mons[PM_WERERAT]),
                    lev, spot.x, spot.y, 0);
        }
        break;
    case 11:
        for (i = (10 + mrn2(1 + ( RNGSAFEDEPTH / 15))); i > 0; i--) {
            spot = aisplace();
            makemon(squadmon(&lev->z), lev, spot.x, spot.y, 0);
        }
        break;
    default:
        for (i = (3 + mrn2(1 + ( RNGSAFEDEPTH / 25))); i > 0; i--) {
            spot = aisplace();
            makemon(&mons[PM_DOPPELGANGER], lev, spot.x, spot.y, 0);
        }
        break;
    }

    /* Place random monsters: */
    for (i = (5 + ( RNGSAFEDEPTH / 25) +
              mrn2(1 + ( RNGSAFEDEPTH / 30))); i > 0; i--) {
        spot = aisplace();
        makemon(NULL, lev, spot.x, spot.y, 0);
    }
    
    /* Place random objects: */
    for (i = 12 + mrn2(8); i > 0; i--) {
        spot = aisplace();
        mkobj_at(0, lev, spot.x, spot.y, TRUE, rng);
    }
}

static coord
aisplace(void)
{
    coord cc;
    int tries = 0;
    cc.x = 0;
    while ((cc.x == 0) ||
           ((map[cc.x][cc.y] & AIS_LAVA) && tries < 9999) ||
           ((map[cc.x][cc.y] & AIS_SOLID) && tries < 800) ||
           ((map[cc.x][cc.y] & AIS_WATER) && tries < 600) ||
           ((!(map[cc.x][cc.y] == AIS_FLOOR)) && tries < 400)) {
        tries++;
        cc.x = 2 + mrn2(COLNO - 4);
        cc.y = 1 + mrn2(ROWNO - 2);
    }
    return cc;
}
