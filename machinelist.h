#ifndef __MACHINELIST_H
#define __MACHINELIST_H

#define POLAC						0

#define MACH_WANT_MIDI				0
#define MACH_MULTI_OUT				1
#define MACH_PVST					2

#define MACH_MULTI_OUT_3			7

typedef int (*MACHINECALLBACK)(CMachineInterface*,int,int,int,float,void*);

void							M_LoadMachines(void);
void							M_FreeMachines(void);
bool							M_IsActive(void);

void							M_Load(CMachineDataInput *pi,GUID *machID);
void							M_Save(CMachineDataOutput *po,GUID *machID);

void							M_Lock(void);
void							M_Unlock(void);

template <class MI>
int MachineCallbackT(CMachineInterface* pMI, int opcode, int iVal, int iVal2, float fVal, void *pV)
{	
	if (HIWORD(opcode)==POLAC)
	{		
		switch (LOWORD(opcode))
		{			
		case MACH_MULTI_OUT_3:
			{				
				MI* p=(MI*)pMI;			
				
				if (iVal)
				{					
					p->outs.AddConnection();

					void** ppV=(void**)iVal;
					*ppV=(void**)&p->om;
				}				
				else
				{					
					p->outs.RemoveConnection();
				}

				return 1;
				
			}
			break;
		}
		
		
	}	
	return -1;
}
//int								MachineCallback(CMachineInterface* pMI, int opCode, int iVal, int iVal2, float fVal, void *pV);

void							M_Offer(int opcode, CMachineInterface* pMI, FARPROC callback, CMachine* pM, CMachineInfo *pInfo, void* pOpt);
int								M_Deoffer(int opcode, CMachineInterface* pMI);

int								M_getListIndex(int n, int opcode, CMachineInterface** ppMI, FARPROC* pCallback, CMachine** ppM, CMachineInfo **ppInfo, void** ppOpt);
int								M_findListElement(int opcode, CMachineInterface* pMI, FARPROC* pCallback, CMachine** ppM, CMachineInfo **ppInfo, void** ppOpt);

#endif
