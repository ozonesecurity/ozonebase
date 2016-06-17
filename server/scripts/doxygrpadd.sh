#!/bin/bash


START=$'/** @addtogroup Providers */\n/*@{*/\n\n'
END=$'\n\n/*@}*/'
echo "$START" > start_file
echo "$END" > stop_file




for f in *.h
do
	cat start_file $f  stop_file >$f.doxy
	rm $f
	mv $f.doxy $f
done
rm -f start_file stop_file
