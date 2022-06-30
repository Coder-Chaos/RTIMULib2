////////////////////////////////////////////////////////////////////////////
//
//  This file is part of RTIMULib
//
//  Copyright (c) 2014-2015, richards-tech, LLC
//
//  Permission is hereby granted, free of charge, to any person obtaining a copy of
//  this software and associated documentation files (the "Software"), to deal in
//  the Software without restriction, including without limitation the rights to use,
//  copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the
//  Software, and to permit persons to whom the Software is furnished to do so,
//  subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included in all
//  copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
//  INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
//  PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
//  HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
//  OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
//  SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


#include "RTIMULib.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <errno.h>
#include <arpa/inet.h>
#include <inttypes.h> 
#define PORT 6001
#define PARAM_OFFSET(Strct, Field)    ((unsigned long)&(((Strct *)0)->Field))

int count = 1;

int main()
{
    int sampleCount = 0;
    int sampleRate = 0;
    uint64_t rateTimer;
    uint64_t displayTimer;
    uint64_t now;

    //  Using RTIMULib here allows it to use the .ini file generated by RTIMULibDemo.
    //  Or, you can create the .ini in some other directory by using:
    //      RTIMUSettings *settings = new RTIMUSettings("<directory path>", "RTIMULib");
    //  where <directory path> is the path to where the .ini file is to be loaded/saved

    RTIMUSettings *settings = new RTIMUSettings("RTIMULib");

    RTIMU *imu = RTIMU::createIMU(settings);

    if ((imu == NULL) || (imu->IMUType() == RTIMU_TYPE_NULL)) {
        printf("No IMU found\n");
        exit(1);
    }

    //  This is an opportunity to manually override any settings before the call IMUInit

    //  set up IMU

    imu->IMUInit();

    //  this is a convenient place to change fusion parameters

    imu->setSlerpPower(0.02);
    imu->setGyroEnable(true);
    imu->setAccelEnable(true);
    imu->setCompassEnable(true);

    //  set up for rate timer

    rateTimer = displayTimer = RTMath::currentUSecsSinceEpoch();
    
    //  now just process data
    int sockfd;
    //char buffer[2014];
    struct sockaddr_in server_addr;
    //struct hostent *host;
    //int nbytes;
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        fprintf(stderr, "Socket Error is %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("192.168.10.100");//server_addr.sin_addr.s_addr = inet_addr("192.168.69.129")，指定ip的代码
    //客户端发出请求
    while (connect(sockfd, (struct sockaddr *)(&server_addr), sizeof(struct sockaddr)) == -1)
    {
        fprintf(stderr, "Connect failed, waitting!\n");
        sleep(1);
        //exit(EXIT_FAILURE);
    }
    //uint64_t _timestamp_old;
    int i = 0;
    while (1) {
        //  poll at the rate recommended by the IMU

        usleep(imu->IMUGetPollInterval() * 1000 * 1000);

        while (imu->IMURead()) {
            RTIMU_DATA imuData = imu->getIMUData();
            sampleCount++;

            now = RTMath::currentUSecsSinceEpoch();

            //  display 10 times per second

            if ((now - displayTimer) > 100000) {
                
                if(imuData.accelValid){
                    //printf("Sample rate1 %d: %s\r", sampleRate, RTMath::displayRadians("", imuData.accel));
                   
                    //char sendbuf[sizeof(imuData)];
                    struct output_t
                    {
                    	uint64_t timestamp;           
                    	float acc[3];
                    	float gyro[3];
                    	bool accelValid;
                    	bool gyroValid;
                    };
                    output_t output =
                    {
                    	imuData.timestamp,                    	
                    	{
                    		imuData.accel.x(),
                    		imuData.accel.y(),
                    		imuData.accel.z()
                    	},                    	
                    	{
                    		imuData.gyro.x(),
                    		imuData.gyro.y(),
                    		imuData.gyro.z()
                    	},
                    	imuData.accelValid,
                    	imuData.gyroValid,
                    };
                    //printf("IMU sampleRate %d!\n", sampleRate);
                    if(i%1 == 0)
                        send(sockfd, (const char *)(&output), sizeof(output), 0);
                    //memset(sendbuf, 0, sizeof(sendbuf));
                    i++;
                    usleep(1000 * 1000);
                    
                     

                } else{
                    printf("Sample rate2 %d: %s\r", sampleRate, RTMath::displayDegrees("", imuData.fusionPose));
                }
                
                fflush(stdout);
                displayTimer = now;
            }

            //  update rate every second

            if ((now - rateTimer) > 1000000) {
                sampleRate = sampleCount;
                sampleCount = 0;
                rateTimer = now;
            }
        }
    }
    close(sockfd);
    exit(EXIT_SUCCESS); 
}

