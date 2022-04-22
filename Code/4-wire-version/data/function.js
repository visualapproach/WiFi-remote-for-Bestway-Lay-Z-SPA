
function topNav() {
  var x = document.getElementById("topnav");
  if (x.className === "topnav") {
    x.className += " responsive";
  } else {
    x.className = "topnav";
  }
}

function togglePlainText(id) {
  var x = document.getElementById(id);
  if (x.type === "password") {
    x.type = "text";
  } else {
    x.type = "password";
  }
}

function validatePassword(id) {
  var x = document.getElementById(id);
  if (x.value == "<enter password>") {
    alert("Please enter a password to continue.");
    return false;
  }
  return true;
}
