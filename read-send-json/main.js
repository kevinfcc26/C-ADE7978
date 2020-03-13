

setInterval(init, 50)

function init() {
    try {
        var XMLHttpRequest = require("xmlhttprequest").XMLHttpRequest;
        var xhr = new XMLHttpRequest();
        const fs = require('fs')
        let rawdata = fs.readFileSync('db.json')
        let registers = JSON.parse(rawdata)
        xhr.open("PUT", 'https://json-ade-metering.herokuapp.com/registers/1', true);
        xhr.setRequestHeader('Content-Type', 'application/json');
        xhr.send(JSON.stringify(registers));
        console.log(registers);
        console.log("ESCRIBE")
    } catch {
        console.log("ERROR")
    }
}
