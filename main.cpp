#include "mbed.h"
#include "fsl_port.h"
#include "fsl_gpio.h"
#define UINT14_MAX        16383
// FXOS8700CQ I2C address
#define FXOS8700CQ_SLAVE_ADDR0 (0x1E<<1) // with pins SA0=0, SA1=0
#define FXOS8700CQ_SLAVE_ADDR1 (0x1D<<1) // with pins SA0=1, SA1=0
#define FXOS8700CQ_SLAVE_ADDR2 (0x1C<<1) // with pins SA0=0, SA1=1
#define FXOS8700CQ_SLAVE_ADDR3 (0x1F<<1) // with pins SA0=1, SA1=1
// FXOS8700CQ internal register addresses
#define FXOS8700Q_STATUS 0x00
#define FXOS8700Q_OUT_X_MSB 0x01
#define FXOS8700Q_OUT_Y_MSB 0x03
#define FXOS8700Q_OUT_Z_MSB 0x05
#define FXOS8700Q_M_OUT_X_MSB 0x33
#define FXOS8700Q_M_OUT_Y_MSB 0x35
#define FXOS8700Q_M_OUT_Z_MSB 0x37
#define FXOS8700Q_WHOAMI 0x0D
#define FXOS8700Q_XYZ_DATA_CFG 0x0E
#define FXOS8700Q_CTRL_REG1 0x2A
#define FXOS8700Q_M_CTRL_REG1 0x5B
#define FXOS8700Q_M_CTRL_REG2 0x5C
#define FXOS8700Q_WHOAMI_VAL 0xC7

Timer debounce;
Ticker ledticker;
Ticker vectorticker;
EventQueue queue1;
EventQueue queue2;
Thread thread1;
Thread thread2; 
I2C i2c( PTD9,PTD8);
Serial pc(USBTX, USBRX);
DigitalOut led1(LED1);
InterruptIn btn(SW2);
int m_addr = FXOS8700CQ_SLAVE_ADDR1;
uint8_t who_am_i, data[2], res[6];
int16_t acc16;
float t[3];
float X[100];
float Y[100];
float Z[100];
int tilt[100];
float origin_x;
float origin_y;
float origin_z;
int i,j;

void FXOS8700CQ_readRegs(int addr, uint8_t * data, int len);
void FXOS8700CQ_writeRegs(uint8_t * data, int len);
void read(void);
void record(void);
void blink_led1(void);
void logger(void);

void read(){
        FXOS8700CQ_readRegs(FXOS8700Q_OUT_X_MSB, res, 6);
    
        acc16 = (res[0] << 6) | (res[1] >> 2);
        if (acc16 > UINT14_MAX/2)
            acc16 -= UINT14_MAX;
        t[0] = ((float)acc16) / 4096.0f;

        acc16 = (res[2] << 6) | (res[3] >> 2);
        if (acc16 > UINT14_MAX/2)
            acc16 -= UINT14_MAX;
        t[1] = ((float)acc16) / 4096.0f;

        acc16 = (res[4] << 6) | (res[5] >> 2);
        if (acc16 > UINT14_MAX/2)
            acc16 -= UINT14_MAX;
        t[2] = ((float)acc16) / 4096.0f;
}

void record(void){
    while(i<100){
        read();
        X[i]=t[0];
        Y[i]=t[1];
        Z[i]=t[2];
        if( Z[i] < origin_z/sqrt(2)) // record every 45 degrees
            tilt[i] = 1;
        else{
            tilt[i] = 0;
        }
        i++;
        wait(0.1);
    }        
    for (j = 0; j < 100; j++){
        pc.printf("%1.4f\r\n", X[j]);
        wait(0.1);
    }
    for (j = 0; j < 100; j++){
        pc.printf("%1.4f\r\n", Y[j]);
        wait(0.1);
    }
    for (j = 0; j < 100; j++){
        pc.printf("%1.4f\r\n", Z[j]);
        wait(0.1);
    }
    for (j = 0; j < 100; j++){
        pc.printf("%d\r\n",tilt[j]);
        wait(0.1);
    }
}
// timer
void logger(void){
    debounce.reset();
    debounce.start();
    i = 0;
    queue2.call(record);   
    ledticker.attach(queue1.event(&blink_led1),0.5); 
}

// here is the normal priority thread
void blink_led1() {
    if(debounce.read()<10){
        led1 = !led1;
    }
    else{
        led1=1; 
    }
}

void FXOS8700CQ_readRegs(int addr, uint8_t * data, int len) {
   char t = addr;
   i2c.write(m_addr, &t, 1, true);
   i2c.read(m_addr, (char *)data, len);
}

void FXOS8700CQ_writeRegs(uint8_t * data, int len) {
   i2c.write(m_addr, (char *)data, len);
}

int main() {
   led1=1;
   pc.baud(115200);
   thread1.start(callback(&queue1, &EventQueue::dispatch_forever));
   thread2.start(callback(&queue2, &EventQueue::dispatch_forever));
   btn.rise(&logger);

   // Enable the FXOS8700Q
   FXOS8700CQ_readRegs( FXOS8700Q_CTRL_REG1, &data[1], 1);
   data[1] |= 0x01;
   data[0] = FXOS8700Q_CTRL_REG1;
   FXOS8700CQ_writeRegs(data, 2);

   // Get the slave address
   FXOS8700CQ_readRegs(FXOS8700Q_WHOAMI, &who_am_i, 1);
   while (true) {
       read();
       origin_x=t[0];
       origin_y=t[1];
       origin_z=t[2];
   }
}