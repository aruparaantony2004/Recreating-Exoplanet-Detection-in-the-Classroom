#include<WiFi.h>
#include<esp_now.h>

const int EN1 = 5; // Enable pin
const int IN1 = 18; // Input pin 1
const int IN2 = 19; // Input pin 2
float tON=278; //find this, it is the time required in milliseconds for moving by 1cm at 130 PWM
float global_radius =0; //used for calibration
float scaled_radius; //used for rotating
float old_radius;
float deltaR=0;
float ref_position=35;

typedef struct struct_message {
  float new_radius;
} struct_message;

struct_message myData;

void whenDataRecieved (const uint8_t * mac, const uint8_t * incomingData, int len){
  memcpy(&myData, incomingData, sizeof(myData));
  Serial.println("Data Recieved");
  Serial.println("Number value is: ");
  Serial.print(myData.new_radius);
  global_radius = myData.new_radius;
  Serial.println();

}



void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);

  if(esp_now_init() != ESP_OK ){
    Serial.println("Error initialzing ESP-NOW");
    return;
  }

  esp_now_register_recv_cb(whenDataRecieved);
 
  pinMode(EN1, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);

  digitalWrite(EN1, LOW);
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  old_radius = 0;
}

void loop() {
  if(global_radius <0){
  Serial.print("Calibration: ");
    if (global_radius ==-100){
      Serial.println(" Coarse forward");
      digitalWrite(IN1, LOW);
      digitalWrite(IN2, HIGH);
      analogWrite(EN1, 130);
      delay(tON);
      digitalWrite(IN1, LOW);
      digitalWrite(IN2, LOW);
      Serial.println("Done Coarse Forward Calilbration.");
    }
    else if (global_radius ==-200){
      Serial.println(" Coarse backward");
      digitalWrite(IN1, HIGH);
      digitalWrite(IN2, LOW);
      analogWrite(EN1, 130);
      delay(tON);
      digitalWrite(IN1, LOW);
      digitalWrite(IN2, LOW);
      Serial.println("Done Coarse backward Calilbration.");
      }
    else if (global_radius == -300){
      Serial.println(" Fine forward");
      digitalWrite(IN1, LOW);
      digitalWrite(IN2, HIGH);
      analogWrite(EN1, 130);
      delay(tON/2);
      digitalWrite(IN1, LOW);
      digitalWrite(IN2, LOW);
      Serial.println("Done Fine forward Calilbration.");  
      }
    else if(global_radius == -400){
      Serial.println(" Fine backward");
      digitalWrite(IN1, HIGH);
      digitalWrite(IN2, LOW);
      analogWrite(EN1, 130);
      delay(tON/2);
      digitalWrite(IN1, LOW);
      digitalWrite(IN2, LOW);
      Serial.println("Done Fine backward Calilbration.");
    }
      digitalWrite(IN1, LOW);
      digitalWrite(IN2, LOW);
    global_radius=0;
    deltaR=0;
  }

  else{
    scaled_radius = map(global_radius, 0, 100, 0, ref_position);
    float time_required;
    Serial.print("Current radius value is: ");
    Serial.println(scaled_radius);
    deltaR = scaled_radius - old_radius;
    Serial.print("Delta R: ");
    Serial.print(deltaR);
    old_radius = scaled_radius;

    if (deltaR ==0){
      digitalWrite(IN1, LOW);
      digitalWrite(IN2, LOW);
      Serial.println("   NO change");
      delay(2000);
    }
    else if(deltaR<0){
      Serial.println("   difference is negative");
      Serial.println("   Moving clockwise");
      digitalWrite(IN1, HIGH);
      digitalWrite(IN2, LOW);
      analogWrite(EN1, 130);
      time_required= abs(deltaR)*tON;
      delay(time_required);

    }
    else if(deltaR>0){
      Serial.println("   differnece is positive");
      digitalWrite(IN1, LOW);
      digitalWrite(IN2, HIGH);
      analogWrite(EN1, 130);
      Serial.println("   Moving anti-clockwise");
      time_required= abs(deltaR)*tON;
      delay(time_required);
    }
  }

}