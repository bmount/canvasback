/*  These queries require a cursor declaration,
    they will be closed when the query is complete.
*/

//edit this as you add or remove queries, gis is osm2pgsql default

int max_query_index = 1;

const char *connection_string = "dbname=sfosm host=/tmp";

static char *query_with_cursor[] = {
   /* 0 tabblock for big viz */ 
                            "declare geo_cur no scroll \
                            cursor for \
                            select *, st_asgeojson(the_geom) from tl_2010_06_tabblock10 \
                            where the_geom && \
                            st_envelope(\
                                    st_transform(\
                                        st_geomfromtext('linestring(%f %f, %f %f)', 4326),\
                                    4326)\
                                )\
                            limit %d;"};
