#!/bin/sh
# Last modified by Jonadab the Unsightly One, 2017-Nov-21
for x in `git ls-tree -r HEAD | cut -f 2`
do
    sed -i -e "1,2s/Last modified by.*, ....-..-../$(git log -n 1 --pretty=format:'Last modified by %an, %at' $x | perl -pe 's/(\d+)$/`date --date=\@$1 +%F`/e')/" $x
done
