var elements = document.querySelectorAll('.disconnect');

var ul = document.getElementById("list");

function event(){
    console.log('some event content here...')
}
Array.prototype.forEach.call(elements, (item) => {
    item.addEventListener('click', clickEvent);
});

function append(content_arr) {
    var li = document.createElement("li");
    li.innerHTML = ' \
    <span class="ipv4"> 192.168.1.108 </span> \
    <span class="cpu"> intel core i9-13900k </span> \
    <span class="gpu"> GeForce RTX 3090 </span> \
    <span class="Disk"> WD Red Plus NAS 8TB</span> \
    <span class="Ram"> 35.3 GB </span> \
    ';
    ul.appendChild(li);
  }
