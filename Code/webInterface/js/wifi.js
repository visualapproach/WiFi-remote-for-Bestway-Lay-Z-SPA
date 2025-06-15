loadNetworkConfig()

function loadNetworkConfig() {
    const req = new XMLHttpRequest()
    req.open('POST', '/getwifi/')
    req.send()
    req.onreadystatechange = function () {
        if (this.readyState == 4 && this.status == 200) {
            const json = JSON.parse(req.responseText)
            console.log(json)

            document.getElementById('enableAp').checked = json.enableAp
            document.getElementById('apSsid').value = json.apSsid
            document.getElementById('apPwd').value = json.apPwd
            document.getElementById('enableWM').checked = json.enableWM

            document.getElementById('enableStaticIp4').checked = json.enableStaticIp4
            document.getElementById('ip4Address').value = json.ip4Address
            document.getElementById('ip4Gateway').value = json.ip4Gateway
            document.getElementById('ip4Subnet').value = json.ip4Subnet
            document.getElementById('ip4DnsPrimary').value = json.ip4DnsPrimary
            document.getElementById('ip4DnsSecondary').value = json.ip4DnsSecondary
            document.getElementById('ip4NTP').value = json.ip4NTP
        }
    }
}

function saveNetworkConfig() {
    if (!validatePassword('apPwd')) return

    buttonConfirm(document.getElementById('save'), "saved &check; (Don't forget to restart the ESP.)", 6)

    var req = new XMLHttpRequest()
    req.open('POST', '/setwifi/')
    var json = {
        'enableAp': (document.getElementById('enableAp').checked),
        'apSsid': (document.getElementById('apSsid').value),
        'apPwd': (document.getElementById('apPwd').value),
        'enableWM': (document.getElementById('enableWM').checked),
        'enableStaticIp4': (document.getElementById('enableStaticIp4').checked),
        'ip4Address': (document.getElementById('ip4Address').value),
        'ip4Gateway': (document.getElementById('ip4Gateway').value),
        'ip4Subnet': (document.getElementById('ip4Subnet').value),
        'ip4DnsPrimary': (document.getElementById('ip4DnsPrimary').value),
        'ip4DnsSecondary': (document.getElementById('ip4DnsSecondary').value),
        'ip4NTP': (document.getElementById('ip4NTP').value)
    }
    req.send(JSON.stringify(json))
    document.getElementById('apPwd').value = '<enter password>'
    console.log(json)
}

function resetWifi() {
    if (confirm('Do you really want to reset the WiFi?')) {
        var req = new XMLHttpRequest()
        req.open('POST', '/resetwifi/')
        req.send()
        document.body.innerHTML = 'ESP restarts now. Please connect to the access point and re-configure your WiFi.'
    }
}