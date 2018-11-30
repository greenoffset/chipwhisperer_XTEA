#include <plib.h>	
#include <stdint.h>
#include <GenericTypeDefs.h>
#include <xc.h>

//Impelmentation for the Mikroleketronike MINI-32
#define RX_BUFFER_SIZE 4096    
#define DEFAULT_BAUD_A 115200

//#define SYSCLOCK 32000000
//#define SYSCLOCK 16000000
#define SYSCLOCK 8000000
#define F_OSC 16000000


#define TRIG_TRIS     TRISEbits.TRISE6
#define LED_TRIS      TRISDbits.TRISD3

#define TRIG           LATEbits.LATE6

// DEVCFG3
// USERID = No Setting
#pragma config FSRSSEL = PRIORITY_7     // SRS Select (SRS Priority 7)
#pragma config FCANIO = ON              // CAN I/O Pin Select (Default CAN I/O)
#pragma config FUSBIDIO = OFF           // USB USID Selection (Controlled by Port Function)
#pragma config FVBUSONIO = OFF          // USB VBUS ON Selection (Controlled by Port Function)

// DEVCFG2
#pragma config FPLLIDIV = DIV_2         // PLL Input Divider (2x Divider)
#pragma config FPLLMUL = MUL_16        // PLL Multiplier (16x Multiplier)
#pragma config UPLLIDIV = DIV_4         // USB PLL Input Divider (4x Divider)
#pragma config UPLLEN = OFF             // USB PLL Enable (Disabled and Bypassed)
#pragma config FPLLODIV = DIV_4         // System PLL Output Clock Divider (PLL Divide by 4)

// DEVCFG1
#pragma config FNOSC = PRIPLL           // Oscillator Selection Bits (Primary Osc w/PLL (XT+,HS+,EC+PLL))
#pragma config FSOSCEN = OFF            // Secondary Oscillator Enable (Disabled)
#pragma config IESO = ON                // Internal/External Switch Over (Enabled)
#pragma config POSCMOD = XT             // Primary Oscillator Configuration (External clock mode)
#pragma config OSCIOFNC = OFF           // CLKO Output Signal Active on the OSCO Pin (Disabled)
#pragma config FPBDIV = DIV_1         // Peripheral Clock Divisor (Pb_Clk is Sys_Clk/1)
#pragma config FCKSM = CSECMD           // Clock Switching and Monitor Selection (Clock Switch Disable, FSCM Disabled)
#pragma config WDTPS = PS1048576        // Watchdog Timer Postscaler (1:1048576)
#pragma config FWDTEN = OFF             // Watchdog Timer Enable (WDT Disabled (SWDTEN Bit Controls))

// DEVCFG0
#pragma config DEBUG = OFF              // Background Debugger Enable (Debugger is disabled)
#pragma config ICESEL = ICS_PGx2        // ICE/ICD Comm Channel Select (ICE EMUC2/EMUD2 pins shared with PGC2/PGD2)
#pragma config PWP = OFF                // Program Flash Write Protect (Disable)
#pragma config BWP = OFF                // Boot Flash Write Protect bit (Protection Disabled)
#pragma config CP = OFF                 // Code Protect (Protection Disabled)

// #pragma config statements should precede project file includes.
// Use project enums instead of #define for ON and OFF.
//#pragma config FPLLMUL = MUL_20, FPLLIDIV = DIV_2, FPLLODIV = DIV_1, FWDTEN = OFF
//#pragma config POSCMOD = HS, FNOSC = PRIPLL, FPBDIV = DIV_1

uint32_t g_32data0 = 0;
uint32_t g_32data1 = 0;
uint32_t g_sub = 0;
void XTEAdecipher2(uint32_t * v, uint32_t * key);
void KeyExtractionTest(uint32_t data,uint32_t key);
void UART1_init(unsigned long baudrate);
void Serial_Overrun_Reset();
unsigned int ParseInputBuffer();

void ManageClock()
{
    while (!OSCCONbits.SLOCK);           
    SYSKEY = 0x0; // ensure OSCCON is locked
    SYSKEY = 0xAA996655; // Write Key1 to SYSKEY
    SYSKEY = 0x556699AA;        
    OSCCONbits.OSWEN = 1;       
    while (OSCCONbits.OSWEN);
    SYSKEY = 0x0; // ensure OSCCON is locked
}

/***************************************************************************
    int main(void)
****************************************************************************/
uint32_t mkey[4] = {0xf8e9ebde, 0x53ce00f7,0x2fe12fe3, 0x04dcee29 };
uint32_t Einput[2];
unsigned char HEX_CHR[] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};

void PrintResult()
{
    
    int i;
    
    U1TXREG = 'r';    
    while (U1STAbits.TRMT == 0); // While the tx buffer is not empty
    for (i=0; i< 8; i++)
    {
        U1TXREG = HEX_CHR[(Einput[0] >> 4*(7-i)) & 0x0F];    
        while (U1STAbits.TRMT == 0); // While the tx buffer is not empty       
    }
    
    for (i=0; i< 8; i++)
    {
        U1TXREG = HEX_CHR[(Einput[1] >> 4*(7-i)) & 0x0F];    
        while (U1STAbits.TRMT == 0); // While the tx buffer is not empty       
    }
    U1TXREG = '\n';    
    while (U1STAbits.TRMT == 0); // While the tx buffer is not empty
    //U1TXREG = 10;    
    //while (U1STAbits.TRMT == 0); // While the tx buffer is not empty
   
}
int main(void)
{
    uint32_t i;
    TRIG_TRIS = 0;
    LED_TRIS = 0;
    ManageClock();
    INTEnableSystemMultiVectoredInt();
    SYSTEMConfig(SYSCLOCK, SYS_CFG_ALL);

    UART1_init(19200);

    TRIG = 1;
    while(1)
    {
        if (ParseInputBuffer())
        {
            Einput[0] = g_32data0;
            Einput[1] = g_32data1;
            //TRIG = 0;
            XTEAdecipher2(Einput, mkey);
            //KeyExtractionTest(g_32data,g_32key);  
            Nop();Nop();Nop();Nop();Nop();Nop();Nop();Nop();Nop();Nop();
            Nop();Nop();Nop();Nop();Nop();Nop();Nop();Nop();Nop();Nop();
            Nop();Nop();Nop();Nop();Nop();Nop();Nop();Nop();Nop();Nop();
            Nop();Nop();Nop();Nop();Nop();Nop();Nop();Nop();Nop();Nop();
            TRIG = 1;
            PrintResult();
            //Print result
        }
        //KeyExtractionTest(g_32data0,g_32data1);       
    }
}

void KeyExtractionTest(uint32_t data,uint32_t key)
{
    volatile uint32_t v1 = g_sub;    
    Nop();Nop();Nop();Nop();Nop();Nop();Nop();Nop();Nop();Nop();
    Nop();Nop();Nop();Nop();Nop();Nop();Nop();Nop();Nop();Nop();
    TRIG = 1;
    v1 += data ^ key;
    Nop();Nop();Nop();Nop();Nop();Nop();Nop();Nop();Nop();Nop();
    Nop();Nop();Nop();Nop();Nop();Nop();Nop();Nop();Nop();Nop();
    Nop();Nop();Nop();Nop();Nop();Nop();Nop();Nop();Nop();Nop();
    Nop();Nop();Nop();Nop();Nop();Nop();Nop();Nop();Nop();Nop();
    TRIG = 0;
}
/////////////////////////////////////////////////////////////////////////ENC routines

/* take 64 bits of data in v[0] and v[1] and 128 bits of key[0] - key[3] */

void XTEAencipher(unsigned int num_rounds, uint32_t *v, uint32_t key[4]) {
    unsigned int i = 0;
    uint32_t v0=v[0], v1=v[1], sum=0, delta=0x9E3779B9;
    for (i=0; i < num_rounds; i++) {
        v0 += (((v1 << 4) ^ (v1 >> 5)) + v1) ^ (sum + key[sum & 3]);
        sum += delta;
        v1 += (((v0 << 4) ^ (v0 >> 5)) + v0) ^ (sum + key[(sum>>11) & 3]);
    }
    v[0]=v0; v[1]=v1;
}

void XTEAdecipher2(uint32_t * v, uint32_t * key) {
    unsigned int i = 0;
    volatile uint32_t v0=v[0], v1=v[1], delta=0x9E3779B9, sum=delta*32;
    mU1ARXIntEnable( 0);
    TRIG = 0;
    for (i=0; i < 32; i++) {
       
        v1 -= (((v0 << 4) ^ (v0 >> 5)) + v0) ^ (sum + key[(sum>>11) & 3]);
       
        sum -= delta;
       
        v0 -= (((v1 << 4) ^ (v1 >> 5)) + v1) ^ (sum + key[sum & 3]);
       
    }
    mU1ARXIntEnable( 1);
    TRIG = 1;
    v[0]=v0; v[1]=v1;
}

void XTEAdecipher(unsigned int num_rounds, uint32_t * v, uint32_t key[4]) {
    unsigned int i = 0;
       
    uint32_t v0=v[0], v1=v[1], delta=0x9E3779B9, sum=delta*num_rounds;
    for (i=0; i < num_rounds; i++) {        
        v1 -= (((v0 << 4) ^ (v0 >> 5)) + v0) ^ (sum + key[(sum>>11) & 3]);
       
        sum -= delta;
       
        v0 -= (((v1 << 4) ^ (v1 >> 5)) + v1) ^ (sum + key[sum & 3]);
       
    }
    v[0]=v0; v[1]=v1;
}

char g_Rx_A_Buffer[RX_BUFFER_SIZE] = {0};
volatile unsigned int g_Wr_A_cnt = 0;
volatile unsigned int g_Rd_A_cnt = 0;

void UART1_init(unsigned long baudrate)
{
    U1MODEbits.STSEL = 0;       // 1-Stop bit
    U1MODEbits.PDSEL = 0;       // No Parity, 8-Data bits
    U1MODEbits.ABAUD = 0;       // Auto-Baud disabled
    U1MODEbits.BRGH = 1;        // Standard-Speed mode
//    U1MODEbits.URXINV = 0;
    U1STAbits.UTXINV = 0;       // Idle state is 0
    U1BRG = (F_OSC/4/(baudrate))-1;   // Set baudrate
     //U1BRG = 38;
    IEC0bits.U1RXIE = 1;        // Enable UART RX interrupt
    U1MODEbits.UARTEN = 1;      // Enable UART
    U1STAbits.URXISEL = 0b00;   // Interrupt after every byte
    IFS0bits.U1RXIF = 0;        // Clear RX interrupt flag
    U1STAbits.UTXEN = 1;
    U1STAbits.URXEN = 1;
    U1MODEbits.UEN = 0; //RTS and cts is disabled.
    U1MODEbits.ON = 1;
 //   U1STAbits.UTXINV = 1;//////////////////////////////////////TX is inverted here
    mU1ASetIntPriority( 1);
    mU1ARXIntEnable( 1);
}

void UART1_Tx(char * String)
{
    unsigned int i = 0;
    while (i < strlen(String))
    {
        U1TXREG = String[i];
        i++;
        while (U1STAbits.TRMT == 0); // While the tx buffer is not empty
    }
}

void __ISR(0 , ipl1AUTO) _Trap(void)
 {
     Nop();
 }

void __ISR(_UART1A_ERR_IRQ, ipl1AUTO) _UART1_ErrHandler(void)
{
     mU1AClearAllIntFlags();
     Nop();
     Serial_Overrun_Reset();
 }

//#pragma interrupt U1RXInterrupt ipl1AUTO vector 0
void __ISR(_UART1_VECTOR, ipl1AUTO) _UART1_Handler(void)
{
 //    Serial_Overrun_Reset();
    IFS0bits.U1RXIF = 0;

    mU1AEClearIntFlag();
    while (U1STAbits.URXDA == 1) // 1 = Receive buffer has data, at least one character
    {
        g_Rx_A_Buffer[g_Wr_A_cnt] = U1RXREG & 0x000000FF;
        g_Wr_A_cnt++;
        if (g_Wr_A_cnt >= RX_BUFFER_SIZE)
            g_Wr_A_cnt = 0;
        if ((g_Rx_A_Buffer[0] != 'p')&&(g_Rx_A_Buffer[0] != 'k'))
            g_Wr_A_cnt = 0;
    }
}

void Serial_Overrun_Reset()
{
    if (U1STAbits.FERR == 1)
        U1STAbits.FERR = 0;
    if (U2STAbits.FERR == 1)
        U2STAbits.FERR = 0;
    if (U1STAbits.OERR == 1)
        U1STAbits.OERR = 0;
    if (U2STAbits.OERR == 1)
        U2STAbits.OERR = 0;
    if (U1STAbits.PERR == 1)
        U1STAbits.PERR = 0;
    if (U2STAbits.PERR == 1)
        U2STAbits.PERR = 0;
}

uint32_t axntoi(unsigned char *hexStg, unsigned int length)
{
    int n = 0; // position in string
    int m = 0; // position in digit[] to shift
    int count = 0; // loop index
    uint32_t intValue = 0; // integer value of hex string
    int digit[10]; // hold values to convert
    while (n < length)
    {
        
        if (hexStg[n] == '\0')
            break;
        if (hexStg[n] > 0x29 && hexStg[n] < 0x40) //if 0 to 9
            digit[n] = hexStg[n] & 0x0f; //convert to int
        else if (hexStg[n] >= 'A' && hexStg[n] <= 'F') //if A to F
            digit[n] = (hexStg[n] & 0x0f) + 9; //convert to int
        else if (hexStg[n] >= 'a' && hexStg[n] <= 'f') //if a to f 
            digit[n] = ((hexStg[n] - 32) & 0x0f) + 9; //convert to int
        else break;
        n++;
    }
    count = n;
    m = n - 1;
    n = 0;
    while (n < count)
    {
        intValue = intValue | (digit[n] << (m << 2));
        m--; // adjust the position to set
        n++; // next digit to process
    }
    return (intValue);
}

unsigned int ParseInputBuffer()
{
    unsigned int size = 0, offset = 0;   
    unsigned char * ret;
     
    ret = strchr(g_Rx_A_Buffer, 'k');
    if ((ret != NULL)  && (strlen(ret) > 32))
    {
        //64bits of data = 8bytes
        //128bits of key = 16bytes       
        mkey[0] = axntoi((unsigned char*)(ret + 1),8);;
        mkey[1] = axntoi((unsigned char*)(ret + 1 + 8),8);
        mkey[2] = axntoi((unsigned char*)(ret + 1 + 8*2),8);
        mkey[3] = axntoi((unsigned char*)(ret + 1 + 8*3),8);        
    }
    ret = strchr(g_Rx_A_Buffer, 'p');
    if ((ret != NULL)  && (strlen(ret) > 16))
    {
        //64bits of data = 8bytes
        //128bits of key = 16bytes       
        g_32data0 = axntoi((unsigned char*)(ret + 1),8);;
        g_32data1 = axntoi((unsigned char*)(ret + 1 + 8),8);;
        //g_sub = axntoi((unsigned char*)(g_Rx_A_Buffer + 1 + 8 + 8),8);;
        //U1TXREG = g_Wr_A_cnt & 0x00FF;
        if (g_Rx_A_Buffer[g_Wr_A_cnt - 1] == '\n')
        {
            g_Wr_A_cnt = 0;
            for (size = 0;size < RX_BUFFER_SIZE;size++)
                g_Rx_A_Buffer[size] = 0;
            size = 0;       
            return 1;
        }
    }
    return 0;
}