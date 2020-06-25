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
#include <time.h>
#include <math.h>
#include <mysql.h>

#define IRQ1_N 11
#define IRQ0_N 4
#define SERVER "us-cdbr-east-05.cleardb.net"
#define USER "b62ba68668ea1c"
#define PASSWORD "4d0579fe"
#define DATABASE "heroku_851e4397b87123b"

using namespace std;

using json = nlohmann::json; // Objeto para manejar los datos en formato Json

//  Funciones globales

void Read_all_registers();       //Función para leer todos los registros
void Config_registers();         //Función para declarar todos los parametros de los registros tomados como objetos
void Burst_mode(int Samples);    //Función para leer todos los registros de energia
void Run_DSP();                  //Pone en marcha la dsp
void Stop_DSP();                 // Apaga la dsp
void Initializing_the_chipset(); // Configura la dsp segun los pasos escritos en el datasheet
void Reset();
void Read_registers(int Sample); // Reinicia toda la tarjeta para configurarla nuevamente
void SetJsonCurrent(int Registro, int Sample);
void SetJsonVol(int Registro, int Sample);

typedef enum
{
    NO_ACTION,
    I2C_BEGIN,
    I2C_END
} i2c_init;

//configuracion del clk y la direcciones del dispositivo esclavo
uint8_t init = NO_ACTION;
uint16_t clk_div = BCM2835_I2C_CLOCK_DIVIDER_626; // Clock establecido a 399.3610 kHz
uint8_t slave_address = 0x38;                     // Dirección de la tarjeta para el puerto I2C
float averageV = 0,averageI = 0,sumV = 0,sumI = 0; 


unsigned t0, t1;
// int Samples;

// create a JSON object global
json dataj, modificadorj;

//clase para los registros de la dsp
class Registro
{
    private:
        int Adress;     //Dirección en exadecimal del registro
        int Len_dato;   // Longitud del dato esperado
        string Name;    // Nombre del registro
        int Value;      // Valor escrito o leido del registro
        float ConValue; // valor convertido

    public:
        Registro(string, int, int); //1 contructor
        Registro(){};               //2 constructor

        string GetName();           //Función para optener el nombre
        int GetValue();
        float GetConValue();       //Función para optener el valor
        void Read();               //Funciónn para optener los datos de la dsp
        void Write();              //Función para escribir en la dsp
        void SetValue(int _Value); //Función para cargar un valor en la variable Valor del objeto
        void SetConValue(float _ConValue);
        void Config_Obj(string _Name, int _Adress, int _Len_dato); //Funcion para modificar los datos de los objetos
};

// 1 contructor de objetos
Registro::Registro(string _Name, int _Adress, int _Len_dato)
{
    Name = _Name;
    Adress = _Adress;
    Len_dato = _Len_dato;
}
// Obtiene el nombre del objeto privado
string Registro::GetName()
{
    return Name;
}
// Obtiene el valor del objeto privado
int Registro::GetValue()
{
    return Value;
}
//
float Registro::GetConValue()
{
    return ConValue;
}
// Modifica el valor del objeto
void Registro::SetValue(int _Value)
{
    Value = _Value;
}
void Registro::SetConValue(float _ConValue)
{
    ConValue = _ConValue;
}
//Carga todos los valores del objeto
void Registro::Config_Obj(string _Name, int _Adress, int _Len_dato)
{
    Name = _Name;
    Adress = _Adress;
    Len_dato = _Len_dato;
}
//Función Read
void Registro::Read()
{
    int i = 0;
    char dir[2];        // variable para separar la dirección en dos partes de 8 bits
    char buf[Len_dato]; // Tamaño del dato esperado
    //limpiar el Value del dato
    Value = 0;
    //separar la dirección del registro en dos partes para enviarla por el bus
    dir[1] = Adress & 0xFF;
    dir[0] = Adress >> 8 & 0xFF;
    //iniciar puerto raspberry
    //bcm2835_init();
    init = I2C_BEGIN;
    if (!bcm2835_init())
    {
        printf("Falló bcm2835_init, corra el programa con permisos de administrador\n"); //("bcm2835_init failed. Are you running as root??\n");
    }
    // I2C begin if specified
    if (init == I2C_BEGIN)
    {
        if (!bcm2835_i2c_begin())
        {
            printf("Falló bcm2835_begin, corra el programa con permisos de administrador\n"); //("bcm2835_i2c_begin failed. Are you running as root??\n");
        }
    }
    //Cargar dirección del esclavo
    bcm2835_i2c_setSlaveAddress(slave_address);
    //Cargar la frecuencia del clock
    bcm2835_i2c_setClockDivider(clk_div);

    //Cargar variable buf con el Value
    bcm2835_i2c_read_register_rs(dir, buf, Len_dato);
    //se carga el valor uniendo los dos bytes reibidos
    for (i = 0; i < Len_dato; i++)
    {
        Value += buf[i] << 8 * (Len_dato - i - 1);
    }
    //termina la comunicación para no sobresaturar el puerto
    bcm2835_i2c_end();
    bcm2835_close();
}

//Fucion Write
void Registro::Write()
{
    int i;
    char wbuf[Len_dato + 2]; //crear el bus con el ancho del dato enviado mas dos bytes de la dirección del registro
    //limpiar buf
    for (i = 0; i < Len_dato + 2; i++)
    {
        wbuf[i] = 0;
    }
    //separar la dirección del registro en dos partes para enviarla por el bus
    wbuf[1] = Adress & 0xFF;
    wbuf[0] = Adress >> 8 & 0xFF;
    // Se arma el dato que va a enviar con la direccion del registro
    for (i = 0; i < Len_dato; i++)
    {
        wbuf[Len_dato + 2 - i - 1] = (Value >> 8 * i) & 0xFF;
    }
    //iniciar puertos raspberry
    //bcm2835_init();
    init = I2C_BEGIN;
    if (!bcm2835_init())
    {
        printf("Falló bcm2835_init, corra el programa con permisos de administrador\n"); //("bcm2835_init failed. Are you running as root??\n");
    }
    // I2C begin if specified
    if (init == I2C_BEGIN)
    {
        if (!bcm2835_i2c_begin())
        {
            printf("Falló bcm2835_begin, corra el programa con permisos de administrador\n"); //("bcm2835_i2c_begin failed. Are you running as root??\n");
        }
    }
    //Cargar dirección del esclavo
    bcm2835_i2c_setSlaveAddress(slave_address);
    //Cargar clock
    bcm2835_i2c_setClockDivider(clk_div);

    bcm2835_i2c_write(wbuf, Len_dato + 2);

    //Terminar la comunicación para que el bus no se sature
    bcm2835_i2c_end();
    bcm2835_close();
}

class RegisterCal
{
    private:
        string Name;
        float Value;
    public:
        RegisterCal(){};

        void conf(string name);
        void set(float value);
        float get();
        string getName();
};
void RegisterCal::conf(string name){
    Name = name;
}
float RegisterCal:: get(){
    return  Value;
}
void RegisterCal::set(float value){
    Value = value;
}
string RegisterCal::getName(){
    return Name;
}
//Arreglo de objetos
// Primero se crea el arreglo de objetos y despues se le pasan los valores
Registro Objregister[181];
RegisterCal RCal[35];
//Configurar Registros como objetos pasando ("Name",Adress,tamaño del dato en la memoria de la dsp en byte)
void Config_registers()
{
    //Objetos generales ("Name",Direccíón,Lonjitud del dato,Tipo de dato)
    //Tipo de dato Normal=0, ZPSE=1, ZP=2, SE=3

    //Table 39 Registers Located in DSP Data Memory RAM
    Objregister[0].Config_Obj("AIGAIN", 0X4380, 4);

    Objregister[1].Config_Obj("AVGAIN", 0X4381, 4);
    Objregister[2].Config_Obj("AV2GAIN", 0X4382, 4);
    Objregister[3].Config_Obj("BIGAIN", 0X4383, 4);
    Objregister[4].Config_Obj("BVGAIN", 0X4384, 4);
    Objregister[5].Config_Obj("BV2GAIN", 0X4385, 4);
    Objregister[6].Config_Obj("CIGAIN", 0X4386, 4);
    Objregister[7].Config_Obj("CVGAIN", 0X4387, 4);
    Objregister[8].Config_Obj("CV2GAIN", 0X4388, 4);
    Objregister[9].Config_Obj("NIGAIN", 0X4389, 4);
    Objregister[10].Config_Obj("NVGAIN", 0X438A, 4);
    Objregister[11].Config_Obj("NV2GAIN", 0X438B, 4);
    Objregister[12].Config_Obj("AIRMSOS", 0X438C, 4);
    Objregister[13].Config_Obj("AVRMSOS", 0X438D, 4);
    Objregister[14].Config_Obj("AV2RMSOS", 0X438E, 4);
    Objregister[15].Config_Obj("BIRMSOS", 0X438F, 4);
    Objregister[16].Config_Obj("BVRMSOS", 0X4390, 4);
    Objregister[17].Config_Obj("BV2RMSOS", 0X4391, 4);
    Objregister[18].Config_Obj("CIRMSOS", 0X4392, 4);
    Objregister[19].Config_Obj("CVRMSOS", 0X4393, 4);
    Objregister[20].Config_Obj("CV2RMSOS", 0X4394, 4);
    Objregister[21].Config_Obj("NIRMSOS", 0X4395, 4);
    Objregister[22].Config_Obj("NVRMSOS", 0X4396, 4);
    Objregister[23].Config_Obj("NV2RMSOS", 0X4397, 4);
    Objregister[24].Config_Obj("ISUMLVL", 0X4398, 4);
    Objregister[25].Config_Obj("APGAIN", 0X4399, 4);
    Objregister[26].Config_Obj("BPGAIN", 0X439A, 4);
    Objregister[27].Config_Obj("CPGAIN", 0X439B, 4);
    Objregister[28].Config_Obj("AWATTOS", 0X439C, 4);
    Objregister[29].Config_Obj("BWATTOS", 0X439D, 4);
    Objregister[30].Config_Obj("CWATTOS", 0X439E, 4);
    Objregister[31].Config_Obj("AVAROS", 0X439F, 4);
    Objregister[32].Config_Obj("BVAROS", 0X43A0, 4);
    Objregister[33].Config_Obj("CVAROS", 0X43A1, 4);
    Objregister[34].Config_Obj("VLEVEL", 0X43A2, 4);
    Objregister[35].Config_Obj("AFWATTOS", 0X43A3, 4);
    Objregister[36].Config_Obj("BFWATTOS", 0X43A4, 4);
    Objregister[37].Config_Obj("CFWATTOS", 0X43A5, 4);
    Objregister[38].Config_Obj("AFVAROS", 0X43A6, 4);
    Objregister[39].Config_Obj("BFVAROS", 0X43A7, 4);
    Objregister[40].Config_Obj("CFVAROS", 0X43A8, 4);
    Objregister[41].Config_Obj("AFIRMSOS", 0X43A9, 4);
    Objregister[42].Config_Obj("BFIRMSOS", 0X43AA, 4);
    Objregister[43].Config_Obj("CFIRMSOS", 0X43AB, 4);
    Objregister[44].Config_Obj("AFVRMSOS", 0X43AC, 4);
    Objregister[45].Config_Obj("BFVRMSOS", 0X43AD, 4);
    Objregister[46].Config_Obj("CFVRMSOS", 0X43AE, 4);
    Objregister[47].Config_Obj("TEMPCO", 0X43AF, 4);
    Objregister[48].Config_Obj("ATEMPo", 0X43B0, 4);
    Objregister[49].Config_Obj("BTEMPo", 0X43B1, 4);
    Objregister[50].Config_Obj("CTEMPo", 0X43B2, 4);
    Objregister[51].Config_Obj("NTEMPo", 0X43B3, 4);
    Objregister[52].Config_Obj("ATGAIN", 0X43B4, 4);
    Objregister[53].Config_Obj("BTGAIN", 0X43B5, 4);
    Objregister[54].Config_Obj("CTGAIN", 0X43B6, 4);
    Objregister[55].Config_Obj("NTGAIN", 0X43B7, 4);

    Objregister[56].Config_Obj("AIRMS", 0X43C0, 4);
    Objregister[57].Config_Obj("AVRMS", 0X43C1, 4);
    Objregister[58].Config_Obj("AV2RMS", 0X43C2, 4);
    Objregister[59].Config_Obj("BIRMS", 0X43C3, 4);
    Objregister[60].Config_Obj("BVRMS", 0X43C4, 4);
    Objregister[61].Config_Obj("BV2RMS", 0X43C5, 4);
    Objregister[62].Config_Obj("CIRMS", 0X43C6, 4);
    Objregister[63].Config_Obj("CVRMS", 0X43C7, 4);
    Objregister[64].Config_Obj("CV2RMS", 0X43C8, 4);
    Objregister[65].Config_Obj("NIRMS", 0X43C9, 4);
    Objregister[66].Config_Obj("ISUM", 0X43CA, 4);
    Objregister[67].Config_Obj("ATEMP", 0X43CB, 4);
    Objregister[68].Config_Obj("BTEMP", 0X43CC, 4);
    Objregister[69].Config_Obj("CTEMP", 0X43CD, 4);
    Objregister[70].Config_Obj("NTEMP", 0X43CE, 4);

    //Table 40 Internal DSP Memory RAM Registers
    Objregister[71].Config_Obj("Run", 0xE228, 2);

    //Table 41 Billable Registers
    Objregister[72].Config_Obj("AWATTHR", 0XE400, 4);
    Objregister[73].Config_Obj("BWATTHR", 0XE401, 4);
    Objregister[74].Config_Obj("CWATTHR", 0XE402, 4);
    Objregister[75].Config_Obj("AFWATTHR", 0XE403, 4);
    Objregister[76].Config_Obj("BFWATTHR", 0XE404, 4);
    Objregister[77].Config_Obj("CFWATTHR", 0XE405, 4);
    Objregister[78].Config_Obj("AVARHR", 0XE406, 4);
    Objregister[79].Config_Obj("BVARHR", 0XE407, 4);
    Objregister[80].Config_Obj("CVARHR", 0XE408, 4);
    Objregister[81].Config_Obj("AFVARHR", 0XE409, 4);
    Objregister[82].Config_Obj("BFVARHR", 0XE40A, 4);
    Objregister[83].Config_Obj("CFVARHR", 0XE40B, 4);
    Objregister[84].Config_Obj("AVAHR", 0XE40C, 4);
    Objregister[85].Config_Obj("BVAHR", 0XE40D, 4);
    Objregister[86].Config_Obj("CVAHR", 0XE40E, 4);

    //Tabla 42 Configuration and Power Quality Registers
    Objregister[87].Config_Obj("IPEAK", 0XE500, 4);
    Objregister[88].Config_Obj("VPEAK", 0XE501, 4);
    Objregister[89].Config_Obj("STATUS0", 0XE502, 4);
    Objregister[90].Config_Obj("STATUS1", 0XE503, 4);

    Objregister[91].Config_Obj("OILVL", 0XE507, 4);
    Objregister[92].Config_Obj("OVLVL", 0XE508, 4);
    Objregister[93].Config_Obj("SAGLVL", 0XE509, 4);
    Objregister[94].Config_Obj("MASK0", 0XE50A, 4);
    Objregister[95].Config_Obj("MASK1", 0XE50B, 4);
    Objregister[96].Config_Obj("IAWV", 0XE50C, 4);
    Objregister[97].Config_Obj("IBWV", 0XE50D, 4);
    Objregister[98].Config_Obj("ICWV", 0XE50E, 4);
    Objregister[99].Config_Obj("INWV", 0XE50F, 4);
    Objregister[100].Config_Obj("VAWV", 0XE510, 4);
    Objregister[101].Config_Obj("VBWV", 0XE511, 4);
    Objregister[102].Config_Obj("VCWV", 0XE512, 4);
    Objregister[103].Config_Obj("VA2WV", 0XE513, 4);
    Objregister[104].Config_Obj("VB2WV", 0XE514, 4);
    Objregister[105].Config_Obj("VC2WV", 0XE515, 4);
    Objregister[106].Config_Obj("VNWV", 0XE516, 4);
    Objregister[107].Config_Obj("VN2WV", 0XE517, 4);
    Objregister[108].Config_Obj("AWATT", 0XE518, 4);
    Objregister[109].Config_Obj("BWATT", 0XE519, 4);
    Objregister[110].Config_Obj("CWATT", 0XE51A, 4);
    Objregister[111].Config_Obj("AVAR", 0XE51B, 4);
    Objregister[112].Config_Obj("BVAR", 0XE51C, 4);
    Objregister[113].Config_Obj("CVAR", 0XE51D, 4);
    Objregister[114].Config_Obj("AVA", 0XE51E, 4);
    Objregister[115].Config_Obj("BVA", 0XE51F, 4);
    Objregister[116].Config_Obj("CVA", 0XE520, 4);
    Objregister[117].Config_Obj("AVTHD", 0XE521, 4);
    Objregister[118].Config_Obj("AITHD", 0XE522, 4);
    Objregister[119].Config_Obj("BVTHD", 0XE523, 4);
    Objregister[120].Config_Obj("BITHD", 0XE524, 4);
    Objregister[121].Config_Obj("CVTHD", 0XE525, 4);
    Objregister[122].Config_Obj("CITHD", 0XE526, 4);

    Objregister[123].Config_Obj("NVRMS", 0XE530, 4);
    Objregister[124].Config_Obj("NV2RMS", 0XE531, 4);
    Objregister[125].Config_Obj("CHECKSUM", 0XE532, 4);
    Objregister[126].Config_Obj("VNOM", 0XE533, 4);

    Objregister[127].Config_Obj("AFIRMS", 0XE537, 4);
    Objregister[128].Config_Obj("AFVRMS", 0XE538, 4);
    Objregister[129].Config_Obj("BFIRMS", 0XE539, 4);
    Objregister[130].Config_Obj("BFVRMS", 0XE53A, 4);
    Objregister[131].Config_Obj("CFIRMS", 0XE53B, 4);
    Objregister[132].Config_Obj("CFVRMS", 0XE53C, 4);

    Objregister[133].Config_Obj("LAST_RWDATA32", 0XE5FF, 4);
    Objregister[134].Config_Obj("PHSTATUS", 0XE600, 2);
    Objregister[135].Config_Obj("ANGLE0", 0XE601, 2);
    Objregister[136].Config_Obj("ANGLE1", 0XE602, 2);
    Objregister[137].Config_Obj("ANGLE2", 0XE603, 2);

    Objregister[138].Config_Obj("PHNOLOAD", 0XE608, 2);

    Objregister[139].Config_Obj("LINECYC", 0XE60C, 2);
    Objregister[140].Config_Obj("ZXTOUT", 0XE60D, 2);
    Objregister[141].Config_Obj("COMPMODE", 0XE60E, 2);

    Objregister[142].Config_Obj("CFMODE", 0XE610, 2);
    Objregister[143].Config_Obj("CF1DEN", 0XE611, 2);
    Objregister[144].Config_Obj("CF2DEN", 0XE612, 2);
    Objregister[145].Config_Obj("CF3DEN", 0XE613, 2);
    Objregister[146].Config_Obj("APHCAL", 0XE614, 2);
    Objregister[147].Config_Obj("BPHCAL", 0XE615, 2);
    Objregister[148].Config_Obj("CPHCAL", 0XE616, 2);
    Objregister[149].Config_Obj("PHSING", 0XE617, 2);
    Objregister[150].Config_Obj("CONFIG", 0xE618, 2);

    Objregister[151].Config_Obj("MMODE", 0xE700, 1);
    Objregister[152].Config_Obj("ACCMODE", 0xE701, 1);
    Objregister[153].Config_Obj("LCYCMODE", 0xE702, 1);
    Objregister[154].Config_Obj("PEAKCYC", 0xE703, 1);
    Objregister[155].Config_Obj("SAGCYC", 0xE704, 1);
    Objregister[156].Config_Obj("CFCYC", 0xE705, 1);
    Objregister[157].Config_Obj("HSDC_CFG", 0xE706, 1);
    Objregister[158].Config_Obj("Version", 0xE707, 1);
    Objregister[159].Config_Obj("CONFIG3", 0xE708, 1);
    Objregister[160].Config_Obj("ATEMPOS", 0xE709, 1);
    Objregister[161].Config_Obj("BTEMPOS", 0xE70A, 1);
    Objregister[162].Config_Obj("CTEMPOS", 0xE70B, 1);
    Objregister[163].Config_Obj("NTEMPOS", 0xE70C, 1);

    Objregister[164].Config_Obj("LAST_RWDATA8", 0xE7FD, 1);

    Objregister[165].Config_Obj("APF", 0xE902, 2);
    Objregister[166].Config_Obj("BPF", 0xE903, 2);
    Objregister[167].Config_Obj("CPF", 0xE904, 2);
    Objregister[168].Config_Obj("APERIOD", 0xE905, 2);
    Objregister[169].Config_Obj("BPERIOD", 0xE906, 2);
    Objregister[170].Config_Obj("CPERIOD", 0xE907, 2);
    Objregister[171].Config_Obj("APNOLOAD", 0xE908, 2);
    Objregister[172].Config_Obj("VARNOLOAD", 0xE909, 2);
    Objregister[173].Config_Obj("VANOLOAD", 0xE90A, 2);

    Objregister[174].Config_Obj("LAST_ADD", 0xE9FE, 2);
    Objregister[175].Config_Obj("LAST_RWDATA16", 0xE9FF, 2);
    Objregister[176].Config_Obj("CONFIG2", 0xEA00, 1);
    Objregister[177].Config_Obj("LAST_OP", 0xEA01, 1);
    Objregister[178].Config_Obj("WTHR", 0xEA02, 1);
    Objregister[179].Config_Obj("VARTHR", 0xEA03, 1);
    Objregister[180].Config_Obj("VATHR", 0xEA04, 1);

}
void ConfigRCal(){
    RCal[0].conf("AIHRMS_CAL");
    RCal[1].conf("AVHRMS_CAL");
    RCal[2].conf("APF1_CAL");
    RCal[3].conf("AP1_CAL");
    RCal[4].conf("APH_CAL");
    RCal[5].conf("ASH_CAL");
    RCal[6].conf("AS1_CAL");
    RCal[7].conf("ASN_CAL");
    RCal[8].conf("ADI_CAL");
    RCal[9].conf("ADV_CAL");
    RCal[10].conf("ADH_CAL");
    RCal[11].conf("AN_CAL");
    RCal[12].conf("BIHRMS_CAL");
    RCal[13].conf("BVHRMS_CAL");
    RCal[14].conf("BPF1_CAL");
    RCal[15].conf("BP1_CAL");
    RCal[16].conf("BPH_CAL");
    RCal[17].conf("BSH_CAL");
    RCal[18].conf("BS1_CAL");
    RCal[19].conf("BSN_CAL");
    RCal[20].conf("BDI_CAL");
    RCal[21].conf("BDV_CAL");
    RCal[22].conf("BDH_CAL");
    RCal[23].conf("BN_CAL");
    RCal[24].conf("CIHRMS_CAL");
    RCal[25].conf("CVHRMS_CAL");
    RCal[26].conf("CPF1_CAL");
    RCal[27].conf("CP1_CAL");
    RCal[28].conf("CPH_CAL");
    RCal[29].conf("CSH_CAL");
    RCal[30].conf("CS1_CAL");
    RCal[31].conf("CSN_CAL");
    RCal[32].conf("CDI_CAL");
    RCal[33].conf("CDV_CAL");
    RCal[34].conf("CDH_CAL");
    RCal[35].conf("CN_CAL");
}
//Función para leer todos los registros de una sola vez
void Read_all_registers()
{
    int i = 0, Valueobj = 0, a, Temp = 0;
    string Nameobj;
    while (i < 181)
    {
        //leer todos los registros y cargarlos en el Json
        if (i == 56 || i == 59 || i == 62 || i == 65 || i == 66)
        {
            Objregister[i].Read();
            Temp = Objregister[i].GetValue();
            Valueobj = (Temp * (0.03125 / 5320000) - (2.8 * pow(10, -4))) / (1.36 * pow(10, -3));
            Objregister[i].SetConValue(Valueobj);
            Nameobj = Objregister[i].GetName();
            dataj["registers"][Nameobj] = Valueobj;
            cout << Nameobj;
            cout << Valueobj << endl;
            printf(" = %f\n", Valueobj);
        }
        else
        {
            Objregister[i].Read();
            Valueobj = Objregister[i].GetValue();
            Nameobj = Objregister[i].GetName();
            dataj["registers"][Nameobj] = Valueobj;
            cout << Nameobj;
            cout << Valueobj << endl;
            // printf(" = %x\n", Valueobj);
        }
        i++;
    }
}
//Función para leer los registros de  0xE50C a 0xE526 en secuencia una tras otro sin necesidad de hacer varios llamados
void Burst_mode(int Samples)
{
    int i = 0, Len_dato = 108, cont = 0, Corrimiento = 0, Temp = 0, Valueobj = 0, a;
    string Nameobj;
    char dir[2];        //Variable para separar los datos en dos bloques de 8 bits
    char buf[Len_dato]; //Longitud del dato a recibir
    //separar la dirección del registro en dos partes para enviarla por el bus
    dir[1] = 0x0C;
    dir[0] = 0xE5;
    //iniciar puerto raspberry
    //bcm2835_init();
    init = I2C_BEGIN;
    if (!bcm2835_init())
    {
        printf("Falló bcm2835_init, corra el programa con permisos de administrador\n"); //("bcm2835_init failed. Are you running as root??\n");
    }
    // I2C begin if specified
    if (init == I2C_BEGIN)
    {
        if (!bcm2835_i2c_begin())
        {
            printf("Falló bcm2835_begin, corra el programa con permisos de administrador\n"); //("bcm2835_i2c_begin failed. Are you running as root??\n");
        }
    }
    //Cargar dirección del esclavo
    bcm2835_i2c_setSlaveAddress(slave_address);
    //Cargar la frecuencia del clock
    bcm2835_i2c_setClockDivider(clk_div);
    //Cargar variable buf con el Value
    bcm2835_i2c_read_register_rs(dir, buf, Len_dato);

    cont = 0;
    for (i = 122; i >= 96; i--)
    {                               // del obj 96 hasta el 122 son los resgistros que lee la dsp
        Objregister[i].SetValue(0); // Limpiar valor
        Temp = 0;                   //variable temporal par armar el dato de cada registro
        // Separación del valor del registro en el bus recibido
        for (Corrimiento = 0; Corrimiento <= 3; Corrimiento++)
        {
            Temp += buf[Len_dato - cont - 1] << 8 * Corrimiento;
            cont++;
        }
        // Carga del valor en recibido en el objeto
        Objregister[i].SetValue(Temp);
        // TODO extracción del valor del objeto para guardarlo en el Json
        // Valueobj = Objregister[i].GetValue();
        // Nameobj = Objregister[i].GetName();
        // dataj[std::to_string(Samples)][Nameobj] = Valueobj;
    }
    // Se termina la comunicación para no saturar el bus
    bcm2835_i2c_end();
    // bcm2835_close();
}
//Función para iniciar DSP
void Run_DSP()
{
    Objregister[71].SetValue(0x0001);
    Objregister[71].Write();
}
//Funcion para parar DSP
void Stop_DSP()
{
    Objregister[71].SetValue(0x0000);
    Objregister[71].Write();
}
//Reset
void Reset()
{
    int Value;
    string Name;

    // Configurar pines de entrada
    bcm2835_gpio_fsel(IRQ1_N, BCM2835_GPIO_FSEL_INPT);
    bcm2835_gpio_fsel(IRQ0_N, BCM2835_GPIO_FSEL_INPT);

    //iniciar el reinicio de la tarjeta
    Objregister[150].SetValue(0x80);
    Objregister[150].Write();

    printf("Reiniciando...\n");

    // Leer los puertos y el registro RSTDONE
    // while(bcm2835_gpio_lev(IRQ1_N)){
    // printf("El pin IRQ1 esta en 1\n");
    // sleep(100);
    // }
    // printf("El pin IRQ1_N se encuentra en 0\n");

    //Esperar que el bit RSTDONE del registro STATUS1 se encuentre en 1
    do
    {
        sleep(5);
        printf("El bit RSTDONE se encuenta en 0\n ");
        Objregister[90].Read();
        Value = Objregister[90].GetValue();

    } while (!((Value & 0x8000) == 0x8000));
    printf("El bit RSTDONE del registro STATUS1 se encuentra en 1\n");

    //Escribir un 1 en el bit RSTDONE en el registro STATUS1
    printf("Escribiendo un 1 en el bit RSTDONE para reiniciar IRQ1_N\n");
    Objregister[90].SetValue(0x8000);
    Objregister[90].Write();
    printf("****Reinicio finalizado****\n");

    // return 1;
}


// Inicializar el chip con los pasos descritos por el fabricante
void Initializing_the_chipset()
{
    int Value, i;
    string Name;

    //Configurar pines de entrada
    bcm2835_gpio_fsel(IRQ1_N, BCM2835_GPIO_FSEL_INPT);
    bcm2835_gpio_fsel(IRQ0_N, BCM2835_GPIO_FSEL_INPT);

    //Comprobar que el pin IRQ1 este en 0
    while (bcm2835_gpio_lev(IRQ1_N))
    {

        printf("El pin IRQ1 esta en 1\n");
    }
    printf("El pin IRQ1_N se encuentra en 0\n");
    //Leer el registro STATUS y comrobar que el bit 15(RSTDONE) esta en 1
    Objregister[90].Read();
    Value = Objregister[90].GetValue();
    Name = Objregister[90].GetName();
    cout << "el nombre del resgitro es " << Name << "\n";
    printf("Value= %x\n", Value);
    if ((Value & 0x8000) != 0x8000)
    {
        printf("RSTDONE se encuentra en 0\n");
        // return ; // retorna el 0 si ocurre un error en la configuración
    }
    printf("RSTDONE se encuentra en 1\n Cargando 1 en todas las banderas de STATUS0 y STATUS1...\n");
    Objregister[89].SetValue(0x7FFFF);
    Objregister[89].Write();
    Objregister[90].SetValue(0x3FFFFFF);
    Objregister[90].Write();
    printf("Done\n");
    //Escribir un 1 en el bit 0 del resgitro CONFIG2
    printf("Bloqueando comunicacion I2C\n");
    Objregister[176].SetValue(0x1);
    Objregister[176].Write();
    printf("Done\n");
    //Escribir 0 en los registros (0x4380,0x4383,0x4386,0x4389)
    printf("Escribiendo 0 es los registros (0x4380,0x4383,0x4386,0x4389)...\n");
    for (i = 0; i < 10; i += 3)
    {
        Objregister[i].SetValue(0x0);
        Objregister[i].Write();
    }
    printf("Done\n");
    //Escribir un 1 en el registro RUN(0xE228)
    printf("Iniciando DSP\n");
    Run_DSP();
    printf("Donde\n");
    //Inicializar todos los registros desde (0x4380 to 0x43BF) y escribir el ultimo 3 veces
    printf("Inicializando todos los resgistros desde (0x4380 to 0x43BF)\n");
    for (i = 0; i <= 55; i++)
    {
        Objregister[i].SetValue(0x0);
        Objregister[i].Write();
        if (i == 55)
        {
            Objregister[i].Write();
            Objregister[i].Write();
        }
    }
    printf("Done\n");
    //inicializar los registros de configuracion basados en hardware localizacon desde (0xE507 a 0x5A04)
    printf("Iniciando todos los registros de configuracioin de hardware\n");
    //OILVL
    Objregister[91].SetValue(0xFFFFFF);
    //OVLVL
    Objregister[92].SetValue(0xFFFFFF);
    //SAGLVL
    Objregister[93].SetValue(0x0);
    //MASKO
    Objregister[94].SetValue(0x20000);
    //MASK1
    Objregister[95].SetValue(0x0);

    for (i = 91; i <= 95; i++)
    {
        Objregister[i].Write();
    }
    //VNOM
    Objregister[126].SetValue(0x0);
    Objregister[126].Write();
    //LINECYC
    Objregister[139].SetValue(0xFFFF);
    //ZXOUT
    Objregister[140].SetValue(0xFFFF);
    //COMPMODE
    Objregister[141].SetValue(0x4000);
    for (i = 139; i <= 141; i++)
    {
        Objregister[i].Write();
    }
    //CF1DEN
    Objregister[143].SetValue(0x0);
    //CF2DEN
    Objregister[144].SetValue(0x0);
    //CF3DEN
    Objregister[145].SetValue(0x0);
    //APHCAL
    Objregister[146].SetValue(0x0);
    //BPHCAL
    Objregister[147].SetValue(0x0);
    //CPHCAL
    Objregister[148].SetValue(0x0);
    for (i = 143; i <= 148; i++)
    {
        Objregister[i].Write();
    }
    //PHSING
    //Objregister[149].SetValue(0x0);
    //HSDC_CFG bus de alta velocidad
    Objregister[157].SetValue(0x1);
    Objregister[157].Write();
    //CONFIG
    Objregister[150].SetValue(0x0050); //activar el puerto HSCD
    Objregister[150].Write();
    //MMODE
    Objregister[151].SetValue(0x1C);
    //ACCMODE
    Objregister[152].SetValue(0x80);
    //LCYCMODE
    Objregister[153].SetValue(0x78);
    //PEAKCYC
    Objregister[154].SetValue(0x0);
    //SAGCYC
    Objregister[155].SetValue(0x0);
    //CFCYC
    Objregister[156].SetValue(0x01);

    for (i = 151; i <= 156; i++)
    {
        Objregister[i].Write();
    }
    //CONFIG3
    Objregister[159].SetValue(0xF);
    Objregister[159].Write();
    //APNOLOAD
    Objregister[171].SetValue(0x0000);
    //VARNOLOAD
    Objregister[172].SetValue(0x0000);
    //VANOLOAD
    Objregister[173].SetValue(0x0000);
    for (i = 171; i <= 173; i++)
    {
        Objregister[i].Write();
    }
    //CONFIG2
    Objregister[176].SetValue(0x1);
    Objregister[176].Write();
    //WTHR, VARTHR, VATHR
    for (i = 178; i <= 180; i++)
    {
        Objregister[i].SetValue(0x03);
        Objregister[i].Write();
    }
    printf("Done\n");
    //Leer todos los registros xWATTHR, xVARHR, xFWATTHR, xFVARHR, and xVAHR
    printf("leyendo registros de potencia para reiniciarlos\n");
    for (i = 72; i <= 86; i++)
    {
        Objregister[i].Read();
    }
    printf("Done\n");
    //limpiar los Bit 9 (CF1DIS), Bit 10 (CF2DIS) y Bit 11 (CF3DIS)
    printf("limpiar los Bit 9 (CF1DIS), Bit 10 (CF2DIS) y Bit 11 (CF3DIS)\n");
    Objregister[142].SetValue(0x88);
    printf("Done\n");

    //leer todos los registros para saber que estan bien escritos
    printf("Leyendo todos los registros de la DSP\n");
    Read_all_registers();
    printf("Done\n");
}
void SetJson(int Registro, int Sample, float Value){

    Objregister[Registro].SetConValue(Value);
    dataj[std::to_string(Sample)][Objregister[Registro].GetName()] = Value;
}
// Cargar los valores de Corriente convertidos
void Current(int Registro, int Sample)
{
    int Temp = 0;
    float Valueobj = 0;

    Objregister[Registro].Read();
    Temp = Objregister[Registro].GetValue();
    // Valueobj = 769.231 * (5.224 * pow(10, -8) * Temp - 0.00028);
    // Valueobj = 4.2570*pow(10,-6)*Temp  - 0.1855;
    //Valueobj = 4.2550*pow(10,-6)*Temp - 0.1858;
    Valueobj = Temp*(4.2554*pow(10,-6)) - 0.1858;
    SetJson(Registro, Sample, Valueobj);
    
    // if(Nameobj == "AIRMS"){
    //     sumI = sumI + Valueobj;
    // }

}
// Cargar los valores de Voltaje convertidos
void Vol(int Registro, int Sample)
{
    int Temp = 0;
    float Valueobj = 0;

    Objregister[Registro].Read();
    Temp = Objregister[Registro].GetValue();
    dataj[std::to_string(Sample)]["ADCV"]=Temp;
    // Valueobj = Temp * 901 / 10640000;
    Valueobj = Temp * (991150 / 1000) / (10640000);
    SetJson(Registro, Sample, Valueobj);

    // if(Nameobj == "AVRMS"){
    //     sumV = sumV + Valueobj;
    // }
    
    // cout << Nameobj << endl;
    // cout << Valueobj << endl;
}

void Power(int Registro, int Sample){
    int Temp = 0;
    float Valueobj = 0;

    Objregister[Registro].Read();
    Temp = Objregister[Registro].GetValue();
    Valueobj = Temp*3.32 * pow(10,-3) - 23.2;
    SetJson(Registro, Sample, Valueobj);
}
void THD(int Registro, int Sample){
    int Temp = 0;
    float Valueobj = 0;

    Objregister[Registro].Read();
    Temp = Objregister[Registro].GetValue();
    Valueobj = 5.06 *pow(10,-7)*Temp - 3.82*pow(10,-3);
    SetJson(Registro, Sample, Valueobj);
}
void Angle(int Registro, int Sample){
    float Temp = 0;   
    float Valueobj = 0;

    Objregister[Registro].Read();
    Temp = Objregister[Registro].GetValue();
    Valueobj = Temp*360*60/256000 ;
    SetJson(Registro, Sample, Valueobj);
}
void PF(int Registro, int Sample){
    int Temp = 0;
    float Valueobj = 0;
    
    Objregister[Registro].Read();
    Temp = Objregister[Registro].GetValue();
    Valueobj = Temp;

    Valueobj = Temp* pow(2,-15); 
    // if( Temp & 0x8000 == 0x8000 ){
    //     Valueobj = -( Temp & 0x7FFF )* pow(2,-15); 
    // }else {
    //     Valueobj = ( Temp & 0x7FFF )* pow(2,-15); 
    // }
    SetJson(Registro, Sample, Valueobj);
}

float hrm( float t, float f  ){
    return sqrt(pow(t,2)-pow(f,2));
}
float pf1( float pf, float thdi ){
    return pf*sqrt(1+thdi);
}
float s1( float i1, float v1 ){
    return v1*i1;
}
float p1( float pf1, float s1 ){
    return fabs(pf1*s1);
}
float ph( float p, float p1){
    return p-p1;
}
float sh( float ih, float vh ){
    return vh*ih;
}
float sn( float s, float s1 ){
    return sqrt( pow(s,2) - pow(s1,2) );
}
float di( float v1, float ih){
    return v1*ih;
}
float dv( float vh, float i1){
    return vh*i1;
}
float dh( float sh, float ph){
    return sqrt( pow(sh,2) - pow(ph,2));
}
float n(float s, float p){
    return sqrt(pow(s,2) - pow(p,2) );
}
void SetMathParameters(){
    RCal[0].set( hrm( Objregister[56].GetConValue(), Objregister[127].GetConValue() ));
    RCal[1].set( hrm( Objregister[57].GetConValue(), Objregister[128].GetConValue() ));
    RCal[2].set( pf1( Objregister[165].GetConValue(), Objregister[118].GetConValue() ));
    RCal[6].set( s1( Objregister[127].GetConValue(), Objregister[128].GetConValue() ));
    RCal[3].set( p1( RCal[2].get(), RCal[6].get() ));
    RCal[4].set( ph( Objregister[108].GetConValue(), RCal[3].get() ));
    RCal[5].set( sh( RCal[0].get(), RCal[1].get() ));
    RCal[7].set( sn( Objregister[114].GetConValue(), RCal[6].get() ));
    RCal[8].set( di( Objregister[128].GetConValue(), RCal[0].get() ));
    RCal[9].set( dv( RCal[1].get(), Objregister[127].GetConValue() ));
    RCal[10].set( dh( RCal[5].get(), RCal[4].get() ));
    RCal[11].set( n( Objregister[114].GetConValue(), Objregister[108].GetConValue() ));

    RCal[12].set( hrm( Objregister[59].GetConValue(), Objregister[129].GetConValue() ));
    RCal[13].set( hrm( Objregister[60].GetConValue(), Objregister[130].GetConValue() ));
    RCal[14].set( pf1( Objregister[166].GetConValue(), Objregister[120].GetConValue() ));
    RCal[18].set( s1( Objregister[129].GetConValue(), Objregister[130].GetConValue() ));
    RCal[15].set( p1( RCal[14].get(), RCal[18].get() ));
    RCal[16].set( ph( Objregister[109].GetConValue(), RCal[15].get() ));
    RCal[17].set( sh( RCal[12].get(), RCal[13].get() ));
    RCal[19].set( sn( Objregister[115].GetConValue(), RCal[18].get() ));
    RCal[20].set( di( Objregister[130].GetConValue(), RCal[12].get() ));
    RCal[21].set( dv( RCal[13].get(), Objregister[129].GetConValue() ));
    RCal[22].set( dh( RCal[17].get(), RCal[16].get() ));
    RCal[23].set( n( Objregister[115].GetConValue(), Objregister[109].GetConValue() ));

    RCal[24].set( hrm( Objregister[62].GetConValue(), Objregister[131].GetConValue() ));
    RCal[25].set( hrm( Objregister[63].GetConValue(), Objregister[132].GetConValue() ));
    RCal[26].set( pf1( Objregister[167].GetConValue(), Objregister[122].GetConValue() ));
    RCal[30].set( s1( Objregister[131].GetConValue(), Objregister[132].GetConValue() ));
    RCal[27].set( p1( RCal[26].get(), RCal[30].get() ));
    RCal[28].set( ph( Objregister[110].GetConValue(), RCal[27].get() ));
    RCal[29].set( sh( RCal[24].get(), RCal[25].get() ));
    RCal[31].set( sn( Objregister[116].GetConValue(), RCal[30].get() ));
    RCal[32].set( di( Objregister[132].GetConValue(), RCal[24].get() ));
    RCal[33].set( dv( RCal[25].get(), Objregister[131].GetConValue() ));
    RCal[34].set( dh( RCal[29].get(), RCal[28].get() ));
    RCal[35].set( n( Objregister[116].GetConValue(), Objregister[110].GetConValue() ));

}
string getTime(){
    time_t rawtime;
    struct tm * timeinfo;
    char buffer [80];

    time (&rawtime);
    timeinfo = localtime (&rawtime);

    strftime (buffer,80,"%F %T.",timeinfo);
    return buffer;
}
void mysqlSet( string query ){
    MYSQL *connect;
    connect=mysql_init(NULL);
    if (!connect){
        cout<<"MySQL Initialization failed";
        // return 1;
    }
    connect=mysql_real_connect(connect, SERVER, USER, PASSWORD , DATABASE ,0,NULL,0);
    if (connect){
        cout<<"connection Succeeded\n";
    }
    else{
        cout<<"connection failed\n";
    }
    mysql_query (connect, query.c_str());
    mysql_close (connect);
}
void Query( int id ){
    string insert ="INSERT INTO `heroku_851e4397b87123b`.`register`(";
    string col = "";
    string values = "";
    int i = 0;
    col = col + "`id`,";
    values = values + std::to_string(id) + ",";
    for ( i= 0 ; i < 181; i ++){
        if(i == 56 || i == 57 || i == 59 || i == 60 || i == 62 || i == 63 || i == 65 || i == 66 || i>=72 && i <= 86 || i >= 108 && i <= 122 || i >= 127 && i <= 132 || i >= 135 && i <= 137 || i >= 165 && i <= 167 ){
        col = col + "`" + Objregister[i].GetName() + "`,";
        values = values + '"' + std::to_string(Objregister[i].GetConValue()) + '"' + "," ;
        }
    }
    for (   i = 0; i<= 35; i++ ){
        col = col + "`" + RCal[i].getName() + "`," ;
        if(isnan(RCal[i].get())){
            values = values + '"' + std::to_string( 0 ) + '"' + ",";
        }else {
        values = values + '"' + std::to_string( RCal[i].get() ) + '"' + ",";
        }
    }
    insert = insert + col + "`DATETIME`" + " ) VALUES (" + values + '"' + getTime() + '"' + ");";
    cout << insert << endl;
    // mysqlSet(insert);
}


// filtro para cargar los registros que se van a enviar
void Read_registers(int Sample)
{
    int i = 0;

    while ( i < 181 )
    {
        if ( i == 56 || i == 59 || i == 62 || i == 65 || i == 66 || i == 127 || i == 129 || i == 131 )
        {
            Current(i, Sample);
        } else if ( i == 57 || i == 60 || i == 63 || i == 126 || i == 128 || i ==130 || i == 132 )
        {
            Vol(i, Sample);
        // } else if ( i == 72 || i == 73 || i == 74 || i == 75 || i == 76 || i == 77 || i == 78 || i == 79 || i == 80 || i == 81 || i == 82 || i == 83){
        } else if ( i >= 72 && i <= 86 || i >= 108 && i <= 116 ){
            Power( i, Sample );
        }
         else if ( i >= 117 && i <= 122 ){
            THD( i, Sample );
        } 
        else if (  i >= 135 && i <= 137 ){
            Angle(i, Sample);
        } else if ( i >= 165 && i <= 167 ){
            PF(i, Sample);
        }
        i++;
    }
    SetMathParameters();
    Query(sample);
}

//Programa principal
int main()
{
    int i = 0, Valueobj, tempstop = 0, Samples = 0;
    string Nameobj;

    //Iniciar el bus de la rasberry
    bcm2835_init();
    //configurar los registros como Objetos
    Config_registers();
    ConfigRCal();
    //Pasos para inicializar el CHIP ADE
    Initializing_the_chipset();

    // ciclo infinito donde permanece el programa
    while (1)
    {
        {
            for ( Samples = 0; Samples <= 5263; Samples++ )
            {
                Read_registers(Samples);
                sleep(1);
            }
            if (tempstop == 1)
            {
                tempstop = 0;
            }
        }
        while (tempstop == 0)
        {
            Stop_DSP();
            tempstop = 1;
    }
    bcm2835_close();
    return 0;
}
