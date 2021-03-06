#include "common.h"
#include "process.h"
#include "x86/memory.h"

ProcessTable idle;
ProcessTable pcb_tb[NR_MAX_PCB];
ProcessTable * current;




extern SegDesc gdt[NR_SEGMENTS];
extern TSS tss;
extern uint32_t __ptr;

#define SECTSIZE 512
#define ELF_START_POS (201*SECTSIZE)

extern uint32_t loader(char *buf,int offset);
extern void readseg(char *address, int count, int offset);
extern void idle_fun();

void initProcess()
{
	idle.state=RUN;
	idle.timeCount=5;
	idle.stackframe.gs=KSEL(SEG_KDATA);
	idle.stackframe.fs=KSEL(SEG_KDATA);
	idle.stackframe.es=KSEL(SEG_KDATA);
	idle.stackframe.ds=KSEL(SEG_KDATA);
	idle.stackframe.cs=KSEL(SEG_KCODE);
	idle.stackframe.eflags=0x202;
	idle.stackframe.esp=0x200000;
	idle.stackframe.ss=KSEL(SEG_KDATA);
	idle.stackframe.eip=(uint32_t)idle_fun;

	// idle.code_seg=SEG(STA_X | STA_R, 0,       0xffffffff, DPL_KERN);
	// idle.data_seg=SEG(STA_W,         0,       0xffffffff, DPL_KERN);
	for(int i=0;i<NR_MAX_PCB;i++)
	{
		pcb_tb[i].state=DEAD;
		pcb_tb[i].stackframe.gs=USEL(SEG_UDATA);
		pcb_tb[i].stackframe.fs=USEL(SEG_UDATA);
		pcb_tb[i].stackframe.es=USEL(SEG_UDATA);
		pcb_tb[i].stackframe.ds=USEL(SEG_UDATA);
		//eip
		pcb_tb[i].stackframe.cs=USEL(SEG_UCODE);
		pcb_tb[i].stackframe.eflags=0x202;
		pcb_tb[i].stackframe.esp=0x200000;
		pcb_tb[i].stackframe.ss=USEL(SEG_UDATA);
		pcb_tb[i].pid=i+1;

		pcb_tb[i].code_seg=SEG(STA_X | STA_R, 4*i*(1<<20),       0xffffffff, DPL_USER);
		pcb_tb[i].data_seg=SEG(STA_W,         4*i*(1<<20),       0xffffffff, DPL_USER);
	}
	/*加载用户程序至内存*/
	char *buf=(char *)0x8000;
	/*read elf from disk*/
	readseg((char*)buf, SECTSIZE, ELF_START_POS);
	pcb_tb[0].stackframe.eip=loader(buf,0);
	pcb_tb[0].state=WAIT;
	current=&idle;
}


void schedule()
{
	// Log("S");
	
	ProcessTable * toFirstBeScheduled=(current==&pcb_tb[0]?&pcb_tb[1]:&pcb_tb[0]);
	ProcessTable * toNextBeScheduled=(toFirstBeScheduled==&pcb_tb[0]?&pcb_tb[1]:&pcb_tb[0]);
	if(WAIT==toFirstBeScheduled->state)
	{
		#ifdef SHOW_PROCESS_CHANGE
			Log("\n%d->%d\n",ProcessTableIndex(current),ProcessTableIndex(toFirstBeScheduled));
		#endif
		current            =toFirstBeScheduled;
		current->timeCount =10;
		current->state     =RUN;
		
		tss.esp0=(uint32_t)((char *)&current->state);
		
		gdt[SEG_UCODE] = current->code_seg;
		gdt[SEG_UDATA] = current->data_seg;
		setGdt(gdt, sizeof(gdt));

		//change the esp to the Process temp's PCB
		__ptr=(uint32_t)&current->stackframe;
		return;
	}

	if(WAIT==toNextBeScheduled->state)
	{
		#ifdef SHOW_PROCESS_CHANGE
			Log("\n%d->%d\n",ProcessTableIndex(current),ProcessTableIndex(toNextBeScheduled));
		#endif

		current            =toNextBeScheduled;
		current->timeCount =10;
		current->state     =RUN;
		
		tss.esp0=(uint32_t)((char *)&current->state);
		
		gdt[SEG_UCODE] = current->code_seg;
		gdt[SEG_UDATA] = current->data_seg;
		setGdt(gdt, sizeof(gdt));

		//change the esp to the Process temp's PCB
		__ptr=(uint32_t)&current->stackframe;
		return;
	}

	#ifdef SHOW_PROCESS_CHANGE
		Log("\n%d->%d\n",ProcessTableIndex(current),-1);
	#endif


	current=&idle;
	current->state=RUN;
	current->timeCount=10;
	__ptr=0;
	idle_fun();
}

int applyANewPcb()
{
	for (int index=0;index<NR_MAX_PCB;index++)
	{
		if(DEAD==pcb_tb[index].state)
			return index;
	}
	assert(0);
	return -1;
}