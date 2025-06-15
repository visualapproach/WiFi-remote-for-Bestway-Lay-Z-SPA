function saveConfig() {
    const maxtemp = document.getElementById('maxtemp').value
    const mintemp = document.getElementById('mintemp').value
    const amb = document.getElementById('amb').value
    const hours = document.getElementById('minutes').value / 60.0
    const r = hours / Math.log((maxtemp - amb) / (mintemp - amb))
    console.log('r: ', r)
    const req = new XMLHttpRequest()
    req.open('POST', '/addcommand/')
    const json = {
        'CMD': 21,
        'VALUE': Math.round(r * 1000000),
        'XTIME': 0,
        'INTERVAL': 0,
        'TXT': 'r set'
    }
    req.send(JSON.stringify(json))
    console.log(json)
}