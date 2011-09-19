import psycopg2
import argparse
import sys
import getpass

osm_default_tables = ["planet_osm_line", "planet_osm_roads", "planet_osm_polygon"]

parser = argparse.ArgumentParser(add_help=True)
parser.add_argument('-d', '--database', action='store', dest='database',\
        help = 'requires `database_name`')
parser.add_argument('-s', '--simplify', action='store',\
        default=1000, dest='douglas_peucker_factor', help='optional factor for \
        Douglas Peucker polygon simplification')
parser.add_argument('-u', '--user', action='store', dest='user', help=\
        'alternate user name (if not logged in as)', default=getpass.getuser())
parser.add_argument('-p', '--password', action='store', dest='password',\
        default='', help='password if not using ident only')

bear = parser.parse_args()

try:
    conn = psycopg2.connect(database=bear.database,\
            user=bear.user, password=bear.password)
except:
    print("must pass database name with flag -d, or try --help")
    sys.exit()

cur = conn.cursor()

# simplified way name is 'sway'

simplified_index_suffix = 'simp_idx'

for table in osm_default_tables:
    cur.execute("alter table {0} add column sway geometry".format(table))
    cur.connection.commit()
    cur.execute("update {0} set sway = st_simplify(way, {1})".format(table,\
        bear.douglas_peucker_factor))
    cur.connection.commit()
    cur.execute("create index {0}_{1} on {0} using gist (sway)".format(table,\
        simplified_index_suffix))
    cur.connection.commit()

cur.connection.close()
