#include <i2c_t3.h>
#include <math.h>

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

byte accel_gyro_buffer[6];   // Array to store ADC values 


short i;

short gyro_y_w_drift;
short sample_points;
double gyro_y;
double gyro_drift;
double angle_y;
double gyro_drift_accum;

double ref_signal;
double error;
double pre_error;


IntervalTimer sync_timer;
int sampling_time; 
boolean tik;
boolean pre_tik;
boolean stop_loop;
boolean scope_debug;

boolean max_angle_reached;
boolean inc_duty_cycle; 

int valve_pin;
double duty_cycle;

byte byte_read; 
int number_of_bytes_read;
char string_read[64];
byte open_loop_traj[40000];
int open_loop_traj_size;
boolean load_led_state;
int load_led;

double pressure_spam;

elapsedMicros step_response_timer;
elapsedMillis initial_pause;
elapsedMillis data_load_led_timer;




void setup()
{
  Serial.begin(115200); 

  valve_pin = 10;
  load_led = 13;
  scope_debug = 8;
  
  pinMode(valve_pin, OUTPUT);
  pinMode(load_led, OUTPUT);
  pinMode(scope_debug,OUTPUT);
  
  
  analogWriteFrequency(valve_pin, 200);
  analogWriteResolution(12);


  analogReadResolution(12);
  analogReadAveraging(32);


  sampling_time = 1250;// sampling time in microseconds
  sync_timer.begin(timerCallback1,sampling_time); 

  delay(1);
  
  byte_read = 0;
  while(byte_read != 'G')
  {   
    byte_read = Serial.read();
  }
  
  
  data_load_led_timer = 0;
  number_of_bytes_read = 0;
  while(string_read[0] != 'T')
  { 
    while (!Serial.available()); 
    Serial.readBytesUntil('y',string_read,10);
    byte_read = atoi(string_read);
    open_loop_traj [number_of_bytes_read] = byte_read;
    number_of_bytes_read++;
    if (data_load_led_timer > 40)
    {
      load_led_state = !load_led_state;
      digitalWrite(load_led,load_led_state);
      data_load_led_timer=0;
    }
  }
  digitalWrite(load_led,false);
  
  number_of_bytes_read --;
  open_loop_traj_size = number_of_bytes_read;
  number_of_bytes_read = 0;
  
  Serial.write('T');
  Serial.write('y');
  
  while(byte_read != 'G')
  {   
    byte_read = Serial.read();
  }
  

  Wire.begin(I2C_MASTER, 0x00, I2C_PINS_18_19, I2C_PULLUP_EXT, I2C_RATE_400);
  // Set Gyro settings

  delay(1);

  writeTo(GYRO, GYRO_SAMPLERATE_ADDR, GYRO_SAMPLERATE_DIVIDER);

  writeTo(GYRO, GYRO_SCALE_ADDR, GYRO_SCALE_VALUE);
  
  writeTo(GYRO, GYRO_PWRCTRL_ADDR, GYRO_PWRCTRL_VALUE);
  
  initial_pause = 0;
  
  max_angle_reached = false;
  
}

void timerCallback1() {
  tik = !tik;
}

void loop()
{
   
  if(!stop_loop)
  {
    if (tik != pre_tik)
    {      
      digitalWrite(scope_debug,HIGH);
      
      
      Wire.beginTransmission(GYRO);
      Wire.write(GYRO_Y_HSB_ADDR); 
      Wire.endTransmission();

      Wire.requestFrom(GYRO,2); // Read 2 bytes

      i = 0;
      while(Wire.available())
      {
        accel_gyro_buffer[i] = Wire.read();
        i++;
      }
      
      
      if (initial_pause <2500)
      {
        sample_points ++;

        gyro_y_w_drift = accel_gyro_buffer[0] << 8 | accel_gyro_buffer[1];

        gyro_drift_accum += double(gyro_y_w_drift);
        
        gyro_drift = gyro_drift_accum/double(sample_points);
        
        gyro_y = 0;
        angle_y = 0;
        duty_cycle = 0;
        pressure_spam = 0;
      }  
      else
      {
        gyro_y_w_drift = accel_gyro_buffer[0] << 8 | accel_gyro_buffer[1]; // Gyro with drift

        gyro_y = (double(gyro_y_w_drift) - gyro_drift);
        
        angle_y += gyro_y*M_PI/2070000; // the resloution, micro s scaled  Ts/14.375*180 multiplied by -1 to follow right hand rule
        
        unsigned short tmp_short;
        tmp_short =  open_loop_traj [number_of_bytes_read] << 8 | open_loop_traj [number_of_bytes_read+1];
        ref_signal = 1.3*double(tmp_short)/100;
        tmp_short =  open_loop_traj [number_of_bytes_read+2] << 8 | open_loop_traj [number_of_bytes_read+3];
//        duty_cycle = double(tmp_short);
        
        if (number_of_bytes_read > open_loop_traj_size - 5)
        {
          number_of_bytes_read = 0;
        }
        else
        {
          number_of_bytes_read +=4 ;
        }
        
        
//        Open Loop duty cycle  triangular wave
//
//          if (inc_duty_cycle)
//            duty_cycle = duty_cycle+ 2; 
//          else
//            duty_cycle = duty_cycle- 2; 
//            
//          if (duty_cycle > 2000)
//            inc_duty_cycle = false;
//          else if (duty_cycle < 1000)
//            inc_duty_cycle = true;



//        Open Loop duty cycle  square wave
        
//        double stepSize = 700;
//        if (step_response_timer>2500000)
//        {
//          step_response_timer = 0;
//          if (ref_signal == stepSize)
//            ref_signal = 400;
//          else
//            ref_signal = stepSize;
//        }





//    PD Controller 
      
      pre_error = error;
      error = ref_signal - pressure_spam;
      
      duty_cycle = (error)*7+(error-pre_error )*1;
      

//    Speed test

//      if (!max_angle_reached && angle_y < 3.0)
//      {
//        duty_cycle = 3000;
//      }
//      else
//      {
//        max_angle_reached = true;
//        duty_cycle = 0;
//      }
        
//        duty_cycle = 500;
        
        pressure_spam = analogRead(9);;
    }
      
      analogWrite(valve_pin,duty_cycle);  
      
      // Outputting Results
      Serial.print(angle_y,7);
      Serial.print(",");
      Serial.print(ref_signal,7);
      Serial.print(",");
      Serial.print(duty_cycle,7); 
      Serial.print(",");
      Serial.print(pressure_spam,1);
      Serial.println("y");
      Serial.send_now();
      
      digitalWrite(scope_debug,LOW);   
      pre_tik = tik;
   }
   

   if (Serial.available() > 0)
   {
        byte_read = Serial.read();
        if(byte_read=='S')
        {
          stop_loop = true;
        }
        
    }
  }
  else if (Serial.available() > 0)
  {
    byte_read = Serial.read();
    if(byte_read=='G')
    {
      stop_loop = false;
      initial_pause = 0;
      max_angle_reached = false;
    }
  }
  else
  {
    analogWrite(valve_pin,0); 
  }
}



// Write a value to address register on device
void writeTo(int device, byte address, byte val) {
  Wire.beginTransmission(device); // start transmission to device 
  Wire.write(address);            // send register address
  Wire.write(val);                // send value to write
  Wire.endTransmission(I2C_NOSTOP);         // end transmission
}


