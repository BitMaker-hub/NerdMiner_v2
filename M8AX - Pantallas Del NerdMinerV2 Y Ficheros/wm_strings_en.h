/**
 * SAMPLE SAMPLE SAMPLE
 * 
 * wm_strings_es.h
 * spanish strings for
 * WiFiManager, a library for the ESPX/Arduino platform
 * for configuration of WiFi credentials using a Captive Portal
 *
 * @author Creator tzapu
 * @author tablatronix
 * @version 0.0.0
 * @license MIT
 */

#ifndef _WM_STRINGS_EN_H_
#define _WM_STRINGS_EN_H_

/**
 * ADD TO BUILD FLAGS
 * -DWM_STRINGS_FILE="\"wm_strings_es.h\""
 */

#ifndef WIFI_MANAGER_OVERRIDE_STRINGS
// !!! ABOVE WILL NOT WORK if you define in your sketch, must be build flag, if anyone one knows how to order includes to be able to do this it would be neat.. I have seen it done..

// strings files must include a consts file!
// Copy and change to custom locale tokens if necessary, but strings should be good enough
#include "wm_consts_en.h" // include constants, tokens, routes

const char WM_LANGUAGE[] PROGMEM = "es-ES"; // i18n lang code

const char HTTP_HEAD_START[]       PROGMEM = "<!DOCTYPE html>"
"<html lang='en'><head>"
"<meta name='format-detection' content='telephone=no'>"
"<meta charset='UTF-8'>"
"<meta  name='viewport' content='width=device-width,initial-scale=1,user-scalable=no'/>"
"<title>{v}</title>";

const char HTTP_SCRIPT[]           PROGMEM = "<script>function c(l){"
"document.getElementById('s').value=l.getAttribute('data-ssid')||l.innerText||l.textContent;"
"p = l.nextElementSibling.classList.contains('l');"
"document.getElementById('p').disabled = !p;"
"if(p)document.getElementById('p').focus();};"
"function f() {var x = document.getElementById('p');x.type==='password'?x.type='text':x.type='password';}"
"</script>"; // @todo add button states, disable on click , show ack , spinner etc

const char HTTP_HEAD_END[]         PROGMEM = "</head><body class='{c}'><div class='wrap'>"; // {c} = _bodyclass
// example of embedded logo, base64 encoded inline, No styling here
// const char HTTP_ROOT_MAIN[]        PROGMEM = "<img title=' alt=' src='data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAADAAAAAwCAYAAABXAvmHAAADQElEQVRoQ+2YjW0VQQyE7Q6gAkgFkAogFUAqgFQAVACpAKiAUAFQAaECQgWECggVGH1PPrRvn3dv9/YkFOksoUhhfzwz9ngvKrc89JbnLxuA/63gpsCmwCADWwkNEji8fVNgotDM7osI/x777x5l9F6JyB8R4eeVql4P0y8yNsjM7KGIPBORp558T04A+CwiH1UVUItiUQmZ2XMReSEiAFgjAPBeVS96D+sCYGaUx4cFbLfmhSpnqnrZuqEJgJnd8cQplVLciAgX//Cf0ToIeOB9wpmloLQAwpnVmAXgdf6pwjpJIz+XNoeZQQZlODV9vhc1Tuf6owrAk/8qIhFbJH7eI3eEzsvydQEICqBEkZwiALfF70HyHPpqScPV5HFjeFu476SkRA0AzOfy4hYwstj2ZkDgaphE7m6XqnoS7Q0BOPs/sw0kDROzjdXcCMFCNwzIy0EcRcOvBACfh4k0wgOmBX4xjfmk4DKTS31hgNWIKBCI8gdzogTgjYjQWFMw+o9LzJoZ63GUmjWm2wGDc7EvDDOj/1IVMIyD9SUAL0WEhpriRlXv5je5S+U1i2N88zdPuoVkeB+ls4SyxCoP3kVm9jsjpEsBLoOBNC5U9SwpGdakFkviuFP1keblATkTENTYcxkzgxTKOI3jyDxqLkQT87pMA++H3XvJBYtsNbBN6vuXq5S737WqHkW1VgMQNXJ0RshMqbbT33sJ5kpHWymzcJjNTeJIymJZtSQd9NHQHS1vodoFoTMkfbJzpRnLzB2vi6BZAJxWaCr+62BC+jzAxVJb3dmmiLzLwZhZNPE5e880Suo2AZgB8e8idxherqUPnT3brBDTlPxO3Z66rVwIwySXugdNd+5ejhqp/+NmgIwGX3Py3QBmlEi54KlwmjkOytQ+iJrLJj23S4GkOeecg8G091no737qvRRdzE+HLALQoMTBbJgBsCj5RSWUlUVJiZ4SOljb05eLFWgoJ5oY6yTyJp62D39jDANoKKcSocPJD5dQYzlFAFZJflUArgTPZKZwLXAnHmerfJquUkKZEgyzqOb5TuDt1P3nwxobqwPocZA11m4A1mBx5IxNgRH21ti7KbAGiyNn3HoF/gJ0w05A8xclpwAAAABJRU5ErkJggg==' /><h1>{v}</h1><h3>M8AX-WiFiManager-M8AX</h3>";
const char HTTP_ROOT_MAIN[]        PROGMEM = "<h1>{t}</h1><h3>{v}</h3>";

const char * const HTTP_PORTAL_MENU[] PROGMEM = {
"<form action='/wifi'    method='get'><button>Configurar WiFi</button></form><br/>\n", // MENU_WIFI
"<form action='/0wifi'   method='get'><button>Configurar WiFi ( Sin Escanear )</button></form><br/>\n", // MENU_WIFINOSCAN
"<form action='/info'    method='get'><button>Información</button></form><br/>\n", // MENU_INFO
"<form action='/param'   method='get'><button>Configuración</button></form><br/>\n",//MENU_PARAM
"<form action='/close'   method='get'><button>Cerca</button></form><br/>\n", // MENU_CLOSE
"<form action='/restart' method='get'><button>Reanudar</button></form><br/>\n",// MENU_RESTART
"<form action='/exit'    method='get'><button>Salir</button></form><br/>\n",  // MENU_EXIT
"<form action='/erase'   method='get'><button class='D'>Borrar</button></form><br/>\n", // MENU_ERASE
"<form action='/update'  method='get'><button>Actualizar</button></form><br/>\n",// MENU_UPDATE
"<hr><br/>" // MENU_SEP
};

// const char HTTP_PORTAL_OPTIONS[]   PROGMEM = strcat(HTTP_PORTAL_MENU[0] , HTTP_PORTAL_MENU[3] , HTTP_PORTAL_MENU[7]);
const char HTTP_PORTAL_OPTIONS[]   PROGMEM = "";
const char HTTP_ITEM_QI[]          PROGMEM = "<div role='img' aria-label='{r}%' title='{r}%' class='q q-{q} {i} {h}'></div>"; // rssi icons
const char HTTP_ITEM_QP[]          PROGMEM = "<div class='q {h}'>{r}%</div>"; // rssi percentage {h} = hidden showperc pref
const char HTTP_ITEM[]             PROGMEM = "<div><a href='#p' onclick='c(this)' data-ssid='{V}'>{v}</a>{qi}{qp}</div>"; // {q} = HTTP_ITEM_QI, {r} = HTTP_ITEM_QP
// const char HTTP_ITEM[]            PROGMEM = "<div><a href='#p' onclick='c(this)'>{v}</a> {R} {r}% {q} {e}</div>"; // test all tokens

const char HTTP_FORM_START[]       PROGMEM = "<form method='POST' action='{v}'>";
const char HTTP_FORM_WIFI[]        PROGMEM = "<label for='s'>SSID</label><input id='s' name='s' maxlength='32' autocorrect='off' autocapitalize='none' placeholder='{v}'><br/><label for='p'>Contraseña</label><input id='p' name='p' maxlength='64' type='password' placeholder='{p}'><input type='checkbox' onclick='f()'> Mostrar Contraseña";
const char HTTP_FORM_WIFI_END[]    PROGMEM = "";
const char HTTP_FORM_STATIC_HEAD[] PROGMEM = "<hr><br/>";
const char HTTP_FORM_END[]         PROGMEM = "<br/><br/><button type='submit'>Grabar</button></form>";
const char HTTP_FORM_LABEL[]       PROGMEM = "<label for='{i}'>{t}</label>";
const char HTTP_FORM_PARAM_HEAD[]  PROGMEM = "<hr><br/>";
const char HTTP_FORM_PARAM[]       PROGMEM = "<br/><input id='{i}' name='{n}' maxlength='{l}' value='{v}' {c}>\n"; // do not remove newline!

const char HTTP_SCAN_LINK[]        PROGMEM = "<br/><form action='/wifi?refresh=1' method='POST'><button name='refresh' value='1'>Refrescar</button></form>";
const char HTTP_SAVED[]            PROGMEM = "<div class='msg'>Grabando Credenciales<br/>Intentando Conectar ESP A La Red.<br />Si Falla, Reconecta Al AP Para Intentarlo De Nuevo.</div>";
const char HTTP_PARAMSAVED[]       PROGMEM = "<div class='msg S'>Grabado<br/></div>";
const char HTTP_END[]              PROGMEM = "</div></body></html>";
const char HTTP_ERASEBTN[]         PROGMEM = "<br/><form action='/erase' method='get'><button class='D'>Borrar Configuración WIFI</button></form>";
const char HTTP_UPDATEBTN[]        PROGMEM = "<br/><form action='/update' method='get'><button>Actualizar</button></form>";
const char HTTP_BACKBTN[]          PROGMEM = "<hr><br/><form action='/' method='get'><button>Atrás</button></form>";

const char HTTP_STATUS_ON[]        PROGMEM = "<div class='msg S'><strong>Conectado</strong> A {v}<br/><em><small>Con IP {i}</small></em></div>";
const char HTTP_STATUS_OFF[]       PROGMEM = "<div class='msg {c}'><strong>No Conectado</strong> A {v}{r}</div>"; // {c=class} {v=ssid} {r=status_off}
const char HTTP_STATUS_OFFPW[]     PROGMEM = "<br/>Fallo De Autenticación"; // STATION_WRONG_PASSWORD,  no eps32
const char HTTP_STATUS_OFFNOAP[]   PROGMEM = "<br/>No Encontrado";   // WL_NO_SSID_AVAIL
const char HTTP_STATUS_OFFFAIL[]   PROGMEM = "<br/>No Se Pudo Conectar"; // WL_CONNECT_FAILED
const char HTTP_STATUS_NONE[]      PROGMEM = "<div class='msg'>Sin AP Establecido</div>";
const char HTTP_BR[]               PROGMEM = "<br/>";

const char HTTP_STYLE[]            PROGMEM = "<style>"
".c,body{text-align:center;font-family:verdana}div,input,select{padding:5px;font-size:1em;margin:5px 0;box-sizing:border-box}"
"input,button,select,.msg{border-radius:.3rem;width: 100%}input[type=radio],input[type=checkbox]{width:auto}"
"button,input[type='button'],input[type='submit']{cursor:pointer;border:0;background-color:#000000;color:#fff;line-height:2.4rem;font-size:1.2rem;width:100%}"
"input[type='file']{border:1px solid #000000}"
".wrap {text-align:left;display:inline-block;min-width:260px;max-width:500px}"
// links
"a{color:#000;font-weight:700;text-decoration:none}a:hover{color:#000000;text-decoration:underline}"
// quality icons
".q{height:16px;margin:0;padding:0 5px;text-align:right;min-width:38px;float:right}.q.q-0:after{background-position-x:0}.q.q-1:after{background-position-x:-16px}.q.q-2:after{background-position-x:-32px}.q.q-3:after{background-position-x:-48px}.q.q-4:after{background-position-x:-64px}.q.l:before{background-position-x:-80px;padding-right:5px}.ql .q{float:left}.q:after,.q:before{content:'';width:16px;height:16px;display:inline-block;background-repeat:no-repeat;background-position: 16px 0;"
"background-image:url('data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAGAAAAAQCAMAAADeZIrLAAAAJFBMVEX///8AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAADHJj5lAAAAC3RSTlMAIjN3iJmqu8zd7vF8pzcAAABsSURBVHja7Y1BCsAwCASNSVo3/v+/BUEiXnIoXkoX5jAQMxTHzK9cVSnvDxwD8bFx8PhZ9q8FmghXBhqA1faxk92PsxvRc2CCCFdhQCbRkLoAQ3q/wWUBqG35ZxtVzW4Ed6LngPyBU2CobdIDQ5oPWI5nCUwAAAAASUVORK5CYII=');}"
// icons @2x media query (32px rescaled)
"@media (-webkit-min-device-pixel-ratio: 2),(min-resolution: 192dpi){.q:before,.q:after {"
"background-image:url('data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAALwAAAAgCAMAAACfM+KhAAAALVBMVEX///8AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAADAOrOgAAAADnRSTlMAESIzRGZ3iJmqu8zd7gKjCLQAAACmSURBVHgB7dDBCoMwEEXRmKlVY3L//3NLhyzqIqSUggy8uxnhCR5Mo8xLt+14aZ7wwgsvvPA/ofv9+44334UXXngvb6XsFhO/VoC2RsSv9J7x8BnYLW+AjT56ud/uePMdb7IP8Bsc/e7h8Cfk912ghsNXWPpDC4hvN+D1560A1QPORyh84VKLjjdvfPFm++i9EWq0348XXnjhhT+4dIbCW+WjZim9AKk4UZMnnCEuAAAAAElFTkSuQmCC');"
"background-size: 95px 16px;}}"
// msg callouts
".msg{padding:20px;margin:20px 0;border:1px solid #eee;border-left-width:5px;border-left-color:#777}.msg h4{margin-top:0;margin-bottom:5px}.msg.P{border-left-color:#000000}.msg.P h4{color:#000000}.msg.D{border-left-color:#dc3630}.msg.D h4{color:#dc3630}.msg.S{border-left-color: #5cb85c}.msg.S h4{color: #5cb85c}"
// lists
"dt{font-weight:bold}dd{margin:0;padding:0 0 0.5em 0;min-height:12px}"
"td{vertical-align: top;}"
".h{display:none}"
"button{transition: 0s opacity;transition-delay: 3s;transition-duration: 0s;cursor: pointer}"
"button.D{background-color:#dc3630}"
"button:active{opacity:50% !important;cursor:wait;transition-delay: 0s}"
// invert
"body.invert,body.invert a,body.invert h1 {background-color:#060606;color:#fff;}"
"body.invert .msg{color:#fff;background-color:#282828;border-top:1px solid #555;border-right:1px solid #555;border-bottom:1px solid #555;}"
"body.invert .q[role=img]{-webkit-filter:invert(1);filter:invert(1);}"
":disabled {opacity: 0.5;}"
"</style>";

#ifndef WM_NOHELP
const char HTTP_HELP[]             PROGMEM =
 "<br/><h3>Páginas Disponibles</h3><hr>"
 "<table class='table'>"
 "<thead><tr><th>Página</th><th>Función</th></tr></thead><tbody>"
 "<tr><td><a href='/'>/</a></td>"
 "<td>Página De Menú.</td></tr>"
 "<tr><td><a href='/wifi'>/wifi</a></td>"
 "<td>Mostrar Resultados Del Escaneo WiFi Y Entrar A La Configuración WiFi. ( /0wifi Sin Escaneo De SSID'S ).</td></tr>"
 "<tr><td><a href='/wifisave'>/wifisave</a></td>"
 "<td>Guardar La Configuración WiFi Y Configurar El Dispositivo. Necesita Variables Suministradas.</td></tr>"
 "<tr><td><a href='/param'>/param</a></td>"
 "<td>Página De Parámetros.</td></tr>"
 "<tr><td><a href='/info'>/info</a></td>"
 "<td>Página De Información.</td></tr>"
 "<tr><td><a href='/u'>/u</a></td>"
 "<td>Actualización OTA.</td></tr>"
 "<tr><td><a href='/close'>/close</a></td>"
 "<td>Cerrar El Popup Del Portal Cautivo, El Portal De Configuración Permanecerá Activo.</td></tr>"
 "<tr><td>/exit</td>"
 "<td>Salir Del Portal De Configuración, El Portal De Configuración Se Cerrará.</td></tr>"
 "<tr><td>/restart</td>"
 "<td>Reiniciar El Dispositivo.</td></tr>"
 "<tr><td>/erase</td>"
 "<td>Borrar La Configuración WiFi Y Reiniciar El Dispositivo. El Dispositivo No Se Reconectará A Una Red Hasta Que Se Ingrese Una Nueva Configuración WiFi.</td></tr>"
 "</table>"
 "<p/>Github - <a href='https://github.com/tzapu/WiFiManager'>https://github.com/tzapu/WiFiManager</a>.<br/>"
 "<p/>M8AX YouTube - <a href='https://youtube.com/m8ax'>https://youtube.com/m8ax</a><br/>"
 "<p/>M8AX NFTs OpenSea - <a href='https://opensea.io/m8ax'>https://opensea.io/m8ax</a>"
 "<p/>Por Muchas Vueltas Que Demos, Siempre Tendremos El Culo Atrás...<br/><br/>El Futuro No Está Establecido, No Hay Destino, Solo Existe El Que Nosotros Hacemos...<br/><br/>EHD - MDDD<br/><br/>M8AX Corp. 2025 - 2050</p>";
#else
const char HTTP_HELP[]             PROGMEM = "";
#endif

const char HTTP_UPDATE[] PROGMEM = "Sube Un Nuevo Firmware A Tu Aparatito...<br/><form method='POST' action='u' enctype='multipart/form-data' onchange=\"(function(el){document.getElementById('uploadbin').style.display = el.value=='' ? 'none' : 'initial';})(this)\"><input type='file' name='update' accept='.bin,application/octet-stream'><button id='uploadbin' type='submit' class='h D'>Update</button></form><small><a href='http://192.168.4.1/update' target='_blank'>Puede Que No Funcione Dentro Del Portal Cautivo, Abre En Tu Navegador http://192.168.4.1</a><small><br><br><b>Por Muchas Vueltas Que Demos, Siempre Tendremos El Culo Atrás...</b>";
const char HTTP_UPDATE_FAIL[] PROGMEM = "<div class='msg D'><strong>Actualización Fallida!</strong><Br/>Reinicia El Dispositivo E Inténtalo De Nuevo...</div>";
const char HTTP_UPDATE_SUCCESS[] PROGMEM = "<div class='msg S'><strong>Actualización Satisfactoria!  </strong> <br/> El Aparatito Se Está Reiniciando...</div>";

#ifdef WM_JSTEST
const char HTTP_JS[] PROGMEM =
"<script>function postAjax(url, data, success) {"
"    var params = typeof data == 'string' ? data : Object.keys(data).map("
"            function(k){ return encodeURIComponent(k) + '=' + encodeURIComponent(data[k]) }"
"        ).join('&');"
"    var xhr = window.XMLHttpRequest ? new XMLHttpRequest() : new ActiveXObject(\"Microsoft.XMLHTTP\");"
"    xhr.open('POST', url);"
"    xhr.onreadystatechange = function() {"
"        if (xhr.readyState>3 && xhr.status==200) { success(xhr.responseText); }"
"    };"
"    xhr.setRequestHeader('X-Requested-With', 'XMLHttpRequest');"
"    xhr.setRequestHeader('Content-Type', 'application/x-www-form-urlencoded');"
"    xhr.send(params);"
"    return xhr;}"
"postAjax('/status', 'p1=1&p2=Hello+World', function(data){ console.log(data); });"
"postAjax('/status', { p1: 1, p2: 'Hello World' }, function(data){ console.log(data); });"
"</script>";
#endif

// Info html
// @todo remove html elements from progmem, repetetive strings
#ifdef ESP32
	const char HTTP_INFO_esphead[]    PROGMEM = "<h3>M8AX - Esp32 - M8AX</h3><hr><dl>";
	const char HTTP_INFO_chiprev[]    PROGMEM = "<dt>Chip Rev</dt><dd>{1}</dd>";
  	const char HTTP_INFO_lastreset[]  PROGMEM = "<dt>Último Motivo De Reinicio</dt><dd>CPU0: {1}<br/>CPU1: {2}</dd>";
  	const char HTTP_INFO_aphost[]     PROGMEM = "<dt>Nombre Del Punto De Acceso</dt><dd>{1}</dd>";
    const char HTTP_INFO_psrsize[]    PROGMEM = "<dt>Tamaño PSRAM</dt><dd>{1} Bytes</dd>";
	const char HTTP_INFO_temp[]       PROGMEM = "<dt>Temperatura</dt><dd>{1} C&deg; / {2} F&deg;</dd><dt>Sensor Hall - Campos Magnéticos</dt><dd>{3}</dd>";
#else
	const char HTTP_INFO_esphead[]    PROGMEM = "<h3>esp8266</h3><hr><dl>";
	const char HTTP_INFO_fchipid[]    PROGMEM = "<dt>Flash Chip ID</dt><dd>{1}</dd>";
	const char HTTP_INFO_corever[]    PROGMEM = "<dt>Versión Del Núcleo</dt><dd>{1}</dd>";
	const char HTTP_INFO_bootver[]    PROGMEM = "<dt>Versión De Arranque</dt><dd>{1}</dd>";
	const char HTTP_INFO_lastreset[]  PROGMEM = "<dt>Último Motivo De Reinicio</dt><dd>{1}</dd>";
	const char HTTP_INFO_flashsize[]  PROGMEM = "<dt>Tamaño Real De Memoria Flash</dt><dd>{1} Bytes</dd>";
#endif

const char HTTP_INFO_memsmeter[]  PROGMEM = "<br/><progress value='{1}' max='{2}'></progress></dd>";
const char HTTP_INFO_memsketch[]  PROGMEM = "<dt>Memoria - Tamaño Del Sketch</dt><dd>Usados / Bytes Totales<br/>{1} / {2}";
const char HTTP_INFO_freeheap[]   PROGMEM = "<dt>Memoria - Montón Libre</dt><dd>{1} Bytes Disponibles</dd>";
const char HTTP_INFO_wifihead[]   PROGMEM = "<br/><h3>M8AX - WiFi - M8AX</h3><hr>";
const char HTTP_INFO_uptime[]     PROGMEM = "<dt>Encendido</dt><dd>{1} Mins {2} Secs</dd>";
const char HTTP_INFO_chipid[]     PROGMEM = "<dt>Chip ID</dt><dd>{1}</dd>";
const char HTTP_INFO_idesize[]    PROGMEM = "<dt>Tamaño Flash</dt><dd>{1} Bytes</dd>";
const char HTTP_INFO_sdkver[]     PROGMEM = "<dt>Versión De SDK</dt><dd>{1}</dd>";
const char HTTP_INFO_cpufreq[]    PROGMEM = "<dt>Frecuencia De CPU</dt><dd>{1}MHz</dd>";
const char HTTP_INFO_apip[]       PROGMEM = "<dt>IP Del Punto De Acceso</dt><dd>{1}</dd>";
const char HTTP_INFO_apmac[]      PROGMEM = "<dt>MAC Del Punto De Acceso</dt><dd>{1}</dd>";
const char HTTP_INFO_apssid[]     PROGMEM = "<dt>SSID Del Punto De Acceso</dt><dd>{1}</dd>";
const char HTTP_INFO_apbssid[]    PROGMEM = "<dt>BSSID</dt><dd>{1}</dd>";
const char HTTP_INFO_stassid[]    PROGMEM = "<dt>Estación SSID</dt><dd>{1}</dd>";
const char HTTP_INFO_staip[]      PROGMEM = "<dt>Estación IP</dt><dd>{1}</dd>";
const char HTTP_INFO_stagw[]      PROGMEM = "<dt>Puerta De Enlace De Estación</dt><dd>{1}</dd>";
const char HTTP_INFO_stasub[]     PROGMEM = "<dt>Subred De La Estación</dt><dd>{1}</dd>";
const char HTTP_INFO_dnss[]       PROGMEM = "<dt>Servidor DNS</dt><dd>{1}</dd>";
const char HTTP_INFO_host[]       PROGMEM = "<dt>Nombre De Host</dt><dd>{1}</dd>";
const char HTTP_INFO_stamac[]     PROGMEM = "<dt>MAC De La Estación</dt><dd>{1}</dd>";
const char HTTP_INFO_conx[]       PROGMEM = "<dt>Conectado</dt><dd>{1}</dd>";
const char HTTP_INFO_autoconx[]   PROGMEM = "<dt>Conexión Automática</dt><dd>{1}</dd>";
const char HTTP_INFO_aboutver[]     PROGMEM = "<dt>M8AX-WiFiManager-M8AX</dt><dd>{1}</dd>";
const char HTTP_INFO_aboutarduino[] PROGMEM = "<dt>Arduino</dt><dd>{1}</dd>";
const char HTTP_INFO_aboutsdk[]     PROGMEM = "<dt>ESP-SDK/IDF</dt><dd>{1}</dd>";
const char HTTP_INFO_aboutdate[]    PROGMEM = "<dt>Fecha De Compilación Del Firmware</dt><dd>{1}</dd>";
const char S_brand[]              PROGMEM = "M8AX-WiFiManager-M8AX";
const char S_debugPrefix[]        PROGMEM = "*wm:";              // *wm:
const char S_y[]                  PROGMEM = "Sí";               // Sí
const char S_n[]                  PROGMEM = "No";               // No
const char S_enable[]             PROGMEM = "Habilitado";       // Habilitado
const char S_disable[]            PROGMEM = "Deshabilitado";    // Deshabilitado
const char S_GET[]                PROGMEM = "Obtener";          // Obtener
const char S_POST[]               PROGMEM = "Enviar";           // Enviar
const char S_NA[]                 PROGMEM = "Desconocido";      // Desconocido
const char S_passph[]             PROGMEM = "********";         // ********
const char S_titlewifisaved[]     PROGMEM = "Credenciales Guardadas"; // Credenciales Guardadas
const char S_titlewifisettings[]  PROGMEM = "Configuración Guardada"; // Configuración Guardada
const char S_titlewifi[]          PROGMEM = "Configurar Esp";  // Configurar Esp
const char S_titleinfo[]          PROGMEM = "Información";      // Información
const char S_titleparam[]         PROGMEM = "Configuración";   // Configuración
const char S_titleparamsaved[]    PROGMEM = "Configuración Guardada"; // Configuración Guardada
const char S_titleexit[]          PROGMEM = "Salir";            // Salir
const char S_titlereset[]         PROGMEM = "Reiniciar";        // Reiniciar
const char S_titleerase[]         PROGMEM = "Borrar";           // Borrar
const char S_titleclose[]         PROGMEM = "Cerrar";           // Cerrar
const char S_options[]            PROGMEM = "Opciones";        // Opciones
const char S_nonetworks[]         PROGMEM = "No Se Encontraron Redes. Actualiza Para Buscar Nuevamente."; // No Se Encontraron Redes. Actualiza Para Buscar Nuevamente.
const char S_staticip[]           PROGMEM = "Ip Estática";      // Ip Estática
const char S_staticgw[]           PROGMEM = "Gateway Estático"; // Gateway Estático
const char S_staticdns[]          PROGMEM = "Dns Estático";     // Dns Estático
const char S_subnet[]             PROGMEM = "Subred";           // Subred
const char S_exiting[]            PROGMEM = "Saliendo";         // Saliendo
const char S_resetting[]          PROGMEM = "El Módulo Se Reiniciará En Unos Segundos."; // El Módulo Se Reiniciará En Unos Segundos.
const char S_closing[]            PROGMEM = "Puedes Cerrar La Página, El Portal Seguirá Funcionando"; // Puedes Cerrar La Página, El Portal Seguirá Funcionando
const char S_error[]              PROGMEM = "Ocurrió Un Error"; // Ocurrió Un Error
const char S_notfound[]           PROGMEM = "Archivo No Encontrado\n\n"; // Archivo No Encontrado
const char S_uri[]                PROGMEM = "Uri: ";           // Uri: 
const char S_method[]             PROGMEM = "\nMétodo: ";      // Método: 
const char S_args[]               PROGMEM = "\nArgumentos: ";  // Argumentos: 
const char S_parampre[]           PROGMEM = "Param_";          // Param_

// debug strings
const char D_HR[]                 PROGMEM = "--------------------";

// softap ssid default prefix
#ifdef ESP8266
    const char S_ssidpre[]        PROGMEM = "ESP";
#elif defined(ESP32)
    const char S_ssidpre[]        PROGMEM = "ESP32";
#else
    const char S_ssidpre[]        PROGMEM = "WM";
#endif

// END WIFI_MANAGER_OVERRIDE_STRINGS
#endif

#endif