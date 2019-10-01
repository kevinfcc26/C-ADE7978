#include <bcm2835.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <iostream>

using namespace std;
typedef enum {
    NO_ACTION,
    I2C_BEGIN,
    I2C_END
} i2c_init;

//configuracion del clk y la direcciones del dispositivo esclavo
uint8_t  init = NO_ACTION;
uint16_t clk_div = BCM2835_I2C_CLOCK_DIVIDER_626;
uint8_t slave_address = 0x38;
unsigned t0,t1;
int Samples;
int main(){
    
    int i,Len_dato=6,Valor;
    char dir[2];
    char buf[Len_dato];
    //limpiar el valor del dato
    //separar la dirección del registro en dos partes para enviarla por el bus
    dir[1]=0x28;
    dir[0]=0xE2;
    //iniciar puerto raspberry
    bcm2835_init();
    init = I2C_BEGIN;
    bcm2835_i2c_begin();
    if (!bcm2835_init())
    {
      printf("bcm2835_init failed. Are you running as root??\n");
    }
    // I2C begin if specified    
    if (init == I2C_BEGIN)
    {
      if (!bcm2835_i2c_begin())
      {
        printf("bcm2835_i2c_begin failed. Are you running as root??\n");
      }
    }
    //Cargar dirección del esclavo
    bcm2835_i2c_setSlaveAddress(slave_address);
    //Cargar la frecuencia del clock
    bcm2835_i2c_setClockDivider(clk_div);
 //   cout<<"Nombre del registro: "<<Nombre<<endl;
    //printf("La dirección es:0x%x%x \n", dir[0],dir[1]);
    printf("Longitud del dato %d byte\n", Len_dato);
    //Cargar variable buf con el valor
    bcm2835_i2c_read_register_rs(dir,buf,Len_dato);
    
    for (i=0; i<Len_dato; i++) {
                if(buf[i] != 'n') printf("Read Buf[%d] = %x\n", i, buf[i]);
        }
    for(i=0;i<Len_dato;i++){
		Valor +=buf[i]<<8*(Len_dato-i-1); 
	}
    printf("Valor leido 0x%x \n", Valor);
}
