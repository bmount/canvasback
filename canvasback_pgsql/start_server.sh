gcc canvasback_pq.c -lpq -lfcgi -o canvasback_pq
spawn-fcgi -a0.0.0.0 -p9000 -n ./canvasback_pq
