function removeFile() {
    const filename = document.getElementById('fileToRemove').value
    if (!filename) {
        alert(getTranslation('alert_enter_filename'))
        return
    }
    // TODO: use async fetch instead of XMLHttpRequest
    const req = new XMLHttpRequest()
    req.open('POST', '/removefile/')
    req.send(JSON.stringify({ file: filename }))
    req.onreadystatechange = function () {
        if (this.readyState === 4 && this.status === 200) {
            alert(getTranslation('alert_file_removed'))
        }
    }
    document.getElementById('fileToRemove').value = ''
}