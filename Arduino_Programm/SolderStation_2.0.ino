//*******************************//
// Soldering Station
// Matthias Wagner
// www.k-pank.de/so
// Get.A.Soldering.Station@gmail.com
//*******************************//

#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Library for the display
#include <SPI.h>            // comunication between the Arduino and the display 

#include "iron.h"           
#include "stationLOGO.h"


#define VERSION "1.5"		//Version der Steuerung
#define INTRO

#define sclk 	13		// Don't change
#define mosi 	11		// Don't change
#define cs_tft	10		// 


//V1.5
#define dc   	9		// 8
#define rst  	12		// 9 


/*
//V1.4
#define dc   	8
#define rst  	9
*/

#define STANDBYin A4
#define POTI   	A5
#define TEMPin 	A7
#define PWMpin 	3
#define BLpin		5

#define CNTRL_GAIN 10

#define DELAY_MAIN_LOOP 	10
#define DELAY_MEASURE 		50
//#define ADC_TO_TEMP_GAIN 	0.415
#define ADC_TO_TEMP_GAIN 	0.53 //Mit original Weller Station verglichen
#define ADC_TO_TEMP_OFFSET 25.0
#define STANDBY_TEMP			175

#define BLACK     0x0000 // definite the colours of the display 
#define BLUE      0x001F
#define RED       0xF800
#define GREEN     0x07E0
#define CYAN      0x07FF
#define MAGENTA   0xF81F
#define YELLOW    0xFFE0
#define WHITE     0xFFFF
#define LightGrey   0xC618
#define DarkGrey    0x7BEF

#define OVER_SHOT 			2
#define MAX_PWM_LOW			180
#define MAX_PWM_HI			210		//254
#define MAX_POTI				400		//400Grad C

#define PWM_DIV 1024						//default: 64   31250/64 = 2ms

Adafruit_ST7735 TFTscreen = Adafruit_ST7735(cs_tft, dc, mosi, sclk, rst); // has to be compatible to the Adafruit library 

int pwm = 0; //pwm Out Val 0.. 255
int soll_temp = 300;
boolean standby_act = false;


void setup(void) {
/*
   Serial.begin(9600);
   TFTscreen.begin();
   TFTscreen.fillScreen(WHITE);
      */
	
	pinMode(BLpin, OUTPUT);
	digitalWrite(BLpin, LOW);
	
	pinMode(STANDBYin, INPUT_PULLUP);
	
	pinMode(PWMpin, OUTPUT);
	digitalWrite(PWMpin, LOW);
	setPwmFrequency(PWMpin, PWM_DIV);
	digitalWrite(PWMpin, LOW);
	
	//TFTscreen.init();
	SPI.setClockDivider(SPI_CLOCK_DIV4);  // 4MHz

  TFTscreen.initR(INITR_BLACKTAB);
	
	TFTscreen.setRotation(0);	// 0 - Portrait, 1 - Lanscape
	TFTscreen.fillScreen(BLACK);
	TFTscreen.setTextWrap(true);
	
	
	
	//Print station Logo
	TFTscreen.drawBitmap(2,1,stationLOGO1,124,47,LightGrey);
	
	TFTscreen.drawBitmap(3,3,stationLOGO1,124,47,YELLOW);		
	TFTscreen.drawBitmap(3,3,stationLOGO2,124,47,Color565(254,147,52));	
	TFTscreen.drawBitmap(3,3,stationLOGO3,124,47,Color565(255,78,0));
 
	
	//BAcklight on
	digitalWrite(BLpin, HIGH);
	
	
#if defined(INTRO)
	
	delay(500);
	
	//Print Iron
	TFTscreen.drawBitmap(15,50,iron,100,106,LightGrey);
	TFTscreen.drawBitmap(17,52,iron,100,106,YELLOW);
	delay(500);
	
	TFTscreen.setTextSize(2);
	TFTscreen.setTextColor(LightGrey);
	TFTscreen.setCursor(70,130);
	TFTscreen.print(VERSION);
	
	TFTscreen.setTextSize(2);
	TFTscreen.setTextColor(YELLOW);
	TFTscreen.setCursor(72,132);
	TFTscreen.print(VERSION);
	
	TFTscreen.setTextSize(1);
	TFTscreen.setTextColor(LightGrey);
	TFTscreen.setCursor(103,0);
	TFTscreen.print("v");
	TFTscreen.print(VERSION);
	
	TFTscreen.setTextColor(YELLOW);
	TFTscreen.setCursor(104,1);
	TFTscreen.print("v");
	TFTscreen.print(VERSION);
	
	delay(2500);
#endif
	
	
	TFTscreen.fillRect(0,47,128,125,BLACK);
	TFTscreen.setTextColor(WHITE);

	TFTscreen.setTextSize(1);
	TFTscreen.setCursor(1,84);
	TFTscreen.print("ist");
	
	TFTscreen.setTextSize(2);
	TFTscreen.setCursor(117,47);
	TFTscreen.print("o");
	
	TFTscreen.setTextSize(1);
	TFTscreen.setCursor(1,129);
	TFTscreen.print("soll");
	
	TFTscreen.setTextSize(2);
	TFTscreen.setCursor(117,92);
	TFTscreen.print("o");
	
	TFTscreen.setCursor(80,144);
	TFTscreen.print("   %");
	
	TFTscreen.setTextSize(1);
	TFTscreen.setCursor(1,151);		//60
	TFTscreen.print("pwm");
	
	TFTscreen.setTextSize(2);
	
}

void loop() {
	
	int actual_temperature = getTemperature();
	soll_temp = map(analogRead(POTI), 0, 1024, 0, MAX_POTI);
	
	
	//TODO: Put in Funktion
	TFTscreen.setCursor(2,55);
	if (digitalRead(STANDBYin) == true)
		TFTscreen.setTextColor(BLACK);
	else
		TFTscreen.setTextColor(WHITE);
	TFTscreen.print("SB");
	
	//
	
	int soll_temp_tmp = soll_temp;
	
	if (digitalRead(STANDBYin) == false)
		standby_act = true;
	else
		standby_act = false;
	
	if (standby_act && (soll_temp >= STANDBY_TEMP ))
		soll_temp_tmp = STANDBY_TEMP;
	
	
	
	int diff = (soll_temp_tmp + OVER_SHOT)- actual_temperature;
	pwm = diff*CNTRL_GAIN;
	
	int MAX_PWM;

	//Set max heating Power 
	MAX_PWM = actual_temperature <= STANDBY_TEMP ? MAX_PWM_LOW : MAX_PWM_HI;
	
	//8 Bit Range
	pwm = pwm > MAX_PWM ? pwm = MAX_PWM : pwm < 0 ? pwm = 0 : pwm;
	
	
	
	//NOTfall sicherheit / Spitze nicht eingesteckt
	if (actual_temperature > 550){
		pwm = 0;
		actual_temperature = 0;
	}
	
	
	analogWrite(PWMpin, pwm);
	//digitalWrite(PWMpin, LOW);
	
	writeHEATING(soll_temp, actual_temperature, pwm);
	
	delay(DELAY_MAIN_LOOP);		//wait for some time
}


int getTemperature()
{
  analogWrite(PWMpin, 0);		//switch off heater
  delay(DELAY_MEASURE);			//wait for some time (to get low pass filter in steady state)
  int adcValue = analogRead(TEMPin); // read the input on analog pin 7:
  Serial.print("ADC Value ");
  Serial.print(adcValue);
  analogWrite(PWMpin, pwm);	//switch heater back to last value
  return round(((float) adcValue)*ADC_TO_TEMP_GAIN+ADC_TO_TEMP_OFFSET); //apply linear conversion to actual temperature
}





void writeHEATING(int tempSOLL, int tempVAL, int pwmVAL){
	static int d_tempSOLL = 2;		//Tiefpass für Anzeige (Poti zittern)

	static int tempSOLL_OLD = 	10;
	static int tempVAL_OLD	= 	10;
	static int pwmVAL_OLD	= 	10;
	//TFT Anzeige
	
	pwmVAL = map(pwmVAL, 0, 254, 0, 100);
	
	TFTscreen.setTextSize(5);
	if (tempVAL_OLD != tempVAL){
		TFTscreen.setCursor(30,57);
		TFTscreen.setTextColor(BLACK);
		//TFTscreen.print(tempSOLL_OLD);
		//erste Stelle unterschiedlich
		if ((tempVAL_OLD/100) != (tempVAL/100)){
			TFTscreen.print(tempVAL_OLD/100);
		}
		else
			TFTscreen.print(" ");
		
		if ( ((tempVAL_OLD/10)%10) != ((tempVAL/10)%10) )
			TFTscreen.print((tempVAL_OLD/10)%10 );
		else
			TFTscreen.print(" ");
		
		if ( (tempVAL_OLD%10) != (tempVAL%10) )
			TFTscreen.print(tempVAL_OLD%10 );
		
		TFTscreen.setCursor(30,57);
		TFTscreen.setTextColor(WHITE);
		
		if (tempVAL < 100)
			TFTscreen.print(" ");
		if (tempVAL <10)
			TFTscreen.print(" ");
		
		int tempDIV = round(float(tempSOLL - tempVAL)*8.5);
		tempDIV = tempDIV > 254 ? tempDIV = 254 : tempDIV < 0 ? tempDIV = 0 : tempDIV;
		TFTscreen.setTextColor(Color565(tempDIV, 255-tempDIV, 0));
		if (standby_act)
			TFTscreen.setTextColor(CYAN);
		TFTscreen.print(tempVAL);
		
		tempVAL_OLD = tempVAL;
	}
	
	//if (tempSOLL_OLD != tempSOLL){
	if ((tempSOLL_OLD+d_tempSOLL < tempSOLL) || (tempSOLL_OLD-d_tempSOLL > tempSOLL)){
		TFTscreen.setCursor(30,102);
		TFTscreen.setTextColor(BLACK);
		//TFTscreen.print(tempSOLL_OLD);
		//erste Stelle unterschiedlich
		if ((tempSOLL_OLD/100) != (tempSOLL/100)){
			TFTscreen.print(tempSOLL_OLD/100);
		}
		else
			TFTscreen.print(" ");
		
		if ( ((tempSOLL_OLD/10)%10) != ((tempSOLL/10)%10) )
			TFTscreen.print((tempSOLL_OLD/10)%10 );
		else
			TFTscreen.print(" ");
		
		if ( (tempSOLL_OLD%10) != (tempSOLL%10) )
			TFTscreen.print(tempSOLL_OLD%10 );
		
		//Neuen Wert in Weiß schreiben
		TFTscreen.setCursor(30,102);
		TFTscreen.setTextColor(WHITE);
		if (tempSOLL < 100)
			TFTscreen.print(" ");
		if (tempSOLL <10)
			TFTscreen.print(" ");
		
		TFTscreen.print(tempSOLL);
		tempSOLL_OLD = tempSOLL;
		
	}
	
	
	TFTscreen.setTextSize(2);
	if (pwmVAL_OLD != pwmVAL){
		TFTscreen.setCursor(80,144);
		TFTscreen.setTextColor(BLACK);
		//TFTscreen.print(tempSOLL_OLD);
		//erste stelle Unterscheidlich
		if ((pwmVAL_OLD/100) != (pwmVAL/100)){
			TFTscreen.print(pwmVAL_OLD/100);
		}
		else
			TFTscreen.print(" ");
		
		if ( ((pwmVAL_OLD/10)%10) != ((pwmVAL/10)%10) )
			TFTscreen.print((pwmVAL_OLD/10)%10 );
		else
			TFTscreen.print(" ");
		
		if ( (pwmVAL_OLD%10) != (pwmVAL%10) )
			TFTscreen.print(pwmVAL_OLD%10 );
		
		TFTscreen.setCursor(80,144);
		TFTscreen.setTextColor(WHITE);
		if (pwmVAL < 100)
			TFTscreen.print(" ");
		if (pwmVAL <10)
			TFTscreen.print(" ");
		
		TFTscreen.print(pwmVAL);
		pwmVAL_OLD = pwmVAL;
		
	}
	
}




uint16_t Color565(uint8_t r, uint8_t g, uint8_t b) {
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}



void setPwmFrequency(int pin, int divisor) {
  byte mode;
  if(pin == 5 || pin == 6 || pin == 9 || pin == 10) {
    switch(divisor) {
      case 1: mode = 0x01; break;
      case 8: mode = 0x02; break;
      case 64: mode = 0x03; break;
      case 256: mode = 0x04; break;
      case 1024: mode = 0x05; break;
      default: return;
    }
    if(pin == 5 || pin == 6) {
      TCCR0B = TCCR0B & 0b11111000 | mode;
    } else {
      TCCR1B = TCCR1B & 0b11111000 | mode;
    }
  } else if(pin == 3 || pin == 11) {
    switch(divisor) {
      case 1: mode = 0x01; break;
      case 8: mode = 0x02; break;
      case 32: mode = 0x03; break;
      case 64: mode = 0x04; break;
      case 128: mode = 0x05; break;
      case 256: mode = 0x06; break;
      case 1024: mode = 0x7; break;
      default: return;
    }
    TCCR2B = TCCR2B & 0b11111000 | mode;
  }
}
