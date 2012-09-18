`canvasback` is a (we hope fast) endpoint for simple 2d mapping in html5 elements, minimal 
enough to enable scrolling and zooming without pre-rendering images. The basic
idea is to be like a geojson layer, but with binary arrays of geospatial geometries,
which are often 1/10 the size of geojson and render really quickly. Most of 
the work will be on the data and javascript sides but I want to test the 
feasibility of this implementation of some of the basics.

## the current master branch

is now using an experimental data format. For original Well-Known Binary implementation, see the `wellknownbinary` tag. This new thing (unknown binary?) is something like:

<pre>
uint32 geometry_type // same as wkb
uint32 num_pts // basically like wkb
uint32 osm_type // based on highway taxonomy, not required
uint8[num_pts] // wat
</pre>

The last bit takes advantage of the fact that tile maps are so commonly 256 px
 wide, if you scale the geometry on the way out to be the offset from the tile
 origin for each tile, all your values will fit in a single byte, a coordinate
 pair in 2 bytes. This results in very small network requirements and is
 pretty inconsequential in terms of server resources. 

To use: edit `db.conf.c` with your database settings, adapt the
queries in `canvasback.c` for your different zoom levels, then:

`make`

`./canvasback`

Access the streaming binary things at: `http://localhost:7987/whatever/tms/Z/X/Y.someext`

(any thing of the form `/tms/k/n/m` should work, but `tms` is required, ie `example.com/anything/who?tms=10,11,12`)

***sample output***

All San Francisco Streets:

![san francisco streets](http://h.sfgeo.org/tmp/pics/osm_streets_padded_wkb.png)

Copy of VECNIK New York Bic map in San Francisco:

![vecnik style sf map](http://h.sfgeo.org/tmp/pics/comolosvizzualiteros.png)

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
