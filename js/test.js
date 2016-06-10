
var sw = new swig.Swig();

var content_;


var replace_image_file = function( oldFile, newFile ){
    console.log('--- oldFile :', oldFile );
    console.log('--- newFile :', newFile );
    return "";
}

var get_new_img_height = function( file, old_height ){
    console.log('--- get_new_img_height file:', file);
    console.log('--- get_new_img_height old_h:', old_height);
    return "999";
}


var test = function(){

    var sample = document.querySelector("#sample").value 
    
    

    console.log('--- sample: ', sample);
    
    var result = sw.render( sample, {
        locals : {
            src : 'new_src_file'
        } 
    } );
    
    console.log('\n--- result: ', result);  
    
    
};

document.addEventListener("DOMContentLoaded", test);  

