/*
This code is provided under the BSD license.
Copyright (c) 2014, Emlid Limited. All rights reserved.
Written by Mikhail Avkhimenia (mikhail.avkhimenia@emlid.com)
twitter.com/emlidtech || www.emlid.com || info@emlid.com

Application: PPM to PWM decoder for Raspberry Pi with Navio.

Requres pigpio library, please install it first - http://abyz.co.uk/rpi/pigpio/
To run this app navigate to the directory containing it and run following commands:
make
sudo ./PPM
*/

#include <pigpio.h>
#include <stdio.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/time.h>

#include <iostream>
#include <sstream>
#include <iomanip>

#include <fcntl.h>
#include <unistd.h>

#include "gpio.h"
#include "PCA9685.h"
#include "MPU9250.h"
#include "AHRS.h"

#ifdef	__cplusplus
extern "C" {
#endif

#include "Navio_250.h"

#ifdef	__cplusplus
}
#endif

#define SERVO_MIN 1000 /*mS*/


// Objects

MPU9250 imu;    // MPU9250
AHRS    ahrs;   // Mahony AHRS

// Sensor data

float ax, ay, az;
float gx, gy, gz;
float mx, my, mz;

// Orientation data

float roll, pitch, yaw;

// Timing data

float offset_gyro[3];
float offset_accel[3];
struct timeval tv;
float dt;
unsigned long previoustime, currenttime;
float dtsumm = 0;
int isFirst = 1;
int isFirstLog = 1;
int loggingOn = 0;

// Network data

int sockfd;
struct sockaddr_in servaddr = {0};
char sendline[80];

//Logger data
int logFd_;


//================================ Options =====================================

unsigned int samplingRate      = 1;      // 1 microsecond (can be 1,2,4,5,10)
unsigned int ppmInputGpio      = 4;      // PPM input on Navio's 2.54 header
unsigned int ppmSyncLength     = 4000;   // Length of PPM sync pause
unsigned int ppmChannelsNumber = 8;      // Number of channels packed in PPM
unsigned int servoFrequency    = 200;     // Servo control frequency
bool verboseOutputEnabled      = false;   // Output channels values to console

//============================ Objects & data ==================================

PCA9685 *pwm;
float channels[8];

//============================== PPM decoder ===================================

unsigned int currentChannel = 0;
unsigned int previousTick;
unsigned int deltaTime;

void imuSetup()
{
    //ceci est un commentaire
    //----------------------- MPU initialization ------------------------------

    imu.initialize();

    //-------------------------------------------------------------------------

	printf("Beginning Gyro and Accel calibration...\n");
	for(int i = 0; i<100; i++)
	{
		imu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
		offset_gyro[0] += gx*0.0175;
		offset_gyro[1] += gy*0.0175;
		offset_gyro[2] += gz*0.0175;
                
                offset_accel[0] += ax;
		offset_accel[1] += ay;
		offset_accel[2] += az;
                
		usleep(10000);
	}
	offset_gyro[0]/=100.0;
	offset_gyro[1]/=100.0;
	offset_gyro[2]/=100.0;
        offset_accel[0]/=100.0;
	offset_accel[1]/=100.0;
	offset_accel[2]/=100.0;
        
        
	printf("gyro offsets are: %f %f %f\n", offset_gyro[0], offset_gyro[1], offset_gyro[2]);
	printf("accel offsets are: %f %f %f\n", offset_accel[0], offset_accel[1], offset_accel[2]);
        
        ahrs.setGyroOffset(offset_gyro[0], offset_gyro[1], offset_gyro[2]);
        ahrs.setAccelOffset(offset_accel[0], offset_accel[1], offset_accel[2]-1.0);
        
}

void ppmOnEdge(int gpio, int level, uint32_t tick)
{
	if (level == 0) {	
		deltaTime = tick - previousTick;
		previousTick = tick;
	
		if (deltaTime >= ppmSyncLength) { // Sync
			currentChannel = 0;

                

			// Console output
			if (verboseOutputEnabled) {
				printf("\n");
				for (int i = 0; i < ppmChannelsNumber; i++)
					printf("%4.f ", channels[i]);
			}
		}
		else
			if (currentChannel < ppmChannelsNumber)
				channels[currentChannel++] = deltaTime;
	}
}

//================================== Main ======================================

using namespace Navio;

int main(int argc, char *argv[])
{
    static const uint8_t outputEnablePin = RPI_GPIO_27;

    Pin pin(outputEnablePin);

    if (pin.init()) {
        pin.setMode(Pin::GpioModeOutput);
        pin.write(0); /* drive Output Enable low */
    } else {
        fprintf(stderr, "Output Enable not set. Are you root?");
    }

        //-------------------- Servo controller setup ----------------------------
	//peripheral initialise
        PCA9685 pwm;
	pwm.initialize();
	pwm.setFrequency(400);
        
        //set speed to 0
        pwm.setPWMuS(3, SERVO_MIN); //output 3 avant droit
        pwm.setPWMuS(4, SERVO_MIN); //output 4 arriere gauche
        pwm.setPWMuS(5, SERVO_MIN); //output 5 avant gauche
        pwm.setPWMuS(6, SERVO_MIN); //output 6 arriere droit

	
        //-------------------- RC interrupt setup----------------------------
	gpioCfgClock(samplingRate, PI_DEFAULT_CLK_PERIPHERAL, 0); /* last parameter is deprecated now */
	gpioInitialise();
	previousTick = gpioTick();
	gpioSetAlertFunc(ppmInputGpio, ppmOnEdge);
        
        
        //-------------------- IMU setup and main loop ----------------------------
        imuSetup();

	Navio_250_initialize();
  
        Navio_250_U.extparams[0] = 0.125;//p_p //0.12
        Navio_250_U.extparams[10] = 0.000;//0.09;//p_i
        Navio_250_U.extparams[11] = 0.004;//p_d //0.0025 steps

        Navio_250_U.extparams[15] = 0.125;//q_p //0.08
        Navio_250_U.extparams[16] = 0.000;//0.09;//q_i
        Navio_250_U.extparams[17] = 0.004;//q_d //0.0025 steps

        Navio_250_U.extparams[1] = 1.2;//r_p
        Navio_250_U.extparams[13] = 0.2;//r_breakout
        Navio_250_U.extparams[12] = -1.0;//head_p

        Navio_250_U.extparams[14] = 1.0;//attitude_mode 

        Navio_250_U.extparams[6] = 45.0;//phi_scale theta_scale
        Navio_250_U.extparams[2] = 180.0;//p_scale q_scqle
        Navio_250_U.extparams[3] = 150.0;//r_scale

        Navio_250_U.extparams[4] = 6.500;//phi_p theta_p //7.0
        Navio_250_U.extparams[5] = 1.000;//0.500;//phi_i theta_i //1.0//0.5


	while(1)
        {
            	
	//----------------------- Calculate delta time ----------------------------
	gettimeofday(&tv,NULL);
	previoustime = currenttime;
	currenttime = 1000000 * tv.tv_sec + tv.tv_usec;
	dt = (currenttime - previoustime) / 1000000.0;
	if(dt < 1/400.0) usleep((1/400.0-dt)*1000000);
        gettimeofday(&tv,NULL);
        currenttime = 1000000 * tv.tv_sec + tv.tv_usec;
	dt = (currenttime - previoustime) / 1000000.0;
        //----------------------- END Calculate delta time ----------------------------
        
        
        //-------- Read raw measurements from the MPU and update AHRS --------------
        imu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
        ahrs.updateIMU(ax, ay, az, gx*0.0175, gy*0.0175, gz*0.0175, dt);
        ahrs.getEuler(&roll, &pitch, &yaw);
        //-------- END Read raw measurements from the MPU and update AHRS --------------

        
        //-------- Update control law parameters --------------
        Navio_250_U.rx[0]=channels[2]*10;
        Navio_250_U.rx[2]=channels[0]*10;
        Navio_250_U.rx[3]=channels[1]*10;
        Navio_250_U.rx[4]=channels[3]*10;
        Navio_250_U.rx[5]=channels[4]*10;
 
        Navio_250_U.rates[0] =-(gy*0.0175+offset_gyro[1]);
        Navio_250_U.rates[1] = gx*0.0175+offset_gyro[0];
        Navio_250_U.rates[2] = gz*0.0175+offset_gyro[2];

        Navio_250_U.ahrs[0] = roll;//roll
        Navio_250_U.ahrs[1] = pitch;//pitch
        Navio_250_U.ahrs[2] = yaw;//yaw
        //-------- END Update control law parameters --------------
        
        
        //-------- Update control outputs --------------
        Navio_250_step(); //Control law
        //-------- END Update control outputs --------------

        
        //-------- Set motors value --------------
        pwm.setPWMuS(3, Navio_250_Y.servos[1]/10); //output 3 avant droit
        pwm.setPWMuS(4, Navio_250_Y.servos[3]/10); //output 4 arriere gauche
        pwm.setPWMuS(5, Navio_250_Y.servos[0]/10); //output 5 avant gauche
        pwm.setPWMuS(6, Navio_250_Y.servos[2]/10); //output 6 arriere droit

        //pwm.setPWMuS(3, channels[2]); //output 3 avant droit
        //pwm.setPWMuS(4, channels[2]); //output 4 arriere gauche
        //pwm.setPWMuS(5, channels[2]); //output 5 avant gauche
        //pwm.setPWMuS(6, channels[2]); //output 6 arriere droit
        //-------- END Set motors value --------------
        
        
        //-------------------- Logger 200Hz ----------------------
        
        //wait for RC command to start logfile
        if(channels[0] > 1850 && channels[3] < 1150 && isFirstLog == 1)
        {
            //turn yellow led on
            uint16_t R = 0, G = 0, B = 4095;
            pwm.setPWM(2, R);
            pwm.setPWM(1, G);
            pwm.setPWM(0, B);
    
    
            std::cout << "logging is on" << std::endl;
            
            //don't get there twice
            isFirstLog = 0;
            
            //activate logging on loop
            loggingOn = 1;
        
            //--create logfile name
            static int seconds_last = 99;
            char TimeString[128];

            struct timeval curTime;
            gettimeofday(&curTime, NULL);
            if (seconds_last == curTime.tv_sec)
             {   
                std::cout << "error in time" << std::endl;
              }
            seconds_last = curTime.tv_sec;

            strftime(TimeString, 80, "%Y-%m-%d %H:%M:%S", localtime(&curTime.tv_sec));
            //--end create logfile name
            
            //--create logfile
            logFd_ = open(TimeString, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
            
            //throw an exception if needed
            if (logFd_ < 0)
            {
                std::cout << "log fd error" << logFd_ << std::endl;
            }
            //--end create logfile
        }
            
        //once logfile has been created
        if(loggingOn == 1)
        {  
            std::ostringstream os;

            os << std::setprecision(12);
            os << currenttime << ",";
            os << std::setprecision(6);
            os << roll << ",";
            os << pitch << ",";
            os << yaw << ",";
            os << channels[0] << ",";
            os << channels[1] << ",";
            os << channels[2] << ",";
            os << channels[3] << ",";
            os << channels[4] << ",";
            os << Navio_250_Y.servos[0]/10 << ",";
            os << Navio_250_Y.servos[1]/10 << ",";
            os << Navio_250_Y.servos[2]/10 << ",";
            os << Navio_250_Y.servos[3]/10 << ",";
            os << std::endl;

            // write it out, not buffered - want to get it on the disk asap
            write(logFd_, os.str().data(), os.str().length());
        }
        
        //wait for end of logfile
        if(channels[0] < 1150 && channels[3] > 1850 && loggingOn == 1)
        {
            //turn led off
            uint16_t R = 0, G = 0, B = 0;
            pwm.setPWM(2, R);
            pwm.setPWM(1, G);
            pwm.setPWM(0, B);
            
            std::cout << "logging is off" << std::endl;

            //re-init variables for next lofile
            loggingOn = 0;
            isFirstLog = 1;
            
            close(logFd_);
            
        }
        //-------------------- END Logger 200Hz ----------------------
        
        
        
        //------------- Console and network output with a lowered rate ------------
        dtsumm += dt;
        
        if(dtsumm > 0.05)
        {            
            // Console output
            //std::cout << Navio_250_U.ahrs[0] << " ";
            //std::cout << Navio_250_U.ahrs[1] << " ";
            //std::cout << Navio_250_U.ahrs[2] << " " << std::endl;
            //std::cout << dt << " " << ax << " " << ay;
            //std::cout << " " << az << std::endl;
            //printf("%.2f %.2f %.2f %.4f %d %d %d\n", Navio_250_U.ahrs[0], );
            //printf("%f %f %f %f\n", Navio_250_U.rx[0], Navio_250_U.rx[2], Navio_250_U.rx[3], Navio_250_U.rx[4]);
            //printf("%f %f %f %f\n\n", Navio_250_Y.addlog[8], Navio_250_Y.addlog[9], Navio_250_Y.addlog[10], Navio_250_Y.addlog[11]);
            //printf("%.4f\n",dt);
            //printf("%f %f %f %f\n", channels[0], channels[1], channels[2], channels[3]);
            dtsumm = 0;
        }
        //-------------end console output
        
    }
    
    return 0;
}