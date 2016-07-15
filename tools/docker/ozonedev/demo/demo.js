'use strict';

var express = require('express');

var errorHandler = require('errorhandler');

var app = express(); //Create the Express app
app.set( 'port', process.env.PORT || 9280 );

var env = process.env.NODE_ENV || 'development';

console.log( env );

// development only
if (env === 'development') {
    app.use(errorHandler());
}

// production only
if (env === 'production') {
    // TODO
}

app.use( express.static(process.env.HOME+'/demo') );
app.get( '/([^.]+)$', function( req, res ) {
    var host = req.headers.host;
    host = host.replace( /:.*$/, ':9292' );
    var url = req.url;
    res.redirect( 'http://'+host+url );
});
app.get( '*', function( req, res ) {
    res.redirect('/demo.html');
});

var server = app.listen(app.get('port'), function() {
  console.log('Express server listening on port ' + server.address().port);
});

process.on('uncaughtException', function(err) {
    console.log( "Caught Exception: "+err );
    console.trace( err );
});
