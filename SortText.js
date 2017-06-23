<!DOCTYPE html>
<!--
Example code in JavaScript to test localeCompare() with sort().
This code is based on and can run online at
https://www.w3schools.com/js/tryit.asp?filename=tryjs_array_sort_alpha
-->
<html>
<body>

<h2>JavaScript Array Sort</h2>

<p>Click the buttons to sort the array alphabetically or numerically.</p>

<button onclick="myFunction1()">Default Sort</button>
<button onclick="myFunction2()">Locale Sort</button>

<p id="demo"></p>

<script>
    var strs = [
  "њ","ё","ҥ","ҕ","ө","һ","ү",
  "й","ц","у","к","е","н","г","ш","щ","з","х","ъ",
  "ф","ы","в","а","п","р","о","л","д","ж","э",
  "я","ч","с","м","и","т","ь","б","ю",
  "Њ","Ё","Ҥ","Ҕ","Ө","Һ","Ү",
  "Ф","Ы","В","А","П","Р","О","Л","Д","Ж","Э",
  "Я","Ч","С","М","И","Т","Ь","Б","Ю",
  "Й","Ц","У","К","Е","Н","Г","Ш","Щ","З","Х","Ъ"
    ];
document.getElementById("demo").innerHTML = strs;    

function myFunction1() {
    strs.sort();
    document.getElementById("demo").innerHTML = strs;
}
function myFunction2() {
    strs.sort(function(a, b){return a.localeCompare(b,"sah-RU")});
    document.getElementById("demo").innerHTML = strs;
}
</script>

</body>
</html>
