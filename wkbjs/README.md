this is a base couchapp for working on typed or just pure bytestreams
of ogc wkb/ewkb data from postgis

easiest way to use: put an http 1.1 proxy in couchprefix/etc/couchdb/local.ini
that forwards to the canvasback port, ie:

`[httpd_global_handlers]
_canvasback = {couch_httpd_proxy, handle_proxy_req, <<"http://localhost:7987">>}`

you can of course use any equivalent to the above, last time i tried nginx spoke 
only http 1.0 to canvasback so canvasback freaked out. CouchDB is super easy for this.
Probably a node.js thing would be a good idea a la the proxy table api of
github.com/nodejitsu/node-http-proxy

this is using the python couchapp from pypi
