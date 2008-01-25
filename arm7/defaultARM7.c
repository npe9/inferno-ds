#include <u.h>
#include "../mem.h"
#include <kern.h>
#include "nds.h"

s32 getFreeSoundChannel(void);
void VblankHandler(void);
void VcountHandler(void);
void poweron(int);

/* So we can initialize our own data section and bss */
extern char bdata[];
extern char edata[];
extern char end[];

void 
memtest(char *a, char c, int n, int read){
	int i;

	for (i=0; i < n; i++){
		if (read)
			c = a[i];
		else{
			a[i] += 1;
			a[i] &= 0x0F;
			a[i] |= c;
		}
	}
}

enum
{
	MaxRetry = 5,
	MaxRange = 30,
};

void
trapinit()
{
	REG_IME = 0;
	readFirmware(0x03FE00,PersonalData,sizeof(PersonalData));
	poweron(POWER_SOUND);
	SOUND_CR = SOUND_ENABLE | SOUND_VOL(0x7F);

	/* dummy read to enable the touchpad PENIRQ */
	readtsc(TscgetX, MaxRetry, MaxRange);
	
	irqInit();
	initclkirq();
	irqset(IRQ_VBLANK, VblankHandler);
	irqen(IRQ_VBLANK);
	VBLANK_INTR_WAIT_FLAGS = 0;
	VBLANK_INTR_WAIT_FLAGS |= IRQ_VBLANK;
	
// 	BUG: something is wrong here
	setytrig(80);
	irqset(IRQ_VCOUNT, VcountHandler);
	irqen(IRQ_VCOUNT);
	VBLANK_INTR_WAIT_FLAGS |= IRQ_VCOUNT;
}

#define DMTEST if(1)memtest
int 
main(int argc, char ** argv)
{
	USED(argc,argv);

	/* fill out the data section by hand */
	memset(edata, 0, end-edata); 		/* clear the BSS */

	DMTEST((char*)(IPC), 0x10, 1, 0);

	trapinit();

	DMTEST((char*)(IPC), 0x20, 1, 0);

	// keep the ARM7 out of main RAM
	while (1){
		swiWaitForVBlank();
		swiDelay(50000);
		DMTEST((char*)(IPC), 0x30, 1, 0);
	}
	return 0;
}

void 
startSound(int sampleRate, const void* data, u32 bytes, u8 channel, u8 vol,  u8 pan, u8 format) 
{
	SCHANNEL_TIMER(channel)  = SOUND_FREQ(sampleRate);
	SCHANNEL_SOURCE(channel) = (u32)data;
	SCHANNEL_LENGTH(channel) = bytes >> 2 ;
	SCHANNEL_CR(channel)     = SCHANNEL_ENABLE | SOUND_ONE_SHOT | SOUND_VOL(vol) | SOUND_PAN(pan) | (format==1?SOUND_8BIT:SOUND_16BIT);
}

s32 
getFreeSoundChannel()
{
	int i;
	for (i=0; i<16; i++) {
		if ( (SCHANNEL_CR(i) & SCHANNEL_ENABLE) == 0 ) return i;
	}
	return -1;
}

void
VcountHandler(void)
{
	touchPosition tp = {0,0,0,0,0, 0};
	static int lastpress = -1;
	uint16 press, x=0, y=0, xpx=0, ypx=0, z1=0, z2=0, batt, aux;
	int t1,t2;
	uint32 temp;

	press = REG_KEYXY;
	if (!((press^lastpress) & Pendown)) {
		touchReadXY(&tp);
		if ( tp.x == 0 || tp.y == 0 ) {
			press |= Pendown;
			lastpress = press;
		} else {
			x = tp.x;
			y = tp.y;
			xpx = tp.px;
			ypx = tp.py;
			z1 = tp.z1;
			z2 = tp.z2;
		}
	} else {
		lastpress = press;
		press |= Pendown;
	}
	
	batt = touchRead(Tscgetbattery);
	aux  = touchRead(Tscgetaux);
	temp = touchReadTemperature(&t1, &t2);

	IPC->touchX			= x; // x/14-24
	IPC->touchY			= y; // y/18-12
	IPC->touchXpx		= xpx;
	IPC->touchYpx		= ypx;
	IPC->touchZ1		= z1;
	IPC->touchZ2		= z2;
	IPC->buttons		= press;
	IPC->battery		= batt;
	IPC->aux			= aux;
	IPC->tdiode1		= t1;
	IPC->tdiode2		= t2;
	IPC->temperature	= temp;
	
	DMTEST((char*)(IPC), 0x60, 1, 0);
	
	// ack. ints
	REG_IF = IRQ_VCOUNT;
	VBLANK_INTR_WAIT_FLAGS |= IRQ_VBLANK;
}

void
VblankHandler(void){
	u32 i;
	TransferSound *snd = IPC->soundData;

	IPC->soundData = 0;	
	if (snd) {
		for (i=0; i<snd->count; i++) {
			s32 chan = getFreeSoundChannel();
			if (chan >= 0) {
				startSound(snd->data[i].rate, snd->data[i].data, snd->data[i].len, chan, snd->data[i].vol, snd->data[i].pan, snd->data[i].format);
			}
		}
	}

	DMTEST((char*)(IPC), 0x50, 1, 0);
	
	// ack. ints
	REG_IF = IRQ_VBLANK;
	VBLANK_INTR_WAIT_FLAGS |= IRQ_VBLANK;
}
