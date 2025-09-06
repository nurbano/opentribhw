// LIBRERIAS
#include <Wire.h>
#include <Adafruit_MLX90614.h>
#include <Q2HX711.h>
#include "pinout.h"

#define DEBUG_ false
// VARIABLES DEL PROGRAMA
const byte hx711_data_pin = SCK2;
const byte hx711_clock_pin = DT2;
int CNY = INT0; //Sensor de vueltas eje principal - CNY70 - Configurar como interrupcion del programa
int D3 = PWM1;  // PWM - Control de frecuencia del variador
int D4 = OUT1;  // Control de encendido del motor
float Freq = 0; // Frecuencia que envio al variador
//char dataRowToSend[100];
unsigned long tinicio = 0;                            // Tiempo en el que inicio el programa (en milisegundos)
bool btinicio = false;         // bandera tiempo inicio
int VTEJE = 0;                              // Vueltas totales leidas en el eje.
unsigned long tiempo_de_muestreo;                    // necesario para timing del puerto serie
//String dataRowToSend;
String dataReadFromSerial = "";             // for incoming serial data
int funcion = 0;                            // Estado de funcionamiento  0-> Espera  1-> Testeando
bool headerSent = false;                    // Flag si se ha enviado el header en el CSV
int setRPM = 0;                             // RPM Seteadas por demanada del serial
unsigned long setSECONDS = 0;                         // SEGUNDOS Seteados por demanda del serial
unsigned long setMILISECONDS = 0;                         // SEGUNDOS Seteados por demanda del serial
unsigned long currentMS;
unsigned long sampled_time= 12500;
unsigned long startTime=0;
unsigned long timeThreshold=50;

bool SENSOR_TEMP= true;
Q2HX711 hx711(hx711_data_pin, hx711_clock_pin);

Adafruit_MLX90614 mlx = Adafruit_MLX90614(); //Para leer datos de temperatura con mlx.

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  tiempo_de_muestreo= millis();      //Define timing inicial
  startTime= millis();
  mlx.begin(); //Inicia el sensor
  if (isnan(mlx.readAmbientTempC())){ 
    Serial.println("Sensor de temperatura no conectado");
    SENSOR_TEMP= false;
  
  }
                  
  pinMode(D4, OUTPUT); // Seteo como salida
  pinMode(D3, OUTPUT); // Seteo como salida
  
  pinMode(CNY, INPUT); //Sensor de vueltas declarado como entrada
  attachInterrupt(digitalPinToInterrupt(CNY), CNY70, RISING); // Interrupcion sensor de vueltas
  Serial.println("Iniciando....");
}


void loop() {
  // put your main code here, to run repeatedly:

        // Start: ESCUCHANDO SERIAL //
        while (funcion == 0) {        // Si esta en funcion = 0 -> Enscuchando serial
            leer_puerto_serie(); 
        }
        // End: ESCUCHANDO SERIAL //

        // Start: ENSAYANDO //
        while (funcion == 1) {        // Si esta en funcion = 1 -> Ensayando

              // Setear Inicio
              if (btinicio == false){ tinicio = millis(); btinicio = true;}   // Define el tiempo de inicio, que se corre solo una vez
              
              // CSV HEADER
              if (headerSent == false) {
                Serial.print("tiempoMs,tempAmbC,tempObjC,vueltas,celdaCarga");     // Setear headers CSV
                Serial. print('\n');              // New Line
                headerSent = true;
              }

              // CSV INFO
              if(micros()>tiempo_de_muestreo+sampled_time)   //AUMENTAR TIEMPO DE MUESTRO +30ms (Envia datos cada 12.5ms = 1000/80 SPS-> Menor delay posible por limitaciones del HX711)
              {     
                  tiempo_de_muestreo= micros();                            // Tiempo transcurrido en milisegundos  

                  // TIEMPO
                  //currentMS=double(micros()/1000-tinicio);                         // Imprimir el tiempo
                  long carga= 0;
                  currentMS=millis()-tinicio; 
                  char dataRowToSend[100];
                  float t_amb = SENSOR_TEMP ? mlx.readAmbientTempC(): 20.16;
                  float t_obj = SENSOR_TEMP ? mlx.readObjectTempC(): 22.3 + float(currentMS)/(2000+currentMS) + float(random(0,10))/100.0 ;
                  char t_amb_str[10], t_obj_str[10];
                  dtostrf(t_amb, 5, 2, t_amb_str);
                  dtostrf(t_obj, 5, 2, t_obj_str);
                  
                  while (hx711.readyToSend()!=1) {
                      
                      //long carga = DEBUG_ ? random(720-(currentMS/1000), 740-(currentMS/1000)) : hx711.read() ;
                  } 
                  carga=hx711.read(); 
                 
                  snprintf(dataRowToSend, sizeof(dataRowToSend), "%lu,%s,%s,%d,%ld\n",
                          currentMS,  t_amb_str,t_obj_str, VTEJE, carga);
                  Serial.print(dataRowToSend);
                  // String dataRowToSend = String(currentMS);
                  // //dataRowToSend =float(tiempo_de_muestreo/1000);
                  // dataRowToSend=dataRowToSend+",";                          // Agregar coma separadora
                  
                  // // TEMPERATURA AMBIENTE       
                  // if (DEBUG_){
                  //   dataRowToSend=dataRowToSend+"20.16";                       // Linea de codigo para testear
                  // }
                  // else{
                  //   dataRowToSend=dataRowToSend+String(mlx.readAmbientTempC(),2);  // Codigo para temperatura ambiente del MLX90614
                  // }
                    
                  // dataRowToSend=dataRowToSend+",";                          // Agregar coma separadora.
                  // // TEMPERATURA OBJETO
                  // if (DEBUG_){
                  //   float temp = 22.3 + float(currentMS)/(2000+currentMS) + float(random(0,10))/100;                // Linea de codigo para testear         Parto de 22.3 grados, le agrego un aumento de temperatura y un ruido
                  //   dataRowToSend=dataRowToSend + temp;                       // Linea de codigo para testear
                  // }
                  // else{
                  //   dataRowToSend=dataRowToSend+String(mlx.readObjectTempC(),2); 
                  // }
                  
                  // dataRowToSend=dataRowToSend+",";                          // Agregar coma separadora.
                 
                  // // VUELTAS DEL EJE       
                  // VTEJE = int(setRPM * float(currentMS) / 60000);            // Linea de codigo para testear   EX: vtEje++;    
                  // dataRowToSend=dataRowToSend+String(VTEJE);                        // Codigo para vueltas totales en el EJE  
                  // dataRowToSend=dataRowToSend+",";                          // Agregar coma separadora
                  // // CELDA DE CARGA
                  // if (DEBUG_){
                  //   dataRowToSend=dataRowToSend+random(720- (currentMS/1000), 740 - (currentMS/1000));                       // Linea de codigo para testear
                  // }
                  // else{
                  //   dataRowToSend=dataRowToSend+String(hx711.read());            // Codigo para celda de carga del HX711
                  //   //dataRowToSend=dataRowToSend+random(720- (currentMS/1000), 740 - (currentMS/1000));
                  // }       
                                    
                  // // dataRowToSend=dataRowToSend+",";                       // Agregar coma separadora (NO en el ultimo, formato CSV)
          
                  // // ENVIAR DATOS POR EL SERIAL
                  // Serial.println(dataRowToSend);
                  // //Serial.print('\n');              // New Line
        


              // CHECKEAR SI PASO EL TIEMPO                  
             if(currentMS > setMILISECONDS)     // Si paso el tiempo (segundos x 1000)
              {  
                 Serial.print("TESTEND");             // Enviar señal de finalizado
                 Serial.print('\n');                 // New Line
                 funcion = 0;                         // Volver a funcion de escucha de Serial
                   
              }            

             // LEER EL PUERTO SERIE   ( ES NECESARIO SOLO SI SE QUIERE REALIZAR CAMBIOS DURANTE LA EJECUCION DEL ENSAYO)
             leer_puerto_serie();   
             

       
             }

              
        }
        // End: ENSAYANDO //



      
}
// FIN DEL VOID LOOP




// FUNCION DE LECTURA PUERTO SERIE
void leer_puerto_serie(){ 

    if (Serial.available() > 0) { dataReadFromSerial = Serial.readString(); Serial.flush();
    
    Serial.println(dataReadFromSerial);
    }    // Leer puerto serie
                                                                                                //Serial.print(dataReadFromSerial);                                                     
    // Estados según la lectura del puerto serie //
//    if (Leido.toInt() == 10000000) {  Estado = 1;}       // ENSAYANDO
//    if (Leido.toInt() == 20000000) {  Estado = 2;}       // FINALIZADO
//    if (Leido.toInt() == 30000000) {  Estado = 3;}       // CALIBRACIÓN
//    if (Leido.toInt() < 256) {  velmot = Leido.toInt();}

    
    

// --- COMENZAR ENSAYO --- //
    
    if (dataReadFromSerial.substring(0,9) == "TESTSTART")
      {
        btinicio = false;                                         // Setear bandera de tiempo de inicio en cero
        funcion = 1;                                              // Cambiar funcion para empezar el ensayo
        headerSent = false;                                       // Reset Header Flag
        setRPM = dataReadFromSerial.substring(10,16).toInt();     // RPM formato: "TESTSTART-##RPM#-##SEC#"  // EJ: 850 RPM, 15 SEG "TESTSTART-000800-000015"
        setSECONDS = dataReadFromSerial.substring(18,24).toInt(); // SEGUNDOS Seteados por demanda del serial
        setMILISECONDS = setSECONDS*1000;                         // Seconds to Miliseconds
        Freq = (setRPM * 17.2) / 100;                             // DEV* Calculo de RPM para variador
        digitalWrite(D4, HIGH);                                    //DEV* Motor Encendido
        analogWrite(D3, 240);                                       //DEV* Frecuencia a valor RPM
                                                                  //Serial.print(setRPM);       // Debug
                                                                  //Serial.print(setSECONDS);   // Debug
        dataReadFromSerial = "";
      }
    


// --- DETENER ENSAYO --- //
    
    if (dataReadFromSerial.substring(0,8) == "TESTSTOP")
      { 
         Serial.print("TESTSTOPPED");                           // Enviar señal de detenido por usuario
         Serial. print('\n');                                   // New Line
         digitalWrite(D4, LOW);                                // DEV* Motor apagado
         analogWrite(D3, 255);                                    // DEV* Seteo frec a valor cero
         funcion = 0;                                           // Volver a funcion de escucha de Serial
      }  



// --- COMPROBAR CONEXION A MAQUINA POD --- //
    
    if (dataReadFromSerial.substring(0,15) == "CHECKCONNECTION")
      { 
         Serial.print("PODCONNECTED");                          // Enviar señal de maquina POD conectada
         Serial. print('\n');                                   // New Line
         funcion = 0;                                           // Volver a funcion de escucha de Serial
      }  




// --- CALIBRACION --- //
    
    if (dataReadFromSerial.substring(0,19) == "CALIBRACIONMEDICION")
      { 
         Serial.print("CALIBRACIONSTART");                     // Enviar señal de maquina POD conectada
         Serial. print('\n');                                  // New Line

         for (int i = 0; i < 60; i++) {                        // Loop de 60 mediciones ( 3 segundos @ 50 ms delay)
           
         //Serial.print(hx711.read());                         // Imprimir valor de celda de carga
           Serial. print(random(350,365));                     // *DEV: Imprimir valor de celda de carga
           Serial. print('\n');                                // New Line
           delay(50);                                          // Delay de 50 ms
         }
         
         Serial. print("CALIBRACIONEND");                       // Fin de la calibracion
         Serial. print('\n');                                   // New Line Necesaria para evitar error
         
         funcion = 0;                                           // Volver a funcion de escucha de Serial
      }  







      dataReadFromSerial = "";
    // FIN Estados según la lectura del puerto serie //
}

void CNY70() {

  if (millis() - startTime > timeThreshold)
  {
    VTEJE++;
    startTime = millis();
  }
  }
  
