setPins()
loadHardwareConfig()

function onOverrideClick() {
    let checked = document.getElementById("pwr_override").checked;
    [...document.querySelectorAll("input[id^='pwr_']")]
        .forEach((e) => {
            if (e.type !== "checkbox") {
                e.disabled = !checked
            }
        })
}

function setPins() {
    var cio = document.querySelector("input[name='cio_model']:checked").value
    var dsp = document.querySelector("input[name='dsp_model']:checked").value
    var pcb = document.querySelector("input[name='pcb']:checked").value
    var p1, p2, p3, p4, p5, p6, p7, p8
    p8 = null

    switch (pcb) {
        case 'v1':
            if (cio == 0 || cio == 1 || cio == 2 || cio == 3) {
                p1 = 7
                p2 = 2
                p3 = 1
            }
            if (cio == 4 || cio == 5 || cio == 6 || cio == 7 || cio == 8) {
                p1 = 3
                p2 = 2
                p3 = null
            }
            if (dsp == 0 || dsp == 1 || dsp == 2 || dsp == 3) {
                p4 = 5
                p5 = 4
                p6 = 3
                p7 = 6
            }
            if (dsp == 4 || dsp == 5 || dsp == 6 || dsp == 7 || dsp == 8) {
                p4 = 6
                p5 = 7
                p6 = null
                p7 = null
            }
            break
        case 'v2':
            if (cio == 0 || cio == 1 || cio == 2 || cio == 3) {
                p1 = 1
                p2 = 2
                p3 = 3
            }
            if (cio == 4 || cio == 5 || cio == 6 || cio == 7 || cio == 8) {
                p1 = 1
                p2 = 2
                p3 = null
            }
            if (dsp == 0 || dsp == 1 || dsp == 2 || dsp == 3) {
                p4 = 4
                p5 = 5
                p6 = 6
                p7 = 7
            }
            if (dsp == 4 || dsp == 5 || dsp == 6 || dsp == 7 || dsp == 8) {
                p4 = 4
                p5 = 5
                p6 = null
                p7 = null
            }
            break
        case 'v2b':
            if (cio == 0 || cio == 1 || cio == 2 || cio == 3) {
                p1 = 1
                p2 = 2
                p3 = 5
            }
            if (cio == 4 || cio == 5 || cio == 6 || cio == 7 || cio == 8) {
                p1 = 2
                p2 = 5
                p3 = null
            }
            if (dsp == 0 || dsp == 1 || dsp == 2 || dsp == 3) {
                p4 = 6
                p5 = 4
                p6 = 3
                p7 = 7
            }
            if (dsp == 4 || dsp == 5 || dsp == 6 || dsp == 7 || dsp == 8) {
                p4 = 4
                p5 = 3
                p6 = null
                p7 = null
            }
            break
        default:
            return
            // p1 = '';
            // p2 = '';
            // p3 = '';
            // p4 = '';
            // p5 = '';
            // p6 = '';
            // p7 = '';
            break
    }
    document.getElementById('pin1').value = p1
    document.getElementById('pin2').value = p2
    document.getElementById('pin3').value = p3
    document.getElementById('pin4').value = p4
    document.getElementById('pin5').value = p5
    document.getElementById('pin6').value = p6
    document.getElementById('pin7').value = p7

    if (document.getElementById('pin8').value === undefined || p8 !== null) {
        document.getElementById('pin8').value = p8
    }
}

function loadHardwareConfig() {
    console.log("requesting hardware settings")
    var req = new XMLHttpRequest()
    req.open('POST', '/gethardware/')
    req.send()
    req.onreadystatechange = function () {
        if (this.readyState == 4 && this.status == 200) {
            var json = JSON.parse(req.responseText)
            console.log("json message: ")
            console.log(json)
            console.log(json.cio.toString())

            document.getElementById("cio_" + json.cio.toString()).checked = true
            document.getElementById("dsp_" + json.dsp.toString()).checked = true
            document.getElementById(json.pcb.toString()).checked = true
            if (json.hasTempSensor == '1') {
                document.getElementById('hasTempSensor').checked = true
            }

            // Set Temperature Sensor Pin from file
            document.getElementById('pin8').value = json.pins[7]
            if (json.pcb == 'custom') {
                document.getElementById('pin1').value = json.pins[0]
                document.getElementById('pin2').value = json.pins[1]
                document.getElementById('pin3').value = json.pins[2]
                document.getElementById('pin4').value = json.pins[3]
                document.getElementById('pin5').value = json.pins[4]
                document.getElementById('pin6').value = json.pins[5]
                document.getElementById('pin7').value = json.pins[6]
            }
            else {
                setPins()
            }

            // Set power levels if override enabled
            if (json.pwr_levels.override) {
                [...document.querySelectorAll("input[id^='pwr_']")]
                    .forEach((e) => {
                        if (e.type !== "checkbox") {
                            e.value = json.pwr_levels[e.id.slice(4)]
                        }
                    })
                document.getElementById("pwr_override").checked = true
                onOverrideClick()
            } else {
                document.getElementById("pwr_override").checked = false
            }
        }
    }
}

function saveHardwareConfig() {
    console.log("Sending sethardware request")
    buttonConfirm(document.getElementById('save'), "saved &check; (Don't forget to restart the ESP.)", 6)
    var hasTempSensor = "0"
    if (document.querySelector("input[name='hasTempSensor']").checked) {
        hasTempSensor = "1"
    }

    var req = new XMLHttpRequest()
    req.open('POST', '/sethardware/')
    var json = {
        'cio': document.querySelector("input[name='cio_model']:checked").value,
        'dsp': document.querySelector("input[name='dsp_model']:checked").value,
        'pcb': document.querySelector("input[name='pcb']:checked").value,
        'hasTempSensor': hasTempSensor,
        'pins': [
            document.getElementById('pin1').value,
            document.getElementById('pin2').value,
            document.getElementById('pin3').value,
            document.getElementById('pin4').value,
            document.getElementById('pin5').value,
            document.getElementById('pin6').value,
            document.getElementById('pin7').value,
            document.getElementById('pin8').value
        ],
        'pwr_levels':
            [...document.querySelectorAll("input[id^='pwr_']")]
                .reduce((acc, e) => {
                    if (e.type === "checkbox") {
                        acc[e.id.slice(4)] = e.checked
                    } else {
                        acc[e.id.slice(4)] = e.value
                    }
                    return acc
                }, {})
    }
    req.send(JSON.stringify(json))
    console.log(JSON.stringify(json))
}