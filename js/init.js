
var swig;
var tmp
function init() {
	tmp = require("://js/swig.js");
	require("://js/img_replacer.js");
	swig = new window.swig.Swig({
		cache : false 
	});
}


function replace_image_file(o,n) { 
	reporter.replace_image_file(o,n); return ''; 
}

function get_new_img_height (f,h) { 
	return reporter.get_new_img_height(f,h); 
}




function require( filename ){
	console.log('require: ', filename);

	if( filename === 'path' )  return path;
	if( filename === 'fs' ) return fs;

	return reporter.requireJS( filename );
}


var console = {
	log : function() {
		var msg = "";
		for( var i = 0; i < arguments.length; i++ )
			msg += arguments[i] + " ";
		print.call( print, msg );
		reporter.sig_info('QtScript: ' + msg );
	},
	error : function() {
		var msg = "";
		for( var i = 0; i < arguments.length; i++ )
			msg += arguments[i] + " ";
		print.call( print, "ERROR: " + msg );
		reporter.sig_error('QtScript: ' + msg );
	}
}



var window 	= 	{};


var fs 		= 	{
	readFileSync : function( filename ) {
		//console.log( "readFileSync: ", filename );
		if( filename.split(".").last() == "odt" ){
			return reporter.mergeOdtFile( filename );
		}
		return reporter.readTxtFile( filename );
	},
	readFile : function( filename, cb ){
		cb( this.readFileSync( filename ) );
	}
};
var _fs = fs;

var path = {
	dirname   : function( path ) { return path; },
	normalize : function( path ) { return path; },
	resolve   : function( path ) { return path; }
}


//independent funcs

Array.prototype.last = function() {
	return this[this.length-1];
}



init();
