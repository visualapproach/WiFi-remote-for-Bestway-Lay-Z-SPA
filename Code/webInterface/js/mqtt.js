loadMqttConfig()

function loadMqttConfig() {
    var req = new XMLHttpRequest()
    req.open('POST', '/getmqtt/')
    req.send()
    req.onreadystatechange = function () {
        if (this.readyState == 4 && this.status == 200) {
            var json = JSON.parse(req.responseText)
            console.log(json)
            document.getElementById('enableMqtt').checked = json.enableMqtt
            document.getElementById('mqttHost').value = json.mqttHost
            // document.getElementById('mqttIpAddress2').value = json.mqttIpAddress[1];
            // document.getElementById('mqttIpAddress3').value = json.mqttIpAddress[2];
            // document.getElementById('mqttIpAddress4').value = json.mqttIpAddress[3];
            document.getElementById('mqttPort').value = json.mqttPort
            document.getElementById('mqttUsername').value = json.mqttUsername
            document.getElementById('mqttPassword').value = json.mqttPassword
            document.getElementById('mqttClientId').value = json.mqttClientId
            document.getElementById('mqttBaseTopic').value = json.mqttBaseTopic
            document.getElementById('mqttTelemetryInterval').value = json.mqttTelemetryInterval
        }
    }
}

function saveMqttConfig() {
    if (!validatePassword('mqttPassword')) return

    buttonConfirm(document.getElementById('save'))

    var req = new XMLHttpRequest()
    req.open('POST', '/setmqtt/')
    var json = {
        'enableMqtt': (document.getElementById('enableMqtt').checked),
        // 'mqttIpAddress':[
        // 	parseInt(document.getElementById('mqttIpAddress1').value),
        // 	parseInt(document.getElementById('mqttIpAddress2').value),
        // 	parseInt(document.getElementById('mqttIpAddress3').value),
        // 	parseInt(document.getElementById('mqttIpAddress4').value)
        // ],
        'mqttHost': (document.getElementById('mqttHost').value),
        'mqttPort': parseInt(document.getElementById('mqttPort').value),
        'mqttUsername': (document.getElementById('mqttUsername').value),
        'mqttPassword': (document.getElementById('mqttPassword').value),
        'mqttClientId': (document.getElementById('mqttClientId').value),
        'mqttBaseTopic': (document.getElementById('mqttBaseTopic').value),
        'mqttTelemetryInterval': (document.getElementById('mqttTelemetryInterval').value)
    }
    req.send(JSON.stringify(json))
    document.getElementById('mqttPassword').value = '<enter password>'
    console.log(json)
}