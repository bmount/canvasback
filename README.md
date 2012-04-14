`canvasback` is inspired by the work of [Cartagen](http://cartagen.org) and 
[Eric Fischer](http://www.flickr.com/photos/walkingsf/). The goal is to create
a fast endpoint for simple 2d mapping in html5 elements, minimal 
enough to enable scrolling and zooming without pre-rendering images. Most of 
the work will be on the data and javascript sides but I want to test the 
feasibility of this implementation of some of the basics. Namely, I want to try
to push pretty unadorned OGC binary formats (or EWKB if I can ever find a spec) into
javascript typed arrays.

To use, edit db.conf.c with your database settings, adapt one of the example
queries in `canvasback.c` as `base_query`, then:

`make`

`./canvasback`

Access the raw streaming binary goodness at: `http://localhost:7987/any/thing?bbox=-122,37,-123.3,38.8`

I put that behind an http/1.1 proxy, there's an example configuration in `wkbjs`.

(incidentally, the diff of this readme commit is likely to be hilarious)

This is intended to work well with polymaps/modestmaps bounding box substitution 
variables and vector tile map stuff.

I'm finding that I like CouchDB for a lot of the background GeoJSON stuff, so as I work on 
this I want to focus on making a little evented bridge to PostGIS, especially for streaming 
binary geometries, routing, geocoding, complex queries, all that awesome. If that sounds like fun to you 
get in touch.

***sample output***

All San Francisco Streets:

![san francisco streets](http://h.sfgeo.org/tmp/pics/osm_streets_padded_wkb.png)

Copy of VECNIK New York Bic map in San Francisco:

![vecnik style sf map](http://h.sfgeo.org/tmp/pics/comolosvizzualiteros.png)

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
`osm2pgsql` with lon-lat flag is what I use for examples.

Dependencies: Postgres+PostGIS with development headers, gcc.
