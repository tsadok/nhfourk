/* vim:set cin ft=c sw=4 sts=4 ts=8 et ai cino=Ls\:0t0(0 : -*- mode:c;fill-column:80;tab-width:8;c-basic-offset:4;indent-tabs-mode:nil;c-file-style:"k&r" -*-*/
/* Last modified by Alex Smith, 2014-04-05 */
/*      Monster symbols and creation information rev 1.0          */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef MONSYM_H
# define MONSYM_H

/*
 * Monster classes.  Below, are the corresponding default characters for
 * them.  Monster class 0 is not used or defined so we can use it as a
 * NULL character.
 */
# define S_ANT          1
# define S_BLOB         2 /* Currently unused */
# define S_COCKATRICE   3
# define S_DOG          4
# define S_EYE          5
# define S_FELINE       6
# define S_GREMLIN      7
# define S_HUMANOID     8
# define S_IMP          9
# define S_JELLY        10
# define S_KOBOLD       11
# define S_LIZARD       12
# define S_MIMIC        13
# define S_NYMPH        14
# define S_ORC          15
# define S_PIERCER      16 /* Currently unused */
# define S_QUADRUPED    17
# define S_RODENT       18
# define S_SPIDER       19
# define S_TRAPPER      20
# define S_UNICORN      21
# define S_VORTEX       22
# define S_WORM         23
# define S_XAN          24
# define S_LIGHT        25
# define S_ANGEL        26
# define S_BAT          27
# define S_CENTAUR      28
# define S_DRAGON       29
# define S_ELEMENTAL    30
# define S_FUNGUS       31
# define S_GNOME        32
# define S_GIANT        33

# define S_JABBERWOCK   35
# define S_KOP          36
# define S_LICH         37
# define S_MUMMY        38
# define S_NAGA         39
# define S_OGRE         40
# define S_PUDDING      41
# define S_QUANTMECH    42
# define S_RUSTMONST    43
# define S_SNAKE        44
# define S_TROLL        45
# define S_UMBER        46
# define S_VAMPIRE      47
# define S_WRAITH       48
# define S_XORN         49
# define S_YETI         50
# define S_ZOMBIE       51
# define S_HUMAN        52
# define S_GOLEM        53
# define S_DEMON        54
# define S_EEL          55

# define S_WORM_TAIL    56
# define S_MIMIC_DEF    57

# define MAXMCLASSES 58 /* number of monster classes */

/*
 * Default characters for monsters.  These correspond to the monster classes
 * above.
 */
# define DEF_ANT        'a'
# define DEF_BLOB       'b' /* Currently unused */
# define DEF_COCKATRICE 'c'
# define DEF_DOG        'd'
# define DEF_EYE        'e'
# define DEF_FELINE     'f'
# define DEF_GREMLIN    'g'
# define DEF_HUMANOID   'h'
# define DEF_IMP        'i'
# define DEF_JELLY      'j'
# define DEF_KOBOLD     'k'
# define DEF_LIZARD     'l'
# define DEF_MIMIC      'm'
# define DEF_NYMPH      'n'
# define DEF_ORC        'o'
# define DEF_PIERCER    'p' /* Currently unused */
# define DEF_QUADRUPED  'q'
# define DEF_RODENT     'r'
# define DEF_SPIDER     's'
# define DEF_TRAPPER    't'
# define DEF_UNICORN    'u'
# define DEF_VORTEX     'v'
# define DEF_WORM       'w'
# define DEF_XAN        'x'
# define DEF_LIGHT      'y'
# define DEF_ANGEL      'A'
# define DEF_BAT        'B'
# define DEF_CENTAUR    'C'
# define DEF_DRAGON     'D'
# define DEF_ELEMENTAL  'E'
# define DEF_FUNGUS     'F'
# define DEF_GNOME      'G'
# define DEF_GIANT      'H'
# define DEF_JABBERWOCK 'J'
# define DEF_KOP        'K'
# define DEF_LICH       'L'
# define DEF_MUMMY      'M'
# define DEF_NAGA       'N'
# define DEF_OGRE       'O'
# define DEF_PUDDING    'P'
# define DEF_QUANTMECH  'Q'
# define DEF_RUSTMONST  'R'
# define DEF_SNAKE      'S'
# define DEF_TROLL      'T'
# define DEF_UMBER      'U'
# define DEF_VAMPIRE    'V'
# define DEF_WRAITH     'W'
# define DEF_XORN       'X'
# define DEF_YETI       'Y'
# define DEF_ZOMBIE     'Z'
# define DEF_HUMAN      '@'
# define DEF_GOLEM      '\''
# define DEF_DEMON      '&'
# define DEF_EEL        ';'

# define DEF_INVISIBLE  'I'
# define DEF_WORM_TAIL  '~'
# define DEF_MIMIC_DEF  ']'

#endif /* MONSYM_H */

