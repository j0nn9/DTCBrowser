function TXListWebsites() {
  
  var list = null;
  var list_pos = 0;
  var msnry = null;

  this.init_list =function(newlist) {
    list = newlist;
    list_pos = 0;
    var wrap_content = document.getElementById("wrap_content");
    wrap_content.className = "wrap_masonry";
    var body_div = document.getElementById("content");
    body_div.innerHTML = "";
    msnry = new Masonry(body_div, {
      itemSelector: '.file_box_website',
      isFitWidth: true,
    });
    msnry.bindResize();
  }

  this.uninit = function() {
    if (msnry != null)
      msnry.destroy();
  }

  this.add_element = function() {
    var i = list_pos;
    if (i >= list.length) return;

    list_pos++;
    
    var body_div = document.getElementById("content");

    var container = document.createElement("div");
    container.className = "file_box_website";
 
    var file_header = document.createElement("div");
    file_header.className = "file_info_website";
    container.appendChild(file_header);
    
    var header_link    = document.createElement("a");
    header_link.href   = "/dtc/get/" + list[i].hash;
    header_link.target = "_blank";
    file_header.appendChild(header_link);
    header_link.innerHTML = "<p>" + HTMLentities(list[i].FileName) + "</p>";


    body_div.appendChild(container);
    msnry.appended(container);
    msnry.layout();
  }
}
