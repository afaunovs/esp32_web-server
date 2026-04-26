#include<WiFi.h>
#include<ESPmDNS.h>
#include<WebSocketsServer.h>
#include<ESPAsyncWebServer.h>
#include<ArduinoJson.h>
#include<ESP32Servo.h>

char webpage[] PROGMEM = R"=====(   
<!DOCTYPE html>
<html lang="en">

<head>
    <meta charset="UTF-8">
    <title>ESP32_WiFI_Control</title>
    <!-- <link rel="stylesheet" type="text/css" href="style.css"> -->
</head>
<style>
    .vertical{
-webkit-transform: rotate(-90deg);
-moz-transform: rotate(-90deg);
-o-transform: rotate(-90deg);
-ms-transform: rotate(-90deg);
transform: rotate(-90deg);
}

.horizontal{
-webkit-transform: rotate(180deg);
-moz-transform: rotate(180deg);
-o-transform: rotate(180deg);
-ms-transform: rotate(180deg);
transform: rotate(180deg);
}

.switch {
  position: relative;
  margin-left: 400px;
  display: inline-block;
  width: 60px;
  height: 34px;
}

/* Hide default HTML checkbox */
.switch input {display:none;}

/* The slider */
.slider {
  position: absolute;
  cursor: pointer;
  top: 0;
  left: 0;
  right: 0;
  bottom: 0;
  background-color: #ccc;
  -webkit-transition: .4s;
  transition: .4s;
}

.slider:before {
  position: absolute;
  content: "";
  height: 26px;
  width: 26px;
  left: 4px;
  bottom: 4px;
  background-color: white;
  -webkit-transition: .4s;
  transition: .4s;
}

input:checked + .slider {
  background-color: #2196F3;
}

input:focus + .slider {
  box-shadow: 0 0 1px #2196F3;
}

input:checked + .slider:before {
  -webkit-transform: translateX(26px);
  -ms-transform: translateX(26px);
  transform: translateX(26px);
}

/* Rounded sliders */
.slider.round {
  border-radius: 34px;
}

.slider.round:before {
  border-radius: 50%;
}
</style>
<body>
<h1 style ="text-align: center;">ESP32_WIFI</h1>

<div class="conrol" style="margin-center: 100px">
<label class="switch">
    <input type="checkbox"min="0" max="180" value="0" id="GIO4" oninput="Switch(this.value)">
    <span class="slider round"></span>
    <label style="margin-left:80px;" id="labely" oninput="Switch" >off</label>
</label>
   

<div class ="container" style ="text-align: center; margin-right: 150px;"><br><br>
    <input class="range vertical" type="range" min="0" max="500" value="0" id="Motor" oninput="Motor(this.value)">
</div>

<div class ="container" style ="text-align: center; margin-left: 100px">
    <input class="range horizontal" type="range" min="40" max="140" value="90" id="Servo" oninput="Servo(this.value)">
</div>


</div>

 <!--<script src="main.js"></script>  gradys(45-90-135) or (60-90-120)  -->
<script>
var connection = new WebSocket('ws://' + window.location.hostname + ':81/');

var data_mot =0 ;
var data_serv =0 ;
var GIO = document.getElementById("Switch(this.value)");
var label = document.getElementById("labely");

function Motor()
{
data_mot = document.getElementById("Motor").value;
send_data();
}

function Servo()
{
data_serv = document.getElementById("Servo").value;
send_data();
}


function send_data()
{
var full_data = '{"Mot" :'+data_mot+', "Serv" :'+data_serv+'}';
connection.send(full_data);
console.log(full_data);
}


</script>


</body>
</html>
)=====";

                          // PROGMEM -записывает в флэш память, иначе если его не будет сохранится в опер.память
                          //= R"=====(    )====="; - необработаный строковый литерал, тюе можно вставить внутрь html код

AsyncWebServer server(80); //server port 80
WebSocketsServer websockets(81);

Servo servo_s;

#define motor  14
#define servo  15
#define diod  4



void NotFound(AsyncWebServerRequest * request){
    request->send(404, "text/plain", "Not found");
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length){
    switch(type){
        case WStype_DISCONNECTED:
            Serial.printf("[%u] Disconnect!\n", num);
            break;
        case WStype_CONNECTED:{
            IPAddress ip = websockets.remoteIP(num);
            Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
            
            // send message to client
            websockets.sendTXT(num, "Connected from server");
            servo_s.write(80);
            delay(1000);
            servo_s.write(100);
            delay(1000);
            servo_s.write(90);
        }
        break;
        case WStype_TEXT:
            Serial.printf("[%u] get Text: %s\n", num, payload);
            String message = String((char *)(payload));
            Serial.println(message);

            DynamicJsonDocument doc(200);
            //deserialize the data
            DeserializationError error = deserializeJson(doc, message);
            //parse the paraments we expect to receive (TO-DO: error handling)
            //Test if parsing succeeds
            if (error){
                Serial.print("deserializeJson() failed: ");
                Serial.println(error.c_str());
            }
            int mot = doc["Mot"];
            int serv = doc["Serv"];
            int GIO = doc["GIO"];
            
            analogWrite(motor, mot);

            servo_s.write(serv);

            if (GIO == 1)
            {
            digitalWrite(diod, HIGH);
            }
            else{
              digitalWrite(diod, LOW);
            }
            
    }
}
const char* ssid = "****";
const char* password = "*******";
void setup(){
    Serial.begin(115200);

    ledcSetup(3, 2000, 8); //chanel 3..(4,5,6) 2000 ГЦ шим сигнал  на 8 бит
    ledcAttachPin(motor, 3); //подключение к пину

    servo_s.setPeriodHertz(50);    // standard 50 hz servo
  
    servo_s.attach(servo, 0, 3000);// //подключение к пину
    
    servo_s.write(80);
    delay(1000);
    servo_s.write(100);
    delay(1000);
    servo_s.write(90);
    
    pinMode(diod, OUTPUT);

    WiFi.softAP("logistics", "12345678");
    Serial.println("softap");
    Serial.println("");
    Serial.println(WiFi.softAPIP());

    websockets.onEvent(webSocketEvent); //initialization websockets
    
    if (MDNS.begin("ESP")){             //Присвоили адрес IP, т.е esp это имя домена и будет открываться командой -->
        //esp.local/                    
        Serial.println("MDNS responder started");
    }

    server.on("/",[](AsyncWebServerRequest *request){       // страницы webserver

        request->send_P(200, "text/html", webpage);     //_P - означает что смотри запись в флэш памяти
    });

      server.on("/page1", HTTP_GET,[](AsyncWebServerRequest *request){
        String message = "Welcome to page1";
        request->send(200, "text/html", message);
    });

    server.onNotFound(NotFound);

    server.begin();   // it will start webseerver
    websockets.begin();

}

void loop(){
    websockets.loop();
}
