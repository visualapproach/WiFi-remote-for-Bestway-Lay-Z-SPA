//
var connection;
var initTargetSlider = true;
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
const setbrightness = 12;

// display brightness multiplier. lower value results lower brightness levels (1-30)
const dspBrtMultiplier = 10;


connect();

function connect()
{
	connection = new WebSocket('ws://'+location.hostname+':81/', ['arduino']);

	connection.onopen = function()
	{
		document.getElementById('header').style = "background-color: #00508F";
	};

	connection.onerror = function(error)
	{
		console.log('WebSocket Error ', error);
		document.getElementById('header').style = "background-color: #FF0000";
		connection.close();
	};

	connection.onclose = function()
	{
		console.log('WebSocket connection closed, reconnecting in 5 s');
		document.getElementById('header').style = "background-color: #FF0000";
		setTimeout(function(){connect()}, 5000);
	};

	connection.onmessage = function(e)
	{
		handlemsg(e);
	}
}


String.prototype.pad = function(String, len)
{
	var str = this;
	while (str.length < len)
	{
		str = String + str;
	}
	return str;
}

function handlemsg(e)
{
	var msgobj = JSON.parse(e.data);
	var mycolor;
	console.log(msgobj);

/*
	for my memory
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
    doc["CH3"] = _cio.states[CHAR3];
	doc["HJT"] = _cio.states[JETSSTATE];
*/

	if (msgobj.CONTENT == "OTHER")
	{
		// MQTT status
		mqtt_states = [
			"CONNECTION_TIMEOUT", // -4 / the server didn't respond within the keepalive time
			"CONNECTION_LOST", // -3 / the network connection was broken
			"CONNECT_FAILED", // -2 / the network connection failed
			"DISCONNECTED", // -1 / the client is disconnected cleanly
			"CONNECTED", // 0 / the client is connected
			"CONNECT_BAD_PROTOCOL", // 1 / the server doesn't support the requested version of MQTT
			"CONNECT_BAD_CLIENT_ID", // 2 / the server rejected the client identifier
			"CONNECT_UNAVAILABLE", // 3 / the server was unable to accept the connection
			"CONNECT_BAD_CREDENTIALS", // 4 / the username/password were rejected
			"CONNECT_UNAUTHORIZED" // 5 / e client was not authorized to connect
		]
		document.getElementById('mqtt').innerHTML = "MQTT: " + mqtt_states[msgobj.MQTT + 4];

		// hydro jets available
		document.getElementById('jets').style.display = (msgobj.HASJETS ? 'inherit' : 'none');
		document.getElementById('jetstotals').style.display = (msgobj.HASJETS ? 'inherit' : 'none');

		//console.log(msgobj.PressedButton);
	}

	if (msgobj.CONTENT == "STATES")
	{
		document.getElementById('atlabel').innerHTML = msgobj.TMP.toString();
		document.getElementById('ttlabel').innerHTML = msgobj.TGT.toString();
		var element = document.getElementById('temp');
		if (initTargetSlider)
		{
			element.value = msgobj.TGT;
		}
		initTargetSlider = false;
		document.getElementById('sliderTempVal').innerHTML = element.value.toString();
		document.getElementById('AIR').checked = msgobj.AIR;
		document.getElementById('UNT').checked = msgobj.UNT;
		document.getElementById('HTR').checked = msgobj.RED || msgobj.GRN;
		document.getElementById('htrspan').style = "background-color: #" + ((msgobj.RED) ? 'FF0000' : ((msgobj.GRN) ? '00FF00' : 'CCC'));
		document.getElementById('FLT').checked = msgobj.FLT;
		if (document.getElementById('UNT').checked)
		{
			document.getElementById('temp').min = 20;
			document.getElementById('temp').max = 40;
		}
		else {
			document.getElementById('temp').min = 68;
			document.getElementById('temp').max = 104;
		}
		document.getElementById('dsp').innerHTML = "[" + String.fromCharCode(msgobj.CH1,msgobj.CH2,msgobj.CH3)+ "]";
		document.getElementById('brt').value = msgobj.BRT;
		document.getElementById('sliderBrtVal').value = msgobj.BRT.toString();
		document.getElementById("dsp").style.color = rgb((255-(dspBrtMultiplier*8))+(dspBrtMultiplier*(parseInt(msgobj.BRT)+1)), 0, 0);
	}

	if (msgobj.CONTENT == "TIMES")
	{
		var date = new Date(msgobj.TIME * 1000);
		document.getElementById('time').innerHTML = date.toLocaleString();
		
		// chlorine add reset timer
		var clDate = (Date.now()/1000-msgobj.CLTIME)/(24*3600.0);
		document.getElementById('cltimer').innerHTML = clDate.toFixed(2);
		document.getElementById('cltimerbtn').className = (clDate > msgobj.CLINT ? "button_red" : "button");

		// filter change reset timer
		var fDate = (Date.now()/1000-msgobj.FTIME)/(24*3600.0);
		document.getElementById('ftimer').innerHTML = fDate.toFixed(2);
		document.getElementById('ftimerbtn').className = (fDate > msgobj.FINT ? "button_red" : "button");
		
		// statistics
		document.getElementById('heatingtime').innerHTML = s2dhms(msgobj.HEATINGTIME);
		document.getElementById('uptime').innerHTML = s2dhms(msgobj.UPTIME);
		document.getElementById('airtime').innerHTML = s2dhms(msgobj.AIRTIME);
		document.getElementById('filtertime').innerHTML = s2dhms(msgobj.PUMPTIME);
		document.getElementById('jettime').innerHTML = s2dhms(msgobj.JETTIME);
		document.getElementById('cost').innerHTML = (msgobj.COST).toFixed(2);
		document.getElementById('tttt').innerHTML = (msgobj.TTTT/3600).toFixed(2) + "h<br>(" + new Date(msgobj.TIME * 1000 + msgobj.TTTT * 1000).toLocaleString() + ")";
	}
};


function s2dhms(val)
{
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


function air()
{
	var sendobj = {};
	sendobj["CMD"] = setbubbles;
	sendobj["VALUE"] = document.getElementById('AIR').checked;
	sendobj["XTIME"] = Math.floor(Date.now()/1000);
	sendobj["INTERVAL"] = 0;
	connection.send(JSON.stringify(sendobj));
	console.log(JSON.stringify(sendobj));
}
function hjt()
{
	var sendobj = {};
	sendobj["CMD"] = setjets;
	sendobj["VALUE"] = document.getElementById('HJT').checked;
	sendobj["XTIME"] = Math.floor(Date.now()/1000);
	sendobj["INTERVAL"] = 0;
	connection.send(JSON.stringify(sendobj));
	console.log(JSON.stringify(sendobj));
}
function unt()
{
	var sendobj = {};
	sendobj["CMD"] = setunit;
	sendobj["VALUE"] = document.getElementById('UNT').checked;
	sendobj["XTIME"] = Math.floor(Date.now()/1000);
	sendobj["INTERVAL"] = 0;
	connection.send(JSON.stringify(sendobj));
	console.log(JSON.stringify(sendobj));
}
function htr()
{
	var sendobj = {};
	sendobj["CMD"] = setheat;
	sendobj["VALUE"] = document.getElementById('HTR').checked;
	sendobj["XTIME"] = Math.floor(Date.now()/1000);
	sendobj["INTERVAL"] = 0;
	connection.send(JSON.stringify(sendobj));
	console.log(JSON.stringify(sendobj));
}
function flt()
{
	var sendobj = {};
	sendobj["CMD"] = setpump;
	sendobj["VALUE"] = document.getElementById('FLT').checked;
	sendobj["XTIME"] = Math.floor(Date.now()/1000);
	sendobj["INTERVAL"] = 0;
	connection.send(JSON.stringify(sendobj));
	console.log(JSON.stringify(sendobj));
}
function tempchange()
{
	var sendobj = {};
	sendobj["CMD"] = settarget;
	sendobj["VALUE"] = parseInt(document.getElementById('temp').value);
	sendobj["XTIME"] = Math.floor(Date.now()/1000);
	sendobj["INTERVAL"] = 0;
	connection.send(JSON.stringify(sendobj));
	console.log(JSON.stringify(sendobj));

	document.getElementById("sliderTempVal").innerHTML = document.getElementById('temp').value.toString();
}
function brtchange()
{
	var v = document.getElementById('brt').value;
	document.getElementById("dsp").style.color = rgb((255-(dspBrtMultiplier*8))+(dspBrtMultiplier*(parseInt(v)+1)), 0, 0);
	var sendobj = {};
	sendobj["CMD"] = setbrightness;
	sendobj["VALUE"] = v;
	sendobj["XTIME"] = 0;
	sendobj["INTERVAL"] = 0;
	connection.send(JSON.stringify(sendobj));
	console.log(JSON.stringify(sendobj));

	document.getElementById("sliderBrtVal").innerHTML = document.getElementById('brt').value.toString();
}
function rgb(r, g, b)
{
	r = Math.floor(r);
	g = Math.floor(g);
	b = Math.floor(b);
	return ["rgb(",r,",",g,",",b,")"].join("");
}
function clTimer()
{
	var sendobj = {};
	sendobj["CMD"] = resetcltimer;
	sendobj["VALUE"] = 0;
	sendobj["XTIME"] = 0;
	sendobj["INTERVAL"] = 0;
	connection.send(JSON.stringify(sendobj));
	console.log(JSON.stringify(sendobj));
}
function fTimer()
{
	var sendobj = {};
	sendobj["CMD"] = resetftimer;
	sendobj["VALUE"] = 0;
	sendobj["XTIME"] = 0;
	sendobj["INTERVAL"] = 0;
	connection.send(JSON.stringify(sendobj));
	console.log(JSON.stringify(sendobj));
}
function reboot()
{
	var sendobj = {};
	sendobj["CMD"] = rebootesp;
	sendobj["VALUE"] = 0;
	sendobj["XTIME"] = 0;
	sendobj["INTERVAL"] = 0;
	connection.send(JSON.stringify(sendobj));
	console.log(JSON.stringify(sendobj));
}
function zero()
{
	var sendobj = {};
	sendobj["CMD"] = resettimes;
	sendobj["VALUE"] = 0;
	sendobj["XTIME"] = 0;
	sendobj["INTERVAL"] = 0;
	connection.send(JSON.stringify(sendobj));
	console.log(JSON.stringify(sendobj));
}
