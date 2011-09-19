/* 

Copyright (c) Brian Mount 2011

Free to use, modify, re-sell, re-gift, etc. per terms of the MIT License, 
Details at: http://www.opensource.org/licenses/mit-license.php

`lonf` is a longitude factor such that the longitude of Philz
Coffee on 24th St. in San Francisco plus 1 times lonf yields the 
longitude 1 km due East of Philz. Same with `latf`, ie latitude of 
Philz Coffee + (-2) * latf equals latitude 2 km due South of Philz. 
There is no fine-grained projection of spheroids beyond this simple 
mercator. May want to adjust if you target far north or south of
the planet.

See README for usage. See canvas_fish_wkb.c for geometry types per
the Open Geospatial specification for Well-Known Binary, described
here: http://portal.opengeospatial.org/files/?artifact_id=829

*/

/* the below query will display results from included .sql file */

#define base_geo_query "select aswkb(geom) from tract00 where MBRContains(GeomFromText('LINESTRING(%f %f, %f %f)'), geom);";

#define DB_NAME "" /* add db info etc */
#define DB_HOST ""
#define DB_USER ""
#define DB_PASSWORD ""

#define lonf 0.01132221938
#define latf 0.0090215040

#include <my_global.h>
#include <mysql.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <fcgi_stdio.h>

typedef unsigned char byte;

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
    perfect data, performance improves without escape_bad_trailing_pt,
    1:1000 such errors in TIGERLINE polygons */
} gm_t; // base canvas_fish geometry type

typedef struct {
    double  lon;
    double  lat;
    double  dist;
    int     px;
} qs_t; // query string type, px = canvas size in px
        // dist = radius of circle such that
        // the canvas is the min bounding rectangle,
        // in approx km, see note on `lonf` above

int tr_pt_geom (gm_t *gm) {
    printf("[%d, ", (int)(((*((double *)(gm->gptr)))-(gm->minlon))*(gm->cvx)));
    gm->gptr += sizeof(double);
    printf("%d] ", (gm->cnv_size - (int)(((*((double *)(gm->gptr)))-(gm->minlat))*(gm->cvy))));
    gm->gptr += sizeof(double);
    return 0;
}

int tr_lnstr_geom (gm_t *gm) {
    int i = 0;
    for (i = 0; i < (gm->num_pts-1); i++) {
        printf("[%d, ", (int)((*((double *)(gm->gptr))-(gm->minlon))*(gm->cvx)));
        gm->gptr += sizeof(double);
        printf("%d], ", 
            (gm->cnv_size - (int)((*((double *)(gm->gptr))-gm->minlat)*(gm->cvy))));
        gm->gptr += sizeof(double);
    }
    i++;
    printf("[%d, ", (int)((*((double *)(gm->gptr))-(gm->minlon))*(gm->cvx)));
    gm->gptr += sizeof(double);
    printf("%d]]",
        (gm->cnv_size - (int)((*((double *)(gm->gptr))-(gm->minlat))*(gm->cvy))));
    gm->gptr += sizeof(double);
    gm->escape_bad_trailing_pt++;
    return 0;
}

int tr_polygon_geom (gm_t *gm) {
    int i = 0;
    int num_rings = *((int *)(gm->gptr));
    gm->gptr += sizeof(int);
    while (i < num_rings && !gm->escape_bad_trailing_pt) {
        gm->num_pts = *((int *)gm->gptr);
        gm->gptr += 4;
        tr_lnstr_geom(gm);
        i++;
    }
    return 0;
}

int canvas_fish (char* cf_geo_query, double clon, double clat, double dist, int cnv_size) {
    gm_t gm_init;
    gm_t *gm = &gm_init;
    gm->cnv_size = cnv_size;
    double mlon = dist*lonf;
    double mlat = dist*latf;
    gm->minlon = clon - mlon;
    gm->maxlon = clon + mlon;
    gm->minlat = clat - mlat;
    gm->maxlat = clat + mlat;
    double dx = (gm->maxlon - gm->minlon);
    double dy = (gm->maxlat - gm->minlat);
    gm->cvx = ((double)(gm->cnv_size)) / dx;
    gm->cvy = ((double)(gm->cnv_size)) / dy;
    MYSQL *conn;
    MYSQL_RES *geoms;
    MYSQL_ROW geometry;
    conn = mysql_init(NULL);
    mysql_real_connect(conn, DB_HOST, DB_USER, DB_PASSWORD, DB_NAME, 0, NULL, 0);
    char *geo_query = cf_geo_query;
    char fmtq[500];
    sprintf(fmtq, geo_query, gm->minlon, gm->minlat, gm->maxlon, gm->maxlat);
    mysql_query(conn, fmtq);
    geoms = mysql_store_result(conn);
    int *geom_num_pts;
    int gnpts = 0;
    geom_num_pts = &gnpts;
    int *geom_type;
    int gtdr = 0;
    geom_type = &gtdr;
    unsigned long *lengths;
    int json_commas = 0;
    printf("[");
    while (geometry = mysql_fetch_row(geoms)) {
        lengths = mysql_fetch_lengths(geoms);
        byte *geom = (byte *)malloc(*lengths);
        memcpy(geom, geometry[0], *lengths);
        memcpy(geom_type, &geom[0]+1, 4);
        memcpy(geom_num_pts, &geom[0]+5, 4);
        gm->num_pts = (int)(*geom_num_pts);
        if (json_commas > 0) {
            printf(", {\"type\":");
        }
        else {
            printf("{\"type\":");
            json_commas++;
        }
        switch ((int)*geom_type) {
            case 1: printf("\"Point\", \"coordinates\":");
                    gm->gptr = &geom[0]+5;
                    tr_pt_geom(gm);
                    break;
            case 2: printf("\"LineString\", \"coordinates\":[");
                    gm->gptr = &geom[0]+9;
                    tr_lnstr_geom(gm);
                    break;
            case 3: printf("\"Polygon\", \"coordinates\":[");
                    gm->gptr = &geom[0]+5;
                    gm->escape_bad_trailing_pt = 0;
                    tr_polygon_geom(gm);
                    break;
            default: printf("\"error\":\"unsupported type or bad data\"");
            break;
        }
        printf("}");
        free(geom);
    }
    printf("]");
    mysql_free_result(geoms);
    mysql_close(conn);
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

int main () {
    while (FCGI_Accept() >= 0) {
        printf("Content-Type: application/json\n\n");
        char *envinfo = NULL;
        envinfo = getenv("QUERY_STRING");
        int no_qs = 0;
        if ((envinfo == NULL) || strlen(envinfo) == 0) {
            cf_error(1);
            no_qs++;
        }
        char *delims = "=&";
        char *qval = NULL;
        int qstr_order = 0;
        qs_t qs;
        qval = strtok(envinfo, delims);
        while (qval != NULL) {
            qstr_order++;
            switch (qstr_order) {
                case 2: qs.lon = atof(qval); break;
                case 4: qs.lat = atof(qval); break;
                case 6: qs.dist = atof(qval); break;
                case 8: qs.px = atoi(qval); break;
                default: ;
            }
            qval = strtok(NULL, delims);
        }
        if (qstr_order != 8) {
            if (!no_qs) {
                cf_error(1);
            } else {
                ;
            }
        } else {
            char *query1 = base_geo_query;
            /* any number of additional queries */
            canvas_fish(query1, qs.lon, qs.lat, qs.dist, qs.px);
        }
    }
    return 0;
}
