var img_replacer = {};

img_replacer.compile = function (compiler, args, content, parents, options, blockName) {
    //console.log('--- default: ', compiler(content, parents, options, blockName) );
    
    var new_href = args.join(' + ');

    var result = "";
    for( var i = 0; i < content.length; i++ ){
        if( typeof(content[i]) === 'string' ){

            var picture_ref = content[i].match( /xlink:href="([^"]+)"/ );       // ODT image reference
            var width_expr  =    /svg:width="(\d+\.?\d+)(\w+)"/;
            var height_expr =    /svg:height="[\w.,]+"/;
            var height_replace = 'svg:height="[replaced]"';
            
            if( !picture_ref ){
                picture_ref = content[i].match( /<a:blip r:embed="(\w+)">/ );   // DOCX image reference
            
                if( picture_ref ){
                    width_expr  =    /cx="(\d+)"/;
                    height_expr =    /cy="\d+"/;
                    height_replace = 'cy="[replaced]"';
                } else {
                    result += compiler([content[i]], parents, options, blockName);
                    continue; 
                }
            }



            var old_href = picture_ref[ 1 ];
            if( old_href && new_href && typeof( replace_image_file ) === 'function' ){
                result += '_output += replace_image_file("' + old_href + '" , ' + new_href + ');';
            }

            var width_match = content[i].match( width_expr );

            var replacing;

            if( width_match ){

                var height  = width_match[1];
                var measure = width_match[2] || '';

                //console.log('--- height:', height, ' measure:', measure );

                replacing = content[i].split( height_expr ).join( height_replace );
                replacing = replacing.split('"').join('\\"').split('\n').join('\\n'); // result must be an evaluatable JS expression
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
