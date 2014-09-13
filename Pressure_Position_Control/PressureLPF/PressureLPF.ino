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
double position_spam;
double gyro_drift_accum;


double ref_position;
double error_position;
double pre_error_position;
double sum_error_position;

double pressure_spam;
double ref_pressure;
double error_pressure;
double pre_error_pressure;
double sum_error_pressure;

double Kff;

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

unsigned short tmp_short_press;
unsigned short tmp_short_pos;
boolean initial_ramp;

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
  
  
  analogWriteFrequency(valve_pin, 300);
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
        position_spam = 0;
        duty_cycle = 0;
        pressure_spam = 0;
		initial_ramp = true;
		ref_position = 0;
		ref_pressure = 0;
		Kff = 1.2;
		
      }  
      else
      {
        gyro_y_w_drift = accel_gyro_buffer[0] << 8 | accel_gyro_buffer[1]; // Gyro with drift

        gyro_y = (double(gyro_y_w_drift) - gyro_drift);
        
        position_spam += gyro_y*M_PI/2070000; // the resloution, micro s scaled  Ts/14.375*180 multiplied by -1 to follow right hand rule
        
		
		tmp_short_press =  open_loop_traj [0] << 8 | open_loop_traj [1];
		tmp_short_pos =  open_loop_traj [2] << 8 | open_loop_traj [3];
		
		if (initial_ramp)
		{
			if (ref_pressure > Kff*double(tmp_short_press)/100)
			{
				initial_ramp = false;
			}
			else
			{
				ref_pressure += Kff*double(tmp_short_press)/80000;
				ref_position += double(tmp_short_pos)/8000000;
			}
		}
		else
		{
			tmp_short_press =  open_loop_traj [number_of_bytes_read] << 8 | open_loop_traj [number_of_bytes_read+1];
			ref_pressure = Kff*double(tmp_short_press)/100;
			//ref_signal = double(tmp_short)/1000 - 7.465;
		
			tmp_short_pos =  open_loop_traj [number_of_bytes_read+2] << 8 | open_loop_traj [number_of_bytes_read+3];
			ref_position = double(tmp_short_pos)/10000;
	//        duty_cycle = double(tmp_short);
        
			if (number_of_bytes_read > open_loop_traj_size - 5)
			{
			  number_of_bytes_read = 0;
			}
			else
			{
			  number_of_bytes_read +=4 ;
			}
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





//    PD Pressure Controller 
      
		pre_error_pressure = error_pressure;
		error_pressure = ref_pressure - pressure_spam;
		sum_error_pressure += error_pressure;
	  
//    PD Position Controller

		pre_error_position = error_position;
		error_position = ref_position - position_spam;  
		sum_error_position += error_position;
	  
	  
	//duty_cycle = error_pressure*10+(error_pressure-pre_error_pressure)*0 + error_position*0 + (error_position-pre_error_position)*60000; // Joy with FF 1.4
	//duty_cycle = error_pressure*9+error_position*0 + (error_position-pre_error_position)*10000; // Anger 
	duty_cycle = error_pressure*9+error_position*0 + (error_position-pre_error_position)*20000 + sum_error_pressure*0+sum_error_position*3; // Sad 
	//duty_cycle = 1800;	

//    PD Velocity Controller

		//pre_error = error;
		//error = ref_signal - gyro_y*M_PI/2587.5;
//
		//duty_cycle += (error)*1 +(error-pre_error )*0;


//    Speed test

//      if (!max_angle_reached && position_spam < 3.0)
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
      Serial.print(position_spam,7);
      Serial.print(",");
      Serial.print(ref_position,7);
      Serial.print(",");
      Serial.print(pressure_spam,7); 
      Serial.print(",");
      Serial.print(ref_pressure,7);
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


