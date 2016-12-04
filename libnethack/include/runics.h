/* vim:set cin ft=c sw=4 sts=4 ts=8 et ai cino=Ls\:0t0(0 : -*- mode:c;fill-column:80;tab-width:8;c-basic-offset:4;indent-tabs-mode:nil;c-file-style:"k&r" -*-*/
/* This file (runics.h) is in the public domain. */

#ifndef RUNICS_H
#define RUNICS_H

#include "global.h"
#include "prop.h"
#include "runes.h"
#include "obj.h"

extern boolean rune_can_occur(enum rune, enum objslot);
extern enum objslot runeslot(struct obj *);

#endif /* RUNICS_H */

