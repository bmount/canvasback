#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <libpq-fe.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include "picoev/picoev.h"
#include "picohttpparser/picohttpparser.h"
#include "db.conf.c"


#define PORT 7987
#define MAX_FDS 100000
#define TIMEOUT_SECS 10

#define EARTH 6378137

unsigned short port = PORT;
int listen_sock;

/* 
typedef unsigned char uint8;
   working on binary branch, default behavior here. Use fmt_res_bin and db_bin_cb as callback
   query should return wkb (postgis st_asbinary) as only column

  For geojson: use a fmt_res_geojson and db_geojson_bin as callback 
  with a query like this (geometry first, everything else will be properties):

  Example geometry-only query:
   
  char base_query[600] = "select st_asgeojson(st_transform(way, 4326)) from planet_osm_roads where way && st_envelope(st_transform(st_geomfromtext('linestring(%f %f,%f %f)', 4326), 900913)) limit 30;";

   All fields besides the geometry as properties:

    char base_query[600] = "select st_asgeojson(way), * from planet_osm_roads where way && st_envelope(st_geomfromtext('linestring(%f %f,%f %f)', 4326)) limit 30;";


*/


//char base_query[600] = "select st_asbinary(sway) from planet_osm_roads where sway && st_envelope(st_geomfromtext('linestring(%f %f,%f %f)', 4326));";

//char base_query[600] = "select st_asbinary(sway) from planet_osm_roads where sway && st_envelope(st_geomfromtext('linestring(%f %f,%f %f)', 4326));";

//char base_query[600] = "select st_asbinary(way) from planet_osm_line where way && st_envelope(st_geomfromtext('linestring(%f %f,%f %f)', 4326));";

//char base_query[600] = "select st_asbinary(lightsway) from planet_osm_line where lightsway && st_envelope(st_geomfromtext('linestring(%f %f,%f %f)', 900913)) and boundary is null;";

char base_query[600] = "select \
        st_asbinary(ST_Intersection(st_envelope(st_geomfromtext('linestring(%f %f,%f %f)', 900913)), lightsway)) \
        from planet_osm_line where lightsway && \
        st_envelope(st_geomfromtext('linestring(%f %f,%f %f)', 900913)) \
        and boundary is null and motorcar is null and route is null \
        limit 3000;";

//char base_query[600] = "select st_asbinary(way) from planet_osm_polygon where way && st_envelope(st_geomfromtext('linestring(%f %f,%f %f)', 900913)) and boundary is null;";

typedef struct {
  double x1;
  double y1;
  double x2;
  double y2;
  int query_code; // maybe zoom level
} bbox_t;

typedef struct {
  int z;
  int x;
  int y;
} tms_t;

typedef struct {
  int fd;
  char* buf;
  char* query;
  bbox_t bbox;
  tms_t tile;
  int pcount;
  int dbfd;
  PGconn* conn;
  picoev_loop* loop;
} client_t;


void fmt_res_bin (client_t* client) {
  PGresult *res = PQgetResult(client->conn);
  int s, r, c, num_rows, num_cols;
  num_rows = PQntuples(res);
  num_cols = PQnfields(res);
  int chunk_cnt = 0;
  if (num_rows > 0) {
    printf("num_rows %d\n", num_rows);
  }
  char* chunk_val = (char*)malloc(23);
  if (num_rows <= 0) {
    s = write(client->fd, "0\r\n\r\n", 5);
    PQclear(res);
    picoev_del(client->loop, client->fd);
    close(client->fd);
    free(client);
    free(chunk_val);
    return;
  }
  for (r = 0; r < num_rows; r++) {
    chunk_cnt += PQgetlength(res, r, 0); 
  }
  sprintf(chunk_val, "%x\r\n", chunk_cnt); 
  printf("reported length: %s", chunk_val);
  s = write(client->fd, chunk_val, strlen(chunk_val));
  for (r = 0; r < num_rows; r++) {
    s = write(client->fd, PQgetvalue(res, r, 0), PQgetlength(res, r, 0));
  }
  s = write(client->fd, "\r\n0\r\n\r\n", 7);
  PQclear(res);
  picoev_del(client->loop, client->fd);
  close(client->fd);
  //picoev_del(client->loop, client->dbfd);
  free(client);
  free(chunk_val);
  return;
}

uint8_t scale (double coord, int zoom, int tile) {
  return (uint8_t)(
  //return (uint8_t)(int)(
      //(coord + 20037509) * (1 << zoom) / 156542.0 - (tile*256));
      (coord + 20037508.342789) * (1 << zoom) / 156543.033928041 - (tile*256));
}

void pump (client_t* client, uint8_t* data, int len) {
  printf("hit pump\n");
  int w, i;
  int shortlen = 0;
  char *rvstr = (char*)malloc(23);
  sprintf(rvstr, "%x\r\n", len*2);
  printf("1/1 header %s\n", rvstr);
  printf("pump length %d\n", len);
  w = write(client->fd, rvstr, strlen(rvstr));
  w = write(client->fd, data, len*2);
  free(rvstr);
}

void short_stream (client_t* client, double* coordbuf, uint8_t* strm, int pt_count, int idx, int ngeoms, uint32_t geom_t) {
  printf("hit shortstream\n");
  printf("pt_count %d\n", pt_count);
  int i;
  uint32_t tval;
  uint32_t* tbuf = &tval;
  *tbuf = geom_t;
  printf("forematter 1: %d\n", *tbuf);
  memcpy(&strm[(idx*2) + 8*ngeoms], tbuf, 4);
  printf("from buf: %d\n", *(uint32_t*)(&strm[(idx*2) + 8*ngeoms]));
  *tbuf = (uint32_t)pt_count;
  printf("forematter 2: %d\n", *tbuf);
  memcpy(&strm[(idx*2) + 8*ngeoms + 4], tbuf, 4);
  printf("from buf: %d\n", *(uint32_t*)(&strm[(idx*2) + 4 + 8*ngeoms]));
  for (i = 0; i < (2 * (pt_count)); i += 2) {
    strm[i + (idx*2) + 8 + 8*ngeoms] = scale(coordbuf[i], client->tile.z, client->tile.x);
    printf("scaled pt %d: %d for tile %d\n", i, strm[i + (idx*2) + 8 + 8*ngeoms], client->tile.x);
    strm[i + (idx*2) + 9 + 8*ngeoms] = scale(coordbuf[i+1], client->tile.z, client->tile.y);
    printf("scaled pt %d: %d for tile %d\n", i+1, strm[i + (idx*2) + 9 + 8*ngeoms], client->tile.y);
  }
  return;
}


void fmt_res_2shrt (client_t* client) {
  PGresult *res = PQgetResult(client->conn);
  uint8_t *strm;
  char *pqres;
  int s, r, c, num_rows, num_cols;
  num_rows = PQntuples(res);
  num_cols = PQnfields(res);
  int chunk_cnt = 0;
  char* chunk_val = (char*)malloc(23);
  if (num_rows <= 0) {
    s = write(client->fd, "0\r\n\r\n", 5);
    PQclear(res);
    picoev_del(client->loop, client->fd);
    close(client->fd);
    free(client);
    free(chunk_val);
    return;
  }
  for (r = 0; r < num_rows; r++) {
    chunk_cnt += PQgetlength(res, r, 0);
  }
  strm = (uint8_t*)malloc(chunk_cnt);
  int pt_count = 0;
  int geom_pos = 1; // skip first endianness flag
  int reslen;
  int idx = 1;
  int pts = 0;
  int ngeoms = 0;
  uint32_t wkb_type;
  uint32_t linear_rings;
  double* coordv;
  int total_linestrings = 0;
  for (r = 0; r < num_rows; r++) {
    geom_pos = 1;
    pqres = (char*)PQgetvalue(res, r, 0);
    reslen = PQgetlength(res, r, 0);
    while (geom_pos < reslen) {
      wkb_type = *(uint32_t*)(&(pqres[geom_pos])); 
      printf("geom type: %d\n", wkb_type);

      if (wkb_type == 1) {
        idx = 17;
        geom_pos += idx;
        pts = 1;
        pt_count += pts;
        //short_stream(client, (double*)(&pqres[geom_pos]), strm, pts, 0, 0, wkb_type);
        ngeoms++;
      }

      else if (wkb_type == 2) {
        total_linestrings++;
        pts = (int)(*(uint32_t*)(&pqres[geom_pos+4]));
        printf("correct pts %d\n", pts);
        coordv = (double*)(&pqres[geom_pos+8]);
        geom_pos += (int)((*(uint32_t*)(&pqres[geom_pos + 4])) * 16 + 9);
        short_stream(client, coordv, strm, pts, pt_count, ngeoms, wkb_type);
        pt_count += pts;
        ngeoms++;
      }

      else if (wkb_type == 3) {
        ngeoms++;
        linear_rings = *(uint32_t*)(&pqres[geom_pos + 4]);
        //printf("linear rings: %d\n", linear_rings);
        geom_pos += 8;
        while (linear_rings) {
          pts = (int)(*(uint32_t*)(&pqres[geom_pos]));
          coordv = (double*)(&pqres[geom_pos+4]);
          //short_stream(client, coordv, strm, pts, pt_count, 0, wkb_type);
          pt_count += pts;
          printf("point count %d\n", pt_count);
          geom_pos += (int)((*(uint32_t*)(&pqres[geom_pos])) * 16 + 4);
          linear_rings--;
        }
        geom_pos += 1;
        //break;
      }
      else {
        printf("\n\n\n\nhit collection \n\n\n\n\n");
        geom_pos += 9;
      }
    }
    printf("total linestring: %d\n", total_linestrings);
  }
  //printf("point count: %d\n", pt_count);

  /*
  for (r = 0; r < num_rows; r++) {
    chunk_cnt += PQgetlength(res, r, 0); 
  }
  sprintf(chunk_val, "%x\r\n", chunk_cnt); 
  printf("reported length: %s", chunk_val);
  s = write(client->fd, chunk_val, strlen(chunk_val));
  for (r = 0; r < num_rows; r++) {
    s = write(client->fd, PQgetvalue(res, r, 0), PQgetlength(res, r, 0));
  }
  */

  char *rvstr = (char*)malloc(23);
  sprintf(rvstr, "%x\r\n", pt_count*2 + ngeoms*8);
  s = write(client->fd, rvstr, strlen(rvstr));
  s = write(client->fd, strm, pt_count*2 + ngeoms*8);

  printf("served tile: %d, %d, %d\n", client->tile.z, client->tile.x, client->tile.y);
  //
  //pump(client, strm, pt_count*2);
  //s = write(client->fd, PQgetvalue(res, r, 0), PQgetlength(res, r, 0));

  s = write(client->fd, "\r\n0\r\n\r\n", 7);
  //printf("about to clear\n");
  PQclear(res);
  //printf("cleared \n");
  //PQfinish(client->conn);
  close(client->fd);
  //printf("close client fd \n");
  picoev_del(client->loop, client->fd);
  //printf("cleared picoev client \n");
  free(client);
  //printf("freed client \n");
  free(chunk_val);
  //printf("cleared chunk val \n");
  free(strm);
  //return;
}

void tms2bbox (client_t* cli) {
  cli->bbox.x1 = (cli->tile.x * 40075016.6856 / (1 << cli->tile.z) - 20037508.34278);
  cli->bbox.x2 = ((cli->tile.x + 1) * 40075016.6856 / (1 << cli->tile.z) - 20037508.34278);
  cli->bbox.y1 = ((cli->tile.y - 1) * 40075016.6856 / (1 << cli->tile.z) - 20037508.34278);
  cli->bbox.y2 = ((cli->tile.y) * 40075016.6856 / (1 << cli->tile.z) - 20037508.34278);
  return;
}



void send_conn (client_t* client) {
  int rv;
  char bbox_query[500];
  PGconn *conn;
  PostgresPollingStatusType status;
  conn = PQconnectStart(connection_string);
  if (PQstatus(conn) == CONNECTION_BAD) {
  client->conn = conn;
    return;
  } 
  else {
    do {
      status = PQconnectPoll(conn);
    } while (status != PGRES_POLLING_FAILED &&
             status != PGRES_POLLING_OK);
    sprintf(bbox_query, base_query, 
        client->bbox.x1,
        client->bbox.y1,
        client->bbox.x2,
        client->bbox.y2,
        client->bbox.x1,
        client->bbox.y1,
        client->bbox.x2,
        client->bbox.y2);
    //printf("%s\n", bbox_query);
  rv = PQsendQueryParams(conn, 
                      bbox_query,
                      0,
                      NULL,
                      NULL,
                      NULL,
                      NULL,
                      1);
  }
  client->conn = conn;
  return;
}

/*
 * int PQsendQuery(PGconn *conn, const char *command);
 * for geojson etc: http://www.postgresql.org/docs/9.1/static/libpq-async.html
 *
int PQsendQueryParams(PGconn *conn,
                      const char *command,
                      int nParams,
                      const Oid *paramTypes,
                      const char * const *paramValues,
                      const int *paramLengths,
                      const int *paramFormats,
                      int resultFormat);
*/

void db_bin_cb (picoev_loop* loop, int fd, int revents, void* cb_arg)
{
  client_t* client = (client_t*)cb_arg;
  fmt_res_2shrt(client);
  return;
}


static void setup_sock(int fd)
{
  int on = 1, r;

  r = setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &on, sizeof(on));
  assert(r == 0);
  r = fcntl(fd, F_SETFL, O_NONBLOCK);
  assert(r == 0);
}

/* you would think
void parse_bbox_qs (char* qs, client_t* client)
*/

void read_cb(picoev_loop* loop, int fd, int revents, void* cb_arg)
{
  char buf[16384];
  const char* method, * path;
  size_t method_len, path_len, num_headers;
  int minor_version, r;
  struct phr_header headers[128];

  client_t* client = (client_t*)malloc(sizeof(client_t));

  /* read request */
  assert((revents & PICOEV_READ) != 0);
  r = read(fd, buf, sizeof(buf));
  if (r == 0) {
    goto CLOSE;
  } else if (r == -1) {
    if (errno == EINTR || errno == EAGAIN) {
      return;
    }
    goto CLOSE;
  }
  /* parse request, should arrive in one packet :-p */
  num_headers = sizeof(headers) / sizeof(headers[0]);
  r = phr_parse_request(buf, r, &method, &method_len, &path, &path_len,
			&minor_version, headers, &num_headers, 0);
  assert(r > 0);


#define RES_HEAD "HTTP/1.1 200 OK\r\n" \
    "Content-Type: application/octet-stream\r\n" \
    "Transfer-Encoding: chunked\r\n" \
    "\r\n" 

  r = write(fd, RES_HEAD, sizeof(RES_HEAD) - 1);
#undef RES_HEAD
  char* qval = NULL;
  const char* delims = "&/?,=. ";
  char* qs_tms_tok = "tms";
  char* qs_end_tok = "HTTP/1";
  int h_more = 1; 
  int bboxcmp = 1;
  int tms_param_cnt = 1;
  int fav = -1;
  int spun_out = 0;
  const char* favicon = "favicon.ico";
  qval = strtok((char*)path, delims);
  long int xx, yy, zz;
  zz = xx = yy = -1;
  while (qval != NULL) {
    spun_out++;
    if (!(fav = memcmp(qval, favicon, (int)sizeof(favicon)))) {
      goto CLOSE;
    }
    bboxcmp = memcmp(qval, qs_tms_tok, 3);
    //printf("%d bboxcmp\n", bboxcmp);
    h_more = memcmp(qval, qs_end_tok, 6);
    if (!bboxcmp) {
      //printf("%s\n", qval);
      qval = strtok(NULL, delims);
      while ((qval != NULL) && (tms_param_cnt <=3)) {
        switch (tms_param_cnt) {
          case 1: zz = strtol(qval, NULL, 10); break;
          case 2: xx = strtol(qval, NULL, 10); break;
          case 3: yy = strtol(qval, NULL, 10); break;
        }
        tms_param_cnt++;
        qval = strtok(NULL, delims);
      }

      break;
    }

    else {
      //printf("%s\n", qval);
      if (!h_more) {
        break;
      } else {
        qval = strtok(NULL, delims);
      }
    }
    if (spun_out > 45) {
      goto CLOSE;
    } 
    //else {
    //  qval = strtok(NULL, delims);
    //}
  }
  client->tile.z = zz;
  client->tile.x = xx;
  client->tile.y = (1 << zz) - yy;
  tms2bbox(client);

  send_conn(client);
  client->fd = fd;
  client->loop = loop;
  printf("client fd: %d\n", fd);
  //printf("client bbox: %f, %f, %f, %f\n", client->bbox.x1, client->bbox.y1, client->bbox.x2, client->bbox.y2);
  printf("client tms: %d, %d, %d\n", client->tile.z, client->tile.x, client->tile.y);
  int dbfd = PQsocket(client->conn);
  //printf("\n\n\n\ndatabase fd: %d\n\n\n\n\n", dbfd);
  picoev_add(loop, dbfd, PICOEV_READ, 0, db_bin_cb, (void*)client);
  return;

 CLOSE:
  printf("hit close\n\n\n\n");
  picoev_del(loop, fd);
  close(fd);
  free(client);
  
}

void* start_thread(void* _unused)
{
  int fd, r, r2;
  char buf[4096];

  while (1) {
    fd = accept(listen_sock, NULL, NULL);
    if (fd == -1)
      continue;
    r2 = 1;
    r = setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &r2, sizeof(r2));
    assert(r == 0);

    while (1) {
      r = read(fd, buf, sizeof(buf));
      if (r == 0 || (r == -1 && errno != EINTR)) {
	break;
      }
      r2 = write(fd, buf, r);
      assert(r == r2);
    }
    close(fd);
  }

  return NULL;
}

static void accept_cb(picoev_loop* loop, int fd, int revents, void* cb_arg)
{
  int newfd;
  assert((revents & PICOEV_READ) != 0);
  if ((newfd = accept(fd, NULL, NULL)) != -1) {
    setup_sock(newfd);
    picoev_add(loop, newfd, PICOEV_READ, 0, read_cb, NULL);
  }
}

int main(int argc, char** argv)
{
  int ch, r, flag;
  struct sockaddr_in listen_addr;
  picoev_loop* loop;

  while ((ch = getopt(argc, argv, "p:")) != -1) {
    switch (ch) {
    case 'p':
      assert(sscanf(optarg, "%hu", &port) == 1);
      break;
    default:
      exit(1);
    }
  }

  listen_sock = socket(AF_INET, SOCK_STREAM, 0);
  assert(listen_sock != -1);
  flag = 1;
  r = setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));
  assert(r == 0);
  listen_addr.sin_family = AF_INET;
  listen_addr.sin_port = htons(PORT);
  listen_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  r = bind(listen_sock, (struct sockaddr*)&listen_addr, sizeof(listen_addr));
  assert(r == 0);
  setup_sock(listen_sock);
  r = listen(listen_sock, SOMAXCONN);
  assert(r == 0);

  picoev_init(1048576 + 10);
  loop = picoev_create_loop(60);
  picoev_add(loop, listen_sock, PICOEV_READ, 0, accept_cb, NULL);
  while (1) {
    picoev_loop_once(loop, 10);
  }

  return 0;
}
