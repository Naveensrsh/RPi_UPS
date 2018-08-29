#include <16F1825.h>
#device ADC=10
#fuses INTRC_IO,NOWDT,NOLVP,NOCLKOUT,NOBROWNOUT,NOPUT,NODEBUG,NOSTVREN
#use delay(clock=8000000)
#define shutdown_delay 800
#define low_cutoff 760//750//770//675//750//735
#define high_cutoff 840//825//720//800//800
/*---------------------------------------------------------------------------*/
// Ext Supply Transistor - C5
// Bat Supply Transistor - C0
// On Battery Led - C2
// Overload Led - C1
// Low Voltage Led - C4
// To Shutdown Pin - A1
// Shutdown Switch - C3
// ADC 735 --> 3.60v or 10.80V
// ADC 800 --> 3.91v or 11.75v
// FVR_4.096_ADC --> { 10.8 - 675 : 11.5 - 720 }
/*---------------------------------------------------------------------------*/
unsigned int16 ext_vol = 0;
unsigned int16 bat_vol = 0;
int on_bat = 0;
int shutdown_flag = 0;
int pi_state = 0;
/*---------------------------------------------------------------------------*/
void read_voltage()
{
   set_adc_channel(0);
   bat_vol = read_adc();
   set_adc_channel(2);
   ext_vol = read_adc();
}
/*---------------------------------------------------------------------------*/
void initialise()
{
   //setup_vref(VREF_ON|VREF_ADC_4v096);  // Setup FVR at 4.096v
   setup_adc(ADC_CLOCK_INTERNAL);
   setup_adc_ports(0);
   setup_adc_ports(2);
   output_low(pin_c0); // Bat Relay OFF
   output_low(pin_c5); // Ext Relay OFF
   output_low(pin_a1); //Shutdown to Pi Pin OFF
   //Test
   delay_ms(250);
   output_high(pin_c2); // On Bat Led ON
   output_high(pin_c4); // Battery Low ON
   delay_ms(1500);
   output_low(pin_c2); // On Bat Led OFF
   output_low(pin_c4); // Battery Low OFF
   delay_ms(250);
}
/*---------------------------------------------------------------------------*/
void main()
{
   
   initialise();
   while(True)
   {  
      delay_ms(10);
      read_voltage();
      
      /*--------------------------------------------------*/
      
      if (ext_vol >= high_cutoff || bat_vol >= high_cutoff )                                             //# 1 #//
      {
         shutdown_flag = 0;
         pi_state = 1;
      }
      
      /*--------------------------------------------------*/
      if (ext_vol > low_cutoff && ext_vol < high_cutoff && pi_state == 0 && shutdown_flag == 0) 
      {
         on_bat = 0;
         output_high(pin_c5); // Ext Relay ON
         pi_state = 1;
      }
      if (bat_vol > low_cutoff && bat_vol < high_cutoff && pi_state == 0 && shutdown_flag == 0)
      {
         on_bat = 1;
         output_high(pin_C2); // On Bat Led ON
         output_high(pin_C0); // Bat Relay ON
         pi_state = 1;
      }
      if (ext_vol > low_cutoff && ext_vol < high_cutoff && bat_vol > low_cutoff && bat_vol < high_cutoff && shutdown_flag == 0)
      {
         //pi_state = 1;
         output_high(pin_C0); // Bat Relay ON
         output_low(pin_C2); // On Bat Led OFF
      }
      /*--------------------------------------------------*/
      if (ext_vol >= high_cutoff) //750                                                         //# 2 #//
      {
         on_bat = 0;
         output_low(pin_C2); // On Bat Led OFF
         output_high(pin_c5); // Ext Relay ON 
      }
      
      else if(ext_vol <= low_cutoff && shutdown_flag == 0)                                     //# 3 #//
      {
         if (pi_state == 1 && bat_vol < low_cutoff)
         {
            output_high(pin_C0); //Temporarly turn on Bat relay before turning off ext relay
                                 //in case where bat vol is too low and ext sup is powered off
            output_high(pin_C2); // On Bat Led ON
         }
         else if (pi_state == 0 && bat_vol < low_cutoff)
         {
            output_low(pin_C2); // On Bat Led off
            output_high(pin_c4); // Low ON
         }
//!         else if (bat_vol > high_cutoff)//addon 9_9
//!         {
//!            output_high(pin_C2); // On Bat Led ON
//!         }
         output_low(pin_c5);  // Ext Relay OFF
         on_bat = 1;
      }
      /*--------------------------------------------------*/ 
      
      if (ext_vol > low_cutoff && ext_vol < high_cutoff ) 
      {
         on_bat = 0;
          // output_low(pin_C2); // On Bat Led OFF       
      }
         
//!      if (bat_vol > low_cutoff && bat_vol < high_cutoff && on_bat == 1)
//!      {
//!         on_bat = 1;
//!         //output_high(pin_C2); // On Bat Led ON
//!      }
//!            
      if (on_bat == 1)
      {
         output_high(pin_C2); // On Bat Led ON
      }
      else
      {
         output_low(pin_C2); // On Bat Led OFF
      }
      
      /*--------------------------------------------------*/
      
   
      
      /*--------------------------------------------------*/ 
            
      if (bat_vol > high_cutoff && on_bat == 1) //if bat is OK but ext is not OK               //# 4 #//
      {
         output_high(pin_C0); // Bat Relay ON
         output_high(pin_C2); // On Bat Led ON // addon to 9_9
         output_low(pin_c4); // Low OFF
      }
      
      /*--------------------------------------------------*/
      
      if (bat_vol < low_cutoff && on_bat == 1 && shutdown_flag == 0 && pi_state == 1)         //# 5 #//
      {
         //output_high(pin_c4); // Low Voltage Led ON
         //Battery Low so Shutdown Pi
         //Pulse to shutdown Pi
         output_high(pin_a1); //Shutdown to Pi Pin
         //if any supply fail during shutdown ; so turning on both supply
         output_high(pin_c5); // Ext Relay ON 
         output_high(pin_C0); // Bat Relay ON
         // Delay for 1 min for Pi to Shutdown
         for (int16 i = shutdown_delay; i > 0; i=i-10)
         {
               output_high(pin_c4);
               output_high(pin_C2);
               delay_ms(i);
               output_low(pin_c4);
               output_low(pin_C2);
               delay_ms(i);
         }

         //Once Pi is shutdown turn off the power to pi by turning off the bat & ext supply relay
         output_low(pin_C0); // Bat Relay OFF so Pi is turned off
         output_low(pin_c5); // Ext Relay OFF 
         output_low(pin_C2); // On Bat Led Off
         output_high(pin_c4); // Keep Low Voltage Led On [lowww]
         output_low(pin_a1); //Shutdown to Pi Pin OFF
         delay_ms(5000); //after shutdown turn off relay for 5 sec
         shutdown_flag=1;
         pi_state = 0;
         
      }
      
      /*--------------------------------------------------*/
      
      if (bat_vol > low_cutoff && shutdown_flag == 1 && on_bat == 0) // after shutdown        //# 6 #//
      {
         output_high(pin_C0); // Bat Relay ON
         shutdown_flag = 0;
      }
      
      /*--------------------------------------------------*/
      
      // turn on bat relay when both ext and bat power is good 
      if (bat_vol > high_cutoff && on_bat == 0)                                                //# 7 #//
      {
         output_high(pin_C0); // Bat Relay ON
         output_low(pin_c4); // Low Voltage Led OFF
      }
      // turn off bat relay when ext is OK and no bat power
      else if (bat_vol < low_cutoff && on_bat == 0)                                           //# 8 #//
      {
         output_low(pin_C0); // Bat Relay OFF
         output_high(pin_c4); // Low Voltage Led ON 
      }
      
      /*--------------------------------------------------*/
      
      if (input(pin_c3) == 0 && shutdown_flag == 0) // If Shutdown Switch is pressed and if not shutdown           //# 9 #//
      {
         delay_ms(2000);
         if (input(pin_c3) == 0) // Check for Bounce
         {
            //if any supply fail during shutdown ; so turning on both supply
            output_high(pin_C0); // Bat Relay ON
            output_high(pin_c5); // Ext Relay ON
            output_high(pin_a1); //Shutdown to Pi Pin
            // Delay for 1 min for Pi to Shutdown
            for (int16 i = shutdown_delay; i > 0; i=i-10)
            {
               output_high(pin_c4);
               output_high(pin_C2);
               delay_ms(i);
               output_low(pin_c4);
               output_low(pin_C2);
               delay_ms(i);
            }
            //Once Pi is shutdown turn off the power to pi by turned off by swithing off the pi power switch
            output_low(pin_C2); // On Bat Led Off
            output_low(pin_c4); // Low Voltage Led Off
            output_low(pin_a1); // shutdown to pi off
            shutdown_flag=1;
            pi_state = 0;
            if (ext_vol < low_cutoff)
            {
               output_high(pin_C2); // On Bat Led on
            }
         }
      }
   }
}
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

