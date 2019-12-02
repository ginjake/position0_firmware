#include <avr/io.h>
#include <avr/interrupt.h>
//#include <util/delay.h>
#include <avr/wdt.h>
#include <limits.h>

#define KEY_NUM 16
#define OUT_NUM 1

#define AVERAGE_NUM 10
//#define THRESHOLD_VAL 4
//#define THRESHOLD_VAL2 20
#define THRESHOLD_VAL 10
#define THRESHOLD_VAL2 90
#define CENSORING_VAL 4096

// PIN ASSIGNE
#define CHARGE_PORT PORTB
#define CHARGE_PIN  PINB0
unsigned char* pin_array[16]= {&PINB,&PINB,&PINB,&PINB,&PINB,&PINB,&PINB,&PINC,&PIND,&PIND,&PIND,&PIND,&PIND,&PIND,&PIND,&PIND};
unsigned char key_array[16] = {PINB1,PINB2,PINB3,PINB4,PINB5,PINB6,PINB7,PINC5,PIND0,PIND1,PIND2,PIND3,PIND4,PIND5,PIND6,PIND7};
unsigned char* out_pin_array[OUT_NUM] = {&PORTC};
unsigned char  out_key_array[OUT_NUM] = {PINC0};

// CALIBRATE


unsigned int   calibrate[KEY_NUM] = {0};
unsigned char  invalid[KEY_NUM] = {0};
unsigned char  map[KEY_NUM] = {0};
unsigned short average[KEY_NUM][AVERAGE_NUM] = {{0}};

void sleep(unsigned int loop){
  volatile unsigned int l = loop;
  while(l>0){
    l--;
  }
}

// Charge
static inline void charge(unsigned short* mesure,unsigned char key){

  unsigned short time;

  mesure[key] = 0;
  CHARGE_PORT &= ~( 1UL << CHARGE_PIN );

  for(time=0; time<CENSORING_VAL ; ++time){
    if(!bit_is_clear(*pin_array[key], key_array[key])){
  	   mesure[key]=time;
       return;
  	}
  }
}

// Discharge
void discharge(){
  CHARGE_PORT |= ( 1UL << CHARGE_PIN );
  sleep(50);
}

void calibrate_val(unsigned short* mesure){
	unsigned char  outlier = 1;
	unsigned char  key;
	unsigned char  num;
    unsigned int   val;

	discharge();
	sleep(5000);
	for(key=0;key<KEY_NUM;key++){
      calibrate[key]=0;
    }

	while(outlier)
	{
      outlier=0;
	  // calibrate
	  for(key=0;key<KEY_NUM;key++){
	    for(num=0;num<AVERAGE_NUM;num++){
	      charge(mesure,key);
		  discharge();
		  sleep(500);
	      average[key][num] = mesure[key];
        }
	    val = 0;
	    for(num=0;num<AVERAGE_NUM;num++){
          val += average[key][num];
        }
//	    val /= AVERAGE_NUM;
		calibrate[key] = (unsigned short)val;
        for(num=0;num<AVERAGE_NUM;num++){
          if((average[key][num]>=((calibrate[key]/AVERAGE_NUM)+THRESHOLD_VAL)) || 
		     ((average[key][num]+THRESHOLD_VAL)<=(calibrate[key]/AVERAGE_NUM))){
            outlier=1;
		  }
        }
      }
	}
    discharge();
	sleep(500);
}


int main(){
	
    unsigned short mesure[KEY_NUM];
	unsigned char  key=0;
	unsigned char  num;
	unsigned int   val;
	unsigned char  index = 0;
	unsigned char  outlier = 1;
	unsigned char  key_select=0;

	cli();
	DDRB =0b00000001;
	PORTB=0b11111111;
    DDRC =0b11000001;
	PORTC=0b11111111;
	DDRD =0b00000000;
	PORTD=0b11111111;
	sei();

    cli();

    calibrate_val(mesure);

    for(key=0;key<KEY_NUM;key++){
	  if(calibrate[key]==0){
	    invalid[key]=1;
	  }
    }

    // main loop
	while(1){
	  for(index=0;index<AVERAGE_NUM;++index){
	    for(key=0;key<KEY_NUM;++key){
          if(invalid[key]){
		    continue;
		  }

          charge(mesure,key);
		  discharge();

          if(mesure[key]<calibrate[key]/AVERAGE_NUM){
            mesure[key]=calibrate[key]/AVERAGE_NUM;
		  }
          average[key][index] = mesure[key];
	      val = 0;
	      for(num=0;num<AVERAGE_NUM;num++){
            val += average[key][num];
          }
//	      val /= AVERAGE_NUM;
		  map[key] = (val >= (calibrate[key]+THRESHOLD_VAL2)) ? 1 : 0; 

          key_select=0;
          key_select |= (1-bit_is_clear(PINC, PINC1));
          key_select |= (1-bit_is_clear(PINC, PINC2)) << 1;
	      key_select |= (1-bit_is_clear(PINC, PINC3)) << 2;
	      key_select |= (1-bit_is_clear(PINC, PINC4)) << 3;
          key_select = key_select % KEY_NUM;


          //key_select = (key_select+1) % KEY_NUM;

          if(map[key_select])
          //if(((float)calibrate[key_select])*THRESHOLD_VAL1 < (float)val)
	      {
	        *out_pin_array[0] = (*out_pin_array[0])|(( 1 << out_key_array[0]));
          }
	      else
	      {
	        *out_pin_array[0] = (*out_pin_array[0])&(~( 1 << out_key_array[0]));
          }
        }
      }
    }

	return 0;
}

