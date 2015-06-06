function TXListImage() {
  
  var list = null;
  var list_pos = 0;
  var msnry = null;

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
    container.appendChild(file_body);

    var body_image_link = document.createElement("a");
    body_image_link.href = "/dtc/get/" + list[i].hash;
    body_image_link.target = "_blank";
    file_body.appendChild(body_image_link);

    var body_image = document.createElement("img");
    body_image.src = "/dtc/get/" + list[i].hash;
    body_image.width = "270";
    body_image.style.marginTop = "5px";
    body_image_link.appendChild(body_image);

    body_image.onload = function() {
      msnry.layout();
    }
    msnry.layout();
  }
}
