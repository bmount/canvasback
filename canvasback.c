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
#include "picoev/picoev.h"
#include "picohttpparser/picohttpparser.h"
#include "db.conf.c"


#define PORT 7987
#define MAX_FDS 100000
#define TIMEOUT_SECS 10

unsigned short port = PORT;
int listen_sock;

/* 
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

char base_query[600] = "select st_asbinary(lightsway) from planet_osm_line where lightsway && st_envelope(st_geomfromtext('linestring(%f %f,%f %f)', 4326)) and boundary is null;";

typedef struct {
  double x1;
  double y1;
  double x2;
  double y2;
  int query_code; // maybe zoom level
} bbox_t;

typedef struct {
  int fd;
  char* buf;
  char* query;
  bbox_t bbox;
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
    printf("num_rows %x\n", num_rows);
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
  free(client);
  free(chunk_val);
  return;
}


void fmt_res_geojson (client_t* client) 
{
  PGresult *res = PQgetResult(client->conn);
  int s, r, c, num_rows, num_cols,
      last_row, last_col;
  num_rows = PQntuples(res);
  num_cols = PQnfields(res);
  last_row = num_rows-1;
  last_col = num_cols-1;
  int chunk_cnt;
  char* chunk_val = (char*)malloc(23);
  if (num_rows <= 0) {
    s = write(client->fd, "f5\r\n{\"error\":\"bad_query\"}", 25);
    PQclear(res);
    close(client->fd);
    picoev_del(client->loop, client->fd);
    // Figure out hanging behavior
    //PQfinish(client->conn);
    free(client);
    free(chunk_val);
    return;
  }
  if (num_cols == 1) {
    chunk_cnt = 41 + 12 + 2 + 12 + 3;
    for (r = 0; r < num_rows; r++) {
      chunk_cnt += PQgetlength(res, r, 0);
    }
    sprintf(chunk_val, "%x\r\n", chunk_cnt);
    s = write(client->fd, chunk_val, strlen(chunk_val));
    s = write(client->fd, 
        "{\"type\":\"FeatureCollection\",\"features\":[", 41);
    for (r = 0; r < (num_rows-1); r++) {
      s = write(client->fd, "{\"geometry\":", 12);
      s = write(client->fd,      
          PQgetvalue(res, r, 0),
          PQgetlength(res, r, 0));
      s = write(client->fd, "},", 2);
    }
    s = write(client->fd,
        "{\"geometry\":", 12);
    s = write(client->fd,
          PQgetvalue(res, r, 0),
          PQgetlength(res, r, 0));
    s = write(client->fd,
          "}]}",
          3);
    PQclear(res);
    close(client->fd);
    picoev_del(client->loop, client->fd);
    //PQfinish(client->conn);
    free(client);
    free(chunk_val);
    return;
  }

  else {
    chunk_cnt = 41 + 12 + 1 + 14 + 1 + 
                3 + 2 + 1 + 3 + 4 + 12
                + 1 + 15 + 3 + 3 + 3 + 5;
    for (r = 0; r < num_rows; r++) {
      for (c = 0; c < num_cols; c++) {
        chunk_cnt += PQgetlength(res, r, c);
      }
    }

    sprintf(chunk_val, "%x\r\n", chunk_cnt);
    s = write(client->fd, chunk_val, strlen(chunk_val));
    s = write(client->fd, 
        "{\"type\":\"FeatureCollection\",\"features\":[", 
        41);

    for (r = 0; r < last_row; r++) {
      c = 0;
      s = write(client->fd,
          "{\"geometry\":", 12);
      s = write(client->fd,
          PQgetvalue(res, r, c),
          PQgetlength(res, r, c));
      s = write(client->fd,
          ",", 1);
      s = write(client->fd,
          "\"properties\":{", 14);
      for (c = 1; c < last_col; c++) {
        s = write(client->fd, "\"", 1);
        s = write(client->fd,
          PQfname(res, c), strlen(PQfname(res, c)));
        s = write(client->fd, "\":\"", 3);
        s = write(client->fd,
          PQgetvalue(res, r, c), PQgetlength(res, r, c));
        s = write(client->fd, "\",", 2);
      }

      s = write(client->fd, "\"", 1);
      s = write(client->fd,
          PQfname(res, c), strlen(PQfname(res, c)));
      s = write(client->fd, "\":\"", 3);
      s = write(client->fd,
          PQgetvalue(res, r, last_col), PQgetlength(res, r, last_col));
      s = write(client->fd, "\"}},", 4);
    }
    s = write(client->fd,
        "{\"geometry\":",
        12);
    s = write(client->fd,
        PQgetvalue(res, last_row, 0),
        PQgetlength(res, last_row, 0));
    s = write(client->fd, ",", 1);
    s = write(client->fd, "\"properties\":{\"", 15);
    for (c = 1; c < last_col; c++) {
      s = write(client->fd, PQfname(res, c), strlen(PQfname(res, c)));
      s = write(client->fd, "\":\"", 3);
      s = write(client->fd,
          PQgetvalue(res, last_row, c),
          PQgetlength(res, last_row, c));
      s = write(client->fd, "\",\"", 3);
    }
    
    s = write(client->fd, 
        PQfname(res, last_col), 
        strlen(PQfname(res, last_col)));
    s = write(client->fd, "\":\"", 3);
    s = write(client->fd,
        PQgetvalue(res, last_row, last_col),
        PQgetlength(res, last_row, last_col));
    s = write(client->fd,
        "\"}}]}", 5);
    PQclear(res);
    picoev_del(client->loop, client->fd);
    close(client->fd);
    //PQfinish(client->conn);
    free(client);
    free(chunk_val);
    return;
  }
}


void send_conn (client_t* client) 
{
  char bbox_query[500];
  PGconn *conn;
  PostgresPollingStatusType status;
  conn = PQconnectStart(connection_string);
  if (PQstatus(conn) == CONNECTION_BAD) {
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
        client->bbox.y2);
    printf("%s\n", bbox_query);
    PQsendQueryParams(conn, 
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
  fmt_res_bin(client);
  return;
}

void db_geojson_cb (picoev_loop* loop, int fd, int revents, void* cb_arg)
{
  client_t* client = (client_t*)cb_arg;
  fmt_res_geojson(client);
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
  const char* delims = "&/?,= ";
  char* qs_bbox_tok = "bbox=";
  char* qs_end_tok = "HTTP/1";
  int h_more, bboxcmp;
  int bbox_param_cnt = 1;
  int fav = -1;
  int spun_out = 0;
  const char* favicon = "favicon.ico";
  qval = strtok((char*)path, delims);
  while (qval != NULL) {
    spun_out++;
    if (!(fav = memcmp(qval, favicon, (int)sizeof(favicon)))) {
      goto CLOSE;
    }
    bboxcmp = memcmp(qval, qs_bbox_tok, 4);
    h_more = memcmp(qval, qs_end_tok, 3);
    if (!bboxcmp) {
      qval = strtok(NULL, delims);
      while ((qval != NULL) && (bbox_param_cnt <=4)) {
        switch (bbox_param_cnt) {
          case 1: client->bbox.x1 = strtod(qval, NULL); break;
          case 2: client->bbox.y1 = strtod(qval, NULL); break;
          case 3: client->bbox.x2 = strtod(qval, NULL); break;
          case 4: client->bbox.y2 = strtod(qval, NULL); break;
        }
        bbox_param_cnt++;
        qval = strtok(NULL, delims);
      }
      break;
    }
    
    else {
      if (!h_more) {
        break;
      } else {
        qval = strtok(NULL, delims);
      }
    }
    if (spun_out > 45) {
      goto CLOSE;
    } else {
      qval = strtok(NULL, delims);
    }
  }

  send_conn(client);
  client->fd = fd;
  client->loop = loop;
  printf("client fd: %d\n", fd);
  printf("client bbox: %f, %f, %f, %f\n", client->bbox.x1, client->bbox.y1, client->bbox.x2, client->bbox.y2);
  int dbfd = PQsocket(client->conn);
  picoev_add(loop, dbfd, PICOEV_READ, 0, db_bin_cb, (void*)client);
  return;

 CLOSE:
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
