/* 
 * Notes:
 * The convention here is variable names like
 * `some_position` or `pos` for location.
 * `qs` for query string.
 * `base_url` is wherever you have the server running.
 * `/_geoms` is one such GeoJSON endpoint. 
 * mostly using PostGIS ST_AsGeoJSON and libpq.
 * See also sample nginx conf. 
 * Uses spherical mercator.
 * Layers go base polygons to highways to roads
 * Even layers are very simplified geometries.
 *
 */


var cf = {};

// simplified (evens) ->
//cf.layers = [2, 0];
// unsimplified (odds) ->
cf.layers = [1, 5];


cf.randcol = function () {
    var randcolor = "#"+(Math.random()*0xFFFFFF<<0).toString(16);
    return randcolor.length == 7 ? randcolor : randcolor + "0"*(6-randcolor.length);
};

/*cf.position = function (loc) {
    return {lon: loc.lon ? loc.lon : -122.45,
    lat: loc.lat ? loc.lat : 37.79,
    dist: loc.dist ? loc.dist : 10.5,
    query_ordinal: loc.query_ordinal ? query_ordinal : 2,
    base_url : "http://"+window.document.domain.toString()+":"+window.location.port.toString()+"/_geoms?"};
};*/


cf.position = {lon: -122.45,
    lat: 37.79,
    dist: 2.5,
    query_ordinal: 2,
    base_url : "http://"+window.document.domain.toString()+":"+window.location.port.toString()+"/_geoms?"};

cf.base_url = "http://"+window.document.domain.toString()+":"+window.location.port.toString()+"/_geoms?";

cf.mercator_origin = function (lon, lat, distkm) {
    var lonf = 0.0113222194;
    var latf = 0.0090215040;
    var xmin = (lon-distkm*lonf)*111319.49;
    var ymin = 180/Math.PI * Math.log(Math.tan(Math.PI/4+(lat-distkm*latf)*(Math.PI/180)/2))*111319.49;
    var xmax = (lon+distkm*lonf)*111319.49;
    var ymax = 180/Math.PI * Math.log(Math.tan(Math.PI/4+(lat+distkm*latf)*(Math.PI/180)/2))*111319.49;
    var dx = xmax - xmin;
    var dy = ymax - ymin;
    return {lonf:lonf, latf:latf, xmin:xmin, ymin:ymin, xmax:xmax, ymax:ymax, dx:dx, dy:dy};
};

cf.fmt = function (qs) {
        return qs.base_url + "lon=" + qs.lon + "&lat=" + qs.lat + "&dist=" + qs.dist + "&query_ordinal=" + qs.query_ordinal;
};

cf.offset = cf.mercator_origin(cf.position.lon, cf.position.lat, cf.position.dist);

cf.start_position = function (init) {
    var pos = init;
    var layer_idx, partial_distx, partial_disty;
    return function (idx, incr) {
        pos[idx] += incr;
        cf.offset = cf.mercator_origin(pos.lon, pos.lat, pos.dist);
        cf.cvx = 800/(cf.offset.xmax - cf.offset.xmin);
        cf.cvy = 800/(cf.offset.ymax - cf.offset.ymin);
        cf.offx = cf.offset.xmin;
        cf.offy = cf.offset.ymin;
        for (layer_idx = 0; layer_idx < cf.layers.length; layer_idx++) {
            pos.query_ordinal = cf.layers[layer_idx];
            //partial_dist = pos.dist -.5;
            this.get_geoms(this.fmt({base_url:cf.base_url,
                                    lon: pos.lon,
                                    lat: pos.lat,
                                    dist: pos.dist,
                                    query_ordinal : pos.query_ordinal}));
     /*       for ((partial_distx = pos.dist -0.5); partial_distx >= (-1)*pos.dist; partial_distx -= 1.0) {
                this.get_geoms(this.fmt({base_url:cf.base_url,
                                        lon: pos.lon + (0.0113222194 * partial_distx),
                                        lat: pos.lat, // + (0.0090215040 * partial_dist),
                                        dist: .5,
                                        query_ordinal : pos.query_ordinal}));
                for ((partial_disty = pos.dist -0.5); partial_disty >= (-1)*pos.dist; partial_disty -= 1.0) {
                    this.get_geoms(this.fmt({base_url:cf.base_url,
                                            lon: pos.lon + (0.0113222194 * partial_distx),
                                            lat: pos.lat + (0.0090215040 * partial_disty),
                                            dist: .5,
                                            query_ordinal : pos.query_ordinal}));
                }
       */   }
        return;
    }
};

cf.mv = cf.start_position(cf.position);

cf.move_map = function(direction) {
    console.log(direction);
    ctx = window.document.getElementById("canv").getContext("2d");
    ctx.clearRect(0,0,800,800);
    switch (direction) {
        case "n": cf.mv("lat", 0.04); break; 
        case "s": cf.mv("lat", -0.04); break;
        case "w": cf.mv("lon", -0.04); break;
        case "e": cf.mv("lon", 0.04); break;
        case "out": cf.mv("dist", 3); break;
        case "in": cf.mv("dist", -3); break;
        default: break;
    }
};

cf.draw_lnstr = function (ctx, lnstr, lnstr_length) {
        ctx.moveTo((lnstr[0][0] - cf.offx)*cf.cvx, 800 - (lnstr[0][1] - cf.offy)*cf.cvy);
        for (var i = 1; i < lnstr_length; i++) {
            ctx.lineTo((lnstr[i][0] - cf.offx)*cf.cvx, 800 - (lnstr[i][1] - cf.offy)*cf.cvy);
        }
        ctx.strokeStyle = "yellow"; // cf.randcol();
        ctx.lineWidth = 4;
    }

cf.draw_poly = function (ctx, polygon, polygon_length) {
        ctx.fillStyle = cf.randcol();
        ctx.beginPath();
        ctx.moveTo((polygon[0][0] - cf.offx)*cf.cvx, 800 - (polygon[0][1] - cf.offy)*cf.cvy);
        for (var i = 1; i < polygon_length; i++) {
            ctx.lineTo((polygon[i][0] - cf.offx)*cf.cvx, 800 - (polygon[i][1] - cf.offy)*cf.cvy);
        }
        ctx.stroke();
        ctx.fill();
    };

cf.render_geoms = function (dats) {
    var data = JSON.parse(dats);
    var i, k;
    var canv = window.document.getElementById("canv");
    var ctx = canv.getContext('2d');
    //ctx.lineWidth = 4;
    var dl = data.features.length-1;
    //var off = [offset.xmin, offset.ymin];
    for (i = 0; i < dl; i += 1) {
        if (data.features[i].geometry.type == "Polygon") {
            for (k = 0; k < data.features[i].geometry.coordinates.length; k++) {
                cf.draw_poly(ctx, data.features[i].geometry.coordinates[k], data.features[i].geometry.coordinates[k].length);
            }
        //} else if (data.features[i].geometry.type == "LineString" && data.features[i].properties.highway) {
        } else if (data.features[i].geometry.type == "LineString") {
            cf.draw_lnstr(ctx, data.features[i].geometry.coordinates, data.features[i].geometry.coordinates.length);
        }
    }
    ctx.stroke();
};

cf.get_geoms = function (pos_qs) {
    //var pos = pos_obj;
    //pos_obj = cf.position;
    var req = new XMLHttpRequest();
    req.open('GET', pos_qs, false);
    console.log(pos_qs);
    req.send(null);
    if (req.status == 200) {
        cf.render_geoms(req.responseText);
    }
};
