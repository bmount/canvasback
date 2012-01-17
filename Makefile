canvasback: canvasback.o picoev/picoev_select.o db.conf.c picohttpparser/picohttpparser.o 
		gcc -o canvasback -lpq canvasback.o picoev/picoev_select.o picohttpparser/picohttpparser.o
canvasback.o: canvasback.c picoev/picoev.h
		gcc -c canvasback.c
picoev_select.o: picoev/picoev_select.c
		gcc -c picoev/picoev_select.c
picohttpparser.o: picohttpparser/picohttpparser.c 
		gcc -c picohttpparser/picohttpparser.c 
