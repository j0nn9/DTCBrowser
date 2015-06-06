function TXListOthers() {
  
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
      itemSelector: '.file_box',
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

    var container = create_file_box(list[i].FileName, list[i].hash);
    body_div.appendChild(container);
    
    var file_body = document.createElement("div");
    file_body.className = "file_container";
    file_body.innerHTML = "<p>" + list[i].ContentType + "</p>";
    container.appendChild(file_body);

    msnry.appended(container);
    msnry.layout();
  }
}
