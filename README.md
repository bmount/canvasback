`undercanvass` is inspired by the work of [Cartagen](http://cartagen.org) and 
[Eric Fischer](http://www.flickr.com/photos/walkingsf/). The goal is to create
a fast json endpoint for simple 2d mapping in html5 canvas elements, minimal 
enough to enable scrolling and zooming without pre-rendering images. Most of 
the work will be on the data and javascript sides but I want to test the 
feasibility of this implementation of some of the basics. 

There is a PostgreSQL/PostGIS version (recommended), intended for use with Open 
Street Map data, and an accompanying js example that includes the transforms 
necessary to work with its default mercator. There is also a MySQL version that 
works directly on the WKB layer, and expects Lon and Lat (it's kind of a demo.)


![undercanvass
example](undercanvass/raw/master/rendered_map_examples/sf_marin_complex.png)

![undercanvas example](undercanvass/raw/master/rendered_map_examples/Bay_Bridge_simplified.png)
![undercanvass
example](undercanvass/raw/master/rendered_map_examples/Nor_Cal_railways.png)
![undercanvass
example](undercanvass/raw/master/rendered_map_examples/SF_all_roads.png)

A good source of bulk partial Open Street Map data is [Cloudmade](http://downloads.cloudmade.com/).
`osm2pgsql` with default flags (mercator projection, etc.) is what I'm calling
'default', and the js example provides a mercator transform. `simplify_osm.py` 
will add a column of simplified OSM `way` geometries.

Dependencies: Postgres+PostGIS or MySQL with development headers, gcc and the 
[fastcgi](http://www.fastcgi.com/) library, something like `nginx` and 
`spawn-fcgi`. `start_server.sh` will build and try to spawn with `spawn-fcgi`. 
`db.conf.c` is a series of queries that you can futz with, the odd-indexed
queries work with the default `osm2pgsql` output, the evens require the python
script or manually adding simplified geometry columns.
