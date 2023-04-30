/* ----------------------------------------------------------- *
 * ----------------- SISTEMA INTELIGENTE DE TEMPERATURA ---------------- *
 * ----------------------------------------------------------- */

/* ----------------------------------------------------------- *
 * ------------------ Variables Preprocesado ----------------- *
 * ----------------------------------------------------------- */

/* ------------------- Variables de la LCD ------------------- */

/*los pines de conexion de la pantalla LCD 16x2 son:
 * LCD RS pin to digital pin 13
 * LCD Enable pin to digital pin 12
 * LCD D4 pin to digital pin 11
 * LCD D5 pin to digital pin 10
 * LCD D6 pin to digital pin 9
 * LCD D7 pin to digital pin 8
 * LCD R/W pin to ground
 * LCD VSS pin to ground
 * LCD VCC pin to 5V
 * ends to +5V and ground
 * wiper to LCD VO pin (pin 3) */

//#include <LiquidCrystal_I2C.h> // incluir libreria de arduino para LCD I2c
#include <LiquidCrystal.h> // incluir libreria de arduino para LCD Normal

//LiquidCrystal_I2C lcd(0x27, 16, 2); // definir objeto LCD (A4, A5) I2C
LiquidCrystal lcd(13,12,11,10,9,8); // definir objeto LCD (RS,En,D4,D5,D6,D7) NORMAL

/* ------------------ Variables Ventilador ------------------- */

const int pin_fan = 7;

/* ------------------- Variables Bombillo -------------------- */

const int pin_bulb = 3;
int light = 0;

/* -------------------- Variables Buzzer --------------------- */

const int pin_buzzer = 6;
int frecuencia = 523;

/* ------------------- Variables LED RGB --------------------- */

int pinR = 5; // pin arduino para color Rojo
int pinB = 4; // pin arduino para color azul
int pinG = 2; // pin arduino para color verde

/* -------------------- Variables LM35 ------------------------ */

const int pin_LM35 = A0;
int lectura_LM35 = 0; // variable para guardar la señal analogica del sensor de temperatura
float temperatura = 0.0; // variable para convertir de voltaje a temperatura


/* ----------------- Variables de Control -------------------- */

double tiempomuestreo=0.5; // tiempo en el que se realiza cada medida de la posicion para luego recalcularse.
double tiempoanterior=0;
double error=0; // variable que define el error (diferencia) del sistema entre el setpoint y la posicion actual.
double erroranterior=0;
double p=0; 
double i=0; 
double d=0; 
double kp=0.005; //  0.005;0.0003;0.003;0.001;                    matlab:49.7208
double ki=0.003; //  0.003;0.0018;0.005;0.001;0.0009;0.0001         matlab:24.661
double kd=0.8; // 0.8
double aint_anterior=0;//acción integral anterior
double pv=0; // sensor del proceso.
double salida=0;
double entrada=0;
int mv=0; // variable manipulada.
int setpoint = 60; // valor (en porcentaje) al que yo quiero que el brushless ascienda.

/* ------------- Variables Proposito General ----------------- */

// variables delay
unsigned long previousMillis1 = 0;
unsigned long previousMillis2 = 0;
unsigned long previousMillis3 = 0;
unsigned long previousMillis4 = 0;
unsigned long previousMillis5 = 0;
unsigned long previousMillis6 = 0;
const long interval1 = 1000;
const long interval2 = 1000;
const long interval3 = 200;
const long interval4 = 300;
const long interval5 = 500;
const long interval6 = 50;


/* ------------------ Variables para menu -------------------- */
int op = 0; // opcion lazo abierto o cerrado
int receivedInt = 0;
String Opcion = "";

/* ----------------------------------------------------------- *
 * ---------------- Zona de Configuracion -------------------- *
 * ----------------------------------------------------------- */

void setup(){

  /* -------------- abrir comunicacion --------------------- */
  Serial.begin(9600); // abrir canal de comunicacion serial

  //lcd.init(); // iniciar comunicacion con LCD I2C
  //lcd.backlight(); // encender luz de pantalla LCD I2C
  lcd.begin(16, 2);   // iniciar comunicacion con lcd 16x2 NORMAL

  /* ------------------ Pines Salida ----------------------- */
  pinMode(pin_fan, OUTPUT);
  pinMode(pin_bulb, OUTPUT);
  pinMode(pin_buzzer, OUTPUT);
  pinMode(pinR, OUTPUT);
  pinMode(pinB, OUTPUT);
  pinMode(pinG, OUTPUT);

  /* ------------------ Pines Entrada ---------------------- */
  pinMode(pin_LM35, INPUT);

  /* ---------------- Mensaje Bienvenida ------------------- */
  lcd.setCursor(5, 0);
  lcd.print("Control");
  lcd.setCursor(0, 1);
  lcd.print("Temp-Inteligente");
  delay(1000);
  lcd.clear();

}

/* ----------------------------------------------------------- *
 * -------------------- Codigo Principal --------------------- *
 * ----------------------------------------------------------- */

void loop(){
  closeloop_PID();
  // muestre menu 1 para seleccionar lazo abierto o cerrado
  //menu1();
  // Espere hasta que el usuario seleccione una opcion de lazo
  //op = recvInt();

  /*if (op == '1'){
    while(1){ // bucle infinito
      openloop();
    }
  }

  if (op == '2'){
    // muestre menu 2 para seleccionar entre ONFOFF o PID
    menu2();
    // Espere hasta que el usuario seleccione una opcion de control a lazo cerrado
    op = recvInt();

    if (op == 'A'){
      while(1){
        closeloop_on_off();
      }
    }

    if (op == 'B'){
      while(1){
        closeloop_PID();
      }
    }
  }*/
}

/* ----------------------------------------------------------- *
 * ------------------------ Funciones ------------------------ *
 * ----------------------------------------------------------- */
int recvInt(){
  while(Serial.available() == 0){};
  receivedInt = Serial.read();
  lcd.clear();
  return receivedInt;
}

void menu1(){
  lcd.setCursor(3,0);
  lcd.print("Seleccione");
  lcd.setCursor(3,1);
  lcd.print("una Opcion ");
  delay(2000);
  lcd.clear();

  lcd.setCursor(1,0);
  lcd.print("1.Lazo Abierto");
  lcd.setCursor(1,1);
  lcd.print("2.Lazo Cerrado");
}

void menu2(){
  lcd.setCursor(3,0);
  lcd.print("Seleccione");
  lcd.setCursor(3,1);
  lcd.print("una Opcion ");
  delay(2000);
  lcd.clear();

  lcd.setCursor(1,0);
  lcd.print("A.Control ONOFF");
  lcd.setCursor(1,1);
  lcd.print("B.Control PID");
}

void openloop(){

  lectura_LM35 = analogRead(pin_LM35); // leer valores analogicos del sensor de temperatura
  temperatura = 0.47826*(lectura_LM35-153)+25;
  //temperatura = lectura_LM35 * 1.1 * 100 / 1024; // ecuacion de regresion que convierte voltaje a temperatura
  //temperatura = (((lectura_LM35/1023.0) * 5000)/10) - 51; // ecuacion de regresion que convierte voltaje a temperatura

  if(Serial.available() > 0){
    light = Serial.parseInt();
    lcd.setCursor(4,1);
    lcd.print("   ");
    analogWrite(pin_bulb,light);
  }

  if(temperatura >= setpoint){
    lcd.setCursor(0,0);
    lcd.print("Setpoint");
    lcd.setCursor(0,1);
    lcd.print("Alcanzado");

    if(millis() >= previousMillis1 + interval1){
      lcd.clear();
      previousMillis1 = millis(); 
    }
    
  } else{
    lcd.setCursor(2,0);
    lcd.print("Lazo Abierto");
    lcd.setCursor(0,1);
    lcd.print("Luz:");
    lcd.setCursor(4,1);
    lcd.print(light);
    lcd.setCursor(10,1);
    lcd.print("C:");
    lcd.setCursor(12,1);
    lcd.print("   ");
    lcd.setCursor(12,1);
    lcd.print(temperatura);

    if(millis() >= previousMillis2 + interval2){
      lcd.clear();
      previousMillis2 = millis(); 
    }
  }
}

void closeloop_on_off(){
  lectura_LM35 = analogRead(pin_LM35); // leer valores analogicos del sensor de temperatura
  temperatura = (((lectura_LM35/1023.0) * 5000)/10) - 51; // ecuacion de regresion que convierte voltaje a temperatura
  error = setpoint - temperatura;

  if(Serial.available() > 0){
    setpoint = Serial.parseInt();
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("New");
    lcd.setCursor(0,1);
    lcd.print("Setpoint:");
    lcd.setCursor(9,1);
    lcd.print(setpoint);
    delay(2000);
    lcd.clear();
  }

  if(error > 3){ // caso en el que la temperatura es menor al valor deseado
    analogWrite(pin_bulb, 255);
    digitalWrite(pin_fan, LOW);
    digitalWrite(pinR, 1);
    digitalWrite(pinG, 0);
    digitalWrite(pinB, 0);
  }

  if(error < -3){ // caso en el que la temperatura es mayor al valor deseado
    analogWrite(pin_bulb, 0);
    digitalWrite(pin_fan, HIGH);
    digitalWrite(pinR, 0);
    digitalWrite(pinG, 0);
    digitalWrite(pinB, 1);
  }

  if(error > -3 && error < 3){
    analogWrite(pin_bulb, 50);
    digitalWrite(pin_fan, LOW);
    
    if(millis() >= previousMillis3 + interval3){
       digitalWrite(pinR, 1);
       digitalWrite(pinG, 0);
       digitalWrite(pinB, 0);
      
      previousMillis3 = millis(); 
    }
    
    if(millis() >= previousMillis4 + interval4){
       digitalWrite(pinR, 0);
       digitalWrite(pinG, 1);
       digitalWrite(pinB, 0);
      
      previousMillis4 = millis(); 
    }
    
    if(millis() >= previousMillis5 + interval5){
       digitalWrite(pinR, 0);
       digitalWrite(pinG, 0);
       digitalWrite(pinB, 1);
      
      previousMillis5 = millis(); 
    }
   
  }

  lcd.setCursor(2,0);
  lcd.print("Control ONOFF");
  lcd.setCursor(0,1);
  lcd.print("Error:");
  lcd.setCursor(6,1);
  lcd.print("    ");
  lcd.setCursor(6,1);
  lcd.print(error);
  lcd.setCursor(10,1);
  lcd.print("C:");
  lcd.setCursor(12,1);
  lcd.print("   ");
  lcd.setCursor(12,1);
  lcd.print(temperatura);
}

void closeloop_PID(){
  lectura_LM35 = analogRead(pin_LM35); // leer valores analogicos del sensor de temperatura
  temperatura = ((lectura_LM35/1023.0) * 5000)/10; // ecuacion de regresion que convierte voltaje a temperatura
  
  entrada = temperatura;
  //entrada = map(temperatura, 18, 120, 0, 100);
  error = setpoint - entrada;
  
  p = (kp*error); // Implementacion del P
  i = ki*((tiempomuestreo*erroranterior)+(tiempomuestreo*((error-erroranterior)/2))); // Implementacion del I
  d = kd *((error-erroranterior)/tiempomuestreo); // Implementacion del D
 
  salida+= p+i+d+aint_anterior; // declaraciòn y sumatoria de las tres variables. 
  
  if (salida <= 0){
    salida = 0; 
  }  
  
  if (salida >= 100){
    salida = 100; 
  } 

  erroranterior = error; // se compara con base al setpoint
  aint_anterior = i; 
  
  mv = map(salida, 0, 100, 0, 255);
  analogWrite(pin_bulb, mv);
  
  if(Serial.available()>0){
    Opcion = Serial.readStringUntil('\n');
    if(Opcion.startsWith("KP") == 1){
      kp = (Opcion.substring(Opcion.indexOf('P',0)+1,Opcion.indexOf('\n',0))).toInt();
      Opcion = "";
    }
    if(Opcion.startsWith("KI") == 1){
      ki = (Opcion.substring(Opcion.indexOf('I',0)+1,Opcion.indexOf('\n',0))).toInt();
      Opcion = "";
    }
    if(Opcion.startsWith("KD") == 1){
      kd = (Opcion.substring(Opcion.indexOf('D',0)+1,Opcion.indexOf('\n',0))).toInt();
      Opcion = "";
    }
    if(Opcion.startsWith("SP") == 1){
      setpoint = (Opcion.substring(Opcion.indexOf('P',0)+1,Opcion.indexOf('\n',0))).toInt();
      Opcion = "";
    }
  }

  if(millis() >= previousMillis6 + interval6){
    Serial.print("SetPoint: ");
    Serial.print(setpoint);
    Serial.print("\t KP: ");
    Serial.print(kp);
    Serial.print("\t KI: ");
    Serial.print(ki);
    Serial.print("\t KD: ");
    Serial.print(kd);
    Serial.print("\t Temp actual: ");
    Serial.print(temperatura);
    Serial.print("\t Actuador: ");
    Serial.print(mv);
    Serial.print("\t Error: ");
    Serial.println(error);
    
    previousMillis6 = millis(); 
  }
  
  lcd.setCursor(2,0);
  lcd.print("Control PID");
  lcd.setCursor(0,1);
  lcd.print("Error:");
  lcd.setCursor(6,1);
  lcd.print("    ");
  lcd.setCursor(6,1);
  lcd.print(error);
  lcd.setCursor(10,1);
  lcd.print("C:");
  lcd.setCursor(12,1);
  lcd.print("   ");
  lcd.setCursor(12,1);
  lcd.print(temperatura);
}