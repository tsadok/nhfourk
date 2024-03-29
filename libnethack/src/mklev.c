/* vim:set cin ft=c sw=4 sts=4 ts=8 et ai cino=Ls\:0t0(0 : -*- mode:c;fill-column:80;tab-width:8;c-basic-offset:4;indent-tabs-mode:nil;c-file-style:"k&r" -*-*/
/* Last modified by Alex Smith, 2015-11-11 */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "lev.h"

/* croom->lx etc are schar (width <= int), so % arith ensures that */
/* conversion of result to int is reasonable */

static void mkfount(struct level *lev, int, struct mkroom *);
static void mksink(struct level *lev, struct mkroom *);
static void mkaltar(struct level *lev, struct mkroom *);
static void mkbench(struct level *lev, struct mkroom *);
static void mkgrave(struct level *lev, struct mkroom *);
static void mkpuddles(struct level *lev, struct mkroom *);
static void makevtele(struct level *lev);
static void makelevel(struct level *lev);
static void mineralize(struct level *lev);
static boolean bydoor(struct level *lev, xchar, xchar);
static struct mkroom *find_branch_room(struct level *lev, coord *);
static struct mkroom *pos_to_room(struct level *lev, xchar x, xchar y);
static boolean place_niche(struct level *lev, struct mkroom *, int *, int *,
                           int *);
static void makeniche(struct level *lev, int);
static void make_niches(struct level *lev);

/* K-Mod, I've implemented a couple of different sorting functions for use with
   different level styles. */
/* static int do_comp(const void *, const void *); */
static int lx_comp(const void *, const void *);
static int angle_comp(const void *, const void *);

static void dosdoor(struct level *lev, xchar, xchar, struct mkroom *, int);
static void join(struct level *lev, int a, int b, boolean, int *);
static void do_room_or_subroom(struct level *lev, struct mkroom *, int, int,
                               int, int, boolean, schar, boolean, boolean,
                               boolean);
static void makerooms(struct level *lev, int *smeq);
static void finddpos(struct level *lev, coord *, xchar, xchar, xchar, xchar);
static void mkinvpos(xchar, xchar, int);
static void mk_knox_portal(struct level *lev, xchar, xchar);

#define create_vault(lev) create_room(lev, -1, -1, 2, 2, -1, -1, \
                                      VAULT, TRUE, NULL, FALSE)
#define do_vault()        (vault_x != -1)
static xchar vault_x, vault_y;
static boolean made_branch;     /* used only during level creation */

#define mrn2(x) mklev_rn2(x, lev)
#define mrng()  rng_for_level(&lev->z)

/* Args must be (const void *) so that qsort will always be happy. */
/* K-Mod, this use to be called "do_comp" */
static int
lx_comp(const void *vx, const void *vy)
{
    const struct mkroom *x, *y;

    x = (const struct mkroom *)vx;
    y = (const struct mkroom *)vy;

    /* sort by x coord first */
    if (x->lx != y->lx)
        return x->lx - y->lx;

    /* sort by ly if lx is equal The additional criterion is necessary to get
       consistent sorting across platforms with different qsort
       implementations. */
    return x->ly - y->ly;
}

static int
angle_comp(const void *vx, const void *vy)
{
    const struct mkroom *a, *b;
    int ax, ay, bx, by; /* x and y, relative the the map centre (COLNO/2, ROWNO/2) */
    /* double ra, rb, ta, tb; // radius and angle */
    int aquad, bquad; /* circle quadrants */

    a = (const struct mkroom *)vx;
    b = (const struct mkroom *)vy;

    ax = (a->lx + a->hx - COLNO)/2;
    bx = (b->lx + b->hx - COLNO)/2;
    ay = (a->ly + a->hy - ROWNO)/2;
    by = (b->ly + b->hy - ROWNO)/2;

    /* normalize */
    ay*=COLNO;
    ay/=ROWNO;
    by*=COLNO;
    by/=ROWNO;

    /* First, the quadrant check. */
    aquad = (ax >= 0)? ((ay >= 0) ? 0 : 3) :((ay >= 0)? 1 : 2);
    bquad = (bx >= 0)? ((by >= 0) ? 0 : 3) :((by >= 0)? 1 : 2);

    if (aquad != bquad) {
        return (aquad < bquad) ? -1 : 1;
    } else {
        /* same quadrant. Compare tan value. ay/ax vs by/bx */
        if (ax == 0)
            return (bx == 0) ? 0 : -1;
        if (bx == 0)
            return 1;
        /* I've slapped an arbitrary factor of 20 on it just to reduce rounding errors */
        if ((20*ay) / ax == (20*by) / bx)
            return 0;
        else
            return ((20*ay) / ax < (20*by) / bx) ? -1 : 1;
    }
}

static void
finddpos(struct level *lev, coord * cc, xchar xl, xchar yl, xchar xh, xchar yh)
{
    xchar x, y;

    /* K-Mod: I'm going to make this a bit more random. */
    xchar rx = rn2(xh-xl+1);
    xchar ry = rn2(yh-yl+1);

    for (x = 0; x <= xh-xl; x++) {
        for (y = 0; y <= yh-yl; y++) {
            if (okdoor(lev, xl+(x+rx)%(xh-xl+1) , yl+(y+ry)%(yh-yl+1))) {
                cc->x = xl+(x+rx)%(xh-xl+1);
                cc->y = yl+(y+ry)%(yh-yl+1);
                return; /* "goto" can get stuffed */
            }
        }
    }

    for (x = xl; x <= xh; x++)
        for (y = yl; y <= yh; y++)
            if (IS_DOOR(lev->locations[x][y].typ) ||
                lev->locations[x][y].typ == SDOOR)
                goto gotit;
    /* cannot find something reasonable -- strange */
    x = xl;
    y = yh;
gotit:
    cc->x = x;
    cc->y = y;
    return;
}

void
sort_rooms(struct level *lev, enum levstyle style)
{
    int (*comp_func)(const void *, const void *);

    switch (style) {
    default: /* standard */
        comp_func = lx_comp;
        break;
    case LEVSTYLE_RING:
        comp_func = angle_comp;
        break;
    case LEVSTYLE_HUB:
        /* no sorting required */
        return;
    }
    qsort(lev->rooms, lev->nroom, sizeof (struct mkroom), comp_func);
}

static void
do_room_or_subroom(struct level *lev, struct mkroom *croom, int lowx, int lowy,
                   int hix, int hiy, boolean lit, schar rtype, boolean special,
                   boolean is_room, boolean canbeshaped)
{
    int x, y;
    struct rm *loc;

    /* locations might bump level edges in wall-less rooms */
    /* add/subtract 1 to allow for edge locations */
    if (!lowx)
        lowx++;
    if (!lowy)
        lowy++;
    if (hix >= COLNO - 1)
        hix = COLNO - 2;
    if (hiy >= ROWNO - 1)
        hiy = ROWNO - 2;

    croom->lx = lowx;
    croom->hx = hix;
    croom->ly = lowy;
    croom->hy = hiy;
    croom->rtype = rtype;
    croom->doorct = 0;
    /* if we're not making a vault, lev->doorindex will still be 0 if we are,
       we'll have problems adding niches to the previous room unless fdoor is
       at least doorindex */
    croom->fdoor = lev->doorindex;
    croom->irregular = FALSE;

    croom->nsubrooms = 0;
    croom->sbrooms[0] = NULL;
    if (!special) {
        for (x = lowx - 1; x <= hix + 1; x++)
            for (y = lowy - 1; y <= hiy + 1; y += (hiy - lowy + 2)) {
                lev->locations[x][y].typ = HWALL;
                lev->locations[x][y].horizontal = 1;
                /* For open/secret doors. */
            }
        for (x = lowx - 1; x <= hix + 1; x += (hix - lowx + 2))
            for (y = lowy; y <= hiy; y++) {
                lev->locations[x][y].typ = VWALL;
                lev->locations[x][y].horizontal = 0;
                /* For open/secret doors. */
            }
        for (x = lowx; x <= hix; x++) {
            loc = &lev->locations[x][lowy];
            for (y = lowy; y <= hiy; y++)
                loc++->typ = ROOM;
        }
        if (is_room) {
            lev->locations[lowx - 1][lowy - 1].typ = TLCORNER;
            lev->locations[hix + 1][lowy - 1].typ = TRCORNER;
            lev->locations[lowx - 1][hiy + 1].typ = BLCORNER;
            lev->locations[hix + 1][hiy + 1].typ = BRCORNER;
        }
        if (canbeshaped && (hix - lowx > 3) && (hiy - lowy > 3)) {
            int xcmax = 0, ycmax = 0, xcut = 0, ycut = 0;
            boolean dotl = FALSE, dotr = FALSE, dobl = FALSE, dobr = FALSE,
                    docenter = FALSE;
            switch (mrn2(12)) {
            case 1:
            case 2:
            case 3:
                /* L-shaped */
                xcmax = (hix - lowx) * 2 / 3;
                ycmax = (hiy - lowy) * 2 / 3;
                switch(mrn2(4)) {
                case 1:
                    dotr = TRUE;
                    break;
                case 2:
                    dobr = TRUE;
                    break;
                case 3:
                    dotl = TRUE;
                    break;
                default:
                    dobl = TRUE;
                    break;
                }
                break;
            case 4:
            case 5:
                /* T-shaped */
                xcmax = (hix - lowx) * 2 / 5;
                ycmax = (hiy - lowy) * 2 / 5;
                switch(mrn2(4)) {
                case 1:
                    dotr = TRUE;
                    dotl = TRUE;
                    break;
                case 2:
                    dobr = TRUE;
                    dobl = TRUE;
                    break;
                case 3:
                    dotr = TRUE;
                    dobr = TRUE;
                    break;
                default:
                    dotl = TRUE;
                    dobl = TRUE;
                    break;
                }
                break;
            case 6:
            case 7:
                /* S/Z shaped ("Tetris Piece") */
                xcmax = (hix - lowx) / 3;
                ycmax = (hiy - lowy) / 3;
                switch(mrn2(2)) {
                case 1:
                    dotr = TRUE;
                    dobl = TRUE;
                    break;
                default:
                    dotl = TRUE;
                    dobr = TRUE;
                    break;
                }
                break;
            case 8:
                /* Plus Shaped */
                xcmax = (hix - lowx) * 2 / 5;
                ycmax = (hiy - lowy) * 2 / 5;
                dotr = TRUE;
                dotl = TRUE;
                dobr = TRUE;
                dobl = TRUE;
                break;
            case 9:
                /* square-O shaped (pillar cut out of middle) */
                xcmax = (hix - lowx) / 2;
                ycmax = (hiy - lowy) / 2;
                docenter = TRUE;
                break;
            case 10:
                xcmax = (hix - lowx) / 3;
                ycmax = (hiy - lowy) / 3;
                if ((xcmax > 1) || (ycmax > 1)) {
                    /* X-shaped (corners _and_ middle cut out) */
                    if (xcmax > 1)
                        xcmax--;
                    else
                        ycmax--;
                    dotr = TRUE;
                    dotl = TRUE;
                    dobr = TRUE;
                    dobl = TRUE;
                } else {
                    /* Not large enough, convert to O: */
                    xcmax = (hix - lowx) / 2;
                    ycmax = (hiy - lowy) / 2;
                }
                docenter = TRUE;
                break;
            /* TODO: oval */
            default:
                /* Rectangular -- nothing to do */
                break;
            }
            if (dotr || dotl || dobr || dobl || docenter) {
                croom->irregular = TRUE;
                xcut = 1 + mrn2(xcmax);
                ycut = 1 + mrn2(ycmax);
                /* Sometimes, instead of a small cut, do a max cut.
                   This improves the probability of a larger cut,
                   without removing the possibility for small ones. */
                if ((xcut < (xcmax / 2)) && !mrn2(3))
                    xcut = xcmax;
                if ((ycut < (ycmax / 2)) && !mrn2(3))
                    ycut = ycmax;
            }
            /* Now do the actual cuts. */
            if (dotr) {
                /* top-right cutout */
                for (y = 0; y < ycut; y++) {
                    for (x = 0; x < xcut; x++) {
                        lev->locations[hix + 1 - x][lowy + y - 1].typ = STONE;
                    }
                    lev->locations[hix + 1 - xcut][lowy + y - 1].typ = VWALL;
                }
                for (x = 0; x < xcut; x++)
                    lev->locations[hix + 1 - x][lowy + ycut - 1].typ = HWALL;
                lev->locations[hix + 1 - xcut][lowy + ycut - 1].typ = BLCORNER;
                lev->locations[hix + 1][lowy + ycut - 1].typ = TRCORNER;
                lev->locations[hix + 1 - xcut][lowy - 1].typ = TRCORNER;
            }
            if (dobr) {
                /* bottom-right cutout */
                for (y = 0; y < ycut; y++) {
                    for (x = 0; x < xcut; x++) {
                        lev->locations[hix + 1 - x][hiy + 1 - y].typ = STONE;
                    }
                    lev->locations[hix + 1 - xcut][hiy + 1 - y].typ = VWALL;
                }
                for (x = 0; x < xcut; x++)
                    lev->locations[hix + 1 - x][hiy + 1 - ycut].typ = HWALL;
                lev->locations[hix + 1 - xcut][hiy + 1 - ycut].typ = TLCORNER;
                lev->locations[hix + 1][hiy + 1 - ycut].typ = BRCORNER;
                lev->locations[hix + 1 - xcut][hiy + 1].typ = BRCORNER;
            }
            if (dotl) {
                /* top-left cutout */
                for (y = 0; y < ycut; y++) {
                    for (x = 0; x < xcut; x++) {
                        lev->locations[lowx + x - 1][lowy + y - 1].typ = STONE;
                    }
                    lev->locations[lowx + xcut - 1][lowy + y - 1].typ = VWALL;
                }
                for (x = 0; x < xcut; x++)
                    lev->locations[lowx + x - 1][lowy + ycut - 1].typ = HWALL;
                lev->locations[lowx + xcut - 1][lowy + ycut - 1].typ = BRCORNER;
                lev->locations[lowx - 1][lowy + ycut - 1].typ = TLCORNER;
                lev->locations[lowx + xcut - 1][lowy - 1].typ = TLCORNER;
            }
            if (dobl) {
                /* bottom-left cutout */
                for (y = 0; y < ycut; y++) {
                    for (x = 0; x < xcut; x++) {
                        lev->locations[lowx + x - 1][hiy + 1 - y].typ = STONE;
                    }
                    lev->locations[lowx + xcut - 1][hiy + 1 - y].typ = VWALL;
                }
                for (x = 0; x < xcut; x++)
                    lev->locations[lowx + x - 1][hiy + 1 - ycut].typ = HWALL;
                lev->locations[lowx + xcut - 1][hiy + 1 - ycut].typ = TRCORNER;
                lev->locations[lowx - 1][hiy + 1 - ycut].typ = BLCORNER;
                lev->locations[lowx + xcut - 1][hiy + 1].typ = BLCORNER;
            }
            if (docenter) {
                /* pillar in the middle */
                int xcenter = lowx + ((hix - lowx) / 2);
                int ycenter = lowy + ((hiy - lowy) / 2);
                int xparity = ((hix - lowx) % 2) ? 1 : 0;
                int yparity = ((hiy - lowy) % 2) ? 1 : 0;
                int xradius = (xcut + 1) / 2;
                int yradius = (ycut + 1) / 2;
                int vcorrmin = xcenter - xradius + 1;
                int vcorrmax = xcenter + xradius + xparity - 1;
                int hcorrmin = ycenter - yradius + 1;
                int hcorrmax = ycenter + yradius + yparity - 1;
                for (x = xcenter - xradius; x <= xcenter + xradius + xparity; x++) {
                    for (y = ycenter - yradius; y <= ycenter + yradius + yparity; y++) {
                        lev->locations[x][y].typ =
                            ((x == xcenter - xradius) &&
                             (y == ycenter - yradius)) ? TLCORNER :
                            ((x == xcenter - xradius) &&
                             (y == ycenter + yradius + yparity)) ? BLCORNER :
                            ((x == xcenter + xradius + xparity) &&
                             (y == ycenter - yradius)) ? TRCORNER :
                            ((x == xcenter + xradius + xparity) &&
                             (y == ycenter + yradius + yparity)) ? BRCORNER :
                            ((x == xcenter - xradius) ||
                             (x == xcenter + xradius + xparity)) ? VWALL :
                            ((y == ycenter - yradius) ||
                             (y == ycenter + yradius + yparity)) ? HWALL : STONE;
                    }
                }
                if ((vcorrmax - vcorrmin) > 1 && mrn2(3)) {
                    x = vcorrmin + mrn2(vcorrmax - vcorrmin);
                    for (y = ycenter - yradius; y <= ycenter + yradius + yparity; y++) {
                        lev->locations[x][y].typ =
                            ((y == ycenter - yradius) ||
                             (y == ycenter + yradius + yparity)) ? SDOOR : SCORR;
                        if (lev->locations[x][y].typ == SDOOR) {
                            lev->locations[x][y].horizontal = 1;
                        }
                    }
                }
                if ((hcorrmax - hcorrmin) > 1 && mrn2(3)) {
                    y = hcorrmin + mrn2(hcorrmax - hcorrmin);
                    for (x = xcenter - xradius; x <= xcenter + xradius + xparity; x++) {
                        lev->locations[x][y].typ =
                            ((x == xcenter - xradius) ||
                             (x == xcenter + xradius + xparity)) ? SDOOR : SCORR;
                    }
                }
            }
        }
        if (!is_room) {        /* a subroom */
            wallification(lev, lowx - 1, lowy - 1, hix + 1, hiy + 1);
        }
    }
    /* Now that we know for sure which tiles are part of the final
       room or not, light up the room, if desired. */
    if (lit) {
        for (x = lowx - 1; x <= hix + 1; x++) {
            loc = &lev->locations[x][max(lowy - 1, 0)];
            for (y = lowy - 1; y <= hiy + 1; y++) {
                if (loc->typ != STONE &&
                    loc->typ != SDOOR && loc->typ != SCORR)
                    loc->lit = 1;
                loc++;
            }
        }
        croom->rlit = 1;
    } else
        croom->rlit = 0;

}


void
add_room(struct level *lev, int lowx, int lowy, int hix, int hiy, boolean lit,
         schar rtype, boolean special, boolean canbeshaped)
{
    struct mkroom *croom;

    croom = &lev->rooms[lev->nroom];
    do_room_or_subroom(lev, croom, lowx, lowy, hix, hiy, lit, rtype, special,
                       (boolean) TRUE, canbeshaped);
    croom++;
    croom->hx = -1;
    lev->nroom++;
}

void
add_subroom(struct level *lev, struct mkroom *proom, int lowx, int lowy,
            int hix, int hiy, boolean lit, schar rtype, boolean special)
{
    struct mkroom *croom;

    croom = &lev->subrooms[lev->nsubroom];
    do_room_or_subroom(lev, croom, lowx, lowy, hix, hiy, lit, rtype, special,
                       FALSE, FALSE);
    proom->sbrooms[proom->nsubrooms++] = croom;
    croom++;
    croom->hx = -1;
    lev->nsubroom++;
}

static void
makerooms(struct level *lev, int *smeq)
{
    boolean tried_vault = FALSE;

    /* make rooms until satisfied */
    /* rnd_rect() will returns 0 if no more rects are available... */
    while (lev->nroom < MAXNROFROOMS && rnd_rect()) {
        if (lev->nroom >= (MAXNROFROOMS / 6) && mrn2(2) &&
            !tried_vault) {
            tried_vault = TRUE;
            if (create_vault(lev)) {
                vault_x = lev->rooms[lev->nroom].lx;
                vault_y = lev->rooms[lev->nroom].ly;
                lev->rooms[lev->nroom].hx = -1;
            }
        } else if (!create_room(lev, -1, -1, -1, -1, -1, -1, OROOM, -1,
                                smeq, (depth(&lev->z) > 2)))
            return;
    }
    return;
}

static void
join(struct level *lev, int a, int b, boolean nxcor, int *smeq)
{
    coord cc, tt, org, dest;
    xchar tx, ty, xx, yy;
    struct mkroom *croom, *troom;
    int dx, dy;

    croom = &lev->rooms[a];
    troom = &lev->rooms[b];

    /* find positions cc and tt for doors in croom and troom and direction for
       a corridor between them */

    if (troom->hx < 0 || croom->hx < 0 || lev->doorindex >= DOORMAX)
        return;
    if (troom->lx > croom->hx) {
        dx = 1;
        dy = 0;
        xx = croom->hx + 1;
        tx = troom->lx - 1;
        finddpos(lev, &cc, xx, croom->ly, xx, croom->hy);
        finddpos(lev, &tt, tx, troom->ly, tx, troom->hy);
    } else if (troom->hy < croom->ly) {
        dy = -1;
        dx = 0;
        yy = croom->ly - 1;
        finddpos(lev, &cc, croom->lx, yy, croom->hx, yy);
        ty = troom->hy + 1;
        finddpos(lev, &tt, troom->lx, ty, troom->hx, ty);
    } else if (troom->hx < croom->lx) {
        dx = -1;
        dy = 0;
        xx = croom->lx - 1;
        tx = troom->hx + 1;
        finddpos(lev, &cc, xx, croom->ly, xx, croom->hy);
        finddpos(lev, &tt, tx, troom->ly, tx, troom->hy);
    } else {
        dy = 1;
        dx = 0;
        yy = croom->hy + 1;
        ty = troom->ly - 1;
        finddpos(lev, &cc, croom->lx, yy, croom->hx, yy);
        finddpos(lev, &tt, troom->lx, ty, troom->hx, ty);
    }
    xx = cc.x;
    yy = cc.y;

    tx = tt.x - dx;
    ty = tt.y - dy;
    if (nxcor && lev->locations[xx + dx][yy + dy].typ)
        return;
    if (okdoor(lev, xx, yy) || !nxcor)
        dodoor(lev, xx, yy, croom);

    org.x = xx + dx;
    org.y = yy + dy;
    dest.x = tx;
    dest.y = ty;

    if (!dig_corridor
        (lev, &org, &dest, nxcor, lev->flags.arboreal ? ROOM : CORR, STONE))
        return;

    /* we succeeded in digging the corridor */
    if (okdoor(lev, tt.x, tt.y) || !nxcor)
        dodoor(lev, tt.x, tt.y, troom);

    if (smeq[a] < smeq[b])
        smeq[b] = smeq[a];
    else
        smeq[a] = smeq[b];
}

void
makecorridors(struct level *lev, int *smeq, enum levstyle style)
{
    int a, b, i;
    boolean any = TRUE;

    /* K-Mod: the implementation of 'styles' in sporkhack is completely broken
       but I liked the idea, so I've been working to try to fix the system. */

    switch (style) {
    default: /* standard style (using unmodified code) */
    for (a = 0; a < lev->nroom - 1; a++) {
        join(lev, a, a + 1, FALSE, smeq);
        if (!mrn2(50))
            break;      /* allow some randomness */
    }
    for (a = 0; a < lev->nroom - 2; a++)
        if (smeq[a] != smeq[a + 2])
            join(lev, a, a + 2, FALSE, smeq);
    for (a = 0; any && a < lev->nroom; a++) {
        any = FALSE;
        for (b = 0; b < lev->nroom; b++)
            if (smeq[a] != smeq[b]) {
                join(lev, a, b, FALSE, smeq);
                any = TRUE;
            }
    }
    if (lev->nroom > 2)
        for (i = mrn2(lev->nroom) + 4; i; i--) {
            a = mrn2(lev->nroom);
            b = mrn2(lev->nroom - 2);
            if (b >= a)
                b += 2;
            join(lev, a, b, TRUE, smeq);
        }
    break;
    case LEVSTYLE_RING:
        /* rooms should be ordered by their angle to the center */
        if (lev->nroom > 1) {
            for (a = 0; a < lev->nroom; a++) {
                b = (a + 1) % lev->nroom;
                join(lev, a, b, FALSE, smeq);
            }
        }
        break;

    case LEVSTYLE_HUB:
        /* central room should have been already chosen and put first in the list */
        if (lev->nroom > 1) {
            for (a = 1; a < lev->nroom; a++) {
                join(lev, a, 0, FALSE, smeq);
            }
        }
        break;

    case LEVSTYLE_MINRAND:
        /* Minimum number of random room-to-room connections needed to ensure
           all rooms are directly or indirectly connected to one another. */
        if (lev->nroom > 1) {
            int tryct = 0, concount;
            boolean connected[lev->nroom][lev->nroom];
            boolean connectcount[lev->nroom];
            boolean finished = FALSE;
            boolean changedanything;
            /* Initially, no rooms are connected to any other rooms, either
               directly nor indirectly.  Start there: */
            for (a = 0; a < lev->nroom; a++) {
                connectcount[a] = 1; /* Just itself. */
                for (b = 0; b < lev->nroom; b++) {
                    connected[a][b] = (a == b) ? TRUE : FALSE;
                }
            }
            while ((tryct++ < 9999) && !finished) {

                /* Pick two rooms; if they're not yet connected, fix that */
                a = mrn2(lev->nroom); /* start with a random pick, but... */
                for (b = 0; b < lev->nroom; b++) {
                    /* Maybe there's a room with fewer connections? */
                    if (connectcount[b] < connectcount[a])
                        a = b;
                }
                b = mrn2(lev->nroom);
                if ((!connected[a][b] && !connected[b][a])) {
                    join(lev, a, b, FALSE, smeq);
                    connected[a][b] = TRUE;
                }

                /* Mark indirect connections: */
                changedanything = TRUE;
                while (changedanything) {
                    changedanything = FALSE;
                    for (a = 0; a < lev->nroom; a++) {
                        concount = 0;
                        for (b = 0; b < lev->nroom; b++) {
                            if (connected[a][b])
                                concount++;
                            if (connected[a][b] && !(b == a)) {
                                if (!connected[b][a]) {
                                    connected[b][a] = TRUE;
                                    changedanything = TRUE;
                                }
                                for (i = 0; i < lev->nroom; i++) {
                                    if (connected[a][i] && !connected[b][i]) {
                                        connected[b][i] = TRUE;
                                        changedanything = TRUE;
                                    }
                                }
                            }
                        }
                        connectcount[a] = concount;
                    }
                }

                /* Now check to see if any rooms are not connected to any other
                   rooms still. */
                finished = TRUE;
                for (a = 0; a < lev->nroom; a++) {
                    for (b = 0; b < lev->nroom; b++) {
                        if (!connected[a][b])
                            finished = FALSE;
                    }
                }
            }
        }
        break;
        /* More styles to come */
    }
}

void
add_door(struct level *lev, int x, int y, struct mkroom *aroom)
{
    struct mkroom *broom;
    int tmp;

    aroom->doorct++;
    broom = aroom + 1;
    if (broom->hx < 0)
        tmp = lev->doorindex;
    else
        for (tmp = lev->doorindex; tmp > broom->fdoor; tmp--)
            lev->doors[tmp] = lev->doors[tmp - 1];
    lev->doorindex++;
    lev->doors[tmp].x = x;
    lev->doors[tmp].y = y;
    for (; broom->hx >= 0; broom++)
        broom->fdoor++;
}

static void
dosdoor(struct level *lev, xchar x, xchar y, struct mkroom *aroom, int type)
{
    boolean shdoor = ((*in_rooms(lev, x, y, SHOPBASE)) ? TRUE : FALSE);

    if (!IS_WALL(lev->locations[x][y].typ))     /* avoid SDOORs on already made
                                                   doors */
        type = DOOR;
    lev->locations[x][y].typ = type;
    if (type == DOOR) {
        if (!mrn2(3)) {  /* locked, closed, or doorway? */
            if (!mrn2(5))
                lev->locations[x][y].doormask = D_ISOPEN;
            else if (!mrn2(6))
                lev->locations[x][y].doormask = D_LOCKED;
            else
                lev->locations[x][y].doormask = D_CLOSED;

            if (lev->locations[x][y].doormask != D_ISOPEN && !shdoor &&
                level_difficulty(&lev->z) >= 5 && !mrn2(25))
                lev->locations[x][y].doormask |= D_TRAPPED;
        } else
            lev->locations[x][y].doormask = (shdoor ? D_ISOPEN : D_NODOOR);
        if (lev->locations[x][y].doormask & D_TRAPPED) {
            struct monst *mtmp;

            if (level_difficulty(&lev->z) >= 9 && !mrn2(5)) {
                /* attempt to make a mimic instead; no longer conditional on
                   mimics not being genocided; makemon() is on the main RNG
                   because mkclass() won't necessarily always return the same
                   result (again, due to genocide) */
                lev->locations[x][y].doormask = D_NODOOR;
                mtmp = makemon(mkclass(&lev->z, S_MIMIC, 0, mrng()),
                               lev, x, y, NO_MM_FLAGS);
                if (mtmp)
                    set_mimic_sym(mtmp, lev, mrng());
            }
        }
        if (level == lev)
            newsym(x, y);
    } else {    /* SDOOR */
        if (shdoor || !mrn2(5))
            lev->locations[x][y].doormask = D_LOCKED;
        else
            lev->locations[x][y].doormask = D_CLOSED;

        if (!shdoor && level_difficulty(&lev->z) >= 4 && !mrn2(20))
            lev->locations[x][y].doormask |= D_TRAPPED;
    }

    add_door(lev, x, y, aroom);
}

static boolean
place_niche(struct level *lev, struct mkroom *aroom, int *dy, int *xx, int *yy)
{
    coord dd;

    if (mrn2(2)) {
        *dy = 1;
        finddpos(lev, &dd, aroom->lx, aroom->hy + 1, aroom->hx, aroom->hy + 1);
    } else {
        *dy = -1;
        finddpos(lev, &dd, aroom->lx, aroom->ly - 1, aroom->hx, aroom->ly - 1);
    }
    *xx = dd.x;
    *yy = dd.y;
    return ((boolean)
            ((isok(*xx, *yy + *dy) &&
              lev->locations[*xx][*yy + *dy].typ == STONE)
             && (isok(*xx, *yy - *dy) &&
                 !IS_POOL(lev->locations[*xx][*yy - *dy].typ)
                 && !IS_FURNITURE(lev->locations[*xx][*yy - *dy].typ))));
}

/* there should be one of these per trap, in the same order as trap.h */
static const char *const trap_engravings[TRAPNUM] = {
    NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    /* 14..17: trap door, VS, teleport, level-teleport */
    "Vlad was here", NULL, "ad aerarium", "ad aerarium",
    NULL, NULL, NULL, NULL, NULL,
    NULL,
};

static void
makeniche(struct level *lev, int trap_type)
{
    struct mkroom *aroom;
    struct rm *rm;
    int vct = 8;
    int dy, xx, yy;
    struct trap *ttmp;

    if (lev->doorindex < DOORMAX)
        while (vct--) {
            aroom = &lev->rooms[mrn2(lev->nroom)];
            if (aroom->rtype != OROOM)
                continue;       /* not an ordinary room */
            if (aroom->doorct == 1 && mrn2(5))
                continue;
            if (!place_niche(lev, aroom, &dy, &xx, &yy))
                continue;

            rm = &lev->locations[xx][yy + dy];
            if (trap_type || !mrn2(4)) {

                rm->typ = SCORR;
                if (trap_type) {
                    if ((trap_type == HOLE || trap_type == TRAPDOOR)
                        && !can_fall_thru(lev))
                        trap_type = ROCKTRAP;
                    ttmp = maketrap(lev, xx, yy + dy, trap_type, mrng());
                    if (ttmp) {
                        if (trap_type != ROCKTRAP)
                            ttmp->once = 1;
                        if (trap_engravings[trap_type]) {
                            make_engr_at(lev, xx, yy - dy,
                                         trap_engravings[trap_type], 0L, DUST);
                            wipe_engr_at(lev, xx, yy - dy, 5);  /* age it */
                        }
                    }
                }
                dosdoor(lev, xx, yy, aroom, SDOOR);
            } else {
                rm->typ = CORR;
                if (mrn2(7))
                    dosdoor(lev, xx, yy, aroom,
                            mrn2(5) ? SDOOR : DOOR);
                else {
                    if (!lev->flags.noteleport)
                        mksobj_at(SCR_TELEPORTATION, lev, xx, yy + dy, TRUE,
                                  FALSE, mrng());
                    if (!mrn2(3))
                        mkobj_at(0, lev, xx, yy + dy, TRUE, mrng());
                }
            }
            return;
        }
}

static void
make_niches(struct level *lev)
{
    int ct = 1 + mrn2((lev->nroom >> 1) + 1), dep = depth(&lev->z);

    boolean ltptr = (!lev->flags.noteleport && dep > 15);
    boolean vamp = (dep > 5 && dep < 25);

    while (ct--) {
        if (ltptr && !mrn2(6)) {
            ltptr = FALSE;
            makeniche(lev, LEVEL_TELEP);
        } else if (vamp && !mrn2(6)) {
            vamp = FALSE;
            makeniche(lev, TRAPDOOR);
        } else
            makeniche(lev, NO_TRAP);
    }
}

static void
makevtele(struct level *lev)
{
    makeniche(lev, TELEP_TRAP);
}

/* Allocate a new level structure and make sure its fields contain sane
 * initial vales.
 * Some of this is only necessary for some types of levels (maze, normal,
 * special) but it's easier to put it all in one place than make sure
 * each type initializes what it needs to separately.
 */
struct level *
alloc_level(d_level *levnum)
{
    struct level *lev = malloc(sizeof (struct level));

    memset(lev, 0, sizeof (struct level));
    lev->struct_type = 'L'; /* See doc/struct_types.txt */
    if (levnum)
        lev->z = *levnum;
    lev->subrooms = &lev->rooms[MAXNROFROOMS + 1];      /* compat */
    lev->rooms[0].hx = -1;
    lev->subrooms[0].hx = -1;
    lev->flags.hero_memory = 1;

    lev->updest.lx = lev->updest.hx = lev->updest.nlx = lev->updest.nhx =
        lev->dndest.lx = lev->dndest.hx = lev->dndest.nlx = lev->dndest.nhx =
        lev->upstair.sx = lev->dnstair.sx = lev->sstairs.sx =
        lev->upladder.sx = lev->dnladder.sx = COLNO;
    lev->updest.ly = lev->updest.hy = lev->updest.nly = lev->updest.nhy =
        lev->dndest.ly = lev->dndest.hy = lev->dndest.nly = lev->dndest.nhy =
        lev->upstair.sy = lev->dnstair.sy = lev->sstairs.sy =
        lev->upladder.sy = lev->dnladder.sy = ROWNO;

    /* these are not part of the level structure, but are only used while
       making new levels */
    vault_x = -1;
    made_branch = FALSE;

    return lev;
}

static void
makelevel(struct level *lev)
{
    struct mkroom *croom, *troom;
    int tryct;
    int x, y, i;
    struct monst *tmonst;       /* always put a web with a spider */
    branch *branchp;
    int room_threshold;
    enum levstyle style = (!Is_rogue_level(&lev->z) &&
                           !Is_special(&lev->z) && !mrn2(8)) ? /* K-Mod */
        mrn2(LEVSTYLE_TYPES) : LEVSTYLE_STANDARD;

    int smeq[MAXNROFROOMS + 1];
    boolean anniv = ((wizard || (SQKY_BOARD==getmonth())) && !(getmday() & 30));

    if (wiz1_level.dlevel == 0)
        init_dungeons();

    {
        s_level *slevnum = Is_special(&lev->z);

        /* check for special levels */
        if (slevnum && !Is_rogue_level(&lev->z)) {
            if (In_hell(&lev->z) && can_aiscav_embed(slevnum->proto)) {
                mkaiscav(lev, slevnum->proto);
                return;
            }
            makemaz(lev, slevnum->proto, smeq);
            return;
        } else if (find_dungeon(&lev->z).proto[0]) {
            makemaz(lev, "", smeq);
            return;
        } else if (In_mines(&lev->z)) {
            makemaz(lev, "minefill", smeq);
            return;
        } else if (In_quest(&lev->z)) {
            char fillname[9];
            s_level *loc_levnum;

            snprintf(fillname, SIZE(fillname), "%s-loca", urole.filecode);
            loc_levnum = find_level(fillname);

            snprintf(fillname, SIZE(fillname), "%s-fil", urole.filecode);
            strcat(fillname,
                   (lev->z.dlevel < loc_levnum->dlevel.dlevel) ? "a" : "b");
            makemaz(lev, fillname, smeq);
            return;
        } else if (In_hell(&lev->z)) {
            mkaiscav(lev, "gehcav");
            return;
        } else if ((lev->z.dnum == medusa_level.dnum) &&
                   (depth(&lev->z) > depth(&medusa_level))) {
            makemaz(lev, "", smeq);
            return;
        }
    }

    /* otherwise, fall through - it's a "regular" level. */

    if (Is_rogue_level(&lev->z)) {
        makeroguerooms(lev, smeq);
        makerogueghost(lev);
    } else {
        if (style == LEVSTYLE_HUB) {
            /* create the central room.  For historical reasons, create_room()
	       takes its (x,y) coords in units of 1/5 of the map. */
#define mrn1(x,y)         (mrn2(x)+(y))
            if (!create_room(lev, 3, mrn1(3,1), mrn1(5, 9), mrn1(4, 5),
                             3 /* xmiddle */, -1, OROOM, -1, smeq,
                             (depth(&lev->z) > 2)))
                style = LEVSTYLE_STANDARD; /* failed */
            else {
                if (!mrn2(10)) {
                    struct mkroom* croom = &lev->rooms[lev->nroom-1];
                    int x = croom->lx + (croom->hx - croom->lx)/2 - 2 + mrn2(5);
                    int y = croom->ly + (croom->hy - croom->ly)/2 - 1 + mrn2(3);
                    struct obj* otmp =
                        mksobj_at(STATUE, lev, x, y, TRUE, FALSE, mrng());
                    if (otmp)
                        otmp->corpsenm = PM_WIZARD_OF_YENDOR;
                }
            }
        } /* Continue to make the rest of the rooms... */
        makerooms(lev, smeq); /* Some rooms may get shaped. */
    }
    sort_rooms(lev, style);
    /* after sorting the rooms, fix up the shaped rooms
       so that somexy can work with them */
    for (i = 0; i < lev->nroom; i++) {
        int rno;
        croom = &lev->rooms[i];
        rno = (croom - lev->rooms) + ROOMOFFSET;
        if (croom->irregular) {
            for (x = croom->lx; x <= croom->hx; x++) {
                for (y = croom->ly; y <= croom->hy; y++) {
                    if (lev->locations[x][y].typ == ROOM)
                        lev->locations[x][y].roomno = rno;
                }
            }
        }
    }

    /* construct stairs (up and down in different rooms if possible) */
    croom = &lev->rooms[mrn2(lev->nroom)];
    if (!Is_botlevel(&lev->z)) {
        int tries = 50;
        y = somey(croom, mrng());
        x = somex(croom, mrng());
        /* Try to put them on the floor. */
        while (lev->locations[x][y].typ != ROOM && tries--) {
            x = somex(croom, mrng());
            y = somey(croom, mrng());
        }
        mkstairs(lev, x, y, 0, croom);  /* down */
    }
    if (lev->nroom > 1) {
        troom = croom;
        croom = &lev->rooms[mrn2(lev->nroom - 1)];
        /* TODO: This looks like the wrong test. */
        if (croom == troom)
            croom++;
    }

    if (lev->z.dlevel != 1) {
        xchar sx, sy;

        do {
            sx = somex(croom, mrng());
            sy = somey(croom, mrng());
        } while (occupied(lev, sx, sy));
        mkstairs(lev, sx, sy, 1, croom);        /* up */
    }

    branchp = Is_branchlev(&lev->z);    /* possible dungeon branch */
    room_threshold = branchp ? 4 : 3;   /* minimum number of rooms needed to
                                           allow a random special room */
    if (Is_rogue_level(&lev->z))
        goto skip0;
    makecorridors(lev, smeq, style);
    make_niches(lev);

    /* make a secret treasure vault, not connected to the rest */
    if (do_vault()) {
        xchar w, h;

        w = 1;
        h = 1;
        if (check_room(lev, &vault_x, &w, &vault_y, &h, TRUE)) {
        fill_vault:
            add_room(lev, vault_x, vault_y, vault_x + w, vault_y + h, TRUE,
                     VAULT, FALSE, FALSE);
            ++room_threshold;
            fill_room(lev, &lev->rooms[lev->nroom - 1], FALSE);
            mk_knox_portal(lev, vault_x + w, vault_y + h);
            if (!lev->flags.noteleport && !mrn2(3))
                makevtele(lev);
        } else if (rnd_rect() && create_vault(lev)) {
            vault_x = lev->rooms[lev->nroom].lx;
            vault_y = lev->rooms[lev->nroom].ly;
            if (check_room(lev, &vault_x, &w, &vault_y, &h, TRUE))
                goto fill_vault;
            else
                lev->rooms[lev->nroom].hx = -1;
        }
    }

    {
        int u_depth = depth(&lev->z);

        if (u_depth > 1 && u_depth < depth(&medusa_level) &&
            lev->nroom >= room_threshold && mrn2(u_depth) < 3)
            mkroom(lev, SHOPBASE);
        else if (u_depth > 4 && !mrn2(6))
            mkroom(lev, COURT);
        else if (u_depth > 5 && !mrn2(8)) {
            if (u_depth > 15)
                mkroom(lev, DRAGONHALL);
            else
            if (!(mvitals[PM_LEPRECHAUN].mvflags & G_GONE))
                mkroom(lev, LEPREHALL);
        } else if (u_depth > 6 && !mrn2(7))
            mkroom(lev, ZOO);
        else if (u_depth > 8 && !mrn2(5))
            mkroom(lev, TEMPLE);
        else if (u_depth > 9 && !mrn2(5)) {
            if (!(mvitals[PM_KILLER_BEE].mvflags & G_GONE))
                mkroom(lev, BEEHIVE);
        } else if (u_depth > 11 && !mrn2(6))
            mkroom(lev, MORGUE);
        else if (u_depth > 12 && !mrn2(8)) {
            if (antholemon(&lev->z))
                mkroom(lev, ANTHOLE);
        } else if (u_depth > 14 && !mrn2(4)) {
            if (!(mvitals[PM_SOLDIER].mvflags & G_GONE))
                mkroom(lev, BARRACKS);
        } else if (u_depth > 15 && !mrn2(6)) {
            mkroom(lev, SWAMP);
        } else if (u_depth > 16 && !mrn2(8)) {
            if (!(mvitals[PM_COCKATRICE].mvflags & G_GONE))
                mkroom(lev, COCKNEST);
        }
    }

skip0:
    /* Place multi-dungeon branch. */
    place_branch(lev, branchp, COLNO, ROWNO);

    /* for each room: put things inside */
    for (croom = lev->rooms; croom->hx > 0; croom++) {
        if (croom->rtype != OROOM)
            continue;

        /* put a sleeping monster inside */
        /* Note: monster may be on the stairs. This cannot be avoided: maybe
           the player fell through a trap door while a monster was on the
           stairs. Conclusion: we have to check for monsters on the stairs
           anyway. */

        if (!mrn2(3)) {
            int tries = 20;
            int ldiff = ilog2(level_difficulty(&lev->z) || 1) / 1600;
            /* ldiff is thus 0 on DL1-2, 1 on DL3-8, 2 on DL9-25, then 3. */
            x = somex(croom, mrng());
            y = somey(croom, mrng());
            /* Try to put it on the floor. */
            while (lev->locations[x][y].typ != ROOM && tries--) {
                x = somex(croom, mrng());
                y = somey(croom, mrng());
            }
            tmonst = makemon(NULL, lev, x, y, MM_ALLLEVRNG);
            if (tmonst && tmonst->data == &mons[PM_GIANT_SPIDER] &&
                !occupied(lev, x, y))
                maketrap(lev, x, y, WEB, mrng());
            if (tmonst && !mrn2(ldiff || 1) && !resists_sleep(tmonst))
                tmonst->msleeping = 1;
        }

        /* K-Mod. put an additional monster in the hub room. */
        if (style == LEVSTYLE_HUB && croom == lev->rooms) {
            /* I'd like to make this an especially strong monster,
               so lets pick the strongest from a few random monsters. */
            const struct permonst *best_mon=0, *temp_mon;
            int i;
            for (i = 0; i < 5; i++) {
                temp_mon = rndmonst(&lev->z, mrng());
                if (!best_mon || temp_mon->mlevel > best_mon->mlevel)
                    best_mon = temp_mon;
            }
            x = somex(croom, mrng()); y = somey(croom, mrng());
            if (best_mon)
                tmonst = makemon(best_mon, lev, x, y, NO_MM_FLAGS);
            else
                tmonst = makemon((struct permonst *) 0, lev, x,y,NO_MM_FLAGS);

            if (tmonst && tmonst->data == &mons[PM_GIANT_SPIDER] &&
                !occupied(lev, x, y))
                (void) maketrap(lev, x, y, WEB, mrng());
        }

        /* put traps and mimics inside */
        x = 8 - (level_difficulty(&lev->z) / 6);
        if (x <= 1)
            x = 2;
        while (!mrn2(x))
            mktrap(lev, 0, 0, croom, NULL);
        if (!mrn2(3)) {
            int tries = 20;
            y = somey(croom, mrng());
            x = somex(croom, mrng());
            /* Try to put it on the floor. */
            while (lev->locations[x][y].typ != ROOM && tries--) {
                x = somex(croom, mrng());
                y = somey(croom, mrng());
            }
            struct obj *gld = mkfloorgold(0L, lev, x, y, mrng());
            u.generated_gold.onfloor += gld->quan;
        }
        if (Is_rogue_level(&lev->z))
            goto skip_nonrogue;
        
        /* greater chance of puddles if a water source is nearby */
        x = 40;
        if(!rn2(10)) {
            mkfount(lev, 0,croom);
            x = 20;
        }
        if (!mrn2(60))
            mksink(lev, croom);
        if (!mrn2(60))
            mkaltar(lev, croom);
        if (!mrn2(150))
            mkbench(lev, croom);
        if (!rn2(x))
            mkpuddles(lev, croom);
        x = 80 - (depth(&lev->z) * 2);
        if (x < 2)
            x = 2;
        if (!mrn2(x))
            mkgrave(lev, croom);

        /* put statues inside */
        if (!mrn2(20)) {
            int tries = 20;
            y = somey(croom, mrng());
            x = somex(croom, mrng());
            /* Try to put it on the floor. */
            while (lev->locations[x][y].typ != ROOM && tries--) {
                x = somex(croom, mrng());
                y = somey(croom, mrng());
            }
            mkcorpstat(STATUE, NULL, NULL, lev, x, y, TRUE, mrng());
        }
        /* and some rocks */
        if (!mrn2(7)) {
            /* Here if the rocks are in a wall we don't care. */
            y = somey(croom, mrng());
            x = somex(croom, mrng());
            mksobj_at(ROCK, lev, x, y, TRUE, FALSE, mrng());
        }
        /* put box/chest inside; 40% chance for at least 1 box, regardless of
           number of rooms; about 5 - 7.5% for 2 boxes, least likely when few
           rooms; chance for 3 or more is neglible. */
        if (!mrn2(lev->nroom * 5 / 2)) {
            int tries = 20;
            /* Fix: somex and somey should not be called from the arg list for
               mksobj_at(). Arg evaluation order is not standardized and may
               differ between compilers and optimization levels, which breaks
               replays. */
            y = somey(croom, mrng());
            x = somex(croom, mrng());
            /* Try to put it on the floor. */
            while (lev->locations[x][y].typ != ROOM && tries--) {
                x = somex(croom, mrng());
                y = somey(croom, mrng());
            }
            mksobj_at((mrn2(3)) ? LARGE_BOX : CHEST, lev, x, y,
                      TRUE, FALSE, mrng());
        }

        /* maybe make some graffiti */
        if (!mrn2(27 + 3 * abs(depth(&lev->z)))) {
            const char *mesg = random_engraving(mrng());

            if (mesg) {
                do {
                    x = somex(croom, mrng());
                    y = somey(croom, mrng());
                } while (lev->locations[x][y].typ != ROOM &&
                         !mrn2(40));
                if (!(IS_POOL(lev->locations[x][y].typ) ||
                      IS_FURNITURE(lev->locations[x][y].typ)))
                    make_engr_at(lev, x, y, mesg, 0L, MARK);
            }
        }

    skip_nonrogue:
        /* always create loot for the hub-room of hub-and-spoke topology */
        if ((!mrn2(3)) || ((style == LEVSTYLE_HUB) && (croom == lev->rooms))) {
            int tries = 20;
            y = somey(croom, mrng());
            x = somex(croom, mrng());
            /* Try to put it on the floor. */
            while (lev->locations[x][y].typ != ROOM && tries--) {
                x = somex(croom, mrng());
                y = somey(croom, mrng());
            }
            mkobj_at(0, lev, x, y, TRUE, mrng());
            tryct = 0;
            while (!mrn2(5)) {
                int tries = 20;
                if (++tryct > 100) {
                    impossible("tryct overflow4");
                    break;
                }
                y = somey(croom, mrng());
                x = somex(croom, mrng());
                /* Try to put it on the floor. */
                while (lev->locations[x][y].typ != ROOM && tries--) {
                    x = somex(croom, mrng());
                    y = somey(croom, mrng());
                }
                mkobj_at(0, lev, x, y, TRUE, mrng());
            }
        }
    }
    /* Supply small numbers of certain normally rare items early: */
    if ((lev->z.dlevel > 1) && (lev->z.dlevel < 8) && (mrn2(8))) {
        int tries = 20;
        croom = &lev->rooms[mrn2(lev->nroom - 1)];
        y = somey(croom, mrng());
        x = somex(croom, mrng());
        /* Try to put it on the floor. */
        while (lev->locations[x][y].typ != ROOM && tries--) {
            x = somex(croom, mrng());
            y = somey(croom, mrng());
        }
        switch (mrn2(17)) {
        case 1:
        case 2:
            mksobj_at(anniv ? WAN_NOTHING : WAN_ENLIGHTENMENT,
                      lev, x, y, TRUE, FALSE, mrng());
            break;
        case 3:
            if (anniv) {
                struct obj *otmp = mksobj(lev, DUNCE_CAP, FALSE, FALSE, mrng());
                if (otmp) {
                    otmp->spe = 5;
                    place_object(otmp, lev, x, y);
                    break;
                }
            }
            mksobj_at(EUCALYPTUS_LEAF, lev, x, y, TRUE, FALSE, mrng());
            break;
        case 4:
        case 5:
            if (anniv) {
                struct obj *otmp = mksobj(lev, FUMBLE_BOOTS,
                                          FALSE, FALSE, mrng());
                if (otmp) {
                    otmp->spe = 5;
                    place_object(otmp, lev, x, y);
                    break;
                }
            }
            mksobj_at(SPRIG_OF_WOLFSBANE, lev, x, y, TRUE, FALSE, mrng());
            break;
        case 6:
        case 7:
        case 8:
            if (anniv) {
                struct obj *otmp = mksobj(lev, GAUNTLETS_OF_FUMBLING,
                                          FALSE, FALSE, mrng());
                if (otmp) {
                    otmp->spe = 5;
                    place_object(otmp, lev, x, y);
                    break;
                }
            }
            mksobj_at(SACK, lev, x, y, TRUE, FALSE, mrng());
            break;
        default:
            if (anniv) {
                struct obj *otmp = mkobj(lev, SPBOOK_CLASS, FALSE, mrng());
                if (otmp) {
                    bless(otmp);
                    otmp->spestudied = 5;
                    place_object(otmp, lev, x, y);
                }
                break;
            }
            mksobj_at(SCR_SCARE_MONSTER, lev, x, y, TRUE, FALSE, mrng());
        }
    }
}

/*
 * Place deposits of minerals (gold and misc gems) in the stone
 * surrounding the rooms on the map.
 * Also place kelp in water.
 */
static void
mineralize(struct level *lev)
{
    s_level *sp;
    struct obj *otmp;
    int goldprob, gemprob, rockprob, x, y, cnt;


    /* Place kelp, except on the plane of water */
    if (In_endgame(&lev->z))
        return;
    for (x = 1; x < (COLNO - 1); x++)
        for (y = 1; y < (ROWNO - 1); y++)
            if ((lev->locations[x][y].typ == POOL && !mrn2(10)) ||
                (lev->locations[x][y].typ == MOAT && !mrn2(30)))
                mksobj_at(KELP_FROND, lev, x, y, TRUE, FALSE, mrng());

    /* determine if it is even allowed; almost all special levels are excluded
       */
    if (In_hell(&lev->z) || In_V_tower(&lev->z) || Is_rogue_level(&lev->z) ||
        lev->flags.arboreal || ((sp = Is_special(&lev->z)) != 0 &&
                                !Is_oracle_level(&lev->z)
                                && (!In_mines(&lev->z) || sp->flags.town)
        ))
        return;

    /* basic level-related probabilities */
    goldprob = 20 + depth(&lev->z) / 6;
    gemprob  = goldprob / 2;
    rockprob = 50 - 2 * depth(&lev->z);

    /* mines have ***MORE*** goodies - otherwise why mine? */
    if (In_mines(&lev->z)) {
        goldprob *= 2;
        gemprob *= 3;
    } else if (In_quest(&lev->z)) {
        goldprob /= 4;
        gemprob /= 6;
    }

    /*
     * Seed rock areas with gold and/or gems.
     * We use fairly low level object handling to avoid unnecessary
     * overhead from placing things in the floor chain prior to burial.
     */
    for (x = 1; x < (COLNO - 1); x++)
        for (y = 1; y < (ROWNO - 1); y++)
            if (lev->locations[x][y + 1].typ != STONE) {
                /* <x,y> spot not eligible */
                y += 2; /* next two spots aren't eligible either */
            } else if (lev->locations[x][y].typ != STONE) {
                /* this spot not eligible */
                y += 1; /* next spot isn't eligible either */
            } else if (!(lev->locations[x][y].wall_info & W_NONDIGGABLE) &&
                       lev->locations[x][y - 1].typ == STONE &&
                       lev->locations[x + 1][y - 1].typ == STONE &&
                       lev->locations[x - 1][y - 1].typ == STONE &&
                       lev->locations[x + 1][y].typ == STONE &&
                       lev->locations[x - 1][y].typ == STONE &&
                       lev->locations[x + 1][y + 1].typ == STONE &&
                       lev->locations[x - 1][y + 1].typ == STONE) {
                if (mrn2(1000) < goldprob) {
                    if ((otmp = mksobj(lev, GOLD_PIECE,
                                       FALSE, FALSE, mrng()))) {
                        otmp->ox = x, otmp->oy = y;
                        otmp->quan = 2L + mrn2(goldprob * 3);
                        otmp->owt = weight(otmp);
                        /* Because we're putting this in STONE, it counts for
                           generated_gold purposes as .buried, not .onfloor,
                           even if it's not buried as such. */
                        u.generated_gold.buried += otmp->quan;
                        if (!mrn2(3))
                            add_to_buried(otmp);
                        else
                            place_object(otmp, lev, x, y);
                    }
                }
                if (mrn2(1000) < gemprob) {
                    for (cnt = 1 + mrn2(2 + dunlev(&lev->z) / 3);
                         cnt > 0; cnt--)
                        if ((otmp = mkobj(lev, GEM_CLASS, FALSE, mrng()))) {
                            if (otmp->otyp == ROCK) {
                                dealloc_obj(otmp);  /* no rocks on gem piles */
                            } else {
                                otmp->ox = x, otmp->oy = y;
                                if (!mrn2(3))
                                    add_to_buried(otmp);
                                else
                                    place_object(otmp, lev, x, y);
                            }
                        }
                } else if (rn2(1000) < rockprob) {
                    otmp = mksobj(lev, ROCK, TRUE, FALSE, mrng());
                    otmp->ox = x; otmp->oy = y;
                    if (rn2(3))
                        add_to_buried(otmp);
                    else
                        place_object(otmp, lev, x, y);
                }
            }
}

struct level *
mklev(d_level * levnum)
{
    struct mkroom *croom;
    int ln = ledger_no(levnum);
    struct level *lev;

    if (levels[ln])
        return levels[ln];

    if (getbones(levnum))
        return levels[ln];      /* initialized in getbones->getlev */

    lev = levels[ln] = alloc_level(levnum);
    init_rect(rng_for_level(levnum));

    in_mklev = TRUE;
    makelevel(lev);
    bound_digging(lev);
    mineralize(lev);
    in_mklev = FALSE;
    /* has_morgue gets cleared once morgue is entered; graveyard stays set
       (graveyard might already be set even when has_morgue is clear [see
       fixup_special()], so don't update it unconditionally) */
    if (search_special(lev, MORGUE))
        lev->flags.graveyard = 1;
    if (!lev->flags.is_maze_lev) {
        for (croom = &lev->rooms[0]; croom != &lev->rooms[lev->nroom]; croom++)
            topologize(lev, croom);
    }
    set_wall_state(lev);

    return lev;
}


void
topologize(struct level *lev, struct mkroom *croom)
{
    int x, y, roomno = (croom - lev->rooms) + ROOMOFFSET;
    int lowx = croom->lx, lowy = croom->ly;
    int hix = croom->hx, hiy = croom->hy;
    int subindex, nsubrooms = croom->nsubrooms;

    /* skip the room if already done; i.e. a shop handled out of order */
    /* also skip if this is non-rectangular (it _must_ be done already) */
    if ((int)lev->locations[lowx][lowy].roomno == roomno || croom->irregular)
        return;
    {
        /* do innards first */
        for (x = lowx; x <= hix; x++)
            for (y = lowy; y <= hiy; y++)
                lev->locations[x][y].roomno = roomno;
        /* top and bottom edges */
        for (x = lowx - 1; x <= hix + 1; x++)
            for (y = lowy - 1; y <= hiy + 1; y += (hiy - lowy + 2)) {
                lev->locations[x][y].edge = 1;
                if (lev->locations[x][y].roomno)
                    lev->locations[x][y].roomno = SHARED;
                else
                    lev->locations[x][y].roomno = roomno;
            }
        /* sides */
        for (x = lowx - 1; x <= hix + 1; x += (hix - lowx + 2))
            for (y = lowy; y <= hiy; y++) {
                lev->locations[x][y].edge = 1;
                if (lev->locations[x][y].roomno)
                    lev->locations[x][y].roomno = SHARED;
                else
                    lev->locations[x][y].roomno = roomno;
            }
    }
    /* subrooms */
    for (subindex = 0; subindex < nsubrooms; subindex++)
        topologize(lev, croom->sbrooms[subindex]);
}

/* Find an unused room for a branch location. */
static struct mkroom *
find_branch_room(struct level *lev, coord *mp)
{
    struct mkroom *croom = 0;

    if (lev->nroom == 0) {
        mazexy(lev, mp);        /* already verifies location */
    } else {
        /* not perfect - there may be only one stairway */
        if (lev->nroom > 2) {
            int tryct = 0;

            do
                croom = &lev->rooms[mrn2(lev->nroom)];
            while ((croom == lev->dnstairs_room || croom == lev->upstairs_room
                    || croom->rtype != OROOM) && (++tryct < 100));
        } else
            croom = &lev->rooms[mrn2(lev->nroom)];

        do {
            if (!somexy(lev, croom, mp, mrng()))
                impossible("Can't place branch!");
        } while (occupied(lev, mp->x, mp->y) ||
                 (lev->locations[mp->x][mp->y].typ != CORR &&
                  lev->locations[mp->x][mp->y].typ != ROOM));
    }
    return croom;
}

/* Find the room for (x,y).  Return null if not in a room. */
static struct mkroom *
pos_to_room(struct level *lev, xchar x, xchar y)
{
    int i;
    struct mkroom *curr;

    for (curr = lev->rooms, i = 0; i < lev->nroom; curr++, i++)
        if (inside_room(curr, x, y))
            return curr;
    return NULL;
}

/* make connected spots of shallow water (or pools) and add sea monsters */
void
mkpuddles(struct level *lev, struct mkroom *croom)
{
    coord m;
    int tryct = 0;
    int puddles = 0; /* how many spaces have we altered? */
    int fish = 0;
    int rng = rng_for_level(&lev->z);
	
    do {
        if (tryct++ > 200)
            return;
        if (!somexy(lev, croom, &m, rng))
            return;
    } while (occupied(lev, m.x, m.y));
	
    do {
        if (!is_damp_terrain(lev, m.x, m.y)) {
            puddles++;
            lev->locations[m.x][m.y].typ =
                (depth(&lev->z) > 3 + rn2_on_rng(35, rng)) ? POOL : PUDDLE;
        }
        if ((puddles > 4 + 2 * fish) && (rn2_on_rng(depth(&lev->z), rng) > 4)) {
            (void)makemon(is_pool(lev, m.x, m.y) ?
                          mkclass(&lev->z, S_KRAKEN, 0, rng) : &mons[PM_PIRANHA],
                          lev, m.x, m.y, NO_MM_FLAGS);
            fish++;
        }
        tryct = 0;
        do {
            /* Always changing both coords by 1 (either increment or decrement)
               ensures that we never place water orthogonally adjacent to
               water, i.e., we confine the water to a checkerboard pattern;
               thus the player is never required to cross water to traverse a
               level as a result of this function.  (Special levels, such as
               Medusa's Island, are another matter.) */
            m.x += sgn(rn2(3)-1);
            m.y += sgn(rn2(3)-1);
        } while ((occupied(lev, m.x, m.y) ||
                  m.x < croom->lx || m.x > croom->hx ||
                  m.y < croom->ly || m.y > croom->hy)
                 && (++tryct <= 27));
    } while (tryct <= 27);
}


/* If given a branch, randomly place a special stair or portal. */
void
place_branch(struct level *lev, branch * br,    /* branch to place */
             xchar x, xchar y)
{       /* location */
    coord m;
    d_level *dest;
    boolean make_stairs;
    struct mkroom *br_room;

    /*
     * Return immediately if there is no branch to make or we have
     * already made one.  This routine can be called twice when
     * a special level is loaded that specifies an SSTAIR location
     * as a favored spot for a branch.
     *
     * As a special case, we also don't actually put anything into
     * the castle level.
     */
    if (!br || made_branch || Is_stronghold(&lev->z))
        return;

    if (x == COLNO) {   /* find random coordinates for branch */
        br_room = find_branch_room(lev, &m);
        x = m.x;
        y = m.y;
    } else {
        br_room = pos_to_room(lev, x, y);
    }

    if (on_level(&br->end1, &lev->z)) {
        /* we're on end1 */
        make_stairs = br->type != BR_NO_END1;
        dest = &br->end2;
    } else {
        /* we're on end2 */
        make_stairs = br->type != BR_NO_END2;
        dest = &br->end1;
    }

    if (!isok(x, y))
        panic("placing dungeon branch outside the map bounds");
    if (!x && !y)
        impossible("suspicious attempt to place dungeon branch at (0, 0)");

    if (br->type == BR_PORTAL) {
        mkportal(lev, x, y, dest->dnum, dest->dlevel);
    } else if (make_stairs) {
        lev->sstairs.sx = x;
        lev->sstairs.sy = y;
        lev->sstairs.up =
            (char)on_level(&br->end1, &lev->z) ? br->end1_up : !br->end1_up;
        assign_level(&lev->sstairs.tolev, dest);
        lev->sstairs_room = br_room;

        lev->locations[x][y].ladder = lev->sstairs.up ? LA_UP : LA_DOWN;
        lev->locations[x][y].typ = STAIRS;
    }
    /*
     * Set made_branch to TRUE even if we didn't make a stairwell (i.e.
     * make_stairs is false) since there is currently only one branch
     * per level, if we failed once, we're going to fail again on the
     * next call.
     */
    made_branch = TRUE;
}

static boolean
bywall(struct level *lev, xchar x, xchar y)
{
    int typ;

    if (isok(x + 1, y)) {
        typ = lev->locations[x + 1][y].typ;
        if (IS_WALL(typ) || typ == SDOOR)
            return TRUE;
    }
    if (isok(x - 1, y)) {
        typ = lev->locations[x - 1][y].typ;
        if (IS_WALL(typ) || typ == SDOOR)
            return TRUE;
    }
    if (isok(x, y + 1)) {
        typ = lev->locations[x][y + 1].typ;
        if (IS_WALL(typ) || typ == SDOOR)
            return TRUE;
    }
    if (isok(x, y - 1)) {
        typ = lev->locations[x][y - 1].typ;
        if (IS_WALL(typ) || typ == SDOOR)
            return TRUE;
    }
    return FALSE;
}

static boolean
bydoor(struct level *lev, xchar x, xchar y)
{
    int typ;

    if (isok(x + 1, y)) {
        typ = lev->locations[x + 1][y].typ;
        if (IS_DOOR(typ) || typ == SDOOR)
            return TRUE;
    }
    if (isok(x - 1, y)) {
        typ = lev->locations[x - 1][y].typ;
        if (IS_DOOR(typ) || typ == SDOOR)
            return TRUE;
    }
    if (isok(x, y + 1)) {
        typ = lev->locations[x][y + 1].typ;
        if (IS_DOOR(typ) || typ == SDOOR)
            return TRUE;
    }
    if (isok(x, y - 1)) {
        typ = lev->locations[x][y - 1].typ;
        if (IS_DOOR(typ) || typ == SDOOR)
            return TRUE;
    }
    return FALSE;
}

/* see whether it is allowable to create a door at [x,y] */
int
okdoor(struct level *lev, xchar x, xchar y)
{
    boolean near_door = bydoor(lev, x, y);

    return ((lev->locations[x][y].typ == HWALL ||
             lev->locations[x][y].typ == VWALL) && lev->doorindex < DOORMAX &&
            !near_door);
}

void
dodoor(struct level *lev, int x, int y, struct mkroom *aroom)
{
    if (lev->doorindex >= DOORMAX) {
        impossible("DOORMAX exceeded?");
        return;
    }

    dosdoor(lev, x, y, aroom, mrn2(8) ? DOOR : SDOOR);
}

boolean
occupied(struct level * lev, xchar x, xchar y)
{
    return ((boolean) (t_at(lev, x, y)
                       || IS_FURNITURE(lev->locations[x][y].typ)
                       || IS_WALL(lev->locations[x][y].typ)
                       || (lev->locations[x][y].typ == STONE)
                       || is_lava(lev, x, y)
                       || is_pool(lev, x, y)
                       || invocation_pos(&lev->z, x, y)
            ));
}

/* make a trap somewhere (in croom if mazeflag = 0 && !tm) */
/* if tm != null, make trap at that location */
void
mktrap(struct level *lev, int num, int mazeflag, struct mkroom *croom,
       coord * tm)
{
    int kind;
    coord m;
    boolean holeok = can_fall_thru(lev); /* HOLE or TRAPDOOR */

    /* no traps in pools */
    if (tm && is_pool(lev, tm->x, tm->y))
        return;

    if (num > 0 && num < TRAPNUM) {
        kind = num;
    } else if (Is_rogue_level(&lev->z)) {
        switch (mrn2(7)) {
        default:
            kind = BEAR_TRAP;
            break;      /* 0 */
        case 1:
            kind = ARROW_TRAP;
            break;
        case 2:
            kind = DART_TRAP;
            break;
        case 3:
            kind = TRAPDOOR;
            break;
        case 4:
            kind = PIT;
            break;
        case 5:
            kind = SLP_GAS_TRAP;
            break;
        case 6:
            kind = RUST_TRAP;
            break;
        }
    } else if (In_hell(&lev->z) && !mrn2(5)) {
        /* bias the frequency of fire traps in Gehennom */
        kind = FIRE_TRAP;
    } else {
        unsigned lvl = level_difficulty(&lev->z);

        do {
            kind = 1 + mrn2(TRAPNUM - 1);
            /* reject "too hard" traps */
            switch (kind) {
            case MAGIC_PORTAL:
            case VIBRATING_SQUARE:
                kind = NO_TRAP;
                break;
            case ROLLING_BOULDER_TRAP:
            case SLP_GAS_TRAP:
                if (lvl < 2)
                    kind = NO_TRAP;
                break;
            case ROCKTRAP:
                /* NetHack Fourk balance change: no instapetkill rock traps on
                 * the first three levels of the main dungeon. */
                if ((lvl < 4) && (lev->z.dnum == medusa_level.dnum))
                    kind = NO_TRAP;
                break;
            case LEVEL_TELEP:
                if (lvl < 5 || lev->flags.noteleport)
                    kind = NO_TRAP;
                break;
            case SPIKED_PIT:
                if (lvl < 5)
                    kind = NO_TRAP;
                break;
            case LANDMINE:
                if (lvl < 6)
                    kind = NO_TRAP;
                break;
            case WEB:
                if (lvl < 7)
                    kind = NO_TRAP;
                break;
            case STATUE_TRAP:
            case POLY_TRAP:
                if (lvl < 8)
                    kind = NO_TRAP;
                break;
            case STINKING_TRAP:
            case FIRE_TRAP:
                if (!In_hell(&lev->z))
                    kind = NO_TRAP;
                break;
            case TELEP_TRAP:
                if (lev->flags.noteleport)
                    kind = NO_TRAP;
                break;
            case HOLE:
                /* make these much less often than other traps */
                if (!holeok || mrn2(7))
                    kind = NO_TRAP;
                break;
            }
        } while (kind == NO_TRAP);
    }

    if ((kind == TRAPDOOR || kind == HOLE) && !holeok)
        kind = ROCKTRAP;

    if (tm)
        m = *tm;
    else {
        int tryct = 0;
        boolean avoid_boulder = (is_pit_trap(kind) ||
                                 kind == TRAPDOOR || kind == HOLE);

        do {
            if (++tryct > 200)
                return;
            if (mazeflag)
                mazexy(lev, &m);
            else if (!somexy(lev, croom, &m, mrng()))
                return;
        } while (occupied(lev, m.x, m.y) ||
                 (avoid_boulder && sobj_at(BOULDER, lev, m.x, m.y)));
    }

    maketrap(lev, m.x, m.y, kind, mrng());
    if (kind == WEB) {
        struct monst *spider = makemon(&mons[PM_GIANT_SPIDER],
                                       lev, m.x, m.y, MM_ALLLEVRNG);
        if (spider && !resists_sleep(spider))
            spider->msleeping = 1;
    }
}


void
mkstairs(struct level *lev, xchar x, xchar y, char up, struct mkroom *croom)
{
    if (!isok(x, y)) {
        impossible("mkstairs: bogus stair attempt at <%d,%d>", x, y);
        return;
    }
    if (!x && !y) {
        /* In 4.3-beta{1,2}, this doesn't save correctly, and there's no known
           way to get stairs here anyway... */
        impossible("mkstairs: suspicious stair attempt at <%d,%d>", x, y);
        return;
    }

    /*
     * We can't make a regular stair off an end of the dungeon.  This
     * attempt can happen when a special level is placed at an end and
     * has an up or down stair specified in its description file.
     */
    if ((dunlev(&lev->z) == 1 && up) ||
        (dunlev(&lev->z) == dunlevs_in_dungeon(&lev->z) && !up))
        return;

    if (up) {
        lev->upstair.sx = x;
        lev->upstair.sy = y;
        lev->upstairs_room = croom;
    } else {
        lev->dnstair.sx = x;
        lev->dnstair.sy = y;
        lev->dnstairs_room = croom;
    }

    lev->locations[x][y].typ = STAIRS;
    lev->locations[x][y].ladder = up ? LA_UP : LA_DOWN;
}


static void
mkfount(struct level *lev, int mazeflag, struct mkroom *croom)
{
    coord m;
    int tryct = 0;

    do {
        if (++tryct > 200)
            return;
        if (mazeflag)
            mazexy(lev, &m);
        else if (!somexy(lev, croom, &m, mrng()))
            return;
    } while (occupied(lev, m.x, m.y) || bydoor(lev, m.x, m.y));

    /* Put a fountain at m.x, m.y */
    lev->locations[m.x][m.y].typ = FOUNTAIN;
    /* Is it a "blessed" fountain? (affects drinking from fountain) */
    if (!mrn2(7))
        lev->locations[m.x][m.y].blessedftn = 1;
}


static void
mksink(struct level *lev, struct mkroom *croom)
{
    coord m;
    int tryct = 0;

    do {
        if (++tryct > 200)
            return;
        if (!somexy(lev, croom, &m, mrng()))
            return;
    } while (occupied(lev, m.x, m.y) || bydoor(lev, m.x, m.y) ||
             (tryct < 50 && !bywall(lev, m.x, m.y)));

    /* Put a sink at m.x, m.y */
    lev->locations[m.x][m.y].typ = SINK;
}


static void
mkaltar(struct level *lev, struct mkroom *croom)
{
    coord m;
    int tryct = 0;
    aligntyp al;

    if (croom->rtype != OROOM)
        return;

    do {
        if (++tryct > 200)
            return;
        if (!somexy(lev, croom, &m, mrng()))
            return;
    } while (occupied(lev, m.x, m.y) || bydoor(lev, m.x, m.y));

    /* Put an altar at m.x, m.y */
    lev->locations[m.x][m.y].typ = ALTAR;

    /* -1 - A_CHAOTIC, 0 - A_NEUTRAL, 1 - A_LAWFUL */
    al = mrn2((int)A_LAWFUL + 2) - 1;
    lev->locations[m.x][m.y].altarmask = Align2amask(al);
}

static void
mkbench(struct level *lev, struct mkroom *croom)
{
    coord m;
    int tryct = 0;

    do {
        if (++tryct > 20)
            return;
        if (!somexy(lev, croom, &m, mrng()))
            return;
    } while (occupied(lev, m.x, m.y) || bydoor(lev, m.x, m.y) ||
             (tryct < 5 && !bywall(lev, m.x, m.y)));

    lev->locations[m.x][m.y].typ = BENCH;
}

static void
mkgrave(struct level *lev, struct mkroom *croom)
{
    coord m;
    int tryct = 0;
    struct obj *otmp;
    boolean dobell = !mrn2(10);

    if (croom->rtype != OROOM)
        return;

    do {
        if (++tryct > 200)
            return;
        if (!somexy(lev, croom, &m, mrng()))
            return;
    } while (occupied(lev, m.x, m.y) || bydoor(lev, m.x, m.y));

    /* Put a grave at m.x, m.y */
    make_grave(lev, m.x, m.y, dobell ? "Saved by the bell!" : NULL);

    /* Possibly fill it with objects */
    if (!mrn2(3))
        mkfloorgold(0L, lev, m.x, m.y, mrng());
    for (tryct = mrn2(5); tryct; tryct--) {
        otmp = mkobj(lev, RANDOM_CLASS, TRUE, mrng());
        if (!otmp)
            return;
        curse(otmp);
        otmp->ox = m.x;
        otmp->oy = m.y;
        add_to_buried(otmp);
    }

    /* Leave a bell, in case we accidentally buried someone alive */
    if (dobell)
        mksobj_at(BELL, lev, m.x, m.y, TRUE, FALSE, mrng());
    return;
}


/* maze levels have slightly different constraints from normal levels */
#define x_maze_min 2
#define y_maze_min 2
/*
 * Major level transmutation: add a set of stairs (to the Sanctum) after
 * an earthquake that leaves behind a a new topology, centered at inv_pos.
 * Assumes there are no rooms within the invocation area and that inv_pos
 * is not too close to the edge of the map.
 */
void
mkinvokearea(void)
{
    int dist;
    xchar xmin = gamestate.inv_pos.x, xmax = gamestate.inv_pos.x;
    xchar ymin = gamestate.inv_pos.y, ymax = gamestate.inv_pos.y;
    xchar i;

    pline(msgc_levelsound, "The floor shakes violently under you!");
    if (Blind)
        pline_implied(msgc_levelsound,
                      "The entire dungeon seems to be tearing apart!");
    else
        pline_implied(msgc_levelsound,
                      "The walls around you begin to bend and crumble!");
    win_pause_output(P_MESSAGE);

    achievement(achieve_invocation);

    mkinvpos(xmin, ymin, 0);    /* middle, before placing stairs */

    for (dist = 1; dist < 7; dist++) {
        xmin--;
        xmax++;

        /* top and bottom */
        if (dist != 3) {        /* the area is wider that it is high */
            ymin--;
            ymax++;
            for (i = xmin + 1; i < xmax; i++) {
                mkinvpos(i, ymin, dist);
                mkinvpos(i, ymax, dist);
            }
        }

        /* left and right */
        for (i = ymin; i <= ymax; i++) {
            mkinvpos(xmin, i, dist);
            mkinvpos(xmax, i, dist);
        }

        flush_screen(); /* make sure the new glyphs shows up */
        win_delay_output();
    }

    if (Blind)
        pline(msgc_levelsound,
              "You feel the stones reassemble below you!");
    else
        pline(msgc_levelsound,
              "You are standing at the top of a stairwell leading down!");
    mkstairs(level, u.ux, u.uy, 0, NULL);       /* down */
    newsym(u.ux, u.uy);
    turnstate.vision_full_recalc = TRUE;     /* everything changed */
}


/* Change level topology.  Boulders in the vicinity are eliminated.
 * Temporarily overrides vision in the name of a nice effect.
 */
static void
mkinvpos(xchar x, xchar y, int dist)
{
    struct trap *ttmp;
    struct obj *otmp;
    struct monst *mtmp;
    boolean make_rocks;
    struct rm *loc = &level->locations[x][y];

    /* clip at existing map borders if necessary */
    if (!within_bounded_area(x, y, 1, 1, COLNO - 1, ROWNO - 1)) {
        /* only outermost 2 columns and/or rows may be truncated due to edge */
        if (dist < (7 - 2)) {
            impossible("mkinvpos: <%d,%d> (%d) off map edge!", x, y, dist);
            if (dist == 0)
                panic("mkinvpos: stairs would be placed off the map.");
        }
        return;
    }
    if (loc->typ == STAIRS) {
        impossible("mkinvpos: <%d,%d> (%d) is already stairs.", x, y, dist);
        if (dist == 0)
                panic("mkinvpos: can't put two sets of stairs in same place.");
        return;
    }

    /* clear traps */
    if ((ttmp = t_at(level, x, y)) != 0)
        deltrap(level, ttmp);

    /* clear boulders; leave some rocks for non-{moat|trap} locations */
    make_rocks = (dist != 1 && dist != 4 && dist != 5) ? TRUE : FALSE;
    while ((otmp = sobj_at(BOULDER, level, x, y)) != 0) {
        if (make_rocks) {
            fracture_rock(otmp);
            make_rocks = FALSE; /* don't bother with more rocks */
        } else {
            obj_extract_self(otmp);
            obfree(otmp, NULL);
        }
    }

    if (!Blind) {
        /* make sure vision knows this location is open */
        unblock_point(x, y);

        /* fake out saved state */
        loc->seenv = 0;
        loc->doormask = 0;
        if (dist < 6)
            loc->lit = TRUE;
        loc->waslit = TRUE;
        loc->horizontal = FALSE;
        clear_memory_glyph(x, y, S_unexplored);
        viz_array[y][x] = (dist < 6) ?
            (IN_SIGHT | COULD_SEE) :     /* short-circuit vision recalc */
            COULD_SEE;
    }

    switch (dist) {
    case 1:    /* fire traps */
        if (is_pool(level, x, y))
            break;
        loc->typ = ROOM;
        ttmp = maketrap(level, x, y, FIRE_TRAP, rng_main);
        if (ttmp)
            ttmp->tseen = TRUE;
        break;
    case 0:    /* lit room locations */
    case 2:
    case 3:
    case 6:    /* unlit room locations */
        loc->typ = ROOM;
        break;
    case 4:    /* pools (aka a wide moat) */
    case 5:
        loc->typ = MOAT;
        mtmp = m_at(level, x, y);
        if (mtmp)
            minliquid(mtmp);
        /* No kelp! */
        break;
    default:
        impossible("mkinvpos called with dist %d", dist);
        break;
    }

    /* display new value of position; could have a monster/object on it */
    newsym(x, y);
}

/*
 * The portal to Ludios is special.  The entrance can only occur within a
 * vault in the main dungeon at a depth greater than 10.  The Ludios branch
 * structure reflects this by having a bogus "source" dungeon:  the value
 * of n_dgns (thus, Is_branchlev() will never find it).
 *
 * Ludios will remain isolated until the branch is corrected by this function.
 */
static void
mk_knox_portal(struct level *lev, xchar x, xchar y)
{
    d_level *source;
    branch *br;
    schar u_depth;

    br = dungeon_branch("Fort Ludios");
    if (on_level(&knox_level, &br->end1)) {
        source = &br->end2;
    } else {
        /* disallow Knox branch on a level with one branch already */
        if (Is_branchlev(&lev->z))
            return;
        source = &br->end1;
    }

    /* Already set or 2/3 chance of deferring until a later level. */
    if (source->dnum < n_dgns || (mrn2(3) && !wizard))
        return;

    if (!(lev->z.dnum == oracle_level.dnum      /* in main dungeon */
          && !at_dgn_entrance(&lev->z, "The Quest")     /* but not Quest's
                                                           entry */
          &&(u_depth = depth(&lev->z)) > 10     /* beneath 10 */
          && u_depth < depth(&medusa_level)))   /* and above Medusa */
        return;

    /* Adjust source to be current level and re-insert branch. */
    *source = lev->z;
    insert_branch(br, TRUE);

    place_branch(lev, br, x, y);
}

/*
 * The portal to the Advent Calender is special. 
 * It does not only lead to a floating branch like knox portal.
 * It also may appear upon reentering a existing level if it is the
 * right time of the year.
 */
boolean
mk_advcal_portal(struct level *lev)
{
    extern int n_dgns;		/* from dungeon.c */
    d_level *source;
    branch *br;

    /*
     * made_branch remains unchanged when entering an already-created
     * level. This leads to the branch inserted in the dungeon level 
     * list but no portal created, so the branch is unreachable.
     *
     * Technically this is a bug but nobody anticipated a branch
     * that could be inserted after level creation.
     */
    if (made_branch) return FALSE;

    br = dungeon_branch("Advent Calendar");
    if (on_level(&advcal_level, &br->end1)) {
        source = &br->end2;
    } else {
        /* disallow branch on a level with one branch already */
        if (Is_branchlev(&u.uz))
            return FALSE;
        source = &br->end1;
    }

    /* Already set. */
    if (source->dnum < n_dgns) return FALSE;

    if (! (u.uz.dnum == oracle_level.dnum	    /* in main dungeon */
           && !at_dgn_entrance(&u.uz, "The Quest")  /* but not Quest's entry */
           && depth(&u.uz) < depth(&medusa_level))) /* and above Medusa */
        return FALSE;

    /* Adjust source to be current level and re-insert branch. */
    *source = u.uz;
    insert_branch(br, TRUE);
    
    /*
    if (wizard)
        pline(msgc_debug, "Made advent calendar portal.");
    */
    place_branch(lev, br, COLNO, ROWNO);

    return TRUE;
}


/*mklev.c*/
