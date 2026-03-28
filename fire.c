#include <reg51.h>

/* ================= PIN DEFINITIONS ================= */

/* Sensors (Comparator Outputs) */
sbit FLAME = P1^0;
sbit SMOKE = P1^1;
sbit TEMP  = P1^2;

/* Outputs */
sbit BUZZER  = P2^0;
sbit SPEAKER = P2^4;

/* LCD Control */
sbit RS = P2^1;
sbit RW = P2^2;
sbit EN = P2^3;

/* Reset Button */
sbit RESET_BTN = P3^2;

/* ================= GLOBAL FLAGS ================= */
bit fire_active = 0;

/* ================= FUNCTION PROTOTYPES ================= */
void delay(unsigned int);
void lcd_cmd(unsigned char);
void lcd_data(unsigned char);
void lcd_init(void);
void lcd_string(char *);
void uart_init(void);
void uart_string(char *);

/* ================= MAIN PROGRAM ================= */
void main()
{
    unsigned int t;

    /* Initial states */
    BUZZER = 0;
    SPEAKER = 0;
    RESET_BTN = 1;

    lcd_init();
    uart_init();

    lcd_cmd(0x01);
    lcd_string("System Ready");

    while(1)
    {
        /* -------- 1. SMOKE ONLY -------- */
        if(SMOKE == 1 && TEMP == 0 && fire_active == 0)
        {
            lcd_cmd(0x01);
            lcd_string("Smoke Detected");

            BUZZER = 1;
            delay(400);
            BUZZER = 0;
            delay(400);
        }

        /* -------- 2. SMOKE + TEMP -------- */
        else if(SMOKE == 1 && TEMP == 1 && fire_active == 0)
        {
            fire_active = 1;
            BUZZER = 1;     // continuous buzzer
            SPEAKER = 0;

            lcd_cmd(0x01);
            lcd_string("High Temp Detected");
            lcd_cmd(0xC0);
            lcd_string("Press Reset");

            /* -------- 10 Second Wait -------- */
            for(t = 0; t < 100; t++)   // 100 x 100ms = 10 seconds
            {
                /* Flame override */
                if(FLAME == 1)
                    SPEAKER = 1;

                /* Reset Button */
                if(RESET_BTN == 0)
                {
                    BUZZER = 0;
                    SPEAKER = 0;
                    fire_active = 0;

                    lcd_cmd(0x01);
                    lcd_string("SAFE");

                    while(RESET_BTN == 0);
                    break;
                }
                delay(100);
            }

            /* -------- GSM ALERT -------- */
            if(fire_active == 1)
            {
                lcd_cmd(0x01);
                lcd_string("Sending SMS");
								
                uart_string("AT\r\n");
                delay(1000);
                uart_string("AT+CMGF=1\r\n");
                delay(1000);
                uart_string("AT+CMGS=\"+911234567890\"\r\n");
                delay(1000);
                uart_string("ALERT! Fire detected. Smoke and high temperature observed.\x1A");
            }
        }
    }
}

/* ================= LCD FUNCTIONS ================= */

void lcd_init()
{
    lcd_cmd(0x38);
    lcd_cmd(0x0C);
    lcd_cmd(0x06);
    lcd_cmd(0x01);
}

void lcd_cmd(unsigned char cmd)
{
    P0 = cmd;
    RS = 0;
    RW = 0;
    EN = 1;
    delay(2);
    EN = 0;
}

void lcd_data(unsigned char dat)
{
    P0 = dat;
    RS = 1;
    RW = 0;
    EN = 1;
    delay(2);
    EN = 0;
}

void lcd_string(char *str)
{
    while(*str)
        lcd_data(*str++);
}

/* ================= UART (GSM) ================= */

void uart_init()
{
    TMOD = 0x20;     // Timer1 mode2
    TH1  = 0xFD;     // 9600 baud
    SCON = 0x50;
    TR1  = 1;
}

void uart_string(char *msg)
{
    while(*msg)
    {
        SBUF = *msg++;
        while(TI == 0);
        TI = 0;
    }
}

/* ================= DELAY ================= */

void delay(unsigned int ms)
{
    unsigned int i, j;
    for(i = 0; i < ms; i++)
        for(j = 0; j < 1275; j++);
}