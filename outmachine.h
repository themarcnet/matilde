#ifndef __OUTMACHINE_H
#define __OUTMACHINE_H

#include "windows.h"

#define POUT_MAX_OUTS				32

//Modes
#define POUT_MODE_LR				0
#define POUT_MODE_INTERLEAVED		1

#define POUT_MODE_USE_IN_GAIN		2


#define POUT_MONO					1
#define POUT_STEREO					2

struct OUTPUT
{		
	OUTPUT()
	{
		ZeroMemory(outputName,sizeof(outputName));
		channelR=channelL=0;
	};

	char outputName[30];

	float *channelL;
	float *channelR;	
};

struct OUTPUTS_INFO
{	
	char *machName;
	unsigned int maxOuts;	
	unsigned int bufSize;
	int	mode;	
};

class COutputsImp;
class COutputs
{

public:
	
	COutputs(OUTPUTS_INFO *o);
	~COutputs();

	OUTPUT *AddOutput(char *name, int mode);
	BOOL RemoveOutput(void);			

	void ResetOutputs(void);
		
	unsigned int GetNumOutputs(void);
	
	char *SetName(unsigned int n, char *c);
	char *GetName(unsigned int n);
		
	float **GetBuffers(void);
	
	BOOL SetReadPos(unsigned int n);
	unsigned int GetReadPos(void);

	OUTPUT *GetOutput(unsigned int n);

	void DoTick(void);
	BOOL DoWork(float *psamples, int numsamples);

	BOOL SetOutputsChanged(void);

	unsigned int AddConnection(void);
	unsigned int RemoveConnection(void);

	unsigned int GetNumConnections(void);

	BOOL SetMachineName(char *name);
	char *GetMachineName(void);

	int GetMode(void);

private:

	COutputsImp				*pOI;
				
};

struct OUTS
{
	COutputs *outs;
};

#endif