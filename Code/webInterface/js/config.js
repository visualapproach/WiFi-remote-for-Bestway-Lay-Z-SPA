loadConfig()

const settarget = 0
const setunit = 1
const setbubbles = 2
const setheat = 3
const setpump = 4
const resetq = 5
const rebootesp = 6
const gettarget = 7
const resettimes = 8
const resetcl = 9
const resetreplacefilter = 10
const setjets = 11
const setbrightness = 12
const setbeep = 13
const setambientf = 14
const setambientc = 15
const resetdaily = 16
const godmode = 17
const fullpower = 18
const printtext = 19
const setready = 20
const setR = 21
const resetrinsefiltertimer = 22
const resetcleanfiltertimer = 23
const setpower = 24

const commandlist = [
    'Set target', 'Set unit', 'Set bubbles', 'Set heat', 'Set pump', 'Reset queue', 'Reboot ESP',
    'Internal cmd', 'Reset times', 'Reset Cl timer', 'Reset change filter timer', 'Set jets', 'Set brightness',
    'Beep', 'Set ambient temp F.', 'Set ambient temp C.', 'Reset daily meter', 'Take control', 'Full power',
    'Print text', 'Set ready', 'Set R', 'Reset rinse filter timer', 'Reset clean filter timer', 'Set power'
]

function totimestamp() {
    console.log(Date.parse(document.getElementById("xtime").value) / 1000)
    console.log("value " + document.getElementById("xtime").value)
}

function loadConfig() {
    var req = new XMLHttpRequest()
    req.open('POST', '/getconfig/')
    req.send()
    req.onreadystatechange = function () {
        if (this.readyState == 4 && this.status == 200) {
            var json = JSON.parse(req.responseText)
            console.log(json)
            //document.getElementById('timezone').value = json.TIMEZONE.toString();
            document.getElementById('price').value = json.PRICE.toString()
            document.getElementById('freplaceinterval').value = json.FREPI.toString()
            document.getElementById('frinseinterval').value = json.FRINI.toString()
            document.getElementById('fcleaninterval').value = json.FCLEI.toString()
            document.getElementById('clinterval').value = json.CLINT.toString()
            document.getElementById('audio').checked = json.AUDIO
            document.getElementById('notification').checked = json.NOTIFY
            document.getElementById('notificationtime').value = json.NOTIFTIME.toString()
            document.getElementById('restore').checked = json.RESTORE
            var date = new Date(json.REBOOTTIME * 1000)
            document.getElementById('lastboot').innerHTML = 'Last boot: ' + date.toLocaleString() + ' ' + json.REBOOTINFO
            document.getElementById('calibration').checked = json.VTCAL
            document.getElementById('lock_btn').checked = json.LCK
            document.getElementById('timer_btn').checked = json.TMR
            document.getElementById('bubbles_btn').checked = json.AIR
            document.getElementById('unit_btn').checked = json.UNT
            document.getElementById('heat_btn').checked = json.HTR
            document.getElementById('pump_btn').checked = json.FLT
            document.getElementById('down_btn').checked = json.DN
            document.getElementById('up_btn').checked = json.UP
            document.getElementById('power_btn').checked = json.PWR
            document.getElementById('hydrojets_btn').checked = json.HJT
        }
    }

    setInterval(loadCommandQueue, 4000)

    var now = new Date()
    var offset = now.getTimezoneOffset() * 60000
    var adjustedDate = new Date(now.getTime() - offset)
    var formattedDate = adjustedDate.toISOString().substring(0, 16) // For minute precision
    var datetimeField = document.getElementById('xtime')
    datetimeField.value = formattedDate
}

function saveConfig() {
    var req = new XMLHttpRequest()
    req.open('POST', '/setconfig/')
    var json = {
        //'TIMEZONE':parseInt(document.getElementById('timezone').value),
        'TIMEZONE': 0,
        'PRICE': parseFloat(document.getElementById('price').value),
        'FREPI': parseInt(document.getElementById('freplaceinterval').value),
        'FRINI': parseInt(document.getElementById('frinseinterval').value),
        'FCLEI': parseInt(document.getElementById('fcleaninterval').value),
        'CLINT': parseInt(document.getElementById('clinterval').value),
        'AUDIO': document.getElementById('audio').checked,
        'NOTIFY': document.getElementById('notification').checked,
        'NOTIFTIME': parseInt(document.getElementById('notificationtime').value),
        'RESTORE': document.getElementById('restore').checked,
        'VTCAL': document.getElementById('calibration').checked,
        'LCK': document.getElementById('lock_btn').checked,
        'TMR': document.getElementById('timer_btn').checked,
        'AIR': document.getElementById('bubbles_btn').checked,
        'UNT': document.getElementById('unit_btn').checked,
        'HTR': document.getElementById('heat_btn').checked,
        'FLT': document.getElementById('pump_btn').checked,
        'DN': document.getElementById('down_btn').checked,
        'UP': document.getElementById('up_btn').checked,
        'PWR': document.getElementById('power_btn').checked,
        'HJT': document.getElementById('hydrojets_btn').checked
    }
    req.send(JSON.stringify(json))
    console.log(json)
}

function toggleAll() {
    document.getElementById('lock_btn').checked = document.getElementById('toggle_all').checked
    document.getElementById('timer_btn').checked = document.getElementById('toggle_all').checked
    document.getElementById('bubbles_btn').checked = document.getElementById('toggle_all').checked
    document.getElementById('unit_btn').checked = document.getElementById('toggle_all').checked
    document.getElementById('heat_btn').checked = document.getElementById('toggle_all').checked
    document.getElementById('pump_btn').checked = document.getElementById('toggle_all').checked
    document.getElementById('down_btn').checked = document.getElementById('toggle_all').checked
    document.getElementById('up_btn').checked = document.getElementById('toggle_all').checked
    document.getElementById('power_btn').checked = document.getElementById('toggle_all').checked
    document.getElementById('hydrojets_btn').checked = document.getElementById('toggle_all').checked
}

function loadCommandQueue() {
    var req = new XMLHttpRequest()
    req.open('POST', '/getcommands/')
    req.send()
    req.onreadystatechange = function () {
        if (this.readyState == 4 && this.status == 200) {
            var json = JSON.parse(req.responseText)
            console.log(json)
            var table = document.getElementById('cmdq')
            table.innerHTML = ''

            for (var i = 0; i < json.LEN; i++) {
                var row = table.insertRow()
                var cell = row.insertCell(0)
                cell.innerHTML = '<button id="editCmd' + i + '" class="button" onclick="editCommandQueue(' + i + ')">Set</button>'
                var date = new Date(json.XTIME[i] * 1000)
                cell = row.insertCell(1)
                cell.innerHTML = commandlist[json['CMD'][i]] + ':' + json['VALUE'][i] + '<br>' + date.toLocaleString() + '<br>Interval: ' + json['INTERVAL'][i] + 's'
                cell = row.insertCell(2)
                cell.innerHTML = '<button id="delCmd' + i + '" class="button_red" onclick="delCommand(' + i + ')">Del</button>'
            }
        }
    }
}

function addCommand() {
    var req = new XMLHttpRequest()
    req.open('POST', '/addcommand/')
    var json = {
        'CMD': parseInt(document.getElementById('commands').value),
        'VALUE': parseFloat(document.getElementById('val').value),
        'XTIME': Date.parse(document.getElementById('xtime').value) / 1000,
        'INTERVAL': parseInt(document.getElementById('interval').value),
        'TXT': document.getElementById('txt').value
    }
    req.send(JSON.stringify(json))
    console.log(json)
}

function resetCommandQueue() {
    var req = new XMLHttpRequest()
    req.open('POST', '/addcommand/')
    var json = {
        'CMD': resetq,
        'VAL': 1,
        'XTIME': 0,
        'INTERVAL': 0
    }
    req.send(JSON.stringify(json))
    console.log(json)
    loadCommandQueue()
}

function savetofile() {
    var filename = prompt("Please enter a filename", "mycommandqueue1")
    if (filename == null) return
    var req = new XMLHttpRequest()
    req.open('POST', '/cmdq_file/')
    var json = {
        'ACT': 'save',
        'NAME': filename
    }
    req.send(JSON.stringify(json))
    console.log(json)
}

function loadfromfile() {
    var filename = prompt("Please enter a filename", "mycommandqueue1")
    if (filename == null) return
    var req = new XMLHttpRequest()
    req.open('POST', '/cmdq_file/')
    var json = {
        'ACT': 'load',
        'NAME': filename
    }
    req.send(JSON.stringify(json))
    console.log(json)
}

function editCommandQueue(index) {
    var req = new XMLHttpRequest()
    req.open('POST', '/editcommand/')
    var json = {
        'CMD': parseInt(document.getElementById('commands').value),
        'VALUE': parseFloat(document.getElementById('val').value),
        'XTIME': Date.parse(document.getElementById('xtime').value) / 1000,
        'INTERVAL': parseInt(document.getElementById('interval').value),
        'TXT': document.getElementById('txt').value,
        'IDX': index
    }
    req.send(JSON.stringify(json))
    console.log(json)
}

function delCommand(index) {
    var req = new XMLHttpRequest()
    req.open('POST', '/delcommand/')
    var json = {
        'IDX': index
    }
    req.send(JSON.stringify(json))
    console.log(json)
}

function toggleCalibration() {
    let obj = document.getElementById('calibration')
    if (obj.checked) obj.checked = false
}