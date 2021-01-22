//
var connection;
connect();

function connect(){
	connection = new WebSocket('ws://'+location.hostname+':81/', ['arduino']);
	connection.onopen = function () { 
		document.getElementById('header').style = "background-color: #00508F";
	};
	connection.onerror = function (error) {
		console.log('WebSocket Error ', error);
		document.getElementById('header').style = "background-color: #FF0000";
		connection.close();
	};
	connection.onclose = function(){
		console.log('WebSocket connection closed, reconnecting in 5 s');
		document.getElementById('header').style = "background-color: #FF0000";
		setTimeout(function() {
			connect();}, 5000);
	};	
	connection.onmessage = function(e){
		handlemsg(e);}
} 


String.prototype.pad = function(String, len) { 
                var str = this; 
                while (str.length < len) 
                    str = String + str; 
                return str; 
            }

function handlemsg(e) {  
	var msgobj = JSON.parse(e.data);
	var mycolor;
	console.log(msgobj);
	
/*     
	doc["LCK"] = _cio.states[LOCKEDSTATE];
    doc["PWR"] = _cio.states[POWERSTATE];
    doc["UNT"] = _cio.states[UNITSTATE];
    doc["AIR"] = _cio.states[BUBBLESSTATE];
    doc["GRN"] = _cio.states[HEATGRNSTATE];
    doc["RED"] = _cio.states[HEATREDSTATE];
    doc["FLT"] = _cio.states[PUMPSTATE];
    doc["TGT"] = _cio.states[TARGET];
    doc["TMP"] = _cio.states[TEMPERATURE];
    doc["CH1"] = _cio.states[CHAR1];
    doc["CH2"] = _cio.states[CHAR2];
    doc["CH3"] = _cio.states[CHAR3]; */
	if(msgobj.CONTENT == "OTHER"){
		//if(msgobj.MQTT){
			document.getElementById('mqtt').innerHTML = "MQTT:" + msgobj.MQTT.toString();
		//}
	}
	if(msgobj.CONTENT == "STATES"){
		var element = document.getElementById('temp');
		if (element.value == 0) element.value = msgobj.TGT;
		document.getElementById('sliderlabel').innerHTML = element.value.toString();
		document.getElementById('AIR').checked = msgobj.AIR;
		document.getElementById('UNT').checked = msgobj.UNT;
		document.getElementById('GOD').checked = msgobj.GOD;
		if(msgobj.UNT){
			document.getElementById('atlabel').innerHTML = msgobj.TMP.toString();
			document.getElementById('ttlabel').innerHTML = msgobj.TGT.toString();
		} else {
			document.getElementById('atlabel').innerHTML = String(msgobj.TMP * 1.8 + 32);
			document.getElementById('ttlabel').innerHTML = String(msgobj.TGT * 1.8 + 32);
		}
		if(msgobj.RED) {
			mycolor = "background-color: #FF0000";
			}
		else {
			mycolor = "background-color: #00FF00";
		}
		if(!(msgobj.RED || msgobj.GRN)) mycolor = "background-color: #CCC";
		//2 is unknown (shown as black)
		if(msgobj.GRN == 2) mycolor = "background-color: #000";

		document.getElementById('HTR').checked = msgobj.RED;
		document.getElementById('htrspan').style = mycolor;
		document.getElementById('FLT').checked = msgobj.FLT;
		if(document.getElementById('UNT').checked){
			document.getElementById('temp').min = 20;
			document.getElementById('temp').max = 40;
		} else {
			document.getElementById('temp').min = 68;
			document.getElementById('temp').max = 104;
		}
		document.getElementById('dsp').innerHTML = "[" + String.fromCharCode(msgobj.CH1,msgobj.CH2,msgobj.CH3)+ "]";
	}
	if(msgobj.CONTENT == "TIMES"){
		var date = new Date(msgobj.TIME * 1000);
		document.getElementById('time').innerHTML = date.toLocaleString();
		
		var d = (Date.now()/1000-msgobj.CLTIME)/(24*3600.0);
		if(d > msgobj.CLINT) {
			mycolor = "background-color:#"+"900";
		} else {
			mycolor = "background-color:#"+"999";
		}
		document.getElementById('cltimer').innerHTML = d.toFixed(2);
		document.getElementById('cltimerbtn').style = mycolor;

		var df = (Date.now()/1000-msgobj.FTIME)/(24*3600.0);
		if(df > msgobj.FINT) {
			mycolor = "background-color:#"+"900";
		} else {
			mycolor = "background-color:#"+"999";
		}
		document.getElementById('ftimer').innerHTML = df.toFixed(2);
		document.getElementById('ftimerbtn').style = mycolor;
		
		document.getElementById('heatingtime').innerHTML = s2dhms(msgobj.HEATINGTIME);			
		document.getElementById('uptime').innerHTML = s2dhms(msgobj.UPTIME);		
		document.getElementById('airtime').innerHTML = s2dhms(msgobj.AIRTIME);		
		document.getElementById('filtertime').innerHTML = s2dhms(msgobj.PUMPTIME);		
		document.getElementById('cost').innerHTML = (msgobj.COST).toFixed(2);		
	}
	
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

const settarget = 0;
const setunit = 1;
const setbubbles = 2;
const setheat = 3;
const setpump = 4;
const resetq = 5;
const rebootesp = 6;
const gettarget = 7;
const resettimes = 8;
const resetcltimer = 9;
const resetftimer = 10;
const setjets = 11;
const setgodmode = 12;

function air() {
	var sendobj = {};
	sendobj["CMD"] = setbubbles;
	sendobj["VALUE"] = document.getElementById('AIR').checked;
	sendobj["XTIME"] = 0; //Math.floor(Date.now()/1000);
	sendobj["INTERVAL"] = 0;
	connection.send(JSON.stringify(sendobj));
	console.log(JSON.stringify(sendobj));
}
function unt() {
	var sendobj = {};
	sendobj["CMD"] = setunit;
	sendobj["VALUE"] = document.getElementById('UNT').checked;
	sendobj["XTIME"] = 0; //Math.floor(Date.now()/1000);
	sendobj["INTERVAL"] = 0;
	connection.send(JSON.stringify(sendobj));
	console.log(JSON.stringify(sendobj));
	alert("Setting is only changing the web interface.\n Change also required on the pump display.");
}
function htr() {
	var sendobj = {};
	sendobj["CMD"] = setheat;
	sendobj["VALUE"] = document.getElementById('HTR').checked;
	sendobj["XTIME"] = 0; //Math.floor(Date.now()/1000);
	sendobj["INTERVAL"] = 0;
	connection.send(JSON.stringify(sendobj));
	console.log(JSON.stringify(sendobj));
}
function flt() {
	var sendobj = {};
	sendobj["CMD"] = setpump;
	sendobj["VALUE"] = document.getElementById('FLT').checked;
	sendobj["XTIME"] = 0; //Math.floor(Date.now()/1000);
	sendobj["INTERVAL"] = 0;
	connection.send(JSON.stringify(sendobj));
	console.log(JSON.stringify(sendobj));
}

function god() {
	var sendobj = {};
	sendobj["CMD"] = setgodmode;
	sendobj["VALUE"] = document.getElementById('GOD').checked;
	sendobj["XTIME"] = 0; //Math.floor(Date.now()/1000);
	sendobj["INTERVAL"] = 0;
	connection.send(JSON.stringify(sendobj));
	console.log(JSON.stringify(sendobj));
	alert("Warning! Control is now given to ESP module. \nFactory safety guards might fail.");
}

function jet() {
	var sendobj = {};
	sendobj["CMD"] = setjets;
	sendobj["VALUE"] = document.getElementById('JET').checked;
	sendobj["XTIME"] = 0; //Math.floor(Date.now()/1000);
	sendobj["INTERVAL"] = 0;
	connection.send(JSON.stringify(sendobj));
	console.log(JSON.stringify(sendobj));
}

function tempchange() {
	var sendobj = {};
	sendobj["CMD"] = settarget;
	var mytemp = parseInt(document.getElementById('temp').value);
	if(!document.getElementById('UNT').checked) {
		mytemp = Math.floor((mytemp - 32) / 1.8);
	}
	sendobj["VALUE"] = mytemp;  //in Celsius
	sendobj["XTIME"] = 0; //Math.floor(Date.now()/1000);
	sendobj["INTERVAL"] = 0;
	connection.send(JSON.stringify(sendobj));
	console.log(JSON.stringify(sendobj));
}

function sliderchange(){
	document.getElementById("sliderlabel").innerHTML = document.getElementById('temp').value.toString();
}

function clTimer() {
	var sendobj = {};
	sendobj["CMD"] = resetcltimer;
	sendobj["VALUE"] = 0;
	sendobj["XTIME"] = 0;
	sendobj["INTERVAL"] = 0;
	connection.send(JSON.stringify(sendobj));
	console.log(JSON.stringify(sendobj));
}

function fTimer() {
	var sendobj = {};
	sendobj["CMD"] = resetftimer;
	sendobj["VALUE"] = 0;
	sendobj["XTIME"] = 0;
	sendobj["INTERVAL"] = 0;
	connection.send(JSON.stringify(sendobj));
	console.log(JSON.stringify(sendobj));
}
function reboot() {
	var sendobj = {};
	sendobj["CMD"] = rebootesp;
	sendobj["VALUE"] = 0;
	sendobj["XTIME"] = 0;
	sendobj["INTERVAL"] = 0;
	connection.send(JSON.stringify(sendobj));
	console.log(JSON.stringify(sendobj));
}

function zero() {
	var sendobj = {};
	sendobj["CMD"] = resettimes;
	sendobj["VALUE"] = 0;
	sendobj["XTIME"] = 0;
	sendobj["INTERVAL"] = 0;
	connection.send(JSON.stringify(sendobj));
	console.log(JSON.stringify(sendobj));
}

