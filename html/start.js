window.onload = function() {
  check_server_version(function(version) {
    if (version < 1)
      alert("Server version to low website won't work");
  });
  var main = new Main();
  window.onscroll = main.scrollHandler;
}
