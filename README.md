`canvasback` is a (we hope fast) endpoint for simple 2d mapping in html5 elements, minimal 
enough to enable scrolling and zooming without pre-rendering images. The basic
idea is to be like a geojson layer, but with binary arrays of geospatial geometries,
which are often 1/4 the size of geojson and render really quickly. Most of 
the work will be on the data and javascript sides but I want to test the 
feasibility of this implementation of some of the basics.

There is a companion project here, `wkmap` which also has node.js and python servers
that do this and are likelier to be immediately usable or adaptable. To use this server, 
edit `db.conf.c` with your database settings, adapt one of the example
queries in `canvasback.c` as `base_query`, then:

`make`

`./canvasback`

Access the raw streaming binary goodness at: `http://localhost:7987/any/thing?bbox=-122,37,-123.3,38.8`

This is intended to work well with polymaps/modestmaps bounding box substitution 
variables and vector tile map stuff.

***sample output***

All San Francisco Streets:

![san francisco streets](http://h.sfgeo.org/tmp/pics/osm_streets_padded_wkb.png)

Copy of VECNIK New York Bic map in San Francisco:

![vecnik style sf map](http://h.sfgeo.org/tmp/pics/comolosvizzualiteros.png)

Northern California's railways over a political subdivision map:

![canvasback
example](http://h.sfgeo.org/tmp/pics/railways_canvasback.jpg)

Sacramento River delta, Northern Contra Costa Co:
![canvasback
example](http://h.sfgeo.org/tmp/pics/delta_canvasback.jpg)

Female canvasback duck with ducklings:

![canvasback example](http://upload.wikimedia.org/wikipedia/commons/3/35/Aythya_valisineria2.jpg)

A good source of bulk partial Open Street Map data is [Cloudmade](http://downloads.cloudmade.com/).
`osm2pgsql` with lon-lat flag is what I use for examples.

Dependencies: Postgres+PostGIS with development headers, gcc.
