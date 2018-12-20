#include <Sodaq_RN2483.h>

#define CONSOLE_STREAM SERIAL_PORT_MONITOR
#define LORA_STREAM Serial2
#define COMMON_ANODE
#define CO2_SENSOR A1

// Variables will contain your personal OTAA Activation Keys
uint8_t devEUI[8] ;   // Device EUI
uint8_t appEUI[8] = { 0x70, 0xB3, 0xD5, 0x7E, 0xD0, 0x01, 0x33, 0x81 };

//Romain' app Key
//const uint8_t appKey[16] = {0xFF, 0xEE, 0xDD, 0xCC, 0xBB, 0xAA, 0x99, 0x88, 0x00, 0x04, 0xA3, 0x0B, 0x00, 0x20, 0x26, 0x22} ;

// Loïc's app Key
const uint8_t appKey[16] = {0x32, 0x37, 0x71, 0x7B, 0x37, 0xA8, 0x34, 0xA5, 0xF4, 0x1D, 0xF0, 0x33, 0xCD, 0x93, 0xD8, 0x91} ;


void setup()
{
  while (!CONSOLE_STREAM && (millis() < 10000)) ;
  CONSOLE_STREAM.begin(115200) ;

  // Temperature sensor
  pinMode(TEMP_SENSOR, INPUT) ;
  analogReadResolution(12) ;

  // Co2 sensor
  pinMode(CO2_SENSOR, INPUT);

  // LED
  pinMode(LED_BUILTIN, OUTPUT) ;
  pinMode(LED_RED, OUTPUT) ;
  pinMode(LED_GREEN, OUTPUT) ;
  pinMode(LED_BLUE, OUTPUT) ;

  // LoRa
  LORA_STREAM.begin(LoRaBee.getDefaultBaudRate()) ;
  // Hardreset the RN module
  pinMode(LORA_RESET, OUTPUT) ;
  digitalWrite(LORA_RESET, HIGH) ;
  delay(100) ;
  digitalWrite(LORA_RESET, LOW) ;
  delay(100) ;
  digitalWrite(LORA_RESET, HIGH) ;
  delay(1000) ;

  // empty the buffer
  LORA_STREAM.end() ;
  LORA_STREAM.begin(LoRaBee.getDefaultBaudRate()) ;
  LoRaBee.init(LORA_STREAM, -1) ;

  //get the devid
  uint8_t len = LoRaBee.getHWEUI(devEUI, sizeof(devEUI)) ;
  if (len == 0) {
    CONSOLE_STREAM.println("Error to get HwEUI") ;
    while (1) ;
  }

  //Display informations
  CONSOLE_STREAM.println() ;
  CONSOLE_STREAM.print("devEUI = ") ; displayArrayInOneLine(devEUI, sizeof(devEUI)) ;
  CONSOLE_STREAM.print("appEUI = ") ; displayArrayInOneLine(appEUI, sizeof(appEUI)) ;
  CONSOLE_STREAM.print("appKey = ") ; displayArrayInOneLine(appKey, sizeof(appKey)) ;

  joinLoraNetwork();
}

void loop()
{
  // Get the temperature
  float temp = getTemperature();
  CONSOLE_STREAM.print("Temperature = ") ;
  CONSOLE_STREAM.print(temp) ;
  CONSOLE_STREAM.println(" C") ;

  char payload[10];
  sprintf(payload, "%.2f", temp);
  // Send on canal 1 with 3 retry
  //int res = LoRaBee.sendReqAck(1, (const uint8_t*)payload, strlen(payload), 5);
  int res = LoRaBee.send(1, (const uint8_t*)payload, strlen(payload));
  checkResult(res);

  delay(20000);

  // Get the Co2 Level
  float co2 = getCo2();

  CONSOLE_STREAM.print("Co2 Level = ") ;
  CONSOLE_STREAM.print(co2) ;
  CONSOLE_STREAM.println(" ppm") ;

  sprintf(payload, "%.2f", co2);
  res = LoRaBee.sendReqAck(2, (const uint8_t*)payload, strlen(payload), 5);
  res = LoRaBee.send(2, (const uint8_t*)payload, strlen(payload));
  checkResult(res);

  delay(20000);
}

void checkResult(int res) {
  switch (res)
  {
    case NoError:
      CONSOLE_STREAM.println("Successful transmission.") ;
      setRgbColor(0x00, 0xFF, 0x00) ;
      delay(2000) ;
      setRgbColor(0x00, 0x00, 0x00) ;
      break;
    case NoResponse:
      CONSOLE_STREAM.println("There was no response from the device.") ;
      setRgbColor(0xFF, 0x00, 0x00) ;
      break ;
    case Timeout:
      CONSOLE_STREAM.println("Connection timed-out. Check your serial connection to the device! Sleeping for 20sec.") ;
      setRgbColor(0xFF, 0x00, 0x00) ;
      delay(20000) ;
      break ;
    case PayloadSizeError:
      CONSOLE_STREAM.println("The size of the payload is greater than allowed. Transmission failed!") ;
      setRgbColor(0xFF, 0x00, 0x00) ;
      break ;
    case InternalError:
      CONSOLE_STREAM.println("Oh No! This shouldn't happen. Something is really wrong! Try restarting the device!\r\nThe program will now halt.") ;
      joinLoraNetwork();
      break ;
    case Busy:
      CONSOLE_STREAM.println("The device is busy. Sleeping for 10 extra seconds.");
      delay(10000) ;
      break ;
    case NetworkFatalError:
      CONSOLE_STREAM.println("There is a non-recoverable error with the network connection. You should re-connect.\r\nThe program will now halt.");
      setRgbColor(0xFF, 0x00, 0x00) ;
      while (1) {} ;
      break ;
    case NotConnected:
      CONSOLE_STREAM.println("The device is not connected to the network. Please connect to the network before attempting to send data.\r\nThe program will now try to reconnect.");
      setRgbColor(0xFF, 0x00, 0x00) ;
      joinLoraNetwork();
      break ;
    case NoAcknowledgment:
      CONSOLE_STREAM.println("There was no acknowledgment sent back!");
      setRgbColor(0xFF, 0x00, 0x00) ;
      break ;
    default:
      break ;
  }
}
// --------------------------------------------------------------------------
// Join the LORA network through OTA Activation
// --------------------------------------------------------------------------
float getTemperature() {
  int sensorValue = analogRead(TEMP_SENSOR) ;
  float mVolts = (float)sensorValue * 3300 / 4096.0 ;
  float temp = (mVolts - 500) ;
  temp = temp / 10.0;

  return temp;
}


float getCo2() {
  int sensorValue = analogRead(CO2_SENSOR);
  float mVolts = (float)sensorValue * 3300 / 4096.0 ;
  float co2 = map(mVolts, 0, 3300, 0, 10000);
  return co2;
}


// --------------------------------------------------------------------------
// Join the LORA network through OTA Activation
// --------------------------------------------------------------------------
void joinLoraNetwork() {
  bool joinRes = 0 ;
  do
  {
    setRgbColor(0x00, 0x00, 0xFF) ;
    CONSOLE_STREAM.println("Try to join the LoRa network through OTA Activation") ;
    joinRes = LoRaBee.initOTA(LORA_STREAM, devEUI, appEUI, appKey, true);

    CONSOLE_STREAM.println(joinRes ? "Join Accepted." : "Join Failed! Trying again after 3 seconds.") ;
    if (!joinRes)
    {
      setRgbColor(0xFF, 0x00, 0x00) ;
      delay(3000) ;
    }
  } while (joinRes == 0) ;
  setRgbColor(0x00, 0xFF, 0x00) ;
  delay(3000) ;


}

// --------------------------------------------------------------------------
// Display array in HEX format routine
// --------------------------------------------------------------------------
void displayArrayInOneLine(const uint8_t tab[], uint8_t tabSize)
{
  char c[2] ;
  for (uint8_t i = 0; i < tabSize; i++)
  {
    sprintf(c, "%02X", tab[i]) ;
    CONSOLE_STREAM.print(c) ;
  }
  CONSOLE_STREAM.println() ;
}

// --------------------------------------------------------------------------
// LED routines
// --------------------------------------------------------------------------

void setRgbColor(uint8_t red, uint8_t green, uint8_t blue)
{
#ifdef COMMON_ANODE
  red = 255 - red ;
  green = 255 - green ;
  blue = 255 - blue ;
#endif
  analogWrite(LED_RED, red) ;
  analogWrite(LED_GREEN, green) ;
  analogWrite(LED_BLUE, blue) ;
}
