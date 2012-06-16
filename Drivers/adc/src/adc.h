/* Defines */

#define GO	0

#define VREFH	5000.0
#define VREFL	2500.0

#define VON	4000.0

#define	ADCRES	1023.0

/* Globals */

struct acc_type{
	char invert;
	float zeroG;
	unsigned int rate;
	float R;
}acc[3];

unsigned int accRaw[8];

unsigned int latch = 0;
unsigned int counter = 0;

float goTmp=0.0;

/* Function Declerations */

unsigned int checkGo(void);
void initADC(void);
void readADC(void);
void findForce(void);
