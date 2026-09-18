#include "ToyConfigWeb.h"

#include <ESPAsyncWebServer.h>

#include "OtaServer.h"
#include "ToyHub.h"
#include "ToyProfiles.h"

namespace {

const char* featureName(ToyFeatureType t) {
    switch (t) {
        case ToyFeatureType::Vibrate: return "Vibrate";
        case ToyFeatureType::Rotate: return "Rotate";
        case ToyFeatureType::Oscillate: return "Oscillate";
        case ToyFeatureType::Constrict: return "Constrict";
        case ToyFeatureType::Spray: return "Spray";
        case ToyFeatureType::Temperature: return "Temperature";
        case ToyFeatureType::Led: return "LED";
        case ToyFeatureType::Position: return "Position";
    }
    return "Vibrate";
}

// Prefill template + range so a novice can just pick a feature type.
const char* defaultTemplate(ToyFeatureType t) {
    switch (t) {
        case ToyFeatureType::Rotate: return "Rotate:%d;";
        case ToyFeatureType::Vibrate: return "Vibrate:%d;";
        default: return "";
    }
}

String pageHtml() {
    String rows;
    const int count = toyProfilesCount();
    if (count == 0) {
        rows = "<tr><td colspan=\"3\" class=\"muted\">No custom toys yet.</td></tr>";
    } else {
        for (int i = 0; i < count; ++i) {
            ToyProfile p;
            if (!toyProfilesGet(i, p)) continue;
            rows += "<tr><td>" + p.prefix + "</td><td>" + featureName(p.type) +
                    "</td><td>" + String(p.minValue) + " … " + String(p.maxValue) +
                    " &nbsp;<code>" + p.commandTemplate + "</code></td>" +
                    "<td><a class=\"del\" href=\"/toys/delete?n=" + String(i) +
                    "\">Remove</a></td></tr>";
        }
    }

    String html;
    html.reserve(2400);

    // "Last scan" card: shows what the M5's BLE radio actually saw.
    String scanSection = "<div class=\"card\"><h2>Last scan</h2>";
    {
        String report = toyHubScanReport();
        if (report.length() == 0) {
            scanSection += "<p class=\"muted\">No scan run yet.</p>";
        } else {
            report.replace("&", "&amp;");
            report.replace("<", "&lt;");
            report.replace(">", "&gt;");
            report.replace("\n", "<br>");
            scanSection += "<pre>" + report + "</pre>";
        }
    }
    scanSection += "<p class=\"tip\">Press <b>Scan now</b>, wait a few seconds, then refresh this page.</p></div>";

    html = R"raw(<!DOCTYPE html><html><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Add a toy — OSSM M5 Remote</title>
<style>
 body{font-family:system-ui,sans-serif;background:#0a0a12;color:#dde6f0;margin:0;padding:16px}
 h1{font-size:20px;margin:0 0 4px} h2{font-size:15px;margin:18px 0 8px}
 .muted{color:#94a3b8} .card{background:#0f172a;border-radius:10px;padding:14px;margin:10px 0}
 label{display:block;margin:10px 0 2px;font-size:13px;color:#94a3b8}
 input,select{width:100%;box-sizing:border-box;padding:8px;border-radius:6px;border:1px solid #334155;background:#1e293b;color:#fff}
 button{background:#83277b;color:#fff;border:0;border-radius:8px;padding:10px 18px;font-size:15px;margin-top:12px}
 table{width:100%;border-collapse:collapse;font-size:13px}
 td,th{padding:6px;border-bottom:1px solid #334155;text-align:left}
 .del{color:#f87171} code{background:#1e293b;padding:1px 4px;border-radius:4px}
 .tip{font-size:12px;color:#94a3b8}
</style></head><body>
<h1>Add a toy</h1>
<div class="muted">Connects a BLE toy you own to this M5 — no programming needed.</div>

<div class="card">
 <h2>New toy</h2>
 <form method="post" action="/toys/add">
  <label>Advertised name starts with (from a BLE scanner app)</label>
  <input name="prefix" placeholder="e.g. WP- or MAX- or LVS-">
  <label>What it does</label>
  <select name="type" id="type" onchange="prefill()">
   <option value="0">Vibrate</option>
   <option value="1">Rotate (two directions)</option>
   <option value="2">Oscillate</option>
   <option value="3">Constrict</option>
   <option value="4">Spray</option>
   <option value="5">Temperature</option>
   <option value="6">LED</option>
   <option value="7">Position</option>
  </select>
  <label>Lowest value</label><input name="min" id="min" type="number" value="0">
  <label>Highest value</label><input name="max" id="max" type="number" value="20">
  <label>Command text (leave auto if unsure)</label>
  <input name="tpl" id="tpl" placeholder="Vibrate:%d;">
  <button type="submit">Add toy</button>
 </form>
</div>

<div class="card">
 <h2>Cheat sheet</h2>
 <table><tr><th>Brand</th><th>Name starts</th><th>Feature</th><th>Command</th></tr>
  <tr><td>Lovense</td><td>LVS- / LOVE-</td><td>Vibrate 0–20</td><td><code>Vibrate:%d;</code></td></tr>
  <tr><td>Lovense</td><td>LVS- / LOVE-</td><td>Rotate −20…20</td><td><code>Rotate:%d;</code></td></tr>
  <tr><td>We-Vibe</td><td>WP-</td><td>Vibrate 0–20</td><td><code>Vibrate:%d;</code></td></tr>
 </table>
 <p class="tip">Lovense is already built in. Look up your toy in the
 buttplug.io device list for its exact command text. The <code>%d</code> is
 where the value goes.</p>
</div>

<div class="card">
 <h2>Your toys</h2>
 <table><tr><th>Name</th><th>Feature</th><th>Range / command</th><th></th></tr>)raw" +
          rows + R"raw(</table>
 <a href="/toys/scan"><button>Scan now</button></a>
 <p class="tip">Turn the toy on, then press Scan now. It normally also auto-connects
 within ~20 seconds.</p>
</div>)raw" +
          scanSection + R"raw(

<script>
 function prefill(){
  var t=document.getElementById('type').value;
  var tpl=document.getElementById('tpl'); var mn=document.getElementById('min');
  var mx=document.getElementById('max');
  if(t==='1'){tpl.value='Rotate:%d;';mn.value='-20';mx.value='20';}
  else if(t==='0'){tpl.value='Vibrate:%d;';mn.value='0';mx.value='20';}
  else {tpl.value='';}
 }
</script></body></html>)raw";

    return html;
}

void handleToys(AsyncWebServerRequest* request) {
    request->send(200, "text/html", pageHtml());
}

void handleAdd(AsyncWebServerRequest* request) {
    ToyProfile p;
    if (request->hasParam("prefix", true)) p.prefix = request->getParam("prefix", true)->value();
    if (request->hasParam("type", true)) p.type = (ToyFeatureType)request->getParam("type", true)->value().toInt();
    if (request->hasParam("min", true)) p.minValue = request->getParam("min", true)->value().toInt();
    if (request->hasParam("max", true)) p.maxValue = request->getParam("max", true)->value().toInt();
    if (request->hasParam("tpl", true)) p.commandTemplate = request->getParam("tpl", true)->value();
    p.commandTemplate.trim();

    if (p.commandTemplate.length() == 0) p.commandTemplate = defaultTemplate(p.type);
    if (p.maxValue <= p.minValue) p.maxValue = p.minValue + 20;
    if (p.prefix.length() == 0) {
        request->send(400, "text/plain", "Missing name prefix");
        return;
    }

    toyProfilesAdd(p);
    request->redirect("/toys");
}

void handleDelete(AsyncWebServerRequest* request) {
    if (request->hasParam("n")) {
        toyProfilesRemove(request->getParam("n")->value().toInt());
    }
    request->redirect("/toys");
}

void handleScan(AsyncWebServerRequest* request) {
    toyHubRequestScan();
    request->redirect("/toys");
}

}  // namespace

void toyConfigWebInit() {
    AsyncWebServer& server = otaServerWebServer();
    server.on("/toys", HTTP_GET, handleToys);
    server.on("/toys/add", HTTP_POST, handleAdd);
    server.on("/toys/delete", HTTP_GET, handleDelete);
    server.on("/toys/scan", HTTP_GET, handleScan);
    server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->redirect("/toys");
    });
}
