if (localStorage.getItem('showSectionTemperature')) {
    setWebConfig()
}
readWebConfig()

function setWebConfig() {
    document.getElementById('showSectionTemperature').checked = (localStorage.getItem('showSectionTemperature') === 'true')
    document.getElementById('showSectionDisplay').checked = (localStorage.getItem('showSectionDisplay') === 'true')
    document.getElementById('showSectionControl').checked = (localStorage.getItem('showSectionControl') === 'true')
    document.getElementById('showSectionButtons').checked = (localStorage.getItem('showSectionButtons') === 'true')
    document.getElementById('showSectionTimer').checked = (localStorage.getItem('showSectionTimer') === 'true')
    document.getElementById('showSectionTotals').checked = (localStorage.getItem('showSectionTotals') === 'true')
    document.getElementById('useControlSelector').checked = (localStorage.getItem('useControlSelector') === 'true')
}

function saveWebConfig() {
    localStorage.setItem('showSectionTemperature', document.getElementById('showSectionTemperature').checked)
    localStorage.setItem('showSectionDisplay', document.getElementById('showSectionDisplay').checked)
    localStorage.setItem('showSectionControl', document.getElementById('showSectionControl').checked)
    localStorage.setItem('showSectionButtons', document.getElementById('showSectionButtons').checked)
    localStorage.setItem('showSectionTimer', document.getElementById('showSectionTimer').checked)
    localStorage.setItem('showSectionTotals', document.getElementById('showSectionTotals').checked)
    localStorage.setItem('useControlSelector', document.getElementById('useControlSelector').checked)
    writeWebConfig()
}

// if we want to just keep this configuration in a browser, we can use following
// html: onclick="toggleWebConfig(this)"
// js:
// function toggleWebConfig(elem){
// 	localStorage.setItem(elem.id, elem.checked);
// }

function readWebConfig() {
    var req = new XMLHttpRequest()
    req.open('POST', '/getwebconfig/')
    req.send()
    req.onreadystatechange = function () {
        if (this.readyState == 4 && this.status == 200) {
            var json = JSON.parse(req.responseText)
            console.log(json)
            document.getElementById('showSectionTemperature').checked = json.SST
            document.getElementById('showSectionDisplay').checked = json.SSD
            document.getElementById('showSectionControl').checked = json.SSC
            document.getElementById('showSectionButtons').checked = json.SSB
            document.getElementById('showSectionTimer').checked = json.SSTIM
            document.getElementById('showSectionTotals').checked = json.SSTOT
            document.getElementById('useControlSelector').checked = json.UCS
        }
    }
}

function writeWebConfig() {
    var req = new XMLHttpRequest()
    req.open('POST', '/setwebconfig/')
    var json = {
        'SST': document.getElementById('showSectionTemperature').checked,
        'SSD': document.getElementById('showSectionDisplay').checked,
        'SSC': document.getElementById('showSectionControl').checked,
        'SSB': document.getElementById('showSectionButtons').checked,
        'SSTIM': document.getElementById('showSectionTimer').checked,
        'SSTOT': document.getElementById('showSectionTotals').checked,
        'UCS': document.getElementById('useControlSelector').checked
    }
    req.send(JSON.stringify(json))
    console.log(json)
}