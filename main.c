/* Name: Olayiwola David Abraham
 * Date: 3/5/24
 * Real-Time Embedded Video Game
 *  DriverLib Includes */
#include <LcdDriver/Crystalfontz128x128_ST7735.h>
#include <ti/grlib/grlib.h>
#include <ti/devices/msp432p4xx/driverlib/driverlib.h>
#include <ti/devices/msp432p4xx/inc/msp.h>



/* Standard Includes */
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

//*****************************************************************************

//Global Variables
/* Graphic library context */
Graphics_Context g_sContext;
static uint16_t resultsBuffer[2];           //ADC results buffer
volatile int8_t ADCFlag = 0;                    //Global ADC ready Flag


// SysTick flag
volatile int8_t systickFlag = 0;

void SysTick_init(int32_t period){
    //add Systick initalization and interrupt setup
       SysTick->VAL=0;
       SysTick_enableModule();
       SysTick_enableInterrupt();
}

void SysTick_Handler(void){
    //set systickFlag
        systickFlag = 1;
        }
 
//*****************************************************************************
void Color_LCD_init() {
   /* Initializes display */
      Crystalfontz128x128_Init();

      /* Set default screen orientation */
      Crystalfontz128x128_SetOrientation(LCD_ORIENTATION_UP);

      /* Initializes graphics context */
      Graphics_initContext(&g_sContext, &g_sCrystalfontz128x128, &g_sCrystalfontz128x128_funcs);
      Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_RED);
      Graphics_setBackgroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);
      GrContextFontSet(&g_sContext, &g_sFontFixed6x8);
      Graphics_clearDisplay(&g_sContext);
}

//*****************************************************************************
void erase(int32_t x, int32_t y, int32_t radius){
    //add erase related codes in here
        Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);
        Graphics_drawCircle(&g_sContext, x, y, radius);

        Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_RED);
}

//*****************************************************************************
void draw(int32_t x, int32_t y, int32_t radius){
//add draw related codes in here
    Graphics_drawCircle(&g_sContext, x, y, radius);

}

//*****************************************************************************
void erase_paddle(int32_t px, int32_t py, int32_t paddle_length){
    //draw over old paddle with background color to erase it
    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);
    Graphics_drawLineH(&g_sContext, px, px + paddle_length -1, py);

    //restore the foreground color
    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_RED);
}
//*****************************************************************************
void draw_paddle(int32_t px, int32_t py, int32_t paddle_length){
    Graphics_drawLineH(&g_sContext, px, px + paddle_length -1, py);
}



//*****************************************************************************
void ADC_init(void){

    /* Configures Pin 6.0 and 4.4 as ADC input */
     GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P6, GPIO_PIN0, GPIO_TERTIARY_MODULE_FUNCTION);
     GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P4, GPIO_PIN4, GPIO_TERTIARY_MODULE_FUNCTION);

    /* Initializing ADC (ADCOSC/64/8) */
     ADC14_enableModule();
     ADC14_initModule(ADC_CLOCKSOURCE_ADCOSC, ADC_PREDIVIDER_64, ADC_DIVIDER_8, 0);

    /* Configuring ADC Memory (ADC_MEM0 - ADC_MEM1 (A15, A9)  with repeat)
         * with internal 2.5v reference */
     ADC14_configureMultiSequenceMode(ADC_MEM0, ADC_MEM1, true);
     ADC14_configureConversionMemory(ADC_MEM0,
            ADC_VREFPOS_AVCC_VREFNEG_VSS,
            ADC_INPUT_A15, ADC_NONDIFFERENTIAL_INPUTS);

     ADC14_configureConversionMemory(ADC_MEM1,
            ADC_VREFPOS_AVCC_VREFNEG_VSS,
            ADC_INPUT_A9, ADC_NONDIFFERENTIAL_INPUTS);

    /* Enabling the interrupt when a conversion on channel 1 (end of sequence)
     *  is complete and enabling conversions */
     ADC14_enableInterrupt(ADC_INT1);

    /* Enabling Interrupts */
     Interrupt_enableInterrupt(INT_ADC14);
     Interrupt_enableMaster();

    /* Setting up the sample timer to automatically step through the sequence
     * convert.
     */
     ADC14_enableSampleTimer(ADC_AUTOMATIC_ITERATION);

    /* Triggering the start of the sample */
     ADC14_enableConversion();
     ADC14_toggleConversionTrigger();
}
//*****************************************************************************
void ADC14_IRQHandler(void)
{
    uint64_t status;

    status =  ADC14_getEnabledInterruptStatus();
     ADC14_clearInterruptFlag(status);

    /* ADC_MEM1 conversion completed */
    if(status & ADC_INT1)
    {
        /* Store ADC14 conversion results */
        resultsBuffer[0] = ADC14_getResult(ADC_MEM0);
        resultsBuffer[1] = ADC14_getResult(ADC_MEM1);
        ADCFlag = 1;

    }
}
//*****************************************************************************
void delay_1second(void){
    SysTick->VAL =0;
    while(!(SysTick->CTRL &0x10000));
}
//*****************************************************************************



 int main(void) {

     //debugging
     GPIO_setAsOutputPin(GPIO_PORT_P1, GPIO_PIN0);
     GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN0);
     GPIO_setAsOutputPin(GPIO_PORT_P2, GPIO_PIN2 | GPIO_PIN1 | GPIO_PIN0);
     GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN2 | GPIO_PIN1 | GPIO_PIN0);
   // variables
   int32_t period = 75000;            // set Systick period so the refresh rate is 20Hz (50ms)
   SysTick_setPeriod(75000);
   int32_t maxDimension = 127;
   int32_t x0;
   int32_t y0;

   int32_t rightTurn=10500;
   int32_t leftTurn=3000;


   char countString[20];
   int32_t px=5;

   int32_t x=50, y=100;        // set start point of the ball
   int32_t vx=1, vy=-1;        // set ball velocity
   const int32_t radius=3;     // set radius of circle
   const int32_t paddle_length=32;
   const int32_t py=3;
   const int32_t moveS=3;


   int32_t count = 0;          // count score

   /* Stop Watchdog  */
   WDT_A_hold(WDT_A_BASE);
   ADC_init();
   SysTick_init(period);      
   Color_LCD_init();          


   while(1){
       if (systickFlag){

draw_paddle(px,  py, paddle_length);
if(resultsBuffer[0]>= rightTurn){
    erase_paddle(px,  py, paddle_length);
    px += moveS;
    draw_paddle(px,  py, paddle_length);
        if(px>=96){
        px=96;
    }
 }

if(resultsBuffer[0]<= leftTurn){
    erase_paddle(px,  py, paddle_length);
    px -= moveS;
    draw_paddle(px,  py, paddle_length);
        if(px<= 0){
        px=0;
    }
 }
    systickFlag=0;
    SysTick->VAL=0;
// clear systickFlag and SysTick->VAL

    x0 = x;
    y0 = y;

// update new ball position
    x+=vx;
    y+=vy;

// bounce logic
if (x - radius <= 0 || x + radius >= maxDimension) {
     vx = -vx;
}
if (y + radius >= maxDimension) {
     vy = -vy;
}
//paddle logic
if(x - radius >= px && x + radius <= px + paddle_length && y + radius >= py && y - radius <= py ){
     vy= -vy;
     ++count;
     sprintf(countString, "Count: %d", count);
     GPIO_setOutputHighOnPin(GPIO_PORT_P1, GPIO_PIN0);
     delay_1second();
     GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN0);
}
//game over
if(x-3 != px && y == 2){
     GPIO_setOutputHighOnPin(GPIO_PORT_P2, GPIO_PIN2 | GPIO_PIN1 | GPIO_PIN0);
     delay_1second();
     GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN2 | GPIO_PIN1 | GPIO_PIN0);
     Graphics_setBackgroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);
     erase_paddle(px,  py, paddle_length);
     Graphics_clearDisplay(&g_sContext);
//message
                    Graphics_drawString(&g_sContext,
                                (int8_t *)"GAME OVER",
                                AUTO_STRING_LENGTH,
                                40,
                                64,
                                OPAQUE_TEXT);

                    Graphics_drawStringCentered(&g_sContext,
                                                (int8_t *)countString,
                                                AUTO_STRING_LENGTH,
                                                64,  // X-coordinate
                                                100, // Y-coordinate
                                                OPAQUE_TEXT);
    while (1) {}

                }
 // Erase previous ball
    erase(x0, y0, radius);
// Draw new ball
    draw(x, y, radius);
    }
  }
}

