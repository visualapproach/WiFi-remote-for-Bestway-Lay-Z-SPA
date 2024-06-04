/*
    THIS FILE IS NOT USED ANYMORE. IT IS MERGED INTO INDEX.HTML FILE
*/






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
  resetTimerReplaceFilter: 10,
  toggleHydroJets: 11,
  setBrightness: 12,
  setBrightnessSelector: 12,
  setBeep: 13,
  setAmbient: 15,
  setAmbientSelector: 15,
  setAmbientF: 14,
  setAmbientC: 15,
  resetDaily: 16,
  toggleGodmode: 17,
  setFullpower: 18,
  printText: 19,
  setReady: 20,
  setR: 21,
  resetTimerRinseFilter: 22,
  resetTimerCleanFilter: 23
};

// button element ID mapping
const btnMap = {
  toggleUnit: "UNT",
  toggleBubbles: "AIR",
  toggleHeater: "HTR",
  togglePump: "FLT",
  toggleHydroJets: "HJT",
  toggleGodmode: "GOD"
};

// to be used for setting the control values once after loading original values from the web socket
var initControlValues = true;

// display brightness multiplier. lower value results lower brightness levels (1-30)
const dspBrtMultiplier = 16;

// update states
updateTempState = false;
updateAmbState = false;
updateBrtState = false;

// initial connect to the web socket
// connect();

function connect() {
  connection = new WebSocket("ws://" + location.hostname + ":81/", ["arduino"]);

  connection.onopen = function () {
    document.body.classList.add("connected");
    initControlValues = true;
  };

  connection.onerror = function (error) {
    console.log("WebSocket Error ", error);
    document.body.classList.add("error");
    connection.close();
  };

  connection.onclose = function () {
    console.log("WebSocket connection closed, reconnecting in 5 s");
    document.body.classList.add("error");
    setTimeout(function () {
      connect();
    }, 5000);
  };

  connection.onmessage = function (e) {
    handlemsg(e);
  };
}

String.prototype.pad = function (String, len) {
  var str = this;
  while (str.length < len) {
    str = String + str;
  }
  return str;
};

function tryParseJSONObject(jsonString) {
  try {
    var o = JSON.parse(jsonString);

    // Handle non-exception-throwing cases:
    // Neither JSON.parse(false) or JSON.parse(1234) throw errors, hence the type-checking,
    // but... JSON.parse(null) returns null, and typeof null === "object",
    // so we must check for that, too. Thankfully, null is falsey, so this suffices:
    if (o && typeof o === "object") {
      return o;
    }
  } catch (e) {}

  return false;
}

function handlemsg(e) {
  console.log(e.data);
  var msgobj = tryParseJSONObject(e.data);
  if (!msgobj) return;
  console.log(msgobj);

  if (msgobj.CONTENT == "OTHER") {
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
      "CONNECT_UNAUTHORIZED", // 5 / the client was not authorized to connect
    ];
    document.getElementById("mqtt").innerHTML = "MQTT: " + mqtt_states[msgobj.MQTT + 4];
    document.getElementById("fw").innerHTML = "Firmware: " + msgobj.FW;
    document.getElementById("model").innerHTML = "Model: " + msgobj.MODEL;
    document.getElementById("rssi").innerHTML = "RSSI: " + msgobj.RSSI;

    // hydro jets available
    document.getElementById("jets").style.display = msgobj.HASJETS ? "table-cell" : "none";
    document.getElementById("jetsswitch").style.display = msgobj.HASJETS ? "table-cell" : "none";
    document.getElementById("jetstotals").style.display = msgobj.HASJETS ? "table-cell" : "none";
    // godmode available
    document.getElementById("god").style.display = msgobj.HASGOD ? "table-cell" : "none";
    document.getElementById("godswitch").style.display = msgobj.HASGOD ? "table-cell" : "none";
  }

  if (msgobj.CONTENT == "STATES") {
    // temperature
    document.getElementById("atlabel").innerHTML = msgobj.TMP.toString();
    document.getElementById("vtlabel").innerHTML = msgobj.VTM.toFixed(2).toString();
    document.getElementById("ttlabel").innerHTML = msgobj.TGT.toString();

    // buttons
    document.getElementById("AIR").checked = msgobj.AIR;
    if (document.getElementById("UNT").checked != msgobj.UNT) {
      document.getElementById("UNT").checked = msgobj.UNT;
      initControlValues = true;
    }
    document.getElementById("FLT").checked = msgobj.FLT;
    document.getElementById("HJT").checked = msgobj.HJT;
    document.getElementById("GOD").checked = msgobj.GOD;
    document.getElementById("HTR").checked = msgobj.RED || msgobj.GRN;

    // heater button color
    document.getElementById("htrspan").classList.remove("heateron");
    document.getElementById("htrspan").classList.remove("heateroff");
    if (msgobj.RED || msgobj.GRN) {
      document.getElementById("htrspan").classList.add(msgobj.RED ? "heateron" : msgobj.GRN ? "heateroff" : "n-o-n-e");
    }

    // display
    document.getElementById("display").innerHTML = "[" + String.fromCharCode(msgobj.CH1, msgobj.CH2, msgobj.CH3) + "]";
    document.getElementById("display").style.color = rgb(255 - dspBrtMultiplier * 8 + dspBrtMultiplier * (parseInt(msgobj.BRT) + 1), 0, 0);

    // set control values (once)
    if (initControlValues) {
      var minTemp = msgobj.UNT ? 20 : 68;
      var maxTemp = msgobj.UNT ? 40 : 104;
      var minAmb = msgobj.UNT ? -40 : -40;
      var maxAmb = msgobj.UNT ? 60 : 140;
      document.getElementById("temp").min = minTemp;
      document.getElementById("temp").max = maxTemp;
      document.getElementById("selectorTemp").min = minTemp;
      document.getElementById("selectorTemp").max = maxTemp;
      document.getElementById("amb").min = minAmb;
      document.getElementById("amb").max = maxAmb;
      document.getElementById("selectorAmb").min = minAmb;
      document.getElementById("selectorAmb").max = maxAmb;

      document.getElementById("temp").value = msgobj.TGT;
      document.getElementById("amb").value = msgobj.AMB;
      document.getElementById("brt").value = msgobj.BRT;

      initControlValues = false;
    }

    document.getElementById("sliderTempVal").innerHTML = msgobj.TGT;
    document.getElementById("sliderAmbVal").innerHTML = msgobj.AMB;
    document.getElementById("sliderBrtVal").innerHTML = msgobj.BRT;

    // get selector elements
    var elemSelectorTemp = document.getElementById("selectorTemp");
    var elemSelectorAmb = document.getElementById("selectorAmb");
    var elemSelectorBrt = document.getElementById("selectorBrt");

    // change values only if element is not active (selected for input)
    // also change only if an update is not in progress
    if (document.activeElement !== elemSelectorTemp && !updateTempState) {
      elemSelectorTemp.value = msgobj.TGT;
      elemSelectorTemp.parentElement.querySelector(".numDisplay").textContent = msgobj.TGT;
    }
    if (document.activeElement !== elemSelectorAmb && !updateAmbState) {
      elemSelectorAmb.value = msgobj.AMB;
      elemSelectorAmb.parentElement.querySelector(".numDisplay").textContent = msgobj.AMB;
    }
    if (document.activeElement !== elemSelectorBrt && !updateBrtState) elemSelectorBrt.value = msgobj.BRT;

    // reset update states when the set target matches the input
    if (elemSelectorTemp.value == msgobj.TGT) updateTempState = false;
    if (elemSelectorAmb.value == msgobj.AMB) updateAmbState = false;
    if (elemSelectorBrt.value == msgobj.BRT) updateBrtState = false;
  }

  if (msgobj.CONTENT == "TIMES") {
    var date = new Date(msgobj.TIME * 1000);
    document.getElementById("time").innerHTML = date.toLocaleString();

    // chlorine add reset timer
    var clDate = (Date.now() / 1000 - msgobj.CLTIME) / (24 * 3600.0);
    var clDateRound = Math.round(clDate);
    document.getElementById("cltimer").innerHTML = clDateRound + " day" + (clDateRound != 1 ? "s" : "");
    document.getElementById("cltimerbtn").className = clDate > msgobj.CLINT ? "button_red" : "button";

    // filter change reset timer
    var fDate = (Date.now() / 1000 - msgobj.FREP) / (24 * 3600.0);
    var fDateRound = Math.round(fDate);
    document.getElementById("freplacetimer").innerHTML = fDateRound + " day" + (fDateRound != 1 ? "s" : "");
    document.getElementById("freplacetimerbtn").className = fDate > msgobj.FREPI ? "button_red" : "button";

    // filter clean reset timer
    var fDate = (Date.now() / 1000 - msgobj.FCLE) / (24 * 3600.0);
    var fDateRound = Math.round(fDate);
    document.getElementById("fcleantimer").innerHTML = fDateRound + " day" + (fDateRound != 1 ? "s" : "");
    document.getElementById("fcleantimerbtn").className = fDate > msgobj.FCLEI ? "button_red" : "button";

    // filter rinse reset timer
    var fDate = (Date.now() / 1000 - msgobj.FRIN) / (24 * 3600.0);
    var fDateRound = Math.round(fDate);
    document.getElementById("frinsetimer").innerHTML = fDateRound + " day" + (fDateRound != 1 ? "s" : "");
    document.getElementById("frinsetimerbtn").className = fDate > msgobj.FRINI ? "button_red" : "button";

    // statistics
    document.getElementById("heatingtime").innerHTML = s2dhms(msgobj.HEATINGTIME);
    document.getElementById("uptime").innerHTML = s2dhms(msgobj.UPTIME);
    document.getElementById("airtime").innerHTML = s2dhms(msgobj.AIRTIME);
    document.getElementById("filtertime").innerHTML = s2dhms(msgobj.PUMPTIME);
    document.getElementById("jettime").innerHTML = s2dhms(msgobj.JETTIME);
    document.getElementById("cost").innerHTML = msgobj.COST.toFixed(2);
    document.getElementById("t2r").innerHTML = s2dhms(msgobj.T2R * 3600) + " (" + msgobj.RS + ")";
  }
}

function s2dhms(val) {
  var day = 3600 * 24;
  var hour = 3600;
  var minute = 60;
  var rem;
  var days = Math.floor(val / day);
  rem = val % day;
  var hours = Math.floor(rem / hour);
  rem = val % hour;
  var minutes = Math.floor(rem / minute);
  rem = val % minute;
  var seconds = Math.floor(rem);
  return days + "d " + hours.toString().pad("0", 2) + ":" + minutes.toString().pad("0", 2) + ":" + seconds.toString().pad("0", 2);
}

function sendCommand(cmd) {
  console.log(cmd);
  console.log(typeof cmdMap[cmd]);
  // check command
  if (typeof cmdMap[cmd] == "undefined") {
    console.log("invalid command");
    return;
  }

  // get the current unit (true=C, false=F)
  var unit = document.getElementById("UNT").checked;

  // get and set value
  var value = 0;
  if (cmd == "setTarget" || cmd == "setTargetSelector") {
    value = parseInt(document.getElementById(cmd == "setTarget" ? "temp" : "selectorTemp").value);
    value = getProperValue(value, unit ? 20 : 68, unit ? 40 : 104);
    document.getElementById("sliderTempVal").innerHTML = value.toString();
    document.getElementById("selectorTemp").value = value.toString();
    document.getElementById("selectorTemp").setAttribute("value", value.toString());
    updateTempState = true;
  } else if (cmd == "setAmbient" || cmd == "setAmbientSelector") {
    value = parseInt(document.getElementById(cmd == "setAmbient" ? "amb" : "selectorAmb").value);
    value = getProperValue(value, unit ? -40 : -40, unit ? 60 : 140);
    document.getElementById("sliderAmbVal").innerHTML = value.toString();
    document.getElementById("selectorAmb").value = value.toString();
    cmd = "setAmbient" + (unit ? "C" : "F");
    updateAmbState = true;
  } else if (cmd == "setBrightness" || cmd == "setBrightnessSelector") {
    value = parseInt(document.getElementById(cmd == "setBrightness" ? "brt" : "selectorBrt").value);
    value = getProperValue(value, 0, 8);
    document.getElementById("sliderBrtVal").innerHTML = value.toString();
    document.getElementById("selectorBrt").value = value.toString();
    document.getElementById("display").style.color = rgb(255 - dspBrtMultiplier * 8 + dspBrtMultiplier * (value + 1), 0, 0);
    updateBrtState = true;
  } else if (btnMap[cmd] && (cmd == "toggleUnit" || cmd == "toggleBubbles" || cmd == "toggleHeater" || cmd == "togglePump" || cmd == "toggleHydroJets" || cmd == "toggleGodmode")) {
    value = document.getElementById(btnMap[cmd]).checked;
    initControlValues = true;
  }

  var obj = {};
  obj["CMD"] = cmdMap[cmd];
  obj["VALUE"] = value;
  obj["XTIME"] = Math.floor(Date.now() / 1000);
  obj["INTERVAL"] = 0;
  obj["TXT"] = "";
  var json = JSON.stringify(obj);
  connection.send(json);
  console.log(json);
}

function getProperValue(val, min, max) {
  return val < min ? min : val > max ? max : val;
}

function rgb(r, g, b) {
  r = Math.floor(r);
  g = Math.floor(g);
  b = Math.floor(b);
  return ["rgb(", r, ",", g, ",", b, ")"].join("");
}
