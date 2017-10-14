#!/bin/bash
i=1
n=`cat perft/fenlist.txt | wc --lines`
while [[ $i -le $n ]]
do
    fen=`head -n $i perft/fenlist.txt | tail -n 1`
    str="uci\nposition fen $fen\nperft $1\nquit"
    echo "$fen"
    echo -e $str | ./teki | sed -n -e '/^[0-9]/p'
    echo
    i=`echo "$i+1" | bc`
done
