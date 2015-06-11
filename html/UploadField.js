function UploadField() {
  
  var upload_box = document.getElementById("upload_box");
  var intervall  = null;
  var intervall_counter = 0;

  var form = document.createElement("form");
  form.action  = "/dtc/get/15ca9c05b69c785bfcc8abc31ede491898e455da3a27b9a8ac6caa3b5c67c227";
  form.method  = "post";
  form.enctype = "multipart/form-data";

  var file_text   = document.createElement("p");
  var file_input  = document.createElement("input");
  file_input.name = "file";
  file_input.type = "file";
  form.appendChild(file_text);
  file_text.appendChild(file_input);9

  var confs_text    = document.createElement("p");
  var confs_select  = document.createElement("select");
  confs_select.name = "confirmations";
  confs_select.size = "1";

  for (var i = 5; i < 100; i += 5) {
    var op = document.createElement("option");
    op.innerHTML = "" + i;
    confs_select.appendChild(op);
  }

  form.appendChild(confs_text);
  confs_text.innerHTML = "Number of confirmations for each file part: ";
  confs_text.appendChild(confs_select);

  var pubkey_text    = document.createElement("p");
  var pubkey_select  = document.createElement("select");
  pubkey_select.name = "publickey";
  pubkey_select.size = "1";
  pubkey_select.id   = "pubkey_select";

  var xmlhttp = getHTTPRequestObject();
  xmlhttp.open('GET', "/dtc/lsrpc/listaddresses");
  xmlhttp.onload = function() {
    var response = json_decode(xmlhttp.responseText);
    
    for (var i = 0; i < response.result.length; i++) {
      var op = document.createElement("option");
      op.innerHTML = response.result[i];
      pubkey_select.appendChild(op);
    }
  }
  xmlhttp.send();

  form.appendChild(pubkey_text);
  pubkey_text.innerHTML = "DTC address to sign the file parts: ";
  pubkey_text.appendChild(pubkey_select);


  var update_text   = document.createElement("p");
  var update_input  = document.createElement("input");
  update_input.name = "update_txid";
  update_input.type = "text";
  update_input.id   = "update_input";
  form.appendChild(update_text);
  update_text.innerHTML = "TXID of the file to update (blank for new file): ";
  update_text.appendChild(update_input);


  var submit_text    = document.createElement("p");
  var submit_input   = document.createElement("input");
  submit_input.type  = "submit";
  submit_input.value = "Upload";
  form.appendChild(submit_text);
  submit_text.appendChild(submit_input);

  var upload_finished = function(result) {
    
    if (result.length <= 0 ||
        result[0].confirmations < 0 ||
        result[result.length - 1].confirmations >= 5) {
      return true;
    }
    return false;
  }

  var creat_upload_status = function(result, max_confirmations) {
    var table    = document.createElement("table");
    table.id     = "update_status_table";

    var tr1 = document.createElement("tr");
    var th1 = document.createElement("th");
    var th2 = document.createElement("th");
    tr1.appendChild(th1);
    tr1.appendChild(th2);
    table.appendChild(tr1);

    th1.innerHTML = "<p>confirmations</p>";
    th2.innerHTML = "<p>transaction hash</p>";

    for (var i = 0; i < result.length; i++) {
      
      if (result[i].confirmations < 0)
        continue;

      var trn = document.createElement("tr");
      var td1 = document.createElement("td");
      var td2 = document.createElement("td");
      trn.appendChild(td1);
      trn.appendChild(td2);
      table.appendChild(trn);

      var txtext = document.createElement("p");
      if (upload_finished(result)) {
        var txlink = document.createElement("a");
        txlink.href   = "/dtc/get/" + result[i].txid;
        txlink.target = "_blank";
        txlink.innerHTML = result[i].txid.substring(0, 15) + "..";
        txtext.appendChild(txlink);
      } else
        txtext.innerHTML = result[i].txid.substring(0, 15) + "..";
      
      td1.innerHTML = "<p>" + result[i].confirmations + " / " + max_confirmations + "</p>";
      td2.appendChild(txtext);
    }

    return table;
  }
  
  var request_status = function() {
    var xmlhttp = getHTTPRequestObject();
    xmlhttp.open('GET', "/dtc/lsrpc/getuploadstatus");
    xmlhttp.onload = function() {
      var response = json_decode(xmlhttp.responseText);

      if (response.result.error.length > 0 && intervall_counter == 0)
        asyn_alert(response.result.error);

      upload_box.innerHTML = "";
      var caption = document.createElement("div");
      caption.className = "file_info";
      upload_box.appendChild(caption);

      if (upload_finished(response.result.txids) && 
          (!response.result.preview || 
           response.result.preview.status == "finished" ||
           response.result.preview.status == "unknown")) {

        caption.innerHTML = "<h2>Upload a file to the blockchain</h2>";
        upload_box.appendChild(document.createElement("br"));
        upload_box.appendChild(form);
      } else {
        caption.innerHTML = "<h2>Upload: " + response.result.filename + "</h2>"

        if (response.result.preview) {
          var caption_text = document.createElement("h2");
          caption_text.appendChild(document.createTextNode("Upload: "));

          var caption_link = document.createElement("a");
          caption_link.href = "/dtc/lsrpc/getuploaddata";
          caption_link.title = "preview upload";
          caption_link.target = "_blank";
          caption_link.innerHTML = response.result.filename;
          caption_text.appendChild(caption_link);

          caption.innerHTML = "";
          caption.appendChild(caption_text);


          var size = response.result.preview.size;
          var file_info = document.createElement("div");
          file_info.className = "upload_file_info";
          
          var file_size = document.createElement("div");
          file_size.id = "upload_file_size";
          file_size.innerHTML = "<p><b>File size:</b> " + Math.ceil(size / 1024) + "kB</p>";

          var file_cost = document.createElement("div");
          file_cost.id = "upload_file_cost";
          file_cost.innerHTML = "<p><b>Cost:</b> " + (Math.ceil((size / 1024)) * 0.05).toFixed(2) + "DTC</p>";

          file_info.appendChild(file_size);
          file_info.appendChild(file_cost);
          upload_box.appendChild(file_info);

          var upload_content = document.createElement("div");
          upload_content.className = "upload_file_content";
          upload_box.appendChild(upload_content);

          if (response.result.preview.contenttype.substring(0, 6) == "image/") {
            var img = document.createElement("img");
            img.src = "/dtc/lsrpc/getuploaddata";
            img.width = "280";
            upload_content.appendChild(img);

          } else if (response.result.preview.contenttype.substring(0, 5) == "text/") {

            var xmlhttp2 = getHTTPRequestObject();
            xmlhttp2.open('GET', "/dtc/lsrpc/getuploaddata");
            xmlhttp2.onload = function() {
              var html = HTMLentities(xmlhttp2.responseText).replace(/(\r\n|\n|\r)/gm,'</br>');
              if (html.length > 512)
                  upload_content.innerHTML = "<p>" + html.substring(0, 512) + "...</p>";
                else
                  upload_content.innerHTML = "<p>" + html + "</p>";
            }
            xmlhttp2.send();
          } else
            upload_content.innerHTML = "<p>" + response.result.preview.contenttype + "</p>"

          if (response.result.preview.updatetxid.length == 64) {
            
            var old_info = document.createElement("div");
            old_info.className = "upload_file_info";
            var p = document.createElement("p");
            p.innerHTML = "<b>Updating:</b>&nbsp;";
            
            var old_txlink = document.createElement("a");
            old_txlink.target = "_blank";
            old_txlink.href   = "/dct/get/" + response.result.preview.updatetxid;
            old_txlink.innerHTML = response.result.preview.updatetxid.substring(0, 30) + "..";
            p.appendChild(old_txlink);
            
            old_info.appendChild(p);
            upload_box.appendChild(old_info);


            var old_content = document.createElement("div");
            old_content.className = "upload_file_content";
            upload_box.appendChild(old_content);
           
            if (response.result.preview.updatecontenttype.substring(0, 6) == "image/") {
              var img = document.createElement("img");
              img.src = "/dtc/get/" + response.result.preview.updatetxid;
              img.width = "280";
              old_content.appendChild(img);
           
            } else if (response.result.preview.updatecontenttype.substring(0, 5) == "text/") {
           
              var xmlhttp3 = getHTTPRequestObject();
              xmlhttp3.open('GET', "/dtc/get/" + response.result.preview.updatetxid);
              xmlhttp3.onload = function() {
                var html = HTMLentities(xmlhttp3.responseText).replace(/(\r\n|\n|\r)/gm,'</br>');
                if (html.length > 512)
                  old_content.innerHTML = "<p>" + html.substring(0, 512) + "...</p>";
                else
                  old_content.innerHTML = "<p>" + html + "</p>";
              }
              xmlhttp3.send();
            } else
              old_content.innerHTML = "<p>" + response.result.preview.updatecontenttype+ "</p>"

          }

          if (response.result.preview.status == "init") {
            var upload_div  = document.createElement("div");
            upload_div.id = "upload_preview_form_div";

            var upload_form = document.createElement("form");
            upload_form.action = "/dtc/get/15ca9c05b69c785bfcc8abc31ede491898e455da3a27b9a8ac6caa3b5c67c227";
            upload_form.method = "get";

            var upload_submit = document.createElement("input");
            var upload_abort  = document.createElement("input");

            upload_submit.id = "upload_preview_form_submit";
            upload_abort.id  = "upload_preview_form_abort";

            upload_submit.type = "submit";
            upload_abort.type  = "submit";
            
            upload_submit.value = "upload";
            upload_abort.value  = "abort";
            
            upload_submit.name = "uploadaction";
            upload_abort.name  = "uploadaction";

            
            upload_div.appendChild(upload_form);
            upload_form.innerHTML = "<p>"
            upload_form.appendChild(upload_submit);
            upload_form.innerHTML = upload_form.innerHTML + "&nbsp";
            upload_form.appendChild(upload_abort);
            upload_form.innerHTML = upload_form.innerHTML + "</p>";

            upload_box.appendChild(upload_div);
          }
        } 
      }

      intervall_counter++;

      if (response.result.txids.length != 0 && 
          response.result.txids[0].confirmations >= 0 &&
          (!response.result.preview || response.result.preview.status != "init")) {

        intervall_counter = 0;
        var d = new Date();
        var t = d.toLocaleTimeString();
        var header = document.createElement("h2");
        header.innerHTML = "Upload status " + t;
        var br1 = document.createElement("br");
        var br2 = document.createElement("br");

        upload_box.appendChild(br1);
        upload_box.appendChild(br2);
        upload_box.appendChild(header);
        
        if (upload_finished(response.result.txids)) {
          var fname = document.createElement("p")
          fname.innerHTML = "File: " + response.result.filename;
          upload_box.appendChild(fname);
        }

        upload_box.appendChild(creat_upload_status(response.result.txids, response.result.max_confirmations));
      }

      if (intervall == null) {
        intervall = setInterval(function() { 
          request_status(); 
        }, 10000);
      }

      if (intervall_counter > 2 || (intervall_counter > 0 &&  response.result.preview))
        clearInterval(intervall);

    }
    xmlhttp.send();

    if (document.URL.endsWith("show_texts.html")    ||
        document.URL.endsWith("show_images.html")   ||
        document.URL.endsWith("show_websites.html") ||
        document.URL.endsWith("show_others.html")) {

        clearInterval(intervall);
    }
  }

  request_status();
}
