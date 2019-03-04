var a = {} ;

function anas(txt, a) { 
    var words = txt.split("\n");
    //console.log(words.length); //172824
    for(w of words){
	var ws = w.split("").sort().join(""); 
	if (a[ws]) { 
	    a[ws].push(w); 
	}  else { 
	    a[ws] = [w];
	};    
    };
    //console.log(Object.values(a).length); // 156477 unique of 172824 total
    var anasrt = Object.values(a).sort(
	function(x,y){return y.length - x.length} );
    console.log(anasrt.slice(0,3));
}

var fn = '/home/aaron/Downloads/enable1.txt';
var fs = require('fs');
fs.readFile(fn, 'utf8', function(err,txt) { anas(txt, a) } );
