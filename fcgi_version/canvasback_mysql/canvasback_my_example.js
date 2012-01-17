canvas_fish = {};

canvas_fish.randcol = function () {
    var randcolor = '#'+(Math.random()*0xFFFFFF<<0).toString(16);
    return randcolor;
}

/* this should come from geolocation api */
canvas_fish.start_position = {"lon": -122.45,
                    "lat": 37.74,
                    "dist": 3.5,
                    "px": 800,
                    "base_url":"http://"+window.document.domain.toString()+":"+window.location.port.toString()+"/_geoms?"};

canvas_fish.pos =  function (init_pos) {
                var position = init_pos;
                    return function (pos_idx, incr) {
                        position[pos_idx] += incr;
                        this.get_geoms(this.fmt(position));
                    }
                }

canvas_fish.state = canvas_fish.pos(canvas_fish.start_position);

canvas_fish.fmt = function (qs) {
                    return qs.base_url + "lon=" + qs.lon + "&lat=" + qs.lat + "&dist=" + qs.dist + "&px=" + qs.px;
                };

canvas_fish.move_map = function(direction) {
    console.log(direction);
    switch (direction) {
        case "n": this.state("lat", .005); break; 
        case "s": this.state("lat", -.005); break;
        case "w": this.state("lon", -.005); break;
        case "e": this.state("lon", .005); break;
        case "out": this.state("dist", .5); break;
        case "in": this.state("dist", -.5); break;
        default: break;
    }
}

canvas_fish.draw_poly = function (canvas, polygon, polygon_length) {
    var contextagon = canvas.getContext('2d');
    contextagon.fillStyle = this.randcol();
    contextagon.beginPath();
    contextagon.moveTo(polygon[0][0], [0][1]);
    for (var i = 1; i < polygon_length; i++) {
        contextagon.lineTo(polygon[i][0], polygon[i][1]);
    }
    contextagon.stroke();
    contextagon.fill();
}

canvas_fish.get_geoms = function (url_) {
        var req = new XMLHttpRequest();
        req.open('GET', url_, false);
        req.send(null);
        if (req.status == 200) {
        function store_geoms (data) {
            data = JSON.parse(data);
            var canv = window.document.getElementById("canv");
            
            var dl = data.length;
            for (var i = 0; i < dl; i += 1) {
                canvas_fish.draw_poly(canv, data[i].coordinates, data[i].coordinates.length);
            };
        };
        var ctx  = canv.getContext("2d");
        ctx.clearRect(0,0,canv.width,canv.height);
        store_geoms(req.responseText);
    };
};
