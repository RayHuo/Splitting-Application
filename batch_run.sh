#!/bin/bash

summary='summary'
> ${summary}

result_out='IO/claspInput.txt'
> ${result_out}

for i in `find IO/input/ -type f`; do
    echo $i
	./splittingapplication $i ${result_out}
	echo '#hide .' >> $i
	echo $i >> $summary
	echo -n 'origin ' >> $summary
	gringo $i | clasp 1 | grep Time >> $summary
	echo -n 'simplification ' >> $summary
	gringo ${result_out} | clasp 1 | grep Time >> $summary
done

