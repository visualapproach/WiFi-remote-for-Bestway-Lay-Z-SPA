// the web socket connection
var connection;

// command mapping
const cmdMap = {
	setTarget: 0,
	setTargetSelector: 0,
	toggleUnit: 1,
	toggleBubbles: 2,
	toggleHeater: 3,
	togglePump: 4,
	//resetq: 5,
	restartEsp: 6,
	//gettarget: 7,
	resetTotals: 8,
	resetTimerChlorine: 9,
	resetTimerFilter: 10,
	toggleHydroJets: 11,
	toggleGodMode: 12,
	setAmbient: 15,
	setAmbientSelector: 15,
	setAmbientF: 14,
	setAmbientC: 15
};

// button element ID mapping
const btnMap = {
	toggleUnit: 'UNT',
	toggleBubbles: 'AIR',
	toggleHeater: 'HTR',
	togglePump: 'FLT',
	toggleHydroJets: 'JET',
	toggleGodMode: 'GOD'
};

// to be used for setting the control values once after loading original values from the web socket
var initControlValues = true;

// display brightness multiplier. lower value results lower brightness levels (1-30)
const dspBrtMultiplier = 16;

// update states
updateTempState = false;
updateAmbState = false;
//updateBrtState = false;

// initial connect to the web socket
connect();

function connect()
{
	connection = new WebSocket('ws://'+location.hostname+':81/', ['arduino']);

	connection.onopen = function()
	{
		document.getElementById('header').style = "background-color: #00508F";
		initControlValues = true;
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
	console.log(msgobj);

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
			"CONNECT_UNAUTHORIZED" // 5 / the client was not authorized to connect
		]
		document.getElementById('mqtt').innerHTML = "MQTT: " + mqtt_states[msgobj.MQTT + 4];
		document.getElementById('ciotx').innerHTML = 'CIO TX: ' + (msgobj.CIOTX ? 'Active' : 'Dead');
		document.getElementById('dsptx').innerHTML = 'DSP TX: ' + (msgobj.DSPTX ? 'Active' : 'Dead');
		document.getElementById('fw').innerHTML = 'Firmware: ' + msgobj.FW;
		document.getElementById('model').innerHTML = 'Model: ' + msgobj.MODEL;
		document.getElementById('rssi').innerHTML = "RSSI: " + msgobj.RSSI;

		document.getElementById('jetstitle').style.display = (msgobj.HASJETS ? 'inherit' : 'none');
		document.getElementById('jetsbutton').style.display = (msgobj.HASJETS ? 'inherit' : 'none');
		document.getElementById('jetstotals').style.display = (msgobj.HASJETS ? 'table-row' : 'none');

		document.getElementById('airtitle').style.display = (msgobj.HASAIR ? 'inherit' : 'none');
		document.getElementById('airbutton').style.display = (msgobj.HASAIR ? 'inherit' : 'none');
		document.getElementById('airtotals').style.display = (msgobj.HASAIR ? 'table-row' : 'none');
	}

	if (msgobj.CONTENT == "STATES")
	{
		// temperature
		document.getElementById('atlabel').innerHTML = (msgobj.UNT ? msgobj.TMP.toString() : String(msgobj.TMP * 1.8 + 32));
		document.getElementById('vtlabel').innerHTML = (msgobj.UNT ? msgobj.VTM.toFixed(2).toString() : String(msgobj.VTM * 1.8 + 32).toFixed(2).toString());
		document.getElementById('ttlabel').innerHTML = (msgobj.UNT ? msgobj.TGT.toString() : String(msgobj.TGT * 1.8 + 32));

		// buttons
		document.getElementById('AIR').checked = msgobj.AIR;
		if (document.getElementById('UNT').checked != msgobj.UNT)
		{
			document.getElementById('UNT').checked = msgobj.UNT;
			initControlValues = true;
		}
		document.getElementById('FLT').checked = msgobj.FLT;
		document.getElementById('JET').checked = msgobj.JET;
		document.getElementById('GOD').checked = msgobj.GOD;
		document.getElementById('HTR').checked = msgobj.RED || msgobj.GRN;

		// heater button color
		document.getElementById('htrspan').classList.remove('heateron');
		document.getElementById('htrspan').classList.remove('heateroff');
		if (msgobj.RED || msgobj.GRN)
		{
			document.getElementById('htrspan').classList.add((msgobj.RED) ? 'heateron' : ((msgobj.GRN) ? 'heateroff' : 'n-o-n-e'));
		}
		// var heaterColor = "CCC";
		// if (msgobj.GRN == 2) heaterColor = "000";
		// else if (msgobj.RED == 1) heaterColor = "FF0000";
		// else if (msgobj.GRN == 1) heaterColor = "00FF00";
		// document.getElementById('htrspan').style = "background-color: #" + heaterColor;

		// display
		document.getElementById('display').innerHTML = "[" + String.fromCharCode(msgobj.CH1,msgobj.CH2,msgobj.CH3)+ "]";
		//document.getElementById('display').style.color = rgb((255-(dspBrtMultiplier*8))+(dspBrtMultiplier*(parseInt(msgobj.BRT)+1)), 0, 0);

		// set control values (once)
		if (initControlValues)
		{
			var minTemp = (msgobj.UNT ? 20 : 68);
			var maxTemp = (msgobj.UNT ? 40 : 104);
			var minAmb = (msgobj.UNT ? -10 : 14);
			var maxAmb = (msgobj.UNT ? 50 : 122);
			document.getElementById('temp').min = minTemp;
			document.getElementById('temp').max = maxTemp;
			document.getElementById('selectorTemp').min = minTemp;
			document.getElementById('selectorTemp').max = maxTemp;
			document.getElementById('amb').min = minAmb;
			document.getElementById('amb').max = maxAmb;
			document.getElementById('selectorAmb').min = minAmb;
			document.getElementById('selectorAmb').max = maxAmb;

			document.getElementById('temp').value = msgobj.TGT;
			document.getElementById('amb').value = msgobj.AMB;
			//document.getElementById('brt').value = msgobj.BRT;

			initControlValues = false;
		}

		document.getElementById('sliderTempVal').innerHTML = msgobj.TGT;
		document.getElementById('sliderAmbVal').innerHTML = msgobj.AMB;
		//document.getElementById('sliderBrtVal').innerHTML = msgobj.BRT;

		// get selector elements
		var elemSelectorTemp = document.getElementById('selectorTemp');
		var elemSelectorAmb = document.getElementById('selectorAmb');
		//var elemSelectorBrt = document.getElementById('selectorBrt');

		// change values only if element is not active (selected for input)
		// also change only if an update is not in progress
		if (document.activeElement !== elemSelectorTemp && !updateTempState) elemSelectorTemp.value = msgobj.TGT;
		if (document.activeElement !== elemSelectorAmb && !updateAmbState) elemSelectorAmb.value = msgobj.AMB;
		//if (document.activeElement !== elemSelectorBrt && !updateBrtState) elemSelectorBrt.value = msgobj.BRT;

		// reset update states when the set target matches the input
		if (elemSelectorTemp.value == msgobj.TGT) updateTempState = false;
		if (elemSelectorAmb.value == msgobj.AMB) updateAmbState = false;
		//if (elemSelectorBrt.value == msgobj.BRT) updateBrtState = false;
	}

	if (msgobj.CONTENT == "TIMES")
	{
		var date = new Date(msgobj.TIME * 1000);
		document.getElementById('time').innerHTML = date.toLocaleString();

		// chlorine add reset timer
		var clDate = (Date.now()/1000-msgobj.CLTIME)/(24*3600.0);
		var clDateRound = Math.round(clDate);
		document.getElementById('cltimer').innerHTML = clDateRound + ' day' + (clDateRound != 1 ? 's' : '');
		document.getElementById('cltimerbtn').className = (clDate > msgobj.CLINT ? 'button_red' : 'button');

		// filter change reset timer
		var fDate = (Date.now()/1000-msgobj.FTIME)/(24*3600.0);
		var fDateRound = Math.round(fDate);
		document.getElementById('ftimer').innerHTML = fDateRound + ' day' + (fDateRound != 1 ? 's' : '');
		document.getElementById('ftimerbtn').className = (fDate > msgobj.FINT ? 'button_red' : 'button');

		// statistics
		document.getElementById('heatingtime').innerHTML = s2dhms(msgobj.HEATINGTIME);
		document.getElementById('uptime').innerHTML = s2dhms(msgobj.UPTIME);
		document.getElementById('airtime').innerHTML = s2dhms(msgobj.AIRTIME);
		document.getElementById('filtertime').innerHTML = s2dhms(msgobj.PUMPTIME);
		document.getElementById('jettime').innerHTML = s2dhms(msgobj.JETTIME);
		document.getElementById('cost').innerHTML = (msgobj.COST).toFixed(2);
		document.getElementById('t2r').innerHTML = (msgobj.T2R);
		document.getElementById('tttt').innerHTML = (msgobj.TTTT/3600).toFixed(2) + 'h<br>(' + new Date(msgobj.TIME * 1000 + msgobj.TTTT * 1000).toLocaleString() + ')';
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

function sendCommand(cmd)
{
	// check command
	if (typeof(cmdMap[cmd]) == 'undefined')
	{
		console.log("invalid command");
		return;
	}

	// get the current unit (true=C, false=F)
	var unit = (document.getElementById('UNT').checked);

	// get and set value
	var value = 0;
	if (cmd == 'setTarget' || cmd == 'setTargetSelector')
	{
		value = parseInt(document.getElementById((cmd == 'setTarget') ? 'temp' : 'selectorTemp').value);
		value = getProperValue(value, (unit ? 20 : 68), (unit ? 40 : 104));
		document.getElementById('sliderTempVal').innerHTML = value.toString();
		document.getElementById('selectorTemp').value = value.toString();
		updateTempState = true;

		document.getElementById('ttlabel').innerHTML = value.toString();
	}
	else if (cmd == 'setAmbient' || cmd == 'setAmbientSelector')
	{
		value = parseInt(document.getElementById((cmd == 'setAmbient') ? 'amb' : 'selectorAmb').value);
		value = getProperValue(value, (unit ? -10 : 14), (unit ? 50 : 122));
		document.getElementById('sliderAmbVal').innerHTML = value.toString();
		document.getElementById('selectorAmb').value = value.toString();
		cmd = 'setAmbient' + (unit ? 'C' : 'F');
		updateAmbState = true;
	}
	// else if (cmd == 'setBrightness' || cmd == 'setBrightnessSelector')
	// {
	// 	value = parseInt(document.getElementById((cmd == 'setBrightness') ? 'brt' : 'selectorBrt').value);
	// 	value = getProperValue(value, 0, 8);
	// 	document.getElementById("sliderBrtVal").innerHTML = value.toString();
	// 	document.getElementById('selectorBrt').value = value.toString();
	// 	document.getElementById("display").style.color = rgb((255-(dspBrtMultiplier*8))+(dspBrtMultiplier*(value+1)), 0, 0);
	// 	updateBrtState = true;
	// }
	else if (val == 'toggleUnit')
	{
		var tmp;
		tmp = parseInt(document.getElementById('temp').value);
		tmp = Math.floor(unit ? String((tmp - 32) / 1.8) : String(tmp * 1.8 + 32));
		document.getElementById('temp').min = (unit ? 20 : 68);
		document.getElementById('temp').max = (unit ? 40 : 104);
		document.getElementById('temp').value = tmp;
		document.getElementById('ttlabel').innerHTML = tmp;
		document.getElementById("sliderTempVal").innerHTML = tmp;
	}
	else if (btnMap[cmd] && (cmd == 'toggleBubbles' || cmd == 'toggleHeater' || cmd == 'togglePump' || cmd == 'toggleHydroJets' || cmd == 'toggleGodMode'))
	{
		value = document.getElementById(btnMap[cmd]).checked;
	}

	var obj = {};
	obj["CMD"] = cmdMap[cmd];
	obj["VALUE"] = value;
	obj["XTIME"] = 0;
	obj["INTERVAL"] = 0;

	var json = JSON.stringify(obj);
	connection.send(json);
	console.log(json);
}

function getProperValue(val, min, max)
{
	return (val < min ? min : (val > max ? max : val));
}

function rgb(r, g, b)
{
	r = Math.floor(r);
	g = Math.floor(g);
	b = Math.floor(b);
	return ["rgb(",r,",",g,",",b,")"].join("");
}
