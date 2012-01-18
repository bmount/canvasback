`canvasback` is inspired by the work of [Cartagen](http://cartagen.org) and 
[Eric Fischer](http://www.flickr.com/photos/walkingsf/). The goal is to create
a fast json endpoint for simple 2d mapping in html5 elements, minimal 
enough to enable scrolling and zooming without pre-rendering images. Most of 
the work will be on the data and javascript sides but I want to test the 
feasibility of this implementation of some of the basics.

To use, edit db.conf.c with your database settings, adapt one of the example
queries in `canvasback.c` as `base_query`, then:

`make
./canvasback`

Access the GeoJSON at: `http://localhost:7987/any/thing?bbox=-122,37,-123.3,38.8`

This is intended to work well with polymaps/modestmaps bounding box substitution 
variables and vector tile map stuff.

There is also a couple of fcgi versions, one that parses mysql's wkb records and 
supports only 3-4 geometry types, and a more standard libpq postgis thing. These 
are all iterations on ideas for getting enough data out of a db quickly enough to 
make visually meaningful maps on the fly. I'm finding that I like CouchDB for a 
lot of the background vector stuff, so as I work on this I want to focus on 
making a little evented bridge to PostGIS, especially for routing, geocoding,
complex queries, all that awesome. If that sounds like fun to you get in touch.

***sample output***

Complex Bay Area road geometries on a simplified background:

![canvasback
example](canvasback/raw/master/rendered_map_examples/sf_marin_complex.png)

The area around the SF Bay Bridge showing important roads simplified:

![canvasback example](canvasback/raw/master/rendered_map_examples/Bay_Bridge_simplified.png)

Northern California's railways over a political subdivision map:

![canvasback
example](canvasback/raw/master/rendered_map_examples/Nor_Cal_railways.png)

All SF roads over a detailed polygon background:
![canvasback
example](canvasback/raw/master/rendered_map_examples/SF_all_roads.png)

Female canvasback duck with ducklings:

![canvasback example](canvasback/raw/master/rendered_map_examples/Aythya_valisineria2.jpg)

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
