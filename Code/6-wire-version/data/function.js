
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

function increaseNumber(id) {
	var x = document.getElementById(id);
	var val = Number(x.value);
	var max = x.max;
  if (max > val) {
    val += 1;
    x.value = val;
  }
}

function decreaseNumber(id) {
	var x = document.getElementById(id);
	var val = Number(x.value);
	var min = x.min;
  if (min < val) {
    val -= 1;
    x.value = val;
   }
}

function buttonConfirm(elem, text='', timeout=3, reset=true)
{
	var originalText = elem.innerHTML;

	elem.innerHTML = (text == '' ? '&check;' : text);
	elem.disabled = true;

	if (reset)
	{
		setTimeout(function(){
			elem.innerHTML = originalText;
			elem.disabled = false;
		}, timeout*1000);
	}
}
