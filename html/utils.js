function getHTTPRequestObject() {
  var xmlHttpObject = false;

  /* is XMLHttpRequest-Class available */
  if (typeof XMLHttpRequest != 'undefined') {
    xmlHttpObject = new XMLHttpRequest();
  }

  /* IE6 or IE5 */
  if (!xmlHttpObject) {
    try {
      xmlHttpObject = new ActiveXObject("Msxml2.XMLHTTP");
    }
    catch(e) {
      try {
        xmlHttpObject = new ActiveXObject("Microsoft.XMLHTTP");
      }
      catch(e) {
        xmlHttpObject = null;
      }
    }
  }
  return xmlHttpObject;
}

function json_decode(text) {
  
  if (typeof JSON === 'object' && typeof JSON.parse === 'function') {
    try {
      return JSON.parse(text);
    } catch (e) {
      return false;
    }
  } 

  return eval("(" + text + ")");
}

function HTMLentities(text) {

  return text.replace(/[\u00A0-\u9999<>\&]/gim, function(i) {
     return '&#'+i.charCodeAt(0)+';';
  });
}

function check_server_version(callback) {

  var xmlhttp = getHTTPRequestObject();
  xmlhttp.open('GET', "/dtc/lsrpc/getversion");
  xmlhttp.onload = function() {
    var response = json_decode(xmlhttp.responseText);

    if (response == false)
      callback(0);
    else 
      callback(response.result);
  }
  xmlhttp.send();
}

function create_file_box(filename, txid) {

  var container = document.createElement("div");
  container.className = "file_box";

  var file_header = document.createElement("div");
  file_header.className = "file_info";
  container.appendChild(file_header);
  
  var header_link    = document.createElement("a");
  header_link.href   = "/dtc/get/" + txid
  header_link.target = "_blank";
  file_header.appendChild(header_link);
  header_link.innerHTML = "<p>" + HTMLentities(filename) + "</p>";

  return container;
}

function setCookie(name, value, exmins) {
  var date = new Date();
  date.setTime(date.getTime() + (exmins*60*1000));
  var expires = "expires="+date.toUTCString();
  document.cookie = name + "=" + value + "; " + expires;
}

function getCookie(name) {
  var name = name + "=";
  var cookies = document.cookie.split(';');

  for(i = 0; i < cookies.length; i++) {
    var c = cookies[i];
    while (c.charAt(0)==' ') 
      c = c.substring(1);

    if (c.indexOf(name) == 0) 
      return c.substring(name.length, c.length);
  }
  return "";
}

function asyn_alert(message) {
  setTimeout(function() { alert(message); }, 1);
}

/* implement String function endsWith if not provided */
if (typeof String.prototype.endsWith !== 'function') {
  String.prototype.endsWith = function(suffix) {
    return this.indexOf(suffix, this.length - suffix.length) !== -1;
  };
}
