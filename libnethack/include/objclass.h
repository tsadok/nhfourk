/* vim:set cin ft=c sw=4 sts=4 ts=8 et ai cino=Ls\:0t0(0 : -*- mode:c;fill-column:80;tab-width:8;c-basic-offset:4;indent-tabs-mode:nil;c-file-style:"k&r" -*-*/
/* Last modified by Alex Smith, 2015-06-15 */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef OBJCLASS_H
# define OBJCLASS_H

# include "global.h"

/* definition of a class of objects */

struct objclass {
    short oc_name_idx;  /* index of actual name */
    short oc_descr_idx; /* description when name unknown */
    char *oc_uname;     /* called by user */
    unsigned oc_name_known:1;
    unsigned oc_merge:1;        /* merge otherwise equal objects */
    unsigned oc_uses_known:1;   /* obj->known affects full decription */
    /* otherwise, obj->dknown and obj->bknown */
    /* tell all, and obj->known should always */
    /* be set for proper merging behavior */
    unsigned oc_pre_discovered:1;       /* Already known at start of game; */
    /* won't be listed as a discovery. */
    unsigned oc_magic:1;        /* inherently magical object */
    unsigned oc_charged:1;      /* may have +n or (n) charges */
    unsigned oc_unique:1;       /* special one-of-a-kind object */
    unsigned oc_nowish:1;       /* cannot wish for this object */

    unsigned oc_disclose_id:1;  /* was identified by DYWYPI */

    unsigned oc_big:1;
# define oc_bimanual    oc_big  /* for weapons & tools used as weapons */
# define oc_bulky       oc_big  /* for armor */
    unsigned oc_tough:1;        /* hard gems/rings */

    unsigned oc_dir:3;
# define NODIR          1       /* for wands/spells: non-directional */
# define IMMEDIATE      2       /* directional */
# define RAY            3       /* zap beams */

# define PIERCE         1       /* for weapons & tools used as weapons */
# define SLASH          2       /* (latter includes iron ball & chain) */
# define WHACK          4

    unsigned oc_material:5;
# define LIQUID         1       /* currently only for venom and vampire blood */
/* Start of burnable items (some code cares about this order) */
# define WAX            2
# define VEGGY          3       /* foodstuffs */
# define FLESH          4       /* ditto */
# define PAPER          5
# define CLOTH          6
# define LEATHER        7
# define WOOD           8
/* End of burnable items */
# define BONE           9
# define DRAGON_HIDE    10      /* not leather! */
# define IRON           11      /* Fe - includes steel */
# define METAL          12      /* Sn, &c. */
# define COPPER         13      /* Cu - includes brass */
# define SILVER         14      /* Ag */
# define GOLD           15      /* Au */
# define PLATINUM       16      /* Pt */
# define MITHRIL        17      /* fantasy version of titanium steel */
# define PLASTIC        18
# define GLASS          19
# define GEMSTONE       20
# define MINERAL        21
/* If the above list of materials changes, it is necessary to update
   matname[] in objnam.c and also foodwords[] in eat.c */

# define is_organic(otmp)       (objects[otmp->otyp].oc_material <= WOOD)
# define is_metallic(otmp)      (objects[otmp->otyp].oc_material >= IRON && \
                                 objects[otmp->otyp].oc_material <= MITHRIL)

/* primary damage: fire/rust/--- */
/* is_flammable(otmp), is_rottable(otmp) in mkobj.c */
# define is_rustprone(otmp)     (objects[otmp->otyp].oc_material == IRON)

/* secondary damage: rot/acid/acid */
# define is_corrodeable(otmp) \
    (objects[otmp->otyp].oc_material == COPPER || \
     objects[otmp->otyp].oc_material == IRON)

# define is_damageable(otmp) (is_rustprone(otmp) || is_flammable(otmp) || \
                                is_rottable(otmp) || is_corrodeable(otmp))

    schar oc_subtyp;
# define oc_skill       oc_subtyp       /* Skills of weapons, spellbooks,
                                           tools, gems */
# define oc_armcat      oc_subtyp       /* for armor */
# define ARM_SHIELD     1       /* needed for special wear function */
# define ARM_HELM       2
# define ARM_GLOVES     3
# define ARM_BOOTS      4
# define ARM_CLOAK      5
# define ARM_SHIRT      6
# define ARM_SUIT       0

    uchar oc_oprop;     /* property (invis, &c.) conveyed */
    uchar oc_oprop2;    /* additional property */
    char oc_class;      /* object class */
    schar oc_delay;     /* delay when using such an object */
    uchar oc_color;     /* color of the object */

    short oc_prob;      /* probability, used in mkobj() */
    unsigned short oc_weight;   /* encumbrance (1 cn = 0.1 lb.) */
    short oc_cost;      /* base cost in shops */
/* Check the AD&D rules!  The FIRST is small monster damage. */
/* for weapons, and tools, rocks, and gems useful as weapons */
    schar oc_wsdam, oc_wldam;   /* max small/large monster damage */
    schar oc_oc1, oc_oc2;
# define oc_hitbon      oc_oc1  /* weapons: "to hit" bonus */

# define oc_defletter   oc_oc1  /* books: default spell letter */
# define a_ac           oc_oc1  /* armor class, used in ARM_BONUS in do.c */
# define a_can          oc_oc2  /* armor: used in mhitu.c */
# define oc_level       oc_oc2  /* books: spell level */
# define a_minsize      oc_wsdam  /* minimim MZ_ size monster it fits. */
# define a_maxsize      oc_wldam  /* maximum MZ_ size monster it fits. */

    unsigned short oc_nutrition;        /* food value */
};

struct objdescr {
    const char *oc_name;        /* actual name */
    const char *oc_descr;       /* description when name unknown */
};

extern struct objclass *objects;
extern const struct objclass const_objects[];
extern const struct objdescr obj_descr[];

/* copy object list into objects */
extern void init_objlist(void);

/*
 * All objects have a class. Make sure that all classes have a corresponding
 * symbol below.
 */
# define RANDOM_CLASS    0      /* used for generating random objects */
# define ILLOBJ_CLASS    1
# define WEAPON_CLASS    2
# define ARMOR_CLASS     3
# define RING_CLASS      4
# define AMULET_CLASS    5
# define TOOL_CLASS      6
# define FOOD_CLASS      7
# define POTION_CLASS    8
# define SCROLL_CLASS    9
# define SPBOOK_CLASS   10      /* actually SPELL-book */
# define WAND_CLASS     11
# define COIN_CLASS     12
# define GEM_CLASS      13
# define ROCK_CLASS     14
# define BALL_CLASS     15
# define CHAIN_CLASS    16
# define VENOM_CLASS    17
# define MAXOCLASSES    18

# define ALLOW_COUNT    (MAXOCLASSES+1) /* Can be used in the object class */
# define ALL_CLASSES    (MAXOCLASSES+2) /* input to getobj().  */
# define ALLOW_NONE     (MAXOCLASSES+3) /* */
# define NONE_ON_COMMA  (MAXOCLASSES+4) /* Render ALLOW_NONE as , not -.  */
# define SPLIT_LETTER   (MAXOCLASSES+5) /* Split on count should use letter */

# define BURNING_OIL    (MAXOCLASSES+1) /* Can be used as input to explode. */
# define MON_EXPLODE    (MAXOCLASSES+2) /* Exploding monster (e.g. gas spore) */

/* Default definitions of all object-symbols (must match classes above). */

# define ILLOBJ_SYM     ']'     /* also used for mimics */
# define WEAPON_SYM     ')'
# define ARMOR_SYM      '['
# define RING_SYM       '='
# define AMULET_SYM     '"'
# define TOOL_SYM       '('
# define FOOD_SYM       '%'
# define POTION_SYM     '!'
# define SCROLL_SYM     '?'
# define SPBOOK_SYM     '+'
# define WAND_SYM       '/'
# define GOLD_SYM       '$'
# define GEM_SYM        '*'
# define ROCK_SYM       '`'
# define BALL_SYM       '0'
# define CHAIN_SYM      '_'
# define VENOM_SYM      '.'

struct fruit {
    char fname[PL_FSIZ];
    int fid;
    struct fruit *nextf;
};

# define newfruit() malloc(sizeof(struct fruit))
# define dealloc_fruit(rind) free(rind)

# define OBJ_NAME(obj)  (obj_descr[(obj).oc_name_idx].oc_name)
# define OBJ_DESCR(obj) (obj_descr[(obj).oc_descr_idx].oc_descr)
#endif /* OBJCLASS_H */

