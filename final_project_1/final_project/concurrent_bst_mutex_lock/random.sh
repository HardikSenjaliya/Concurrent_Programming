#!/bin/bash

get_seeded_random()
{
  seed="$1"
  openssl enc -aes-256-ctr -pass pass:"$seed" -nosalt \
    </dev/zero 2>/dev/null
}

shuf -i1-10 --random-source=<(get_seeded_random 1000) > search_nodes.txt


#https://www.gnu.org/software/coreutils/manual/html_node/Random-sources.html