#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <libpq-fe.h>
#include <fcgi_stdio.h>
#include "db.conf.c"

#define lonf 0.0113222194
#define latf 0.0090215040

//char *base_queries[] = {
//    "select place, st_asgeojson(way) from planet_osm_roads;",
//
//};

typedef struct {
    unsigned char       *gptr;
    double              minlon;
    double              minlat;
    double              maxlon;
    double              maxlat;
    double              cvx;
    double              cvy;
    int                 cnv_size;
    int                 num_pts;
    int                 num_sub_geoms;
    int                 escape_bad_trailing_pt; 
    /* fix malformed polygons, if you have 
    perfect data, no need for escape_bad_trailing_pt,
    1:1000 such errors in TIGERLINE polygons */
} gm_t; // base canvas_fish geometry type

typedef struct {
    double xmin;
    double ymin;
    double xmax;
    double ymax;
    int query_ordinal;
} mbr_t;

typedef struct {
    double  lon;
    double  lat;
    double  dist;
    int     query_ordinal;
} qs_t; // query string type, px = canvas size in px
        // dist = radius of circle such that
        // the canvas is the min bounding rectangle,
        // in approx km, see note on `lonf` above

static void exit_clean (PGconn *conn) {
    PQfinish(conn);
    exit(1);
}

void json_out (PGresult *res, int *num_fields) {
    int i, k;
    //int num_fields = PQnfields(res);
    for (i = 0; i < PQntuples(res); i++) {
        printf("{\"type\":\"Feature\", \"geometry\":");
        if (!(*PQgetvalue(res, i, *num_fields-1))) {
            printf("\"\"");
        } else {
            printf(PQgetvalue(res, i, *num_fields-1));
        }
        printf(", \"properties\":{");
        int properties_exist = 0;
        for (k = 1; k < (*num_fields - 3); k++) {
            //printf("%s", PQfname(res, k));
            if (*PQgetvalue(res, i, k)) {
                printf("\"%s\":\"%s\",", PQfname(res, k), PQgetvalue(res, i, k));
                properties_exist++;
            }
            //printf("%-15s", PQgetvalue(res, i, k));
        }
        if (!properties_exist) {
            printf("false");
        }
        printf(" \"osm_id\":%d", *PQgetvalue(res, 0, 0));
        printf("}},");
    }
    return;
}

void next_rv (PGresult *res, int *num_fields) {
    if (PQresultStatus(res) == PGRES_TUPLES_OK) {
        json_out(res, num_fields);
        PQclear(res);
        return;
    } else {
        PQclear(res);
    }
    return;
}

int geopgq (qs_t *r) {
    PGconn *conn;
    PGresult *res;
    int i, k, initq;
    int *nfields;
    double cx = lonf;
    double cy = latf;
    double convlon = r->dist * cx;
    double convlat = r->dist * cy;
    double minlon = r->lon - convlon;
    double minlat = r->lat - convlat;
    double maxlon = r->lon + convlon;
    double maxlat = r->lat + convlat;
    conn = PQconnectdb(connection_string);

    if (PQstatus(conn) != CONNECTION_OK) {
        fprintf(stderr, "connection failed, error msg: %s\n", PQerrorMessage(conn));
        exit_clean(conn);
    }

    res = PQexec(conn, "begin"); //bgq);

    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        fprintf(stderr, "transaction failed on begin\n");
        exit_clean(conn);
    }

    PQclear(res);

    char geo_query[1000];
    char base_geo_query[500];
    char *cursor_declaration = "declare geo_cur no scroll cursor for %s";
    
    sprintf(geo_query, query_with_cursor[r->query_ordinal], minlon, minlat, maxlon, maxlat, 10000); 
    //sprintf(geo_query, cursor_declaration, "select place, st_asgeojson(way) from planet_osm_roads;");
    res = PQexec(conn, geo_query);

    PQclear(res);

    res = PQexec(conn, "fetch next in geo_cur");
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        exit_clean(conn);
    } else {
        int number_of_fields = PQnfields(res);
        nfields = &number_of_fields;
        next_rv(res, nfields);
    }
    
    res = PQexec(conn, "fetch next in geo_cur");

    while (PQntuples(res)) {
        next_rv(res, nfields);
        res = PQexec(conn, "fetch next in geo_cur");
        *nfields = PQnfields(res);
    }        

    PQclear(res);

    res = PQexec(conn, "close geo_cur;"); //bgq);
    PQclear(res);

    res = PQexec(conn, "end"); //bgq);

    PQclear(res);

    PQfinish(conn);

    return 0;
}

int cf_error (int reason) {
    switch (reason) {
        case 1: printf("{\"error\":\"bad_query_string\"}");
                break;
        default: printf("{\"error\":\"unspecified\"}");
        }
    return 0;
}

int cf_fcgi_loop () {
    char *envinfo = NULL;
    int no_qs = 0;
    char *delims = "=&";
    char *qval = NULL;
    int qstr_order = 0;
    qs_t qs;
    qs_t *qs_ptr;
    while (FCGI_Accept() >= 0) {
        qval = NULL;
        qstr_order = 0;
        printf("Content-Type: application/json\n\n");
        envinfo = getenv("QUERY_STRING");
        if ((envinfo == NULL) || strlen(envinfo) == 0) {
            cf_error(1);
            no_qs++;
        }
        qval = strtok(envinfo, delims);
        while (qval != NULL) {
            qstr_order++;
            switch (qstr_order) {
                case 2: qs.lon = atof(qval); break;
                case 4: qs.lat = atof(qval); break;
                case 6: qs.dist = atof(qval); break;
                case 8: qs.query_ordinal = atoi(qval); break;
                default: ;
            }
            qval = strtok(NULL, delims);
        }
        if ((qstr_order != 8) || (qs.query_ordinal >= max_query_index) || (qs.query_ordinal < 0)) {
            if (!no_qs) {
                cf_error(1);
            } else {
                ;
            }
        } else {
            //char *query1 = "some dummy query";
            /* any number of additional queries */
            //canvas_fish(query1, qs.lon, qs.lat, qs.dist, qs.px);
            printf("{\"type\":\"FeatureCollection\", \"features\": [");
            geopgq(&qs);
            printf("{\"type\":\"feature\", \"geometry\":{}, \"properties\":{\"name\":\"donezo\"}}]}");
            /*
            printf("%s\n, %f, %f, %f, %d\n", query1, qs.lon, qs.lat, qs.dist, qs.px);
            */
         }
    }
    return 0;
}

int main () {
    cf_fcgi_loop();
    return -1;
}
