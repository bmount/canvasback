/*  These queries require a cursor declaration,
    they will be closed when the query is complete.
*/

//edit this as you add or remove queries
int max_query_index = 7;

const char *connection_string = "dbname=cal_osm_full host=/tmp";

static char *query_with_cursor[] = {
   /* 0 simp roads */ 
                            "declare geo_cur no scroll \
                            cursor for \
                            select *, st_asgeojson(sway) from planet_osm_roads \
                            where sway && \
                            st_envelope(\
                                    st_transform(\
                                        st_geomfromtext('linestring(%f %f, %f %f)', 4326),\
                                    900913)\
                                )\
                            limit %d;",
   /* 1 unsimp roads */
                            "declare geo_cur no scroll \
                            cursor for \
                            select *, st_asgeojson(way) from planet_osm_roads \
                            where way && \
                            st_envelope(\
                                    st_transform(\
                                        st_geomfromtext('linestring(%f %f, %f %f)', 4326),\
                                    900913)\
                                    ) limit %d;",
   /* 2 simp polygons */
                            "declare geo_cur no scroll \
                            cursor for \
                            select *, st_asgeojson(sway) from planet_osm_polygon \
                            where sway && \
                            st_envelope(\
                                    st_transform(\
                                        st_geomfromtext('linestring(%f %f, %f %f)', 4326),\
                                    900913)\
                                    ) limit %d;",
   /* 3 unsimp polygons */  
                            "declare geo_cur no scroll \
                            cursor for \
                            select *, st_asgeojson(way) from planet_osm_polygon \
                            where way && \
                            st_envelope(\
                                    st_transform(\
                                        st_geomfromtext('linestring(%f %f, %f %f)', 4326),\
                                    900913)\
                                    ) limit %d;",
   /* 4 simp line */ 
                            "declare geo_cur no scroll \
                            cursor for \
                            select *, st_asgeojson(sway) from planet_osm_line \
                            where sway && \
                            st_envelope(\
                                    st_transform(\
                                        st_geomfromtext('linestring(%f %f, %f %f)', 4326),\
                                    900913)\
                                    ) limit %d;",
    /* 5 unsimp line */
                            "declare geo_cur no scroll \
                            cursor for \
                            select *, st_asgeojson(way) from planet_osm_line \
                            where way && \
                            st_envelope(\
                                    st_transform(\
                                        st_geomfromtext('linestring(%f %f, %f %f)', 4326),\
                                    900913)\
                                    ) limit %d;",
   /* 6 simp railway */ 
                            "declare geo_cur no scroll \
                            cursor for \
                            select *, st_asgeojson(sway) from planet_osm_roads \
                            where railway = 'rail' and sway && \
                            st_envelope(\
                                    st_transform(\
                                        st_geomfromtext('linestring(%f %f, %f %f)', 4326),\
                                    900913)\
                                    ) limit %d;",
    /* 7 unsimp railway */
                            "declare geo_cur no scroll \
                            cursor for \
                            select *, st_asgeojson(way) from planet_osm_roads \
                            where railway = 'rail' and way && \
                            st_envelope(\
                                    st_transform(\
                                        st_geomfromtext('linestring(%f %f, %f %f)', 4326),\
                                    900913)\
                                    ) limit %d;"
                            };
