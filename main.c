#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_timer.h"
#include "inc/hw_uart.h"
#include "inc/hw_gpio.h"
#include "inc/hw_pwm.h"
#include "inc/hw_types.h"
#include "driverlib/pin_map.h"

#include "driverlib/timer.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"
#include "driverlib/udma.h"
#include "driverlib/pwm.h"
#include "driverlib/ssi.h"
#include "driverlib/systick.h"
#include "lcd.h"

#include "utils/uartstdio.h"
#include <string.h>

void inputInt();
void Captureinit();
void InitConsole(void);

//This is to avoid doing the math everytime you do a reading
const double temp = 1.0/80.0;
//Stores the pulse length
volatile uint32_t pulse=0;
volatile int data=0;
volatile char b=0;
int LED;
//Tells the main code if the a pulse is being read at the moment
volatile uint8_t echowait=0;
void bluetoothSendMessage(char *array){
    while(*array){
        UARTCharPut(UART5_BASE,*array);
        array++;}}
int main()
{
 //Set system clock to 80Mhz
  SysCtlClockSet(SYSCTL_SYSDIV_2_5|SYSCTL_USE_PLL|SYSCTL_OSC_MAIN|SYSCTL_XTAL_16MHZ);
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
  GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3);

  //Configures the timer
  Captureinit();

  //Configure Trigger pin
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
  SysCtlDelay(3);
  GPIOPinTypeGPIOOutput(GPIO_PORTA_BASE, GPIO_PIN_3);

  //Configure Echo pin
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
  SysCtlDelay(3);
  GPIOPinTypeGPIOInput(GPIO_PORTA_BASE, GPIO_PIN_2);
  GPIOIntEnable(GPIO_PORTA_BASE, GPIO_PIN_2);
  GPIOIntTypeSet(GPIO_PORTA_BASE, GPIO_PIN_2,GPIO_BOTH_EDGES);
  GPIOIntRegister(GPIO_PORTA_BASE,inputInt);

  //Configures the UART
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
  GPIOPinConfigure(GPIO_PE4_U5RX);
  GPIOPinConfigure(GPIO_PE5_U5TX);
  GPIOPinTypeUART(GPIO_PORTE_BASE,GPIO_PIN_4|GPIO_PIN_5);
  SysCtlPeripheralEnable(SYSCTL_PERIPH_UART5);
  UARTConfigSetExpClk(UART5_BASE,SysCtlClockGet(),9600,(UART_CONFIG_WLEN_8 | UART_CONFIG_PAR_NONE | UART_CONFIG_STOP_ONE));
  UARTStdioConfig(5,9600,SysCtlClockGet());
  UARTEnable(UART5_BASE);

  lcd_init();


  while(1)
  {
    //Checks if a pulse read is in progress
    if(echowait != 1)
    {
      //Does the required pulse of 10uS
      GPIOPinWrite(GPIO_PORTA_BASE, GPIO_PIN_3, GPIO_PIN_3);
      SysCtlDelay(266);
      GPIOPinWrite(GPIO_PORTA_BASE, GPIO_PIN_3, ~GPIO_PIN_3);

      while(echowait != 0);

      //Converts the counter value to cm.
      pulse =(uint32_t)(temp * pulse);
      data = pulse / 58;
      lcd_clear();
      lcd_gotoxy(0,0);
      lcd_puts("Khoang cach:");
      lcd_gotoxy(5,1);
      lcd_put_num(data,0,0);
      lcd_gotoxy(8,1);
      lcd_puts("cm");
    }
    b=UARTCharGetNonBlocking(UART5_BASE);
    if(b=='r')
    {
        LED = 2;
        SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
        GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_1);
        GPIOPinWrite(GPIO_PORTF_BASE,GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3, LED);
        bluetoothSendMessage("DEN DO SANG\n");
    }
    if(b=='g')
    {
        LED = 8;
        SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
        GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_1);
        GPIOPinWrite(GPIO_PORTF_BASE,GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3, LED);
        bluetoothSendMessage("DEN XANH SANG\n");
    }
     else if(b=='t')
     {
         LED = 0;
         SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
         GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE,GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3);
         GPIOPinWrite(GPIO_PORTF_BASE,GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3, LED);
         bluetoothSendMessage("DEN DA TAT\n");
    }
      //wait about 10ms until the next reading.
      SysCtlDelay(400000);
  }
}
void inputInt(){
  //Clear interrupt flag. Since we only enabled on this is enough
  GPIOIntClear(GPIO_PORTA_BASE, GPIO_PIN_2);

  /*
    If it's a rising edge then set he timer to 0
    It's in periodic mode so it was in some random value
  */
  if ( GPIOPinRead(GPIO_PORTA_BASE, GPIO_PIN_2) == GPIO_PIN_2){
    HWREG(TIMER2_BASE + TIMER_O_TAV) = 0; //Loads value 0 into the timer.
    TimerEnable(TIMER2_BASE,TIMER_A);
    echowait=1;
  }
  /*
    If it's a falling edge that was detected, then get the value of the counter
  */
  else{
    pulse = TimerValueGet(TIMER2_BASE,TIMER_A); //record value
    TimerDisable(TIMER2_BASE,TIMER_A);
    echowait=0;
  }
}
void Captureinit(){
  /*
    Set the timer to be periodic.
  */
  SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER2);
  SysCtlDelay(3);
  TimerConfigure(TIMER2_BASE, TIMER_CFG_PERIODIC_UP);
  TimerEnable(TIMER2_BASE,TIMER_A);
}

