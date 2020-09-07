//
var connection = new WebSocket('ws://'+location.hostname+':81/', ['arduino']);
var temp = 0;
var target = 0;



connection.onopen = function () {
    connection.send('F');
};
connection.onerror = function (error) {
    console.log('WebSocket Error ', error);
};
connection.onmessage = function (e) {  
	var msgobj = JSON.parse(e.data);
	var mycolor;
	document.getElementById('atlabel').innerHTML = msgobj.temp.toString();
	document.getElementById('ttlabel').innerHTML = msgobj.target.toString();
	if(msgobj.locked){
		mycolor = "background-color:#"+"900";
	}  else {
		mycolor = "background-color:#"+"000";
	}
	document.getElementById('locked').style = mycolor;
	if(msgobj.air){
		mycolor = "background-color:#"+"090";
	}  else {
		mycolor = "background-color:#"+"000";
	}
	document.getElementById('air').style = mycolor;
	if(msgobj.celsius) document.getElementById('unit').innerHTML = "Celsius"
	else document.getElementById('unit').innerHTML = "Fahrenheit";
	if(msgobj.heater){
		mycolor = "background-color:#"+"090";
	}  else {
		mycolor = "background-color:#"+"000";
	}
	if(msgobj.heating) mycolor = "background-color:#"+"900";
	document.getElementById('heater').style = mycolor;
	
	document.getElementById('filter').style = "background-color:#0"+msgobj.filter*9+"0";
	document.getElementById('power').style = "background-color:#0"+msgobj.power*9+"0";
	document.getElementById('time').innerHTML = msgobj.time;
	var d = msgobj.clts/(24*3600.0);
	if(d > 7.0) {
		mycolor = "background-color:#"+"900";
	} else {
		mycolor = "background-color:#"+"999";
	}
	document.getElementById('cltimer').innerHTML = d.toFixed(2);
	document.getElementById('cltimerbtn').style = mycolor;
	document.getElementById('heatingtime').innerHTML = 'Heating time: '+(msgobj.heattime/3600.0).toFixed(2);
	document.getElementById('uptime').innerHTML = 'Up time: '+(msgobj.uptime/3600.0).toFixed(2);
	document.getElementById('airtime').innerHTML = 'Air time: '+(msgobj.airtime/3600.0).toFixed(2);
	document.getElementById('filtertime').innerHTML = 'Filter time: '+(msgobj.filtertime/3600.0).toFixed(2);
	document.getElementById('cost').innerHTML = 'Cost: '+(msgobj.cost).toFixed(2);
	document.getElementById('auto').checked = msgobj.auto;
};
connection.onclose = function(){
    console.log('WebSocket connection closed');
};


function lck() {
	//document.getElementById('actual').value = 38;
	connection.send('e');
}
function tmr() {
	connection.send('d');
}
function air() {
	connection.send('c');
}
function unit() {
	connection.send('b');
}
function htr() {
	connection.send('a');
}
function flt() {
	connection.send('8');
}
function down() {
	connection.send('7');
}
function up() {
	connection.send('6');
}
function pwr() {
	connection.send('5');
}
function clTimer() {
	connection.send('10');
}

function automode() {
	if(document.getElementById("auto").checked) {
		connection.send('1');
	} else {
		connection.send('2');
	}
}

function reboot() {
	connection.send('0');
}
