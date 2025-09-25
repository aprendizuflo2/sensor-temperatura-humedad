// Apps Script: recibe datos por GET o POST y los agrega a la hoja "Lecturas"
function doGet(e) {
  return handleRequest(e);
}

function doPost(e) {
  return handleRequest(e);
}

function handleRequest(e) {
  try {
    // Si el script está *vinculado* a la hoja, podemos usar getActiveSpreadsheet
    var ss = SpreadsheetApp.getActiveSpreadsheet();
    var sheet = ss.getSheetByName("Lecturas"); // Asegúrate de tener la hoja con ese nombre

    // e.parameter funciona para GET y POST con form-urlencoded
    var temp = e.parameter.temperature || "";
    var hum  = e.parameter.humidity  || "";
    var source = e.parameter.source || "";

    // timestamp
    var now = new Date();

    // Añadir una fila con [Timestamp, Temperature, Humidity, Source]
    sheet.appendRow([now, temp, hum, source]);

    var result = {status: "OK", timestamp: now.toString()};
    return ContentService
      .createTextOutput(JSON.stringify(result))
      .setMimeType(ContentService.MimeType.JSON);
  } catch (err) {
    var error = {status: "ERROR", message: err.toString()};
    return ContentService
      .createTextOutput(JSON.stringify(error))
      .setMimeType(ContentService.MimeType.JSON);
  }
}
