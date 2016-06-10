var img_replacer = {};

img_replacer.compile = function (compiler, args, content, parents, options, blockName) {
    //console.log('--- default: ', compiler(content, parents, options, blockName) );
    
    var new_href = args.join(' + ');

    var result = "";
    for( var i = 0; i < content.length; i++ ){
        if( typeof(content[i]) === 'string' ){

            var old_href =  content[i].match( /xlink:href="([^"]+)"/ )[ 1 ];
            if( old_href && new_href && typeof( replace_image_file ) === 'function' ){
                result += '_output += replace_image_file("' + old_href + '" , ' + new_href + ');';
            }

            var width_match = content[i].match( /svg:width="(\d+\.?\d+)(\w+)"/ );

            var replacing;

            if( width_match ){

                var height = width_match[1];
                var measure = width_match[2];

                //console.log('--- height:', height, ' measure:', measure );

                replacing = content[i].split( /svg:height="[^"]+"/ ).join( 'svg:height="[replaced]"' );
                replacing = replacing.split('"').join('\\"').split('\n').join('\\n');
                replacing = replacing.split('[replaced]').join('" + get_new_img_height(' + new_href + ',' + height + ') + "' + measure);

                //console.log( '--- replacing:', replacing );

            } else {
                replacing = replacing.split('"').join('\\"').split('\n').join('\\n');
            }

            result += '_output += "' + replacing + '";';

        } else {
            result += compiler([content[i]], parents, options, blockName);
        }
    }
    return  result;
    //return compiler( content, parents, options, blockName) + result;;
};

img_replacer.parse = function (str, line, parser, types) {
    return true;   
}

img_replacer.ends = true;


window.swig.setTag('img', img_replacer.parse, img_replacer.compile, img_replacer.ends );
