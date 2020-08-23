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
	console.log(e.data);
    console.log('Temp: ', msgobj.temp);
    console.log('Target temp: ', msgobj.target);
	console.log('timer ', msgobj.timer);
	document.getElementById('temp').value = msgobj.temp;
	document.getElementById('target').value = msgobj.target;
	document.getElementById('locked').style = "background-color:#"+msgobj.locked*9+"00";
	document.getElementById('air').style = "background-color:#0"+msgobj.air*9+"0";
	if(msgobj.celsius == 1) document.getElementById('unit').innerHTML = "Celsius"
	else document.getElementById('unit').innerHTML = "Frankenstein";
	document.getElementById('heater').style = "background-color:#"+msgobj.heating*9+""+msgobj.heater*9+"0";
	document.getElementById('filter').style = "background-color:#0"+msgobj.filter*9+"0";
	document.getElementById('power').style = "background-color:#0"+msgobj.power*9+"0";
	document.getElementById('targetlabel').innerHTML = msgobj.target;
	document.getElementById('actuallabel').innerHTML = msgobj.temp;
	document.getElementById('time').innerHTML = msgobj.time;
	document.getElementById('cltimer').innerHTML = msgobj.timer;
	document.getElementById('heater').innerHTML = 'Heater ('+(msgobj.ratio*100).toFixed()+'%)';
};
connection.onclose = function(){
    console.log('WebSocket connection closed');
};

function sendCommand(e) {
	var s = document.getElementById('target').value.toString(16);
    connection.send(s);
	console.log(s);
	document.getElementById('targetlabel').innerHTML = document.getElementById('target').value.toString();
	document.getElementById('actuallabel').innerHTML = document.getElementById('temp').value.toString();
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
	//document.getElementById('5').style.backgroundColor = '#FF0000';
	connection.send('5');
}
function clTimer() {
	//document.getElementById('5').style.backgroundColor = '#FF0000';
	connection.send('10');
}
