#!/bin/sh
# Last modified by Jonadab the Unsightly One, 2017-Nov-14

# Generate a TAGS file for use with Emacs.
# Run from the root of the distribution, not the scripts folder.
etags */*/*.[ch] */*/*/*.[ch]
