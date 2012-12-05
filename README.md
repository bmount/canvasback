`canvasback` serves geometries for use in web maps. The basic
idea is to be like a geojson vector layer, but with binary arrays of geospatial records,
which can be less than 1/10 the size of geojson and render really quickly.

## the current master branch

uses an experimental data format. For original Well-Known Binary implementation, see the `wellknownbinary` tag. This new thing serves up an array of records like this:

<pre>
uint32 geometry_type // same as wkb
uint32 num_pts // basically like wkb
uint32 osm_type // based on highway taxonomy, not required, alternatively for things like building height
int16[num_pts] // wat
</pre>

Basically, a tile map service (google style) query of the form:

`someurl/tms/z/x/y`

returns an array of the above records, but the coordinates are
pairs of 2 byte integer offsets from the tile origin (in all other ways,
the format is basically [well-known binary (pdf spec)](http://portal.opengeospatial.org/files/?artifact_id=829)).

Caveats to the approach in general are that you
have to 1) use typical clippped tiles and/or 2) know in advance
that your geometry won't be too big. 1 makes sense anyway and for 2, a primary use case
for this is loading
lots of building geometries where the query is done on centroids and ground
area is [predictably scaled](http://en.wikipedia.org/wiki/List_of_largest_buildings_in_the_world).

The advantage is speed and compactness, for example here is every unsimplified
building in a good chunk of northeast San Francisco at zoom 16, which renders
imperceptibly quick and averages 35k per tile (more realistic zoom levels are much smaller!):

![](http://farm9.staticflickr.com/8490/8247298268_6286b0c33c_b.jpg)

Here's every node and way in OSM SF at zoom level 13, at a size comparable to png:

![](http://farm9.staticflickr.com/8210/8246255601_b2d2303d89_b.jpg)

More demos etc forthcoming

To use: edit `db.conf.c` with your database settings, adapt the
queries in `canvasback.c` for your different zoom levels, then:

`make`

`./canvasback`

Access at: `http://localhost:7987/whatever/tms/Z/X/Y.someext`

(any thing of the form `/tms/k/n/m` should work, but `tms` is required, ie `example.com/anything/who?tms=10,11,12`)

Dependencies: Postgres and PostGIS

Canvasback duck with ducklings:

![canvasback example](http://upload.wikimedia.org/wikipedia/commons/3/35/Aythya_valisineria2.jpg)

