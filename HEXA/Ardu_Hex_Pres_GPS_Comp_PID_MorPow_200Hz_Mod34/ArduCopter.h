/*
  ArduCopter 1.3 - August 2010
  www.ArduCopter.com
  Copyright (c) 2010. All rights reserved.
  An Open Source Arduino based multicopter.
 
  This program is free software: you can redistribute it and/or modify 
  it under the terms of the GNU General Public License as published by 
  the Free Software Foundation, either version 3 of the License, or 
  (at your option) any later version. 

  This program is distributed in the hope that it will be useful, 
  but WITHOUT ANY WARRANTY; without even the implied warranty of 
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the 
  GNU General Public License for more details. 

  You should have received a copy of the GNU General Public License 
  along with this program. If not, see <http://www.gnu.org/licenses/>. 
*/

#include "WProgram.h"

/*******************************************************************/
// ArduPilot Mega specific hardware and software settings
// 
// DO NOT EDIT unless you are absolytely sure of your doings. 
// User configurable settings are on UserConfig.h
/*******************************************************************/

/**************************************************************/
// Special features that might disapear in future releases

/* APM Hardware definitions */
#define LED_Yellow 36
#define LED_Red 35
#define LED_Green 37
#define RELE_pin 47
#define SW1_pin 41
#define SW2_pin 40

// Sensor: GYROX, GYROY, GYROZ, ACCELX, ACCELY, ACCELZ
uint8_t sensors[6] = {1, 2, 0, 4, 5, 6};  // For ArduPilot Mega Sensor Shield Hardware

// Sensor: GYROX, GYROY, GYROZ,   ACCELX, ACCELY, ACCELZ,     MAGX, MAGY, MAGZ
int SENSOR_SIGN[]={
  1, -1, -1,    -1, 1, 1,     -1, -1, -1}; 

/* APM Hardware definitions, END */

/* General definitions */

#define TRUE 1
#define FALSE 0
#define ON 1
#define OFF 0


// ADC : Voltage reference 3.3v / 12bits(4096 steps) => 0.8mV/ADC step
// ADXL335 Sensitivity(from datasheet) => 330mV/g, 0.8mV/ADC step => 330/0.8 = 412
// Tested value : 408
#define GRAVITY 408 //this equivalent to 1G in the raw data coming from the accelerometer 
#define Accel_Scale(x) x*(GRAVITY/9.81)//Scaling the raw data of the accel to actual acceleration in meters for seconds square

#define ToRad(x) (x*0.01745329252)  // *pi/180
#define ToDeg(x) (x*57.2957795131)  // *180/pi

// IDG500 Sensitivity (from datasheet) => 2.0mV/º/s, 0.8mV/ADC step => 0.8/3.33 = 0.4
// Tested values : 
#define Gyro_Gain_X 0.4  //X axis Gyro gain
#define Gyro_Gain_Y 0.41 //Y axis Gyro gain
#define Gyro_Gain_Z 0.41 //Z axis Gyro gain
#define Gyro_Scaled_X(x) x*ToRad(Gyro_Gain_X) //Return the scaled ADC raw data of the gyro in radians for second
#define Gyro_Scaled_Y(x) x*ToRad(Gyro_Gain_Y) //Return the scaled ADC raw data of the gyro in radians for second
#define Gyro_Scaled_Z(x) x*ToRad(Gyro_Gain_Z) //Return the scaled ADC raw data of the gyro in radians for second

/*For debugging purposes*/
#define OUTPUTMODE 1  //If value = 1 will print the corrected data, 0 will print uncorrected data of the gyros (with drift), 2 Accel only data

int AN[6]; //array that store the 6 ADC channels
int AN_OFFSET[6]; //Array that store the Offset of the gyros and accelerometers
int gyro_temp;


float G_Dt=0.02;                  // Integration time for the gyros (DCM algorithm)
float Accel_Vector[3]= {0, 0, 0}; //Store the acceleration in a vector
float Accel_Vector_unfiltered[3]= {0, 0, 0}; //Store the acceleration in a vector
float Gyro_Vector[3]= {0, 0, 0};  //Store the gyros rutn rate in a vector
float Omega_Vector[3]= {0, 0, 0}; //Corrected Gyro_Vector data
float Omega_P[3]= {0, 0, 0};      //Omega Proportional correction
float Omega_I[3]= {0, 0, 0};      //Omega Integrator
float Omega[3]= {0, 0, 0};

float errorRollPitch[3] = {0, 0, 0};
float errorYaw[3] = {0, 0, 0};
float errorCourse = 0;
float COGX = 0; //Course overground X axis
float COGY = 1; //Course overground Y axis

float roll = 0;
float pitch = 0;
float yaw = 0;

//  Counter for Loop Control
unsigned int Magneto_counter = 0;
unsigned int BMP_counter = 0;
unsigned int GPS_counter = 0;

float DCM_Matrix[3][3]= {
  { 1,0,0 },
  { 0,1,0 },
  { 0,0,1 }}; 
float Update_Matrix[3][3]={
  { 0,1,2 },
  { 3,4,5 },
  { 6,7,8 }}; //Gyros here

float Temporary_Matrix[3][3]={
  { 0,0,0 },
  { 0,0,0 },
  { 0,0,0 }};

// GPS variables
float speed_3d=0;
int GPS_ground_speed=0;

// Main timers
long timer=0; 
long timer_old;
long GPS_timer;
long GPS_timer_old;
float GPS_Dt=0.2;   // GPS Dt

// Attitude control variables
float command_rx_roll=0;        // User commands
float command_rx_pitch=0;
float command_rx_yaw=0;
//float amount_rx_yaw=0;
int control_roll;           // PID control results
int control_pitch;
int control_yaw;
float K_aux;

// Attitude PID controls
float roll_I=0;
float roll_D;
float err_roll;
float pitch_I=0;
float pitch_D;
float err_pitch;
float yaw_I=0;
float yaw_D;
float err_yaw;

//Position and Stable control
long target_longitude;
long target_lattitude;
byte target_position = 0;
byte target_alt_position = 0;
byte heading_hold_mode = 0;
float current_heading_hold;
float target_altitude;
float gps_err_roll;
float gps_err_roll_old;
float gps_roll_D;
float gps_roll_I=0;
float gps_err_pitch;
float gps_err_pitch_old;
float gps_pitch_D;
float gps_pitch_I=0;
float command_gps_roll;
float command_gps_pitch;
float command_throttle;

//Altitude control
int Initial_Throttle;
int target_sonar_altitude;
int err_altitude;
int err_altitude_old;
float command_altitude;
float altitude_I;
float altitude_D;

//Pressure Sensor variables
#ifdef UseBMP
float BMP_target_altitude;
float BMP_err_altitude;
float BMP_err_altitude_old;
float BMP_command_altitude;
float BMP_altitude_I;
float BMP_altitude_D;
float tempPresAlt;
float BMP_Altitude;
//float BMP_Altitude_old;
#endif


#define BATTERY_VOLTAGE(x) (x*(INPUT_VOLTAGE/1024.0))*VOLT_DIV_RATIO

#define AIRSPEED_PIN 1		// Need to correct value
#define BATTERY_PIN 1		// Need to correct value
#define RELAY_PIN 47
#define LOW_VOLTAGE	11.4    // Pack voltage at which to trigger alarm
#define INPUT_VOLTAGE 5.2	// (Volts) voltage your power regulator is feeding your ArduPilot to have an accurate pressure and battery level readings. (you need a multimeter to measure and set this of course)
#define VOLT_DIV_RATIO 1.0	//  Voltage divider ratio set with thru-hole resistor (see manual)

float 	battery_voltage 	= LOW_VOLTAGE * 1.05;		// Battery Voltage, initialized above threshold for filter


// Sonar variables
int Sonar_value=0;
#define SonarToCm(x) (x*1.26)   // Sonar raw value to centimeters
int Sonar_Counter=0;

// AP_mode : 1=> Position hold  2=> Stabilization assist mode (normal mode) 0=> Acrobatic mode
byte AP_mode = 0;  
byte BMP_mode = 0;  //0 = Altitude hold off

//  PID Tuning
byte Plus = 0;
byte Minus = 0;
byte Plus_mode = 0;
byte Minus_mode = 0;
byte P_of_PID_mode = 0;
byte I_of_PID_mode = 0;
byte D_of_PID_mode = 0;
byte toggle_switch = 0;

// Mode LED timers and variables, used to blink LED_Green
byte gled_status = HIGH;
long gled_timer;
int gled_speed;

long t0;
int num_iter;
float aux_debug;

// Radio definitions
int roll_mid;
int pitch_mid;
int yaw_mid;
int throttle_mid = 1450;

int Neutro_yaw;
int ch_roll;
int ch_pitch;
int ch_throttle;
int ch_yaw;
int ch_gear;
int ch_aux2;
int ch_aux1;

int LeftCWMotor;
int LeftCCWMotor;
int RightCWMotor;
int RightCCWMotor;
int BackCWMotor;
int BackCCWMotor;
byte motorArmed = 0;
int minThrottle = 0;

// Serial communication
char queryType;
long tlmTimer = 0;

// Arming/Disarming
uint8_t Arming_counter=0;
uint8_t Disarming_counter=0;
