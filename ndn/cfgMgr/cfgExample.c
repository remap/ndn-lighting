//
//  cfgExample.c
//  
//
//  Created by nano on 7/7/11.
//  Copyright 2011 UCLA Regents. All rights reserved.
//
//  This is an example of how to use the c/python 'configuration manager'
//  library from C.
//
//  all configuration data should be placed in 'config', and accessed via
//  cfgMgr library


#include "cfgMgr.h"
#include "stdio.h"


int main(){
    
    printf("Config Manager Example: \n");
    
    // careful - will segfault if you don't ask for a value in config file
	
    printf("my app URI is %s \n", getConfigString("appPath"));
    
    printf("num devices: %i \n", getMfrDeviceCount());
    
    // note index count starts from 1
    // 0 works, but is changed to 1 internally.
    
    printf("MAC ADDR of ith device: %s \n", getMfrDeviceID(0));
    printf("MAC ADDR of ith device: %s \n", getMfrDeviceID(2));
    printf("MAC ADDR of ith device: %s \n", getMfrDeviceID(3));
       
    printf("IP of ith device: %s \n", getMfrDeviceIP(1));
    printf("IP of ith device: %s \n", getMfrDeviceIP(2));
    printf("IP of ith device: %s \n", getMfrDeviceIP(3));

    printf("Type of ith device: %s \n", getMfrDeviceType(1));
    printf("Type of ith device: %s \n", getMfrDeviceType(2));
    printf("Type of ith device: %s \n", getMfrDeviceType(3));
    
   // printf("typeComponent of ith device %s \n", getMfrTypeComponent(0));
    
    return 0;
}