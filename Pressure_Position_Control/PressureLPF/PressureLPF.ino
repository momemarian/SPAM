#define GYRO 0x68 //GYRO I2C Address    

#define GYRO_SAMPLERATE_ADDR 0x15 // Address of the divider for the sampling rate
#define GYRO_SAMPLERATE_DIVIDER 0x09 //Divder for 800Hz sampling rate

#define GYRO_SCALE_ADDR 0x16 // Scaling and LPF BW  
#define GYRO_SCALE_VALUE 0x18 //2000 range with 256Hz as BW

#define GYRO_PWRCTRL_ADDR 0x3E // Address of the power control byte for sleep and clock settings 
#define GYRO_PWRCTRL_VALUE 0x01 // set it to Y GYRO Ref

#define GYRO_Y_HSB_ADDR 0x1F // GYRO Y axis HSB byte

#define ACC_WEIGHT_VALID_COUNT 40 // for the this amount of sample, the acceleration vector needs be within the acceptable tolerance for all samples  
#define ACC_MAG_AVRG_PERIOD 1 // number of samples for averaging the accel magnitude data for accel weight calculation

byte buffer[6];   // Array to store ADC values 


short i;

short gyro_y_w_drift;
short sample_points;



double gyro_y;
double gyro_drift;
double angle_y;
double gyro_drift_accum;

IntervalTimer syncTimer;
int Ts; 
boolean Tik;
boolean preTik;
boolean startRun = false;
boolean stopRun;
boolean scopeDebug;

int valvePin;
double duty_cycle;

byte byteRead; 

double pressureSPAM;
double pressureSPAM_0;
double pressureSPAM_1;
double pressureSPAM_2;
double pressureSPAM_3;
double pressureSPAM_4;


double sensorPressure_0;
double sensorPressure_1;
double sensorPressure_2;
double sensorPressure_3;
double sensorPressure_4;

elapsedMicros stepResponseTimer;
elapsedMillis initialPause;


#include <i2c_t3.h>
#include <math.h>

void setup()
{
  Serial.begin(115200); 
  

  analogReadResolution(12);
  analogReadAveraging(32);

  valvePin = 10;
  
  pinMode(valvePin, OUTPUT);
  analogWriteFrequency(valvePin, 300);
  analogWriteResolution(12);

  scopeDebug = 8;
  pinMode(scopeDebug,OUTPUT);
  
  Ts = 1250;// sampling time in microseconds
  syncTimer.begin(timerCallback1,Ts); 

  delay(1);
  
  byteRead = 0;
  while(byteRead != 'G')
  {   
    byteRead = Serial.read();
  }
  
  
  Wire.begin(I2C_MASTER, 0x00, I2C_PINS_18_19, I2C_PULLUP_EXT, I2C_RATE_400);
  // Set Gyro settings

  delay(1);

  writeTo(GYRO, GYRO_SAMPLERATE_ADDR, GYRO_SAMPLERATE_DIVIDER);

  writeTo(GYRO, GYRO_SCALE_ADDR, GYRO_SCALE_VALUE);
  
  writeTo(GYRO, GYRO_PWRCTRL_ADDR, GYRO_PWRCTRL_VALUE);
  
  initialPause = 0;
}

void timerCallback1() {
  Tik = !Tik;
}

void loop()
{
   
  if(!stopRun)
  {
    if (Tik != preTik)
    {      
      digitalWrite(scopeDebug,HIGH);
      
      
      Wire.beginTransmission(GYRO);
      Wire.write(GYRO_Y_HSB_ADDR); 
      Wire.endTransmission();

      Wire.requestFrom(GYRO,2); // Read 2 bytes

      i = 0;
      while(Wire.available())
      {
        buffer[i] = Wire.read();
        i++;
      }
      
      
      if (initialPause <2500)
      {
        sample_points ++;

        gyro_y_w_drift = buffer[0] << 8 | buffer[1];

        gyro_drift_accum += double(gyro_y_w_drift);
        
        gyro_drift = gyro_drift_accum/double(sample_points);
        
        gyro_y = 0;
        angle_y = 0;
        duty_cycle = 0;
        pressureSPAM = 0;
      }  
      else
      {
        gyro_y_w_drift = buffer[0] << 8 | buffer[1]; // Gyro with drift

        gyro_y = double(gyro_y_w_drift) - gyro_drift;
        
        angle_y += gyro_y*M_PI/2070000;
        
        int stepValue = 700;
        
//        if (stepResponseTimer>2500000)
//        {
//          stepResponseTimer = 0;
//          if (duty_cycle == stepValue)
//            duty_cycle = 2900;
//          else
//            duty_cycle = stepValue;
//        }
        duty_cycle = 2000;
        sensorPressure_4 = sensorPressure_3;
        sensorPressure_3 = sensorPressure_2;
        sensorPressure_2 = sensorPressure_1;
        sensorPressure_1 = sensorPressure_0;
        sensorPressure_0 =  analogRead(9);

        pressureSPAM = (sensorPressure_0*-0.0315645)+(sensorPressure_1*0.1142015)+(sensorPressure_2*0.8341504)+(sensorPressure_3*0.1142015)+(sensorPressure_4*-0.0315645);
        pressureSPAM = sensorPressure_0;
    }
      
      analogWrite(valvePin,duty_cycle);  
      
      // Outputting Results
      Serial.print(angle_y,7);
      Serial.print(",");
      Serial.print(gyro_y,7);
      Serial.print(",");
      Serial.print(duty_cycle,1); 
      Serial.print(",");
      Serial.print(pressureSPAM,1);
      Serial.println("y");
      Serial.send_now();
      
      digitalWrite(scopeDebug,LOW);   
      preTik = Tik;
   }
   

   if (Serial.available() > 0)
   {
        byteRead = Serial.read();
        if(byteRead=='S')
        {
          stopRun = true;
        }
        
    }
  }
  else if (Serial.available() > 0)
  {
    byteRead = Serial.read();
    if(byteRead=='G')
    {
      stopRun = false;
      initialPause = 0;
    }
  }
  else
  {
    analogWrite(valvePin,0); 
  }
}



// Write a value to address register on device
void writeTo(int device, byte address, byte val) {
  Wire.beginTransmission(device); // start transmission to device 
  Wire.write(address);            // send register address
  Wire.write(val);                // send value to write
  Wire.endTransmission(I2C_NOSTOP);         // end transmission
}


