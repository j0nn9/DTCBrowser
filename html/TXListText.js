function TXListText() {
  
  var list = null;
  var list_pos = 0;
  var msnry = null;

  var up_arrow_img = document.createElement("img");
  /* /img/up_arrow.png */
  up_arrow_img.src = "/dtc/get/daf3c111dc4fed36316e8a355f85616e10006df2cc1e012d8bfbbe20a98bdb5b";
  up_arrow_img.className = "arrow_img";

  var down_arrow_img = document.createElement("img");
  /* img/down_arrow.png */
  down_arrow_img.src = "/dtc/get/21eb5d76e79837c06c9b923db459dd40990bae04cd06ec91429af6f95e231c68";
  down_arrow_img.className = "arrow_img";

  this.init_list = function(newlist) {
    list = newlist;
    list_pos = 0;
    var wrap_content = document.getElementById("wrap_content");
    wrap_content.className = "wrap_masonry";
    var body_div = document.getElementById("content");
    body_div.innerHTML = "";
    msnry = new Masonry(body_div, {
      itemSelector: '.file_box',
      isFitWidth: true,
    });
    msnry.bindResize();
  }

  this.uninit = function() {
    if (msnry != null)
      msnry.destroy();
  }

  var fill_body = function(i) {

    /* set file content wen loaded */
    var xmlhttp = getHTTPRequestObject();
    xmlhttp.open('GET', "/dtc/get/" + list[i].hash);
    xmlhttp.onload = function() {

      var file_body_div = document.getElementById("file_container" + i);
      var html = HTMLentities(xmlhttp.responseText).replace(/(\r\n|\n|\r)/gm,'</br>');;

      /* if content is to long create append link */
      if (html.length > 1024) {
        html = html.substring(0, 1024) + "..";
        file_body_div.innerHTML = "<p>"  + html + "</p>";

        /* load all content when link clicked */
        var more_link   = document.createElement("a");
        more_link.href  = "#!show_texts.html"
        more_link.title = "load full content";
        more_link.appendChild(down_arrow_img.cloneNode(true));
        more_link.onclick = function() {

          var xmlhttp = getHTTPRequestObject();
          xmlhttp.open('GET', "/dtc/get/" + list[i].hash);
          xmlhttp.onload = function() {
            var file_body_div = document.getElementById("file_container" + i);
            var html = HTMLentities(xmlhttp.responseText).replace(/(\r\n|\n|\r)/gm,'</br>');;
            file_body_div.innerHTML = "<p>"  + html + "</p>";
            
            /* link to reduce content */
            var less_link   = document.createElement("a");
            less_link.href  = "#!show_texts.html";
            less_link.title = "reduce content";
            less_link.appendChild(up_arrow_img.cloneNode(true));
            less_link.onclick = function() {
              fill_body(i);
            }
            file_body_div.appendChild(less_link); 
            msnry.layout();
          }
          xmlhttp.send();
          msnry.layout();
          
        }
        file_body_div.appendChild(more_link);
      } else
        file_body_div.innerHTML = "<p>"  + html + "</p>";
      msnry.layout();
    }
    xmlhttp.send();
    msnry.layout();
  }

  this.add_element = function() {
    var i = list_pos;
    if (i >= list.length) return;

    list_pos++;

    var body_div = document.getElementById("content");

    var container = create_file_box(list[i].FileName, list[i].hash);
    body_div.appendChild(container);
    msnry.appended(container);

    var file_body = document.createElement("div");
    file_body.className = "file_container";
    file_body.id = "file_container" + i;
    container.appendChild(file_body);
    fill_body(i);
  }
}
