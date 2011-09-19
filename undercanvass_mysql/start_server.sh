#!/bin/sh

gcc undercanvass_my.c -o undercanvass_my `mysql_config --libs --cflags` -lfcgi
# if you installed mysql from macports mysql_config is probably mysql_config5
# gcc canvas_fish.c -o canvas_fish `mysql_config5 --libs --cflags` -lfcgi
spawn-fcgi -a127.0.0.1 -p9000 -n ./undercanvass_my
