
#define DB_NAME "" /* add db info etc */
#define DB_HOST ""
#define DB_USER ""
#define DB_PASSWORD ""

char *base_geo_query = "select aswkb(geom) from tract00 where MBRContains(GeomFromText('LINESTRING(%f %f, %f %f)'), geom);";
