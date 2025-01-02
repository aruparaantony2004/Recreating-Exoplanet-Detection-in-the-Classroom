#include <WiFi.h>
#include <esp_now.h>

//Reference positions and time
float ref_position=30; // max length of rod (in cm)
float ref_time=60000; // ref time taken-1rpm, in millisecond
float tON=278; //test this, it is the time required in milliseconds for moving by 1cm at 130 PWM.

int ledPin = 2;
// Motor pins
const int EN1 = 5; // Enable pin
const int IN1 = 18; // Input pin 1
const int IN2 = 19; // Input pin 2

WiFiServer server(80);

float radius = 0; //  the current radius
float old_radius=0;

// MAC Address of responder - edit as required
uint8_t broadcastAddress[] = {0xE8, 0x68, 0xE7, 0x35, 0x7B, 0x40};

// Define a data structure
typedef struct struct_message {
  float data_to_send;

} struct_message;
 
// Create a structured object
struct_message myData;

// Peer info
esp_now_peer_info_t peerInfo;

// Callback function called when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

const char *ssid = "IIST TRANSIT MODEL";
const char *password = "sspaceIIST";

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("Configuring access point...");
  WiFi.mode(WIFI_STA);

  WiFi.softAP(ssid, password);
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
  server.begin();
  Serial.println("Server started");

  // Initilize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
 
  // Register the send callback
  esp_now_register_send_cb(OnDataSent);
  
  // Register peer
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  
  // Add peer        
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }
  // Set LED as output
  pinMode(ledPin, OUTPUT);
  pinMode(EN1, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);

  // Initially stop the motor
  digitalWrite(EN1, LOW);
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);

    
}

void send_to_responder(){
  // Send message via ESP-NOW
  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));
  if (result == ESP_OK) {
    Serial.println("Sending confirmed");
  }
  else {
    Serial.println("Sending error");
    Serial.println(result); 
  }
  if(result == ESP_ERR_ESPNOW_NOT_INIT) {
    Serial.println("ESPNOW is not initialized");
  }
  else if(result == ESP_ERR_ESPNOW_ARG) {
    Serial.println("invalid argument");
  }

  else if(result == ESP_ERR_ESPNOW_INTERNAL){
    Serial.println("internal error");
  }
  else if (result == ESP_ERR_ESPNOW_NO_MEM){
    Serial.println("out of memory, when this happens, you can delay a while before sending the next data");
  }
  else if (result = ESP_ERR_ESPNOW_IF){
    Serial.println(": current Wi-Fi interface doesn't match that of peer");
  }
  else if (result == ESP_ERR_ESPNOW_NOT_FOUND){
    Serial.println(" : peer is not found");
  }
}

void loop() {
  bool webPageSent=0;  // variable to check whether website has been sent or not
  WiFiClient client = server.available();   // checking if client is connected
  
  //if there is client
  if (client) {
    Serial.println("Client connected"); // Debug statement

    String request = client.readStringUntil('\r');
    request.trim();
    Serial.print("Request received: "); // Debug statement
    Serial.println(request); // Debug statement

    // Extracting the slider value from the request
    int pos = request.indexOf("slider=");
    Serial.print("pos :");
    
    Serial.println(pos);
    if (pos != -1) {
      String sliderValue = request.substring(pos + 7);
      Serial.print("Slider value received: "); // Debug statement
      Serial.println(sliderValue); // Debug statement
      radius = sliderValue.toFloat(); // Update the radius value


      if (radius<0)
      {
        if(radius==-1)
        {
          sendcalibrationWebPage(client);
          webPageSent=1;
          Serial.println("Calibration webpage sent");
        }
        
        else if(radius!=-500) {

          //Radius<0 this means calibration webpage is sent to client
          sendcalibrationWebPage(client);
          Serial.println("now in calibration webpage");
            
          webPageSent=1;

          //Turn off the motors during calibration
          digitalWrite(IN1, LOW);
          digitalWrite(IN2, LOW);
             
          float on_time_upper_motor;
          if(radius ==-100){
            Serial.println("Coarse forward calibration.");
            on_time_upper_motor = tON;
          }
          else if(radius == -200){
            Serial.println("Coarse backward calibration.");
            on_time_upper_motor = -tON;
          }
          if(radius ==-300){
            Serial.println("Fine forward calibration.");
            on_time_upper_motor = tON/2;
          }
          else if(radius == -400){
            Serial.println("Fine backward calibration.");
            on_time_upper_motor = -tON/2;
          }

          //sending radius to responder
          myData.data_to_send = radius;
          //sending to responder
          send_to_responder();
          Serial.print("Calibration ongoing ->");
          delay(1000);
          Serial.println("Calibration over");
          Serial.print("radius");
          Serial.println(radius);     
        }
        else {

          //radius =-500  :- this means the calibration is over and we need to go back to the main page

          //sending main webpage to client
          // sendWebPage(client,radius); // Send main webpage to the client
          //setting radius=0
          radius=0;
          myData.data_to_send=0;
          old_radius=0;
          sendWebPage(client,radius); // Send main webpage to the client
          webPageSent=1;
          Serial.println("Main website sent");
          }
        
      }
      else
      {
        //Radius > 0 this means main webpage has been sent to client and client has set some radius
        sendWebPage(client,radius); // Send main webpage to the client
        webPageSent=1;

        float model_radius=map(radius,0,100,0,30);
        float model_old_radius=map(old_radius,0,100,0,30);
       
        float model_deltaR = model_radius - model_old_radius;
        Serial.print("Model's_Delta R: ");
        Serial.println(model_deltaR);

        float on_time_upper_motor= (model_deltaR)*tON;
        Serial.print("On time upper motor: ");
        Serial.println(on_time_upper_motor);
        
        //sending radius to responder
        myData.data_to_send = radius;

        //sending to responder
        if(radius!=old_radius)
        {
          send_to_responder();
        }

         Serial.print("Radius updated to: "); // Debug statement
        // radius=slider;
        Serial.println(radius); // Debug statement

        delay(abs(on_time_upper_motor)+3000); // time for the upper part to set the specified radius

        if(radius>0) // no revolution for 0 radius
        {
          //turning motors
          float current_time_period = (pow(model_radius/ref_position ,1.5))*ref_time ; //in milliseconds
          Serial.print("Current time period of revolution.");
          Serial.println(current_time_period); 
          Serial.print("rpm of the lower motor: ");
          float rpm_of_lower_motor=60000/current_time_period; 
          Serial.println(rpm_of_lower_motor);
          
          float pwm_value= map(rpm_of_lower_motor,1,11.2,78,255);// 1.5 min rpm of motor, 9 max rpm of motor, TEST 60 AND 255 THE RANGES OF PWM
          
          digitalWrite(IN1, HIGH);
          digitalWrite(IN2, LOW);
          Serial.println("Motor rotating at(pwm value): ");
          Serial.println(pwm_value);
          analogWrite(EN1,pwm_value); // Set motor speed
          delay(3*current_time_period); //waiting for 3 orbits
        }
        
        //stopping the motors after revolution
        digitalWrite(IN1, LOW);
        digitalWrite(IN2, LOW);

        old_radius=radius;

      }

    }
    
  }

  if(webPageSent==0)
  {
   sendWebPage(client,radius); // Send main webpage to the client
  }
}


//main page
void sendWebPage(WiFiClient &client,int receivedradius) {

  client.println("<!DOCTYPE html>");
  client.println("<html lang=\"en\">");
  client.println("<head>");
  client.println("    <meta charset=\"UTF-8\">");
  client.println("    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">");
  client.println("    <title>Exoplanet Detection Model</title>");
  client.println("    <!-- <style>");
  client.println("        body {");
  client.println("            margin: 0;");
  client.println("            padding: 0;");
  client.println("            font-family: Arial, sans-serif;");
  client.println("            color: #ffffff;");
  client.println("            background: linear-gradient(to bottom, #1d2671, #c33764);");
  client.println("            background-size: cover;");
  client.println("            display: flex;");
  client.println("            flex-direction: column;");
  client.println("            align-items: center;");
  client.println("            min-height: 100vh;");
  client.println("            box-sizing: border-box;");
  client.println("        }");
  client.println("");
  client.println("        .hero_area header {");
  client.println("            text-align: center;");
  client.println("            padding: 30px;");
  client.println("            background-color: rgba(0, 0, 0, 0.3);");
  client.println("            color: #ffd700;");
  client.println("        }");
  client.println("");
  client.println("        .main-title {");
  client.println("            font-size: 2.5em;");
  client.println("            font-weight: bold;");
  client.println("            text-shadow: 2px 2px 4px rgba(0, 0, 0, 0.5);");
  client.println("        }");
  client.println("");
  client.println("        .sub-title, .author-names {");
  client.println("            font-size: 1em;");
  client.println("            color: #f1f1f1;");
  client.println("            margin-top: 10px;");
  client.println("            letter-spacing: 1.5px;");
  client.println("        }");
  client.println("");
  client.println("        .about_section {");
  client.println("            width: 90%;");
  client.println("            max-width: 900px;");
  client.println("            background: rgba(0, 0, 0, 0.7);");
  client.println("            color: #ffd700;");
  client.println("            border-radius: 10px;");
  client.println("            padding: 20px;");
  client.println("            margin: 20px;");
  client.println("            box-shadow: 0 0 10px rgba(0, 0, 0, 0.3);");
  client.println("        }");
  client.println("");
  client.println("        h2 {");
  client.println("            font-size: 1.8em;");
  client.println("            color: #ffd700;");
  client.println("            text-align: center;");
  client.println("            margin-bottom: 15px;");
  client.println("        }");
  client.println("");
  client.println("        p {");
  client.println("            line-height: 1.6;");
  client.println("            font-size: 1em;");
  client.println("            color: #d1d1d1;");
  client.println("        }");
  client.println("");
  client.println("        #adjustDistance {");
  client.println("            width: 90%;");
  client.println("            max-width: 900px;");
  client.println("            padding: 20px;");
  client.println("            margin: 20px;");
  client.println("            background: rgba(0, 0, 0, 0.7);");
  client.println("            border-radius: 10px;");
  client.println("            box-shadow: 0 0 10px rgba(0, 0, 0, 0.3);");
  client.println("        }");
  client.println("");
  client.println("        #distanceScale {");
  client.println("            display: flex;");
  client.println("            justify-content: space-between;");
  client.println("            font-size: 0.8em;");
  client.println("            color: #ccc;");
  client.println("            margin: 15px 0;");
  client.println("        }");
  client.println("");
  client.println("        #distanceSlider {");
  client.println("            width: 100%;");
  client.println("            margin: 10px 0;");
  client.println("            accent-color: #ffd700;");
  client.println("        }");
  client.println("");
  client.println("        #additionalFields {");
  client.println("            display: flex;");
  client.println("            flex-wrap: wrap;");
  client.println("            gap: 15px;");
  client.println("            margin: 15px 0;");
  client.println("        }");
  client.println("");
  client.println("        #additionalFields label, #additionalFields input {");
  client.println("            flex: 1 1 45%;");
  client.println("            font-size: 1em;");
  client.println("            color: #ffd700;");
  client.println("        }");
  client.println("");
  client.println("        #additionalFields input {");
  client.println("            background: rgba(255, 255, 255, 0.1);");
  client.println("            color: #fff;");
  client.println("            border: 1px solid #ffd700;");
  client.println("            padding: 8px;");
  client.println("            border-radius: 5px;");
  client.println("        }");
  client.println("");
  client.println("        button {");
  client.println("            background-color: #4CAF50;");
  client.println("            color: #fff;");
  client.println("            padding: 10px 15px;");
  client.println("            border: none;");
  client.println("            border-radius: 5px;");
  client.println("            cursor: pointer;");
  client.println("            font-size: 1em;");
  client.println("            margin-top: 10px;");
  client.println("            transition: background-color 0.3s ease;");
  client.println("        }");
  client.println("");
  client.println("        button:hover {");
  client.println("            background-color: #45a049;");
  client.println("        }");
  client.println("");
  client.println("        /* Media Queries */");
  client.println("        @media (min-width: 768px) {");
  client.println("            .main-title {");
  client.println("                font-size: 3em;");
  client.println("            }");
  client.println("");
  client.println("            .about_section, #adjustDistance {");
  client.println("                max-width: 60vw;");
  client.println("            }");
  client.println("        }");
  client.println("    </style> -->");
  client.println("");
  client.println("    <style>");
  client.println("        body {");
  client.println("            margin: 0;");
  client.println("            padding: 0;");
  client.println("            font-family: Arial, sans-serif;");
  client.println("            color: #e0e0e0;");
  client.println("            background: radial-gradient(circle at top right, #0f2027, #203a43, #2c5364);");
  client.println("            display: flex;");
  client.println("            flex-direction: column;");
  client.println("            align-items: center;");
  client.println("            min-height: 100vh;");
  client.println("            box-sizing: border-box;");
  client.println("        }");
  client.println("    ");
  client.println("        .hero_area header {");
  client.println("            text-align: center;");
  client.println("            padding: 40px 20px;");
  client.println("            background-color: rgba(0, 0, 0, 0.5);");
  client.println("            color: #ffc107;");
  client.println("            box-shadow: 0 4px 10px rgba(0, 0, 0, 0.2);");
  client.println("            border-radius: 8px;");
  client.println("            max-width: 1000px;");
  client.println("            margin: 20px auto;");
  client.println("            width: 90%;");
  client.println("        }");
  client.println("    ");
  client.println("        .main-title {");
  client.println("            font-size: 2.8em;");
  client.println("            font-weight: 700;");
  client.println("            text-shadow: 1px 2px 6px rgba(0, 0, 0, 0.6);");
  client.println("            margin-bottom: 10px;");
  client.println("        }");
  client.println("    ");
  client.println("        .sub-title, .author-names {");
  client.println("            font-size: 1em;");
  client.println("            color: #ccc;");
  client.println("            letter-spacing: 1.2px;");
  client.println("        }");
  client.println("    ");
  client.println("        .about_section {");
  client.println("            width: 90%;");
  client.println("            max-width: 900px;");
  client.println("            background: rgba(17, 24, 39, 0.8);");
  client.println("            border: 1px solid rgba(255, 193, 7, 0.3);");
  client.println("            border-radius: 10px;");
  client.println("            padding: 25px;");
  client.println("            margin: 20px;");
  client.println("            box-shadow: 0 4px 12px rgba(0, 0, 0, 0.2);");
  client.println("            text-align: center;");
  client.println("        }");
  client.println("    ");
  client.println("        h2 {");
  client.println("            font-size: 2em;");
  client.println("            color: #ffc107;");
  client.println("            margin-bottom: 15px;");
  client.println("        }");
  client.println("    ");
  client.println("        p {");
  client.println("            line-height: 1.7;");
  client.println("            font-size: 1.1em;");
  client.println("            margin: 0;");
  client.println("            color: #ddd;");
  client.println("        }");
  client.println("    ");
  client.println("        #adjustDistance {");
  client.println("            width: 90%;");
  client.println("            max-width: 900px;");
  client.println("            padding: 20px;");
  client.println("            margin: 20px;");
  client.println("            background: rgba(0, 0, 0, 0.7);");
  client.println("            border-radius: 10px;");
  client.println("            box-shadow: 0 4px 12px rgba(0, 0, 0, 0.3);");
  client.println("            text-align: center;");
  client.println("        }");
  client.println("    ");
  client.println("        #distanceScale {");
  client.println("            display: flex;");
  client.println("            justify-content: space-between;");
  client.println("            font-size: 0.9em;");
  client.println("            color: #ccc;");
  client.println("            margin: 15px 0;");
  client.println("        }");
  client.println("    ");
  client.println("        #distanceSlider {");
  client.println("            width: 100%;");
  client.println("            margin: 10px 0;");
  client.println("            accent-color: #ffc107;");
  client.println("        }");
  client.println("    ");
  client.println("        #additionalFields {");
  client.println("            display: flex;");
  client.println("            flex-wrap: wrap;");
  client.println("            gap: 15px;");
  client.println("            margin: 15px 0;");
  client.println("        }");
  client.println("    ");
  client.println("        #additionalFields label, #additionalFields input {");
  client.println("            flex: 1 1 45%;");
  client.println("            font-size: 1em;");
  client.println("            color: #ffc107;");
  client.println("        }");
  client.println("    ");
  client.println("        #additionalFields input {");
  client.println("            background: rgba(255, 255, 255, 0.1);");
  client.println("            color: #fff;");
  client.println("            border: 1px solid #ffc107;");
  client.println("            padding: 8px;");
  client.println("            border-radius: 5px;");
  client.println("        }");
  client.println("    ");
  client.println("        button {");
  client.println("            background-color: #28a745;");
  client.println("            color: #fff;");
  client.println("            padding: 10px 15px;");
  client.println("            border: none;");
  client.println("            border-radius: 5px;");
  client.println("            cursor: pointer;");
  client.println("            font-size: 1em;");
  client.println("            margin-top: 10px;");
  client.println("            transition: background-color 0.3s ease;");
  client.println("        }");
  client.println("    ");
  client.println("        button:hover {");
  client.println("            background-color: #218838;");
  client.println("        }");
  client.println("    ");
  client.println("        /* Media Queries */");
  client.println("        @media (min-width: 768px) {");
  client.println("            .main-title {");
  client.println("                font-size: 3em;");
  client.println("            }");
  client.println("    ");
  client.println("            .about_section, #adjustDistance {");
  client.println("                max-width: 60vw;");
  client.println("            }");
  client.println("        }");
  client.println("    </style>");
  client.println("</head>");
  client.println("<body>");
  client.println("    <div class=\"hero_area\">");
  client.println("        <header>");
  client.println("            <div class=\"heading\">");
  client.println("                <div class=\"main-title\">Discovering Alien Worlds</div>");
  client.println("                <div class=\"sub-title\">Understanding the Transit Method of Exoplanet Detection</div>");
  client.println("                <div class=\"author-names\">by Soham Mali, Antony Arupara, Siddhey Nagpure, Dr Anand Narayan, Dr Priyadrashan, Lokaveer, Yasir, Yoga</div>");
  client.println("            </div>");
  client.println("        </header>");
  client.println("    </div>");
  client.println("");
  client.println("    <section class=\"about_section\">");
  client.println("        <h2>About Transit Method</h2>");
  client.println("        <p>");
  client.println("            The Transit Method is a scientific technique used to detect exoplanets, planets that orbit stars outside our solar system. It involves observing the slight dimming of a star's light when an orbiting exoplanet passes in front of it. This temporary reduction in brightness, known as a transit, provides valuable information about the exoplanet, such as its size and orbital characteristics.");
  client.println("        </p>");
  client.println("    </section>");
  client.println("");
  client.println("    <section id=\"adjustDistance\">");
  client.println("        <h2>Adjust Radial Distance</h2>");
  client.println("        <p>");
  client.println("            Use the slider below to adjust the radial distance of the model planet from the model sun. Explore how changes in distance affect the simulated transits and learn more about the factors influencing exoplanet detection.");
  client.println("        </p>");
  client.println("        <div id=\"distanceScale\">");
  client.println("            <span>0 AU</span>");
  client.println("            <span>2 AU</span>");
  client.println("            <span>4 AU</span>");
  client.println("            <span>6 AU</span>");
  client.println("            <span>8 AU</span>");
  client.println("            <span>10 AU</span>");
  client.println("        </div>");
  client.println("                               ");
  client.println("        <form id=\"distanceValue\">");
  client.println("                                    ");
  client.println("                <input onchange=\"temp();\" type=\"range\" min=\"0\" max=\"100\" step=\"20\" value="+ String(receivedradius) +" name=\"slider\" id=\"distanceSlider\">");
  client.println("                                    ");
  client.println("                                 ");
  client.println("                               ");
  client.println("                                ");
  client.println("        <div id=\"additionalFields\">");
  client.println("            <label for=\"radiusRatio\">Radius ratio of orbit:</label>");
  client.println("            <input type=\"text\" id=\"radiusRatio\" readonly>");
  client.println("        </div>");
  client.println("        ");
  client.println("        <button type=\"button\" id=\"submitButton\">Submit</button>");
  client.println("        <button type=\"button\" id=\"resetButton\">Reset</button>");
  client.println("        <button type=\"button\" id=\"calibrationButton\">Calibration</button>");
  client.println("    </section>");
  client.println("");
  client.println("    <script>");
  client.println("        const distanceSlider = document.getElementById('distanceSlider');");
  client.println("        const distanceValue = document.getElementById('distanceValue');");
  client.println("        const radiusRatioField = document.getElementById('radiusRatio');");
  client.println("        const transitTime = document.getElementById('transitTime');");
  client.println("        const resetButton = document.getElementById('resetButton');");
  client.println("        const submitButton = document.getElementById('submitButton');");
  client.println("        const calibrationButton = document.getElementById('calibrationButton');");
  client.println("");
  client.println("        function temp() {");
  client.println("            const radialDistance = distanceSlider.value;");
  client.println("            distanceText.innerHTML = radialDistance;");
  client.println("            updateValues();");
  client.println("        }");
  client.println("");
  client.println("        function initialize() {");
  client.println("                const radialDistance = distanceSlider.value;");
  client.println("                distanceText.innerHTML = receivedradius;");
  client.println("                        updateValues();");
  client.println("            }");
  client.println("");
  client.println("        function updateValues() {");
  client.println("            const radialDistance = parseInt(distanceSlider.value);");
  client.println("            const radiusRatio = radialDistance/10 ;");
  client.println("            const calculatedTransitTime = 2 * radialDistance;");
  client.println("            // const calculatedRotationSpeed = 2 * radialDistance;");
  client.println("");
  client.println("            radiusRatioField.value = radiusRatio.toFixed(2) + 'AU ';");
  client.println("            transitTime.value = calculatedTransitTime + ' hours';");
  client.println("        }");
  client.println("");
  client.println("    //      // Event listener for slider hover");
  client.println("    // distanceSlider.addEventListener('input', function () {");
  client.println("    //     const radialDistance = parseInt(distanceSlider.value);");
  client.println("    //     if ([0,20 , 40, 60, 80,100].includes(radialDistance)) {");
  client.println("    //         updateValues(radialDistance);");
  client.println("    //     }");
  client.println("    // });");
  client.println("");
  client.println("        function resetSliderValue() {");
  client.println("            distanceSlider.value = 0;");
  client.println("            submitSliderValue();");
  client.println("            radialDistance = 0;");
  client.println("            const radialDistance =0;");
  client.println("            window.location.href = \"slider=\" + radialDistance;");
  client.println("            updateValues();");
  client.println("        }");
  client.println("");
  client.println("         // Event listener for any slider input");
  client.println("        distanceSlider.addEventListener('input', updateValues);");
  client.println("");
  client.println("        submitButton.addEventListener('click', submitSliderValue);");
  client.println("        resetButton.addEventListener('click', resetSliderValue);");
  client.println("        calibrationButton.addEventListener('click', () => { window.location.href = \"slider=-1\"; });");
  client.println("");
  client.println("        function submitSliderValue() {");
  client.println("                const radialDistance = parseInt(distanceSlider.value);");
  client.println("                window.location.href = \"slider=\" + radialDistance;");
  client.println("            }");
  client.println("        ");
  client.println("                    // Set interval to update values every 500 milliseconds (adjust as necessary)");
  client.println("                    setInterval(() => {");
  client.println("                        updateValues();");
  client.println("                    }, 500); // Update every half second");
  client.println("        temp();");
  client.println("                    initialize();");
  client.println("        updateValues();");
  client.println("");
  client.println("    </script>");
  client.println("</body>");
  client.println("</html>");
}

//calibration page
void sendcalibrationWebPage(WiFiClient &client) {
  client.println("<!DOCTYPE html>");
  client.println("<html lang=\"en\">");
  client.println("<head>");
  client.println("    <meta charset=\"UTF-8\">");
  client.println("    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">");
  client.println("    <title>Calibration Page</title>");
  client.println("    <!-- <style>");
  client.println("        body {");
  client.println("            margin: 0;");
  client.println("            padding: 0;");
  client.println("            font-family: Arial, sans-serif;");
  client.println("            color: #ffffff;");
  client.println("            background: linear-gradient(to bottom, #1d2671, #c33764);");
  client.println("            background-size: cover;");
  client.println("            display: flex;");
  client.println("            flex-direction: column;");
  client.println("            align-items: center;");
  client.println("            min-height: 100vh;");
  client.println("            box-sizing: border-box;");
  client.println("        }");
  client.println("");
  client.println("        .hero_area header {");
  client.println("            text-align: center;");
  client.println("            padding: 30px;");
  client.println("            background-color: rgba(0, 0, 0, 0.3);");
  client.println("            color: #ffd700;");
  client.println("        }");
  client.println("");
  client.println("        .main-title {");
  client.println("            font-size: 2.5em;");
  client.println("            font-weight: bold;");
  client.println("            text-shadow: 2px 2px 4px rgba(0, 0, 0, 0.5);");
  client.println("        }");
  client.println("");
  client.println("        .sub-title, .author-names {");
  client.println("            font-size: 1em;");
  client.println("            color: #f1f1f1;");
  client.println("            margin-top: 10px;");
  client.println("            letter-spacing: 1.5px;");
  client.println("        }");
  client.println("");
  client.println("        .about_section {");
  client.println("            width: 90%;");
  client.println("            max-width: 900px;");
  client.println("            background: rgba(0, 0, 0, 0.7);");
  client.println("            color: #ffd700;");
  client.println("            border-radius: 10px;");
  client.println("            padding: 20px;");
  client.println("            margin: 20px;");
  client.println("            box-shadow: 0 0 10px rgba(0, 0, 0, 0.3);");
  client.println("        }");
  client.println("");
  client.println("        h2 {");
  client.println("            font-size: 1.8em;");
  client.println("            color: #ffd700;");
  client.println("            text-align: center;");
  client.println("            margin-bottom: 15px;");
  client.println("        }");
  client.println("");
  client.println("        p {");
  client.println("            line-height: 1.6;");
  client.println("            font-size: 1em;");
  client.println("            color: #d1d1d1;");
  client.println("        }");
  client.println("");
  client.println("        #adjustDistance {");
  client.println("            width: 90%;");
  client.println("            max-width: 900px;");
  client.println("            padding: 20px;");
  client.println("            margin: 20px;");
  client.println("            background: rgba(0, 0, 0, 0.7);");
  client.println("            border-radius: 10px;");
  client.println("            box-shadow: 0 0 10px rgba(0, 0, 0, 0.3);");
  client.println("        }");
  client.println("");
  client.println("        #distanceScale {");
  client.println("            display: flex;");
  client.println("            justify-content: space-between;");
  client.println("            font-size: 0.8em;");
  client.println("            color: #ccc;");
  client.println("            margin: 15px 0;");
  client.println("        }");
  client.println("");
  client.println("        #distanceSlider {");
  client.println("            width: 100%;");
  client.println("            margin: 10px 0;");
  client.println("            accent-color: #ffd700;");
  client.println("        }");
  client.println("");
  client.println("        #additionalFields {");
  client.println("            display: flex;");
  client.println("            flex-wrap: wrap;");
  client.println("            gap: 15px;");
  client.println("            margin: 15px 0;");
  client.println("        }");
  client.println("");
  client.println("        #additionalFields label, #additionalFields input {");
  client.println("            flex: 1 1 45%;");
  client.println("            font-size: 1em;");
  client.println("            color: #ffd700;");
  client.println("        }");
  client.println("");
  client.println("        #additionalFields input {");
  client.println("            background: rgba(255, 255, 255, 0.1);");
  client.println("            color: #fff;");
  client.println("            border: 1px solid #ffd700;");
  client.println("            padding: 8px;");
  client.println("            border-radius: 5px;");
  client.println("        }");
  client.println("");
  client.println("        button {");
  client.println("            background-color: #4CAF50;");
  client.println("            color: #fff;");
  client.println("            padding: 10px 15px;");
  client.println("            border: none;");
  client.println("            border-radius: 5px;");
  client.println("            cursor: pointer;");
  client.println("            font-size: 1em;");
  client.println("            margin-top: 10px;");
  client.println("            transition: background-color 0.3s ease;");
  client.println("        }");
  client.println("");
  client.println("        button:hover {");
  client.println("            background-color: #45a049;");
  client.println("        }");
  client.println("");
  client.println("        /* Media Queries */");
  client.println("        @media (min-width: 768px) {");
  client.println("            .main-title {");
  client.println("                font-size: 3em;");
  client.println("            }");
  client.println("");
  client.println("            .about_section, #adjustDistance {");
  client.println("                max-width: 60vw;");
  client.println("            }");
  client.println("        }");
  client.println("    </style> -->");
  client.println("");
  client.println("    <style>");
  client.println("        body {");
  client.println("            margin: 0;");
  client.println("            padding: 0;");
  client.println("            font-family: Arial, sans-serif;");
  client.println("            color: #e0e0e0;");
  client.println("            background: radial-gradient(circle at top right, #0f2027, #203a43, #2c5364);");
  client.println("            display: flex;");
  client.println("            flex-direction: column;");
  client.println("            align-items: center;");
  client.println("            min-height: 100vh;");
  client.println("            box-sizing: border-box;");
  client.println("        }");
  client.println("    ");
  client.println("        .hero_area header {");
  client.println("            text-align: center;");
  client.println("            padding: 40px 20px;");
  client.println("            background-color: rgba(0, 0, 0, 0.5);");
  client.println("            color: #ffc107;");
  client.println("            box-shadow: 0 4px 10px rgba(0, 0, 0, 0.2);");
  client.println("            border-radius: 8px;");
  client.println("            max-width: 1000px;");
  client.println("            margin: 20px auto;");
  client.println("            width: 90%;");
  client.println("        }");
  client.println("    ");
  client.println("        .main-title {");
  client.println("            font-size: 2.8em;");
  client.println("            font-weight: 700;");
  client.println("            text-shadow: 1px 2px 6px rgba(0, 0, 0, 0.6);");
  client.println("            margin-bottom: 10px;");
  client.println("        }");
  client.println("    ");
  client.println("        .sub-title, .author-names {");
  client.println("            font-size: 1em;");
  client.println("            color: #ccc;");
  client.println("            letter-spacing: 1.2px;");
  client.println("        }");
  client.println("    ");
  client.println("        .about_section {");
  client.println("            width: 90%;");
  client.println("            max-width: 900px;");
  client.println("            background: rgba(17, 24, 39, 0.8);");
  client.println("            border: 1px solid rgba(255, 193, 7, 0.3);");
  client.println("            border-radius: 10px;");
  client.println("            padding: 25px;");
  client.println("            margin: 20px;");
  client.println("            box-shadow: 0 4px 12px rgba(0, 0, 0, 0.2);");
  client.println("            text-align: center;");
  client.println("        }");
  client.println("    ");
  client.println("        h2 {");
  client.println("            font-size: 2em;");
  client.println("            color: #ffc107;");
  client.println("            margin-bottom: 15px;");
  client.println("        }");
  client.println("    ");
  client.println("        p {");
  client.println("            line-height: 1.7;");
  client.println("            font-size: 1.1em;");
  client.println("            margin: 0;");
  client.println("            color: #ddd;");
  client.println("        }");
  client.println("    ");
  client.println("        #adjustDistance {");
  client.println("            width: 90%;");
  client.println("            max-width: 900px;");
  client.println("            padding: 20px;");
  client.println("            margin: 20px;");
  client.println("            background: rgba(0, 0, 0, 0.7);");
  client.println("            border-radius: 10px;");
  client.println("            box-shadow: 0 4px 12px rgba(0, 0, 0, 0.3);");
  client.println("            text-align: center;");
  client.println("        }");
  client.println("    ");
  client.println("        #distanceScale {");
  client.println("            display: flex;");
  client.println("            justify-content: space-between;");
  client.println("            font-size: 0.9em;");
  client.println("            color: #ccc;");
  client.println("            margin: 15px 0;");
  client.println("        }");
  client.println("    ");
  client.println("        #distanceSlider {");
  client.println("            width: 100%;");
  client.println("            margin: 10px 0;");
  client.println("            accent-color: #ffc107;");
  client.println("        }");
  client.println("    ");
  client.println("        #additionalFields {");
  client.println("            display: flex;");
  client.println("            flex-wrap: wrap;");
  client.println("            gap: 15px;");
  client.println("            margin: 15px 0;");
  client.println("        }");
  client.println("    ");
  client.println("        #additionalFields label, #additionalFields input {");
  client.println("            flex: 1 1 45%;");
  client.println("            font-size: 1em;");
  client.println("            color: #ffc107;");
  client.println("        }");
  client.println("    ");
  client.println("        #additionalFields input {");
  client.println("            background: rgba(255, 255, 255, 0.1);");
  client.println("            color: #fff;");
  client.println("            border: 1px solid #ffc107;");
  client.println("            padding: 8px;");
  client.println("            border-radius: 5px;");
  client.println("        }");
  client.println("    ");
  client.println("        button {");
  client.println("            background-color: #28a745;");
  client.println("            color: #fff;");
  client.println("            padding: 10px 15px;");
  client.println("            border: none;");
  client.println("            border-radius: 5px;");
  client.println("            cursor: pointer;");
  client.println("            font-size: 1em;");
  client.println("            margin-top: 10px;");
  client.println("            transition: background-color 0.3s ease;");
  client.println("        }");
  client.println("    ");
  client.println("        button:hover {");
  client.println("            background-color: #218838;");
  client.println("        }");
  client.println("    ");
  client.println("        /* Media Queries */");
  client.println("        @media (min-width: 768px) {");
  client.println("            .main-title {");
  client.println("                font-size: 3em;");
  client.println("            }");
  client.println("    ");
  client.println("            .about_section, #adjustDistance {");
  client.println("                max-width: 60vw;");
  client.println("            }");
  client.println("        }");
  client.println("    </style>");
  client.println("</head>");
  client.println("        <body>");
  client.println("            <div class=\"row\">");
  client.println("                <div class=\"hero_area\">");
  client.println("                    <header>");
  client.println("                        <div class=\"heading\">");
  client.println("                            <div class=\"main-title\">Discovering Alien Worlds</div>");
  client.println("                            <div class=\"sub-title\">Understanding the Transit Method of Exoplanet Detection</div>");
  client.println("                            <div class=\"author-names\">by Soham Mali, Antony Arupara, Siddhey Nagpure, Dr Anand Narayan, Dr Priyadrashan, Lokaveer,Yasir, Yoga</div>");
  client.println("                        </div>");
  client.println("                    </header>");
  client.println("                </div>");
  client.println("            </div>");
  client.println("            <section class=\"about_section layout_padding\">");
  client.println("                <div class=\"container\">");
  client.println("                    <div class=\"row\">");
  client.println("                        <div class=\"col-md-6\">");
  client.println("                            <div class=\"detail-box\">");
  client.println("                                <div class=\"heading_container\">");
  client.println("                                    <h2 style=\"color: #ffd700;\">Calibration</h2>");
  client.println("                                </div>");
  client.println("                                <p>");
  client.println("                                    Careful calibration of the model has to be carried out to sync the value displayed on the screen with the real-life scenario. For this using the below buttons of forward and backward, the user has to adjust the position of the planet to its zero position and click \"Complete Calibration\".");
  client.println("                                </p>");
  client.println("                            </div>");
  client.println("                        </div>");
  client.println("                        <div class=\"col-md-6\">");
  client.println("                            <div class=\"img-box\">");
  client.println("                                <img src=\"images/transit_method.jpg\" alt=\"\">");
  client.println("                            </div>");
  client.println("                        </div>");
  client.println("                    </div>");
  client.println("                </div>");
  client.println("            </section>");
  client.println("            <div class=\"row\">");
  client.println("                <section id=\"adjustDistance\" class=\"adjustDistance\">");
  client.println("                    <h2 style=\"color: #ffd700;\">Adjust zero position</h2>");
  client.println("                    <form id=\"distanceValue\" method=\"get\" action=\"your_action_url\">");
  client.println("                        <input type=\"hidden\" name=\"direction\" id=\"directionInput\" value=\"\">");
  client.println("                        <div class=\"button-container\">");
  client.println("                            <button type=\"button\" onclick=\"coarseforwardPage()\"><b>Coarse Forward</b></button>");
  client.println("                            <button type=\"button\" onclick=\"coarsebackwardPage()\"><b>Coarse Backward</b></button>");
  client.println("                        </div>");
  client.println("                        <div class=\"button-container\">");
  client.println("                            <button type=\"button\" onclick=\"fineforwardPage()\"><b>Fine Forward</b></button>");
  client.println("                            <button type=\"button\" onclick=\"finebackwardPage()\"><b>Fine Backward</b></button>");
  client.println("                        </div>");
  client.println("                        <div class=\"button-container\">");
  client.println("                            <button type=\"button\" onclick=\"gotoMainPage()\"><b>End Calibration</b></button>");
  client.println("                        </div>");
  client.println("                    </form>");
  client.println("                    <br><br>");
  client.println("                </section>");
  client.println("            </div>");
  client.println("            <script>");
  client.println("                function coarseforwardPage() {");
  client.println("                    window.location.href =\"slider=\"+ \"-100\";");
  client.println("                }");
  client.println("                function coarsebackwardPage() {");
  client.println("                    window.location.href =\"slider=\"+ \"-200\";");
  client.println("                }");
  client.println("                function fineforwardPage() {");
  client.println("                    window.location.href =\"slider=\"+ \"-300\";");
  client.println("                }");
  client.println("                function finebackwardPage() {");
  client.println("                    window.location.href =\"slider=\"+ \"-400\";");
  client.println("                }");
  client.println("                function gotoMainPage() {");
  client.println("                    var url = window.location.href;");
  client.println("                    var baseUrl = url.split('?')[0];"); // Get the base URL without query parameters
  client.println("                    window.location.href = '\slider=-500';"); // Append slider=-500 as a query parameter
  client.println("                }");
  client.println("            </script>");
  client.println("        </body>");
  client.println("        </html>");

}