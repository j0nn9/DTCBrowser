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
      if (result[result.length - 1].confirmations >= 5) {
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
        alert(response.result.error);

      upload_box.innerHTML = "<h2>Upload a file to the blockchain</h2>";
      upload_box.appendChild(form);
      intervall_counter++;
 
      if (response.result.txids.length != 0) {
        intervall_counter = 0;
        var d = new Date();
        var t = d.toLocaleTimeString();
        var header = document.createElement("h2");
        header.innerHTML = "Upload status " + t;
        var br1 = document.createElement("br");
        var br2 = document.createElement("br");

        var fname = document.createElement("p")
        fname.innerHTML = "File: " + response.result.filename;

        upload_box.appendChild(br1);
        upload_box.appendChild(br2);
        upload_box.appendChild(header);
        upload_box.appendChild(fname);
        upload_box.appendChild(creat_upload_status(response.result.txids, response.result.max_confirmations));
      }

      if (intervall == null) {
        intervall = setInterval(function() { 
          request_status(); 
        }, 10000);
      }

      if (intervall_counter > 2)
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
