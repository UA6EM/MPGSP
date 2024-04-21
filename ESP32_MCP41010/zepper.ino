/*
  G0 - TFT CS
  G1 - TX
  G2 - <-> TFT_CS
  G3 - RX
  g4 - <-> TFT_DC
  G5 - MCP41x1_CS   5 <-> 2_MCP_CS       // Define chipselect pin for MCP41010
  G6 -
  G7 -
  G8 -
  G9 -
  G10 -
  G11 -
  G12 - AD9833_MISO 12  X  NOT CONNECTED
  G13 - AD9833_MOSI 13 <-> AD9833_SDATA  <-> TFT_MOSI
  G14 - AD9833_SCK  14 <-> AD9833_SCLK   <-> TFT_SCK
  G15 - AD9833_CS   15 <-> AD9833_FSYNC
  G16 - - - - - - - - - - - - - - - - -  <-> TFT_RST
  
  G17 - MCP41x1_ALC  17 <-> MCP41010_ALC           // Define chipselect pin for MCP41010 for ALC
  G18 - MCP41x1_SCK  18 <-> MCP41010_SCLK          // Define SCK pin for MCP41010
  G19 - MCP41x1_MISO 19  X  NOT CONNECTED          // Define MISO pin for MCP4131 or MCP41010
  G20 -
  G21 - SDA // LCD, INA219
  G22 - SCK // LCD, INA219
  G23 - MCP41x1_MOSI 23 <-> MCP41010_SDATA          // Define MOSI pin for MCP4131 or MCP41010
  G24 -
  G25 -
  G26 - PIN_RELE    26 <-> RELAY
  G27 -
  G28 -
  G29 -
  G30 -
  G31 -
  G32 - ON_OFF_CASCADE_PIN        32 <-> LT1206_SHUTDOWN
  G33 - PIN_ZUM                   33 <-> BUZZER
  G34 - ROTARY_ENCODER_A_PIN      34 <-> ENC_CLK
  G35 - ROTARY_ENCODER_B_PIN      35 <-> ENC_DT
  G36 - ROTARY_ENCODER_BUTTON_PIN 36 <-> ENC_SW

  G39 - CORRECT_PIN A3 (ADC3) (VN)39 <-> SENS_IMPLOSION

  //  Пины модуля 1 - 38
  3 - ENC_SW
  4 - SENS_IMPLOSION
  5 - ENC_DT
  6 - ENC_CLK
  7 - LT1206_SHUTDIWN
  8 - BUZZER
  12 - AD9833_SCLK
  15 - AD9833_SDATA
  19 - +5V
  20 - GND
  21 - MCP41010_SDATA
  22 - SCL
  25 - SDA
  28 - MCP41010_SCLK
  29 - 2_MCP_CS
  30 - MCP41010_ALC
  34 - RELAY
  35 - AD9833_FSYNC
*/

/*
#define ROTARY_ENCODER_STEPS 4
AiEsp32RotaryEncoder rotaryEncoder = AiEsp32RotaryEncoder(ROTARY_ENCODER_A_PIN, ROTARY_ENCODER_B_PIN, ROTARY_ENCODER_BUTTON_PIN, -1, ROTARY_ENCODER_STEPS);

void IRAM_ATTR readEncoderISR()
{
    rotaryEncoder.readEncoder_ISR();
}
*

void rotary_onButtonClick() {  // простой пример нажатия кнопки энкодера, не используется
  Serial.print("maxTimers = ");
  Serial.println(maxTimers);
  static unsigned long lastTimePressed = 0;
  //ignore multiple press in that time milliseconds
  if (millis() - lastTimePressed < 500) {
    return;
  }
  lastTimePressed = millis();
  Serial.print("button pressed ");
  Serial.print(millis());
  Serial.println(" milliseconds after restart");
}

void rotary_loop() {
  //dont print anything unless value changed
 if (rotaryEncoder.encoderChanged()) {
    Serial.print("Value: ");
    Serial.println(rotaryEncoder.readEncoder());
  }
  if (rotaryEncoder.isEncoderButtonClicked()) {
    rotary_onButtonClick();
  }
}
*/

#define ROTARY_ENCODER_VCC_PIN -1 /* 27 put -1 of Rotary encoder Vcc is connected directly to 3,3V; else you can use declared output pin for powering rotary encoder */
//depending on your encoder - try 1,2 or 4 to get expected behaviour
#define ROTARY_ENCODER_STEPS 1
//#define ROTARY_ENCODER_STEPS 2
//#define ROTARY_ENCODER_STEPS 4
