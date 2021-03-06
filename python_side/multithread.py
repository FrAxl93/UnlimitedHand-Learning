# -*- coding: utf-8 -*-
"""
Created on Tue Jun  6 16:06:45 2017

@author: Francesco
"""
import threading
import sys

import serial
import numpy as np
import time

import matplotlib.pyplot as plt

global PORT
global BAUD
global NUM_CHANNELS
global END_BUNDLE_BYTE
global BYTE_PER_CHANNEL
global BUNDLE_LENGTH

#BUNDLE SHAPE: |!|!|!|CH0_msb|CH0_lsb|ch1_msb|ch1_lsb|......|ch7_lsb|!|!|!|

PORT = "COM3"
BAUD = 115200
NUM_CHANNELS = 8
END_BUNDLE_BYTE = 3
BYTE_PER_CHANNEL = 2 #two bytes to represent int
BUNDLE_LENGTH = NUM_CHANNELS*BYTE_PER_CHANNEL


global ROWS
global COLS
ROWS = 4
COLS = 2

global ACQUISITION_TIME
ACQUISITION_TIME = 60
global SAMPLING_INTERVAL
SAMPLING_INTERVAL = 0.5
global n_MOVEMENT_TO_UNDERSTAND
n_MOVEMENT_TO_UNDERSTAND = 4 #up down left right
global movements
movements = ["up\n","down\n","left\n","right\n"]


class SerialReader(threading.Thread):
    
    def __init__(self, name, port_name, baud, data, event_run):
        threading.Thread.__init__(self)
        self.name = name
        self.port_name = port_name
        self.baud = baud
        self.event_run = event_run
        self.data = data
        print("Attempting to open port %s at baud %d" %(self.port_name,self.baud))
        self.port = serial.Serial(self.port_name,self.baud,timeout=1)
        if(self.port.isOpen()): print("Port Open")
        
    def run(self):
        
        start_time = time.time()
        running = True
        while(running):
            try:
                #actual decoding
                if(self.port.read(END_BUNDLE_BYTE).decode("raw_unicode_escape") == '!!!'):
                    temp = self.port.read(BUNDLE_LENGTH)
                    #print(temp)
                    for channel in range(0,NUM_CHANNELS):
                        self.data[channel] = (temp[channel*BYTE_PER_CHANNEL]<<8)|(temp[channel*BYTE_PER_CHANNEL + 1 ])
                    #allow the plotting thread to access the data
                    self.event_run.set()
                    self.event_run.clear()
                    
                    time.sleep(SAMPLING_INTERVAL)
                    
                total_elapsed = time.time() - start_time
                
                
                if(total_elapsed > ACQUISITION_TIME): #more than 30 seconds of acquisition
                    print("From %s, closing port"%self.name)    
                    self.port.close()
                    running = False
            except KeyboardInterrupt:
                self.port.close()
                break
        
class DynamicPlotter(threading.Thread):
    
    
    def __init__(self,name,data,event_run):
        threading.Thread.__init__(self)
        # Scrive un file.
        self.out_file = open("test.txt","w")
        self.data = data
        self.event_run = event_run
        self.name = name
        self.number_of_acq_total = ACQUISITION_TIME/SAMPLING_INTERVAL 
        self.number_of_acq_per_movement = self.number_of_acq_total/n_MOVEMENT_TO_UNDERSTAND
        
    def run(self):
        
        running = True
        counter_total = 0
        counter = 0
        while(running):
            self.event_run.wait()
            #print("From %s, writing to the file!"%self.name)
            self.out_file.write(str(self.data[0])) #only to avoid printing a coma
            for value in self.data[1:]:
                self.out_file.write(',')
                self.out_file.write(str(value))
                
            self.out_file.write('\n')
            
            if(counter == 0):
                print("Counter total:%d"%counter_total)
                index = int(counter_total/self.number_of_acq_per_movement)
                message = "Movement: %s"%movements[index]
                #why is this working? 
                #let's suppose: counter=0 and counter_total = 
                print("%s: %s"%(self.name,message))
                self.out_file.write(message)
                
            counter_total += 1
            counter += 1
            
            if(counter == self.number_of_acq_per_movement):
                #reset counter
                counter = 0
            
            #print("From %s, checking if set: %d!"%(self.name,self.event_close.is_set()))
            if(counter_total == self.number_of_acq_total ): #6 acquisitions, 
                
                print("From %s, closing the file!"%self.name)
                self.out_file.close()
                running = False
                
        
if __name__ == "__main__":
    
    #matrix that holds values read by 
    data = np.zeros(NUM_CHANNELS)
    
    event_run = threading.Event()
    
    try:
        
        s = SerialReader("Serial Reader",PORT,BAUD,data,event_run)    
        p = DynamicPlotter("File maker",data,event_run)
        
        s.start()
        p.start()
    
        s.join()
        p.join()
        
    except KeyboardInterrupt:
        raise ValueError('Catch command to stop from keyboard')
        sys.exit(0)
        
        
        
        
        
        
        
        
        