#include <bcm2835.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <iostream>
#include <json.hpp>
#include <iomanip>
#include <fstream>
#include <ctime>


using namespace std;
using json = nlohmann::json;

void Leer_todos_los_registros();
void Config_registros();
void Burst_mode();
void Run_DSP();
void Stop_DSP();

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

// create a JSON object global
json dataj,modificadorj;

//clase para los registros de la dsp
class Registro{
    private:
        int Direccion;
        int Len_dato;
        string Nombre;
        int Valor;
        
    public:
        Registro(string,int,int);
        Registro(){};
        string GetNombre();
        float GetValor();
        void Leer();
        void Escribir();
        void SetValor(int _Valor);
        void Inicializacion(string _Nombre,int _Direccion,int _Len_dato);
};
Registro::Registro (string _Nombre,int _Direccion, int _Len_dato){
    Nombre = _Nombre;
    Direccion = _Direccion;
    Len_dato = _Len_dato;
}
string Registro::GetNombre(){
return Nombre;
}
float Registro::GetValor(){
    return Valor;
}
void Registro::SetValor(int _Valor){
Valor=_Valor;
}
void Registro::Inicializacion(string _Nombre,int _Direccion,int _Len_dato){
    Nombre = _Nombre;
    Direccion = _Direccion;
    Len_dato = _Len_dato;
}
//Función leer
void Registro::Leer(){
    int i=0;
    char dir[2];
    char buf[Len_dato];
    //limpiar el valor del dato
    Valor=0;
    //separar la dirección del registro en dos partes para enviarla por el bus
    dir[1]=Direccion & 0xFF;
    dir[0]=Direccion>>8 & 0xFF;
    //iniciar puerto raspberry
    //bcm2835_init();
    init = I2C_BEGIN;
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
    //cout<<"Nombre del registro: "<<Nombre<<endl;
    //printf("La dirección es:0x%x%x \n", dir[0],dir[1]);
    //printf("Longitud del dato %d byte\n", Len_dato);
    //Cargar variable buf con el valor
    
    
    bcm2835_i2c_read_register_rs(dir,buf,Len_dato);
    
    
    for (i=0; i<Len_dato; i++) {
                if(buf[i] != 'n') printf("Read Buf[%d] = %x\n", i, buf[i]);
        }
    for(i=0;i<Len_dato;i++){
		Valor +=buf[i]<<8*(Len_dato-i-1); 
	}
    bcm2835_i2c_end();   
    bcm2835_close();
    //printf("Valor leido 0x%x \n", Valor);
}
//Fucion escribir
void Registro::Escribir(){
    int i;
    char wbuf[Len_dato+2];
    //limpiar buf
    for(i=0;i<Len_dato+2;i++){
	wbuf[i]=0;
    }
    //separar la dirección del registro en dos partes para enviarla por el bus
    wbuf[1]=Direccion & 0xFF;
    wbuf[0]=Direccion>>8 & 0xFF;
    for(i=0;i<Len_dato;i++){
        wbuf[Len_dato+2-i-1] = (Valor >> 8*i) & 0xFF;
    }
    /*for(i=0;i<Len_dato+2;i++){
        printf("Write wbuf[%d] = %x\n", i, wbuf[i]);
    }*/
    //iniciar puertos raspberry
    bcm2835_init();
    init = I2C_BEGIN;
    if (!bcm2835_init()){
      printf("bcm2835_init failed. Are you running as root??\n");
    }
    // I2C begin if specified    
    if (init == I2C_BEGIN){
      if (!bcm2835_i2c_begin()){
        printf("bcm2835_i2c_begin failed. Are you running as root??\n");
      }
    }
    //Cargar dirección del esclavo
    bcm2835_i2c_setSlaveAddress(slave_address);
    //Cargar clock
    bcm2835_i2c_setClockDivider(clk_div);
    cout<<"Nombre del registro: "<<Nombre<<endl;
    printf("La dirección es:0x%x \n",Direccion);
    printf("Longitud del dato %d byte\n", Len_dato);  
    bcm2835_i2c_write(wbuf,Len_dato+2);
    printf("Valor= %d\n",Valor);
    bcm2835_i2c_end();   
    bcm2835_close();
}
//Arreglo de objetos
Registro Objregistro[181];

void Config_registros(){
    //objetos generales ("Nombre",Direccíón,Lonjitud del dato,Tipo de dato)
    //Tipo de dato Normal=0, ZPSE=1, ZP=2, SE=3
    //Table 39 Registers Located in DSP Data Memory RAM
    Objregistro[181].Inicializacion("PRUEBA",0x0000,4);
    Objregistro[0].Inicializacion("AIGAIN",0X4380,4);

    Objregistro[1].Inicializacion("AVGAIN",0X4381,4);
    Objregistro[2].Inicializacion("AV2GAIN",0X4382,4);
    Objregistro[3].Inicializacion("BIGAIN",0X4383,4);
    Objregistro[4].Inicializacion("BVGAIN",0X4384,4);
    Objregistro[5].Inicializacion("BV2GAIN",0X4385,4);
    Objregistro[6].Inicializacion("CIGAIN",0X4386,4);
    Objregistro[7].Inicializacion("CVGAIN",0X4387,4);
    Objregistro[8].Inicializacion("CV2GAIN",0X4388,4);
    Objregistro[9].Inicializacion("NIGAIN",0X4389,4);
    Objregistro[10].Inicializacion("NVGAIN",0X438A,4);
    Objregistro[11].Inicializacion("NV2GAIN",0X438B,4);
    Objregistro[12].Inicializacion("AIRMSOS",0X438C,4);
    Objregistro[13].Inicializacion("AVRMSOS",0X438D,4);
    Objregistro[14].Inicializacion("AV2RMSOS",0X438E,4);
    Objregistro[15].Inicializacion("BIRMSOS",0X438F,4);
    Objregistro[16].Inicializacion("BVRMSOS",0X4390,4);
    Objregistro[17].Inicializacion("BV2RMSOS",0X4391,4);
    Objregistro[18].Inicializacion("CIRMSOS",0X4392,4);
    Objregistro[19].Inicializacion("CVRMSOS",0X4393,4);
    Objregistro[20].Inicializacion("CV2RMSOS",0X4394,4);
    Objregistro[21].Inicializacion("NIRMSOS",0X4395,4);
    Objregistro[22].Inicializacion("NVRMSOS",0X4396,4);
    Objregistro[23].Inicializacion("NV2RMSOS",0X4397,4);
    Objregistro[24].Inicializacion("ISUMLVL",0X4398,4);
    Objregistro[25].Inicializacion("APGAIN",0X4399,4);
    Objregistro[26].Inicializacion("BPGAIN",0X439A,4);
    Objregistro[27].Inicializacion("CPGAIN",0X439B,4);
    Objregistro[28].Inicializacion("AWATTOS",0X439C,4);
    Objregistro[29].Inicializacion("BWATTOS",0X439D,4);
    Objregistro[30].Inicializacion("CWATTOS",0X439E,4);
    Objregistro[31].Inicializacion("AVAROS",0X439F,4);
    Objregistro[32].Inicializacion("BVAROS",0X43A0,4);
    Objregistro[33].Inicializacion("CVAROS",0X43A1,4);
    Objregistro[34].Inicializacion("VLEVEL",0X43A2,4);
    Objregistro[35].Inicializacion("AFWATTOS",0X43A3,4);
    Objregistro[36].Inicializacion("BFWATTOS",0X43A4,4);
    Objregistro[37].Inicializacion("CFWATTOS",0X43A5,4);
    Objregistro[38].Inicializacion("AFVAROS",0X43A6,4);
    Objregistro[39].Inicializacion("BFVAROS",0X43A7,4);
    Objregistro[40].Inicializacion("CFVAROS",0X43A8,4);
    Objregistro[41].Inicializacion("AFIRMSOS",0X43A9,4);
    Objregistro[42].Inicializacion("BFIRMSOS",0X43AA,4);
    Objregistro[43].Inicializacion("CFIRMSOS",0X43AB,4);
    Objregistro[44].Inicializacion("AFVRMSOS",0X43AC,4);
    Objregistro[45].Inicializacion("BFVRMSOS",0X43AD,4);
    Objregistro[46].Inicializacion("CFVRMSOS",0X43AE,4);
    Objregistro[47].Inicializacion("TEMPCO",0X43AF,4);
    Objregistro[48].Inicializacion("ATEMPo",0X43B0,4);
    Objregistro[49].Inicializacion("BTEMPo",0X43B1,4);
    Objregistro[50].Inicializacion("CTEMPo",0X43B2,4);
    Objregistro[51].Inicializacion("NTEMPo",0X43B3,4);
    Objregistro[52].Inicializacion("ATGAIN",0X43B4,4);
    Objregistro[53].Inicializacion("BTGAIN",0X43B5,4);
    Objregistro[54].Inicializacion("CTGAIN",0X43B6,4);
    Objregistro[55].Inicializacion("NTGAIN",0X43B7,4);

    Objregistro[56].Inicializacion("AIRMS",0X43C0,4);
    Objregistro[57].Inicializacion("AVRMS",0X43C1,4);
    Objregistro[58].Inicializacion("AV2RMS",0X43C2,4);
    Objregistro[59].Inicializacion("BIRMS",0X43C3,4);
    Objregistro[60].Inicializacion("BVRMS",0X43C4,4);
    Objregistro[61].Inicializacion("BV2RMS",0X43C5,4);
    Objregistro[62].Inicializacion("CIRMS",0X43C6,4);
    Objregistro[63].Inicializacion("CVRMS",0X43C7,4);
    Objregistro[64].Inicializacion("CV2RMS",0X43C8,4);
    Objregistro[65].Inicializacion("NIRMS",0X43C9,4);
    Objregistro[66].Inicializacion("ISUM",0X43CA,4);
    Objregistro[67].Inicializacion("ATEMP",0X43CB,4);
    Objregistro[68].Inicializacion("BTEMP",0X43CC,4);
    Objregistro[69].Inicializacion("CTEMP",0X43CD,4);
    Objregistro[70].Inicializacion("NTEMP",0X43CE,4);

    //Table 40 Internal DSP Memory RAM Registers
    Objregistro[71].Inicializacion("Run",0xE228,2);

    //Table 41 Billable Registers
    Objregistro[72].Inicializacion("AWATTHR",0XE400,4);
    Objregistro[73].Inicializacion("BWATTHR",0XE401,4);
    Objregistro[74].Inicializacion("CWATTHR",0XE402,4);
    Objregistro[75].Inicializacion("AFWATTHR",0XE403,4);
    Objregistro[76].Inicializacion("BFWATTHR",0XE404,4);
    Objregistro[77].Inicializacion("CFWATTHR",0XE405,4);
    Objregistro[78].Inicializacion("AVARHR",0XE406,4);
    Objregistro[79].Inicializacion("BVARHR",0XE407,4);
    Objregistro[80].Inicializacion("CVARHR",0XE408,4);
    Objregistro[81].Inicializacion("AFVARHR",0XE409,4);
    Objregistro[82].Inicializacion("BFVARHR",0XE40A,4);
    Objregistro[83].Inicializacion("CFVARHR",0XE40B,4);
    Objregistro[84].Inicializacion("AVAHR",0XE40C,4);
    Objregistro[85].Inicializacion("BVAHR",0XE40D,4);
    Objregistro[86].Inicializacion("CVAHR",0XE40E,4);
    //Tabla 42 Configuration and Power Quality Registers

    Objregistro[87].Inicializacion("IPEAK",0XE500,4);
    Objregistro[88].Inicializacion("VPEAK",0XE501,4);
    Objregistro[89].Inicializacion("STATUS0",0XE502,4);
    Objregistro[90].Inicializacion("STATUS1",0XE503,4);

    Objregistro[91].Inicializacion("OILVL",0XE507,4);
    Objregistro[92].Inicializacion("OVLVL",0XE508,4);
    Objregistro[93].Inicializacion("SAGLVL",0XE509,4);
    Objregistro[94].Inicializacion("MASK0",0XE50A,4);
    Objregistro[95].Inicializacion("MASK1",0XE50B,4);
    Objregistro[96].Inicializacion("IAWV",0XE50C,4);
    Objregistro[97].Inicializacion("IBWV",0XE50D,4);
    Objregistro[98].Inicializacion("ICWV",0XE50E,4);
    Objregistro[99].Inicializacion("INWV",0XE50F,4);
    Objregistro[100].Inicializacion("VAWV",0XE510,4);
    Objregistro[101].Inicializacion("VBWV",0XE511,4);
    Objregistro[102].Inicializacion("VCWV",0XE512,4);
    Objregistro[103].Inicializacion("VA2WV",0XE513,4);
    Objregistro[104].Inicializacion("VB2WV",0XE514,4);
    Objregistro[105].Inicializacion("VC2WV",0XE515,4);
    Objregistro[106].Inicializacion("VNWV",0XE516,4);
    Objregistro[107].Inicializacion("VN2WV",0XE517,4);
    Objregistro[108].Inicializacion("AWATT",0XE518,4);
    Objregistro[109].Inicializacion("BWATT",0XE519,4);
    Objregistro[110].Inicializacion("CWATT",0XE51A,4);
    Objregistro[111].Inicializacion("AVAR",0XE51B,4);
    Objregistro[112].Inicializacion("BVAR",0XE51C,4);
    Objregistro[113].Inicializacion("CVAR",0XE51D,4);
    Objregistro[114].Inicializacion("AVA",0XE51E,4);
    Objregistro[115].Inicializacion("BVA",0XE51F,4);
    Objregistro[116].Inicializacion("CVA",0XE520,4);
    Objregistro[117].Inicializacion("AVTHD",0XE521,4);
    Objregistro[118].Inicializacion("AITHD",0XE522,4);
    Objregistro[119].Inicializacion("BVTHD",0XE523,4);
    Objregistro[120].Inicializacion("BITHD",0XE524,4);
    Objregistro[121].Inicializacion("CVTHD",0XE525,4);
    Objregistro[122].Inicializacion("CITHD",0XE526,4);

    Objregistro[123].Inicializacion("NVRMS",0XE530,4);
    Objregistro[124].Inicializacion("NV2RMS",0XE531,4);
    Objregistro[125].Inicializacion("CHECKSUM",0XE532,4);
    Objregistro[126].Inicializacion("VNOM",0XE533,4);

    Objregistro[127].Inicializacion("AFIRMS",0XE537,4);
    Objregistro[128].Inicializacion("AFVRMS",0XE538,4);
    Objregistro[129].Inicializacion("BFIRMS",0XE539,4);
    Objregistro[130].Inicializacion("BFVRMS",0XE53A,4);
    Objregistro[131].Inicializacion("CFIRMS",0XE53B,4);
    Objregistro[132].Inicializacion("CFVRMS",0XE53C,4);

    Objregistro[133].Inicializacion("LAST_RWDATA32",0XE5FF,4);
    Objregistro[134].Inicializacion("PHSTATUS",0XE600,2);
    Objregistro[135].Inicializacion("ANGLE0",0XE601,2);
    Objregistro[136].Inicializacion("ANGLE1",0XE602,2);
    Objregistro[137].Inicializacion("ANGLE2",0XE603,2);

    Objregistro[138].Inicializacion("PHNOLOAD",0XE608,2);

    Objregistro[139].Inicializacion("LINECYC",0XE60C,2);
    Objregistro[140].Inicializacion("ZXTOUT",0XE60D,2);
    Objregistro[141].Inicializacion("CAMPMODE",0XE60E,2);

    Objregistro[142].Inicializacion("CFMODE",0XE610,2);
    Objregistro[143].Inicializacion("CF1DEN",0XE611,2);
    Objregistro[144].Inicializacion("CF2DEN",0XE612,2);
    Objregistro[145].Inicializacion("CF3DEN",0XE613,2);
    Objregistro[146].Inicializacion("APHCAL",0XE614,2);
    Objregistro[147].Inicializacion("BPHCAL",0XE615,2);
    Objregistro[148].Inicializacion("CPHCAL",0XE616,2);
    Objregistro[149].Inicializacion("PHSING",0XE617,2);
    Objregistro[150].Inicializacion("CONFIG",0xE618,2);

    Objregistro[151].Inicializacion("MMODE",0xE700,1);
    Objregistro[152].Inicializacion("ACCMODE",0xE701,1);
    Objregistro[153].Inicializacion("LCYCMODE",0xE702,1);
    Objregistro[154].Inicializacion("PEAKCYC",0xE703,1);
    Objregistro[155].Inicializacion("SAGCYC",0xE704,1);
    Objregistro[156].Inicializacion("CFCYC",0xE705,1);
    Objregistro[157].Inicializacion("HSDC_CFG",0xE706,1);
    Objregistro[158].Inicializacion("Version",0xE707,1);
    Objregistro[159].Inicializacion("CONFIG3",0xE708,1);
    Objregistro[160].Inicializacion("ATEMPOS",0xE709,1);
    Objregistro[161].Inicializacion("BTEMPOS",0xE70A,1);
    Objregistro[162].Inicializacion("CTEMPOS",0xE70B,1);
    Objregistro[163].Inicializacion("NTEMPOS",0xE70C,1);

    Objregistro[164].Inicializacion("LAST_RWDATA8",0xE7FD,1);

    Objregistro[165].Inicializacion("APF",0xE902,2);
    Objregistro[166].Inicializacion("BPF",0xE903,2);
    Objregistro[167].Inicializacion("CPF",0xE904,2);
    Objregistro[168].Inicializacion("APERIOD",0xE905,2);
    Objregistro[169].Inicializacion("BPERIOD",0xE906,2);
    Objregistro[170].Inicializacion("CPERIOD",0xE907,2);
    Objregistro[171].Inicializacion("APNOLOAD",0xE908,2);
    Objregistro[172].Inicializacion("VARNOLOAD",0xE909,2);
    Objregistro[173].Inicializacion("VANOLOAD",0xE90A,2);

    Objregistro[174].Inicializacion("LAST_ADD",0xE9FE,2);
    Objregistro[175].Inicializacion("LAST_RWDATA16",0xE9FF,2);
    Objregistro[176].Inicializacion("CONFIG2",0xEA00,1);
    Objregistro[177].Inicializacion("LAST_OP",0xEA01,1);
    Objregistro[178].Inicializacion("WTHR",0xEA02,1);
    Objregistro[179].Inicializacion("VARTHR",0xEA03,1);
    Objregistro[180].Inicializacion("VATHR",0xEA04,1);
}
void Leer_todos_los_registros(){
    int i=0,Valorobj=0,a;
    string Nombreobj;
    //unsigned t0,t1;
    while(i<181){
/*        if(i>= 96 && i<=122){
            for(a=0;a<5;a++){
               //t0=clock();
                Objregistro[i].Leer();
                Valorobj=Objregistro[i].GetValor();
                Nombreobj=Objregistro[i].GetNombre();
                dataj["registers"][Nombreobj]={{Nombreobj,Valorobj}};
                //t1 = clock();
                //double time = (double(t1-t0)/CLOCKS_PER_SEC);
                //cout << "Execution Time: " << time << endl;  
            }
            
            i++;
        }
        else{*/
            
            for(a=0;a<10;a++){
                
                Objregistro[i].Leer();
                Valorobj=Objregistro[i].GetValor();
                Nombreobj=Objregistro[i].GetNombre();
                dataj["registers"][Nombreobj][a]=Valorobj;
            
              
            }
            i++; 
       // }
    }
}
void Burst_mode(){
    int i=0,Len_dato=108,cont=0,Corrimiento=0,Temp=0,Valorobj=0,a;
    string Nombreobj;
    char dir[2];
    char buf[Len_dato];
    //separar la dirección del registro en dos partes para enviarla por el bus
    dir[1]=0x0C;
    dir[0]=0xE5;
    //iniciar puerto raspberry
    //bcm2835_init();
    init = I2C_BEGIN;
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
    //cout<<"Nombre del registro: "<<endl;
    //printf("La dirección es:0x%x%x \n", dir[0],dir[1]);
    //printf("Longitud del dato %d byte\n", Len_dato);
    //Cargar variable buf con el valor
    bcm2835_i2c_read_register_rs(dir,buf,Len_dato);
    
    /*for (i=0; i<Len_dato; i++) {
                if(buf[i] != 'n') printf("Read Buf[%d] = %x\n", i, buf[i]);
        }*/
    
    cont=0;
    for (i=122 ; i>=96; i--){// del obj 96 hasta el 122 son los resgistros que lee la dsp
            Objregistro[i].SetValor(0);
            Temp=0;
            for(Corrimiento=0;Corrimiento<=3; Corrimiento++){
                Temp+=buf[Len_dato-cont-1] << 8*Corrimiento;
                cont++;
            }
            Objregistro[i].SetValor(Temp);
            Valorobj=Objregistro[i].GetValor();
            Nombreobj=Objregistro[i].GetNombre();
            dataj[std::to_string(Samples)][Nombreobj]=Valorobj;
    }
    
    bcm2835_i2c_end();   
    bcm2835_close();
}
void Run_DSP(){
    Objregistro[71].SetValor(0x0001);
    Objregistro[71].Escribir();
}
void Stop_DSP(){
    Objregistro[71].SetValor(0x0000);
    Objregistro[71].Escribir();
}
int main() {
    int i=0,Valorobj;
    string Nombreobj;
    //unsigned t0,t1;  
    Config_registros();
    // read a JSON file
    //std::ifstream a("/home/pi/Desktop/Programa/ade-metering-angular/src/data.json");
    //a >> dataj;
    std::ifstream b("modificador.json");
    b >> modificadorj;
    //std::ofstream o("/home/pi/Desktop/Programa/ade-metering-angular/src/data.json"); 
    //o << std::setw(4) << dataj << std::endl;
    
    std::cout << std::setw(4) << modificadorj << '\n';
    
    bcm2835_init();
    auto Write = modificadorj.find("Write");
    Run_DSP();
    //Burst_mode();
    //Stop_DSP();
    while(1){
        dataj["id"]=1;
        b.close();
        b.open("modificador.json");
        b >> modificadorj;
        Write = modificadorj.find("Write");
        while(*Write==1){
            b.close();
            //Objregistro[71].Leer();
            t0=clock();
            //Run_DSP();
            for(Samples=0;Samples<=100;Samples++){
                Burst_mode();            
            }
           
            //Stop_DSP();
            //Burst_mode();
            //Leer_todos_los_registros();
            b.open("modificador.json");
            b >> modificadorj;
            Write = modificadorj.find("Write");
            //std::cout << std::setw(4) << modificadorj << '\n';
            remove("/home/pi/Desktop/Programa/read-send-json/db.json");
            std::ofstream o("/home/pi/Desktop/Programa/read-send-json/db.json"); 
            o << std::setw(4) << dataj << std::endl;
            t1=clock();
            double time = (double(t1-t0)/CLOCKS_PER_SEC);
            //dataj[std::to_string(Samples_)][std::to_string(time)]=time;
            cout << "Execution Time: " << time << endl;
            
        }
        Stop_DSP();
        sleep(1);
    }
    
    
    // write prettified JSON to another file
    
    //std::cout << std::setw(4) << dataj << '\n';
    
    /*auto CONFIG = je.find("CONFIG");
    std::cout << "\"CONFIG\" was found: " << (CONFIG != je.end()) << '\n';
    std::cout << "value at key \"CONFIG\": " << *CONFIG << '\n';
    */
    bcm2835_close();
    return 0;
}
