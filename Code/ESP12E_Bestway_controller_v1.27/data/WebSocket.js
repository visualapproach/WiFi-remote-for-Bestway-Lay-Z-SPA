//
var connection = new WebSocket('ws://'+location.hostname+':81/', ['arduino']);
var temp = 0;
var target = 0;

String.prototype.pad = function(String, len) { 
                var str = this; 
                while (str.length < len) 
                    str = String + str; 
                return str; 
            }

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
	
	document.getElementById('heatingtime').innerHTML = 'Acc. Heating time: '+s2dhms(msgobj.heattime);	
	
	document.getElementById('uptime').innerHTML = 'Acc. Up time: '+s2dhms(msgobj.uptime);
	
	document.getElementById('airtime').innerHTML = 'Acc. Air time: '+s2dhms(msgobj.airtime);
	
	document.getElementById('filtertime').innerHTML = 'Acc. Filter time: '+s2dhms(msgobj.filtertime);
	
	document.getElementById('cost').innerHTML = 'Est. acc. cost: '+(msgobj.cost).toFixed(2);
	
	document.getElementById('auto').checked = msgobj.auto;
};
connection.onclose = function(){
    console.log('WebSocket connection closed');
};

function s2dhms(val) {
	var day = 3600*24;
	var hour = 3600;
	var minute = 60;
	var rem;
	var days = Math.floor(val/day);
	rem = val % day;
	var hours = Math.floor(rem/hour);
	rem = val % hour;
	var minutes = Math.floor(rem/minute);
	rem = val % minute;
	var seconds = Math.floor(rem);
	return days + "d " + hours.toString().pad("0", 2) + ":" + minutes.toString().pad("0", 2) + ":" + seconds.toString().pad("0", 2);
}

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

function zero() {
	connection.send('3');
}
