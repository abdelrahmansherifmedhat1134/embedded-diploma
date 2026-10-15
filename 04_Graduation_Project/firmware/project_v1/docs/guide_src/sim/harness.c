/*
 * harness.c - cycle-accurate timing cross-check of the unchanged app.elf in simavr.
 *
 * Emulated around the ATmega32:
 *   I2C : PCF8574 0x20 (lamps) 0x21/0x22 (7-seg) 0x27 (LCD) , 24C08 0x50..0x53 (optional write-cycle busy time)
 *   keypad matrix (rows PA4..PA7 , columns PB0 PB1 PB2 PB4) , buttons PD6 PD7 PB5 , LM35 on ADC0/ADC1 ,
 *   UART terminal (bytes injected 1 frame time apart).
 * Logs (CSV , times in CPU cycles @ 16 MHz):
 *   slots.csv  task,start,dur        one line per served scheduler slot (from the IsTaskDue call that
 *                                    returned 1 to the next IsTaskDue call) - ISR time included
 *   isr.csv    vector,start,dur      vector entry -> reti
 *   twi.csv    start,end,addr,bytes  one line per I2C transaction (START .. STOP)
 *   pins.csv   cycle,pin,value       PA3 PB6 PB7 PD3 changes
 *   uart.txt   cycle<TAB>char        bytes sent by the MCU
 *   events.csv cycle,what            stimulus applied
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sim_avr.h>
#include <sim_elf.h>
#include <sim_irq.h>
#include <avr_twi.h>
#include <avr_uart.h>
#include <avr_ioport.h>
#include <avr_adc.h>
#include "syms.h"

#define F_CPU 16000000UL
#define MS(x) ((avr_cycle_count_t)((x) * (F_CPU / 1000.0)))

static avr_t *avr;
static FILE *f_slots, *f_isr, *f_twi, *f_pins, *f_uart, *f_ev;

/* ---------------- I2C slaves ---------------- */
static avr_irq_t *twi_irq;
static uint8_t ee[1024];
static int ee_delay_ms = 5;
static avr_cycle_count_t ee_busy_until = 0;
static uint8_t pcf[8];
static int sel = 0;       /* 8-bit address of the selected slave , 0 = none */
static int idx = 0;
static uint16_t ee_ptr = 0;
static int ee_wrote = 0;
static avr_cycle_count_t t_start = 0;
static int t_addr = -1, t_n = 0;
static uint8_t t_bytes[32];

static void twi_log_end(void){
	if(t_addr >= 0){
		fprintf(f_twi,"%llu,%llu,%d,%d", (unsigned long long)t_start,(unsigned long long)avr->cycle,t_addr,t_n);
		for(int i=0;i<t_n && i<32;i++) fprintf(f_twi,",%d",t_bytes[i]);
		fprintf(f_twi,"\n");
	}
	t_addr = -1; t_n = 0;
}
static int is_ee(int a7){ return (a7 & 0x7C) == 0x50; }
static int is_pcf(int a7){ return a7==0x20||a7==0x21||a7==0x22||a7==0x27; }

static void twi_hook(struct avr_irq_t *irq, uint32_t value, void *param){
	avr_twi_msg_irq_t v; v.u.v = value;
	if(v.u.twi.msg & TWI_COND_STOP){
		if(sel && is_ee(sel>>1) && ee_wrote && ee_delay_ms>0) ee_busy_until = avr->cycle + MS(ee_delay_ms);
		sel = 0; idx = 0; ee_wrote = 0;
		twi_log_end();
	}
	if(v.u.twi.msg & TWI_COND_START){
		if(t_addr >= 0) twi_log_end();   /* repeated start : keep it in one record */
		sel = 0; idx = 0;
		int a7 = v.u.twi.addr >> 1;
		if(t_addr < 0){ t_start = avr->cycle; t_addr = v.u.twi.addr; t_n = 0; }
		int ack = 0;
		if(is_pcf(a7)) ack = 1;
		if(is_ee(a7) && avr->cycle >= ee_busy_until) ack = 1;
		if(ack){
			sel = v.u.twi.addr;
			if(is_ee(a7) && (sel & 1)==0) ee_ptr = (ee_ptr & 0xFF) | ((a7 & 3) << 8);
			avr_raise_irq(twi_irq + TWI_IRQ_INPUT, avr_twi_irq_msg(TWI_COND_ACK, sel, 1));
		}
	}
	if(sel){
		int a7 = sel >> 1;
		if(v.u.twi.msg & TWI_COND_WRITE){
			avr_raise_irq(twi_irq + TWI_IRQ_INPUT, avr_twi_irq_msg(TWI_COND_ACK, sel, 1));
			if(t_n < 32) t_bytes[t_n] = v.u.twi.data; t_n++;
			if(is_pcf(a7)) pcf[a7 & 7] = v.u.twi.data;
			else {
				if(idx == 0) ee_ptr = ((a7 & 3) << 8) | v.u.twi.data;
				else { ee[ee_ptr] = v.u.twi.data; ee_ptr = (ee_ptr & ~0x0F) | ((ee_ptr + 1) & 0x0F); ee_wrote = 1; }
			}
			idx++;
		}
		if(v.u.twi.msg & TWI_COND_READ){
			uint8_t d = is_pcf(a7) ? pcf[a7 & 7] : ee[ee_ptr];
			if(!is_pcf(a7)) ee_ptr = (ee_ptr + 1) & 0x3FF;
			if(t_n < 32) t_bytes[t_n] = d; t_n++;
			avr_raise_irq(twi_irq + TWI_IRQ_INPUT, avr_twi_irq_msg(TWI_COND_READ, sel, d));
		}
	}
}

static int dbgn=0;
static void st_hook(struct avr_irq_t *irq, uint32_t value, void *param){ if(dbgn++<40) fprintf(stderr,"%.1fus TWSR=%02x\n",avr->cycle/16.0,value); }
/* ---------------- keypad matrix + buttons ---------------- */
static const char KMAT[4][4] = {{'7','8','9','/'},{'4','5','6','*'},{'1','2','3','-'},{'C','0','=','+'}};
static const int COLPIN[4] = {0,1,2,4};   /* PB */
static int key_r = -1, key_c = -1;
static uint8_t portb_out = 0xFF;
static avr_irq_t *pa_irq, *pb_irq, *pd_irq;
static int btn_lvl[3] = {1,1,1};
static uint8_t extA = 0xF0, extB = 0x20, extD = 0xC0;   /* external levels of the input pins we drive */
static void set_ext(char port, uint8_t mask, uint8_t val, avr_irq_t *base){
	avr_ioport_external_t e; e.mask = mask; e.value = val & mask;
	avr_ioctl(avr, AVR_IOCTL_IOPORT_SET_EXTERNAL(port), &e);
	for(int i=0;i<8;i++) if(mask & (1<<i)) avr_raise_irq(base + i, (val>>i)&1);
}

static void keypad_update(void){
	for(int r=0;r<4;r++){
		int lvl = 1;
		if(key_r == r && key_c >= 0 && ((portb_out >> COLPIN[key_c]) & 1) == 0) lvl = 0;
		if(lvl) extA |= (1<<(4+r)); else extA &= ~(1<<(4+r));
	}
	set_ext('A', 0xF0, extA, pa_irq);
}
static void portb_hook(struct avr_irq_t *irq, uint32_t value, void *param){
	int pin = (int)(intptr_t)param;
	if(value) portb_out |= (1<<pin); else portb_out &= ~(1<<pin);
	if(pin==0||pin==1||pin==2||pin==4) keypad_update();
	if(pin==6||pin==7) fprintf(f_pins,"%llu,PB%d,%u\n",(unsigned long long)avr->cycle,pin,value);
}
static void pin_hook(struct avr_irq_t *irq, uint32_t value, void *param){
	fprintf(f_pins,"%llu,%s,%u\n",(unsigned long long)avr->cycle,(const char*)param,value);
}

/* ---------------- UART ---------------- */
static avr_irq_t *uart_in;
static void uart_out_hook(struct avr_irq_t *irq, uint32_t value, void *param){
	fprintf(f_uart,"%llu\t%d\n",(unsigned long long)avr->cycle,(int)value);
}

/* ---------------- scenario ---------------- */
typedef struct { double ms; char kind; char arg[64]; double hold; int val; } act_t;
static act_t acts[4096]; static int nacts = 0;
/* pending timed actions (expanded) */
typedef struct { avr_cycle_count_t at; int type; int a; int b; } ev_t;
static ev_t evs[100000]; static int nevs = 0;
static void add_ev(avr_cycle_count_t at,int type,int a,int b){ evs[nevs].at=at; evs[nevs].type=type; evs[nevs].a=a; evs[nevs].b=b; nevs++; }
static int ev_cmp(const void*x,const void*y){ const ev_t*a=x,*b=y; return (a->at>b->at)-(a->at<b->at); }
enum { E_UART=1, E_KEYDN, E_KEYUP, E_BTN, E_ADC, E_EEDELAY, E_MARK };

int main(int argc, char **argv){
	if(argc < 3){ fprintf(stderr,"usage: harness app.elf scenario.txt [outdir]\n"); return 1; }
	const char *out = argc > 3 ? argv[3] : ".";
	char path[512];
	double end_ms = 10000; int blank = 1;
	FILE *sc = fopen(argv[2],"r"); char line[256];
	while(fgets(line,sizeof line,sc)){
		double t; char kind[16]; char rest[200]="";
		if(line[0]=='#'||line[0]=='\n') continue;
		if(sscanf(line,"end %lf",&t)==1){ end_ms=t; continue; }
		if(strncmp(line,"eeprom keep",11)==0){ blank=0; continue; }
		if(sscanf(line,"eedelay %d",&ee_delay_ms)==1) continue;
		if(sscanf(line,"%lf %15s %199[^\n]",&t,kind,rest)>=2){
			avr_cycle_count_t at = MS(t);
			if(!strcmp(kind,"uart")){   /* uart text , \r = Enter */
				char buf[200]; int n=0;
				for(char*p=rest;*p;p++){ if(*p=='\\'&&p[1]=='r'){buf[n++]='\r';p++;} else if(*p=='\\'&&p[1]=='b'){buf[n++]=8;p++;} else buf[n++]=*p; }
				for(int i=0;i<n;i++) add_ev(at + (avr_cycle_count_t)(i*16640), E_UART, (unsigned char)buf[i], 0); /* 10 bits @ 9615 baud */
				add_ev(at,E_MARK,nacts,0); strncpy(acts[nacts].arg,line,63); nacts++;
			}else if(!strcmp(kind,"keys")){  /* keys <string> : 120 ms down , 120 ms up each */
				double tt=t; for(char*p=rest;*p&&*p!=' ';p++){ add_ev(MS(tt),E_KEYDN,*p,0); add_ev(MS(tt+120),E_KEYUP,0,0); tt+=240; }
				add_ev(at,E_MARK,nacts,0); strncpy(acts[nacts].arg,line,63); nacts++;
			}else if(!strcmp(kind,"btn")){   /* btn onoff|up|down hold_ms */
				char b[16]; double h=150; sscanf(rest,"%15s %lf",b,&h);
				int id = !strcmp(b,"onoff")?0:!strcmp(b,"up")?1:2;
				add_ev(at,E_BTN,id,0); add_ev(MS(t+h),E_BTN,id,1);
				add_ev(at,E_MARK,nacts,0); strncpy(acts[nacts].arg,line,63); nacts++;
			}else if(!strcmp(kind,"adc")){   /* adc ch mV */
				int ch,mv; sscanf(rest,"%d %d",&ch,&mv); add_ev(at,E_ADC,ch,mv);
				add_ev(at,E_MARK,nacts,0); strncpy(acts[nacts].arg,line,63); nacts++;
			}else if(!strcmp(kind,"mark")){
				add_ev(at,E_MARK,nacts,0); strncpy(acts[nacts].arg,line,63); nacts++;
			}
		}
	}
	fclose(sc);
	qsort(evs,nevs,sizeof(ev_t),ev_cmp);

	elf_firmware_t fw; memset(&fw,0,sizeof fw);
	if(elf_read_firmware(argv[1],&fw)){ fprintf(stderr,"elf read failed\n"); return 1; }
	avr = avr_make_mcu_by_name("atmega32");
	if(!avr){ fprintf(stderr,"no atmega32\n"); return 1; }
	avr_init(avr);
	avr->frequency = F_CPU;
	avr_load_firmware(avr,&fw);
	avr->frequency = F_CPU;
	avr->log = getenv("TRACE") ? LOG_TRACE : 0;

	memset(ee, blank ? 0xFF : 0, sizeof ee);
	if(!blank){ FILE*e=fopen("ee.bin","rb"); if(e){ fread(ee,1,1024,e); fclose(e);} }

	sprintf(path,"%s/slots.csv",out); f_slots=fopen(path,"w");
	sprintf(path,"%s/isr.csv",out);   f_isr=fopen(path,"w");
	sprintf(path,"%s/twi.csv",out);   f_twi=fopen(path,"w");
	sprintf(path,"%s/pins.csv",out);  f_pins=fopen(path,"w");
	sprintf(path,"%s/uart.txt",out);  f_uart=fopen(path,"w");
	sprintf(path,"%s/events.csv",out);f_ev=fopen(path,"w");

	/* TWI slave */
	static const char *twin[] = {"twi.in","twi.out","twi.st"};
	twi_irq = avr_alloc_irq(&avr->irq_pool, 0, TWI_IRQ_COUNT, twin);
	avr_irq_register_notify(twi_irq + TWI_IRQ_OUTPUT, twi_hook, NULL);
	avr_connect_irq(avr_io_getirq(avr, AVR_IOCTL_TWI_GETIRQ(0), TWI_IRQ_OUTPUT), twi_irq + TWI_IRQ_OUTPUT);
	avr_connect_irq(twi_irq + TWI_IRQ_INPUT, avr_io_getirq(avr, AVR_IOCTL_TWI_GETIRQ(0), TWI_IRQ_INPUT));

	if(getenv("TWIDBG")) avr_irq_register_notify(avr_io_getirq(avr, AVR_IOCTL_TWI_GETIRQ(0), TWI_IRQ_STATUS), st_hook, NULL);
	/* UART */
	uint32_t fl = 0; avr_ioctl(avr, AVR_IOCTL_UART_GET_FLAGS('0'), &fl);
	fl &= ~AVR_UART_FLAG_STDIO; avr_ioctl(avr, AVR_IOCTL_UART_SET_FLAGS('0'), &fl);
	avr_irq_register_notify(avr_io_getirq(avr, AVR_IOCTL_UART_GETIRQ('0'), UART_IRQ_OUTPUT), uart_out_hook, NULL);
	uart_in = avr_io_getirq(avr, AVR_IOCTL_UART_GETIRQ('0'), UART_IRQ_INPUT);

	/* ports */
	pa_irq = avr_io_getirq(avr, AVR_IOCTL_IOPORT_GETIRQ('A'), 0);
	pb_irq = avr_io_getirq(avr, AVR_IOCTL_IOPORT_GETIRQ('B'), 0);
	pd_irq = avr_io_getirq(avr, AVR_IOCTL_IOPORT_GETIRQ('D'), 0);
	for(int p=0;p<8;p++) avr_irq_register_notify(pb_irq + p, portb_hook, (void*)(intptr_t)p);
	avr_irq_register_notify(pa_irq + 3, pin_hook, "PA3");
	avr_irq_register_notify(pd_irq + 3, pin_hook, "PD3");
	keypad_update();
	set_ext('B',0x20,extB,pb_irq); set_ext('D',0xC0,extD,pd_irq);
	avr_irq_t *adc = avr_io_getirq(avr, AVR_IOCTL_ADC_GETIRQ, 0);
	avr_raise_irq(adc + 0, 250); avr_raise_irq(adc + 1, 400);   /* 25 C , 40 C */

	/* run */
	avr_cycle_count_t end = MS(end_ms);
	int ei = 0;
	int in_isr = 0; uint32_t isr_vec = 0; avr_cycle_count_t isr_t0 = 0;
	int slot = -1; avr_cycle_count_t slot_t0 = 0;
	avr_cycle_count_t first_due = 0;
	while(avr->cycle < end){
		while(ei < nevs && evs[ei].at <= avr->cycle){
			ev_t *e = &evs[ei++];
			switch(e->type){
			case E_UART: avr_raise_irq(uart_in, e->a); break;
			case E_KEYDN: for(int r=0;r<4;r++) for(int c=0;c<4;c++) if(KMAT[r][c]==e->a){key_r=r;key_c=c;} keypad_update(); break;
			case E_KEYUP: key_r=-1; key_c=-1; keypad_update(); break;
			case E_BTN: { btn_lvl[e->a] = e->b;
				if(e->a==2){ if(e->b) extB |= 0x20; else extB &= ~0x20; set_ext('B',0x20,extB,pb_irq); }
				else { uint8_t bit = e->a==0 ? 0x40 : 0x80; if(e->b) extD |= bit; else extD &= ~bit; set_ext('D',0xC0,extD,pd_irq); } } break;
			case E_ADC: avr_raise_irq(adc + e->a, e->b); break;
			case E_MARK: fprintf(f_ev,"%llu,%s",(unsigned long long)avr->cycle,acts[e->a].arg); break;
			}
		}
		uint32_t pc = avr->pc;
		/* ISR bookkeeping */
		if(!in_isr && (pc==A_VEC4||pc==A_VEC13||pc==A_VEC14)){ in_isr=1; isr_vec=pc; isr_t0=avr->cycle; }
		if(in_isr){
			uint16_t op = avr->flash[pc] | (avr->flash[pc+1]<<8);
			if(op == 0x9518){ /* reti : its 4 cycles are counted by stepping once more */
				avr_run(avr);
				int v = isr_vec==A_VEC4?4:isr_vec==A_VEC13?13:14;
				fprintf(f_isr,"%d,%llu,%llu\n",v,(unsigned long long)isr_t0,(unsigned long long)(avr->cycle-isr_t0));
				in_isr=0; continue;
			}
		}
		/* slot bookkeeping : entry of SCHED_u8IsTaskDue.part.0 with r24 = task */
		if(pc == A_ISDUE && !in_isr){
			if(slot >= 0){ fprintf(f_slots,"%d,%llu,%llu\n",slot,(unsigned long long)slot_t0,(unsigned long long)(avr->cycle-slot_t0)); slot=-1; }
			int task = avr->data[24];
			if(avr->data[A_FLAGS] & (1<<task)){ slot = task; slot_t0 = avr->cycle; if(!first_due){ first_due=avr->cycle; fprintf(f_ev,"%llu,first slot served\n",(unsigned long long)first_due);} }
		}
		int st = avr_run(avr);
		if(st == cpu_Done || st == cpu_Crashed){ fprintf(stderr,"cpu state %d at %llu\n",st,(unsigned long long)avr->cycle); break; }
	}
	FILE*e=fopen("ee_out.bin","wb"); fwrite(ee,1,1024,e); fclose(e);
	fclose(f_slots); fclose(f_isr); fclose(f_twi); fclose(f_pins); fclose(f_uart); fclose(f_ev);
	fprintf(stderr,"done at %.1f ms , SCHED overruns = %d\n", avr->cycle/16000.0, avr->data[A_OVR] | (avr->data[A_OVR+1]<<8));
	return 0;
}
