function Main() {

  var textlist = new TXListText();
  var imagelist = new TXListImage();
  var websitelist = new TXListWebsites();
  var otherlist = new TXListOthers();
  var xmlHttpObject = getHTTPRequestObject();

  var set_clicked_link = function(link) {

    if (link == "main_page")
      document.getElementById("main_page").className     = "current";
    else
      document.getElementById("main_page").className     = "";

    if (link == "show_text")
      document.getElementById("show_text").className     = "current";
    else
      document.getElementById("show_text").className     = "";

    if (link == "show_image")
      document.getElementById("show_image").className    = "current";
    else
      document.getElementById("show_image").className    = "";

    if (link == "show_websites")
      document.getElementById("show_websites").className = "current";
    else
      document.getElementById("show_websites").className = "";

    if (link == "show_other")
      document.getElementById("show_other").className    = "current";
    else
      document.getElementById("show_other").className    = "";
  }

  /* show_text page */
  var show_text_page = function() {

    xmlHttpObject.open('GET', '/dtc/lsrpc/listtxids/text');
    xmlHttpObject.onload = function() {
      if (xmlHttpObject.readyState == 4 && xmlHttpObject.status == 200) {
   
        var js = json_decode(xmlHttpObject.responseText);
        textlist.init_list(js.result);
        for (i = 0; i < 20; i++) {
          textlist.add_element();
        }
      }
    }
    xmlHttpObject.send(null);
    set_clicked_link("show_text");
  }

  var show_image_page = function() {

    cur_page = "show_";
    xmlHttpObject.open('GET', '/dtc/lsrpc/listtxids/image');
    xmlHttpObject.onload = function() {
      if (xmlHttpObject.readyState == 4 && xmlHttpObject.status == 200) {

        var js = json_decode(xmlHttpObject.responseText);
        imagelist.init_list(js.result);
        for (i = 0; i < 20; i++) {
          imagelist.add_element();
        }
      }
    }
    xmlHttpObject.send(null);
    set_clicked_link("show_image");
  }

  var show_websites_page = function() {

    xmlHttpObject.open('GET', '/dtc/lsrpc/listtxids/website');
    xmlHttpObject.onload = function() {
      if (xmlHttpObject.readyState == 4 && xmlHttpObject.status == 200) {

        var js = json_decode(xmlHttpObject.responseText);
        websitelist.init_list(js.result);
        for (i = 0; i < 30; i++) {
          websitelist.add_element();
        }
      }
    }
    xmlHttpObject.send(null);
    set_clicked_link("show_websites");
  }

  var show_others_page = function() {

    xmlHttpObject.open('GET', '/dtc/lsrpc/listtxids/other');
    xmlHttpObject.onload = function() {
      if (xmlHttpObject.readyState == 4 && xmlHttpObject.status == 200) {

        var js = json_decode(xmlHttpObject.responseText);
        otherlist.init_list(js.result);
        for (i = 0; i < 30; i++) {
          otherlist.add_element();
        }
      }
    }
    xmlHttpObject.send(null);
    set_clicked_link("show_other");
  }

  var main_left  = document.getElementById("left").cloneNode(true);
  var main_right = document.getElementById("right").cloneNode(true);

  var main_page = function() { 
    
    var wrap_content = document.getElementById("wrap_content");
    wrap_content.className = "wrap";

    var content = document.getElementById("content");
    content.appendChild(main_left.cloneNode(true));
    content.appendChild(main_right.cloneNode(true));

    var upload_box = new UploadField();
    set_clicked_link("main_page");
  }

  var last_page = "none";

  var start_page = function() {
    
    if (last_page != document.URL) {
      last_page = document.URL;
      
      /* reset page */
      textlist.uninit();
      imagelist.uninit();
      websitelist.uninit();
      otherlist.uninit();

      document.getElementById("content").innerHTML = "";

      var show_text = document.getElementById("show_text");
      show_text.href = "#!show_texts.html";
     
      var show_image = document.getElementById("show_image");
      show_image.href = "#!show_images.html";
     
      var show_websites = document.getElementById("show_websites");
      show_websites.href = "#!show_websites.html";
     
      var show_others = document.getElementById("show_other");
      show_others.href = "#!show_others.html";
     
      var main = document.getElementById("main_page");
      main.onclick = function() {
        document.getElementById("content").innerHTML = "";
        main_page;
      }
     
      if (document.URL.endsWith("show_texts.html")) {
        show_text_page();
      } else if (document.URL.endsWith("show_images.html")) {
        show_image_page();
      } else if (document.URL.endsWith("show_websites.html")) {
        show_websites_page();
      } else if (document.URL.endsWith("show_others.html")) {
        show_others_page();
      } else {
        main_page();
      }
    }
  }

  var intervall = setInterval(function() { start_page(); }, 100);

  /* refresh page on scroll down */
  this.scrollHandler = function() {
    var wrap0 = document.getElementById('wrap_head');
    var wrap1 = document.getElementById('wrap_content');
    var wrap2 = document.getElementById('wrap_footer');
    var contentHeight = wrap0.offsetHeight + wrap1.offsetHeight; // + wrap2.offsetHeight;
    var yOffset = window.pageYOffset; 
    var y = yOffset + window.innerHeight;

    if(y >= contentHeight){
      if (document.URL.endsWith("show_texts.html")) {
        for (i = 0; i < 10; i++)
          textlist.add_element();
      } else if (document.URL.endsWith("show_images.html")) {
        for (i = 0; i < 10; i++)
          imagelist.add_element();
      } else if (document.URL.endsWith("show_websites.html")) {
        for (i = 0; i < 10; i++)
          websitelist.add_element();
      } else if (document.URL.endsWith("show_others.html")) {
        for (i = 0; i < 10; i++)
          otherlist.add_element();
      } 
    }
  }
}
