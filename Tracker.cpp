
/*
[22:45] <WhiteNoiz> inline float mi::Filter(float input)
[22:45] <WhiteNoiz> {
[22:45] <WhiteNoiz>   if (Cutoff>0.999f) Cutoff=0.999f;		// necessary?
[22:45] <WhiteNoiz>   float fa = float(1.0 - Cutoff);
[22:45] <WhiteNoiz>   float fb = float(Reso * (1.0 + (1.0/fa)));
[22:45] <WhiteNoiz>   buf0 = fa * buf0 + Cutoff * (input + fb * (buf0 - buf1));
[22:45] <WhiteNoiz>   buf1 = fa * buf1 + Cutoff * buf0;
[22:45] <WhiteNoiz>   return buf1;
[22:45] <WhiteNoiz> }
[
*/

#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <float.h>

#include <windowsx.h>
#include <windows.h>

#include "Resource.h"
#include "Tracker.h"
#include "Surfs DSP Lib/SRF_DSP.h"

#pragma optimize ("a", on)

#define TRACKER_LOCK(pCB) CMILock __machinelock(pCB)

HINSTANCE dllInstance;

// -----------------  GLOBAL  -----------------

#define NumberOfGlobalParameters 4

// new by JM
const CMachineParameter	CMatilde::m_paraAmpDecay=
{
	pt_byte,										// type
	"Ampl.Decay",
	"Volume decay (00-FE)",							// description
	0,												// MinValue
	0xFE,											// MaxValue
	0xFF,											// NoValue
	MPF_STATE,										// Flags
	0
};

// new by JM
const CMachineParameter	CMatilde::m_paraPercOffset=
{
	pt_byte,										// type
	"Offset",
	"Sample offset for percussion / breakbeats (00-FE)",	// description
	0,												// MinValue
	0xFE,											// MaxValue
	0xFF,											// NoValue
	MPF_STATE,										// Flags
	0
};

// new by JM
const CMachineParameter	CMatilde::m_paraPercQuantize=
{
	pt_byte,										// type
	"Quantize",
	"SampleOffset - quantization method (1=non-looping percussion)",			// description
	1,												// MinValue
	64,												// MaxValue
	0xFF,											// NoValue
	MPF_STATE,										// Flags
	1
};

// new by JM	--		suggesion: only -5 to +5 semitones
const CMachineParameter	CMatilde::m_paraTuning=
{
	pt_byte,										// type
	"Tuning",
	"Extra tuning (00-FE, 7F=reset)",				// description
	0,												// MinValue
	0xFE,											// MaxValue
	0xFF,											// NoValue
	MPF_STATE,										// Flags
	0x7F											// default value
};

// -----------------  TRACK  -----------------

const CMachineParameter	CMatilde::m_paraNote=
{
	pt_note,										// type
	"Note",
	"Note",											// description
	NOTE_MIN,										// MinValue
	NOTE_MAX,										// MaxValue
	NOTE_NO,										// NoValue
	0,												// Flags
	0
};

const CMachineParameter	CMatilde::m_paraInstrument=
{
	pt_byte,										// type
	"Wave",
	"Wave to use (01-C8)",							// description
	WAVE_MIN,										// MinValue
	WAVE_MAX,										// MaxValue
	WAVE_NO,										// NoValue
	MPF_WAVE|MPF_STATE,								// Flags
	WAVE_MIN
};

const CMachineParameter	CMatilde::m_paraVolume=
{
	pt_byte,										// type
	"Volume",
	"Volume (00-FE)",								// description
	0,												// MinValue
	0xFE,											// MaxValue
	0xFF,											// NoValue
	MPF_STATE,										// Flags
	0
};

const CMachineParameter	CMatilde::m_paraEffect1=
{
	pt_byte,										// type
	"Effect1",
	"Effect #1 (00-FE)",							// description
	0,												// MinValue
	0xFE,											// MaxValue
	0xFF,											// NoValue
	0,												// Flags
	0
};

const CMachineParameter	CMatilde::m_paraArgument1=
{
	pt_byte,										// type
	"Argument1",
	"Argument #1 (00-FF)",							// description
	0,												// MinValue
	0xFF,											// MaxValue
	0,												// NoValue
	0,												// Flags
	0
};

const CMachineParameter	CMatilde::m_paraEffect2=
{
	pt_byte,										// type
	"Effect2",
	"Effect #2 (00-FE)",							// description
	0,												// MinValue
	0xFE,											// MaxValue
	0xFF,											// NoValue
	0,												// Flags
	0
};

const CMachineParameter	CMatilde::m_paraArgument2=
{
	pt_byte,										// type
	"Argument2",
	"Argument #2 (00-FF)",							// description
	0,												// MinValue
	0xFF,											// MaxValue
	0,												// NoValue
	0,												// Flags
	0
};

const CMachineParameter	*	CMatilde::m_pParameters[]=
{
	// track
	&m_paraAmpDecay,
	&m_paraPercOffset,
	&m_paraPercQuantize,
	&m_paraTuning,
	&m_paraNote,
	&m_paraInstrument,
	&m_paraVolume,
	&m_paraEffect1,
	&m_paraArgument1,
	&m_paraEffect2,
	&m_paraArgument2
};
/*
const CMachineParameter	*	CMatilde::m_pGlobalParameters[]=
{
	// global
	&m_paraAmpDecay,
	&m_paraPercOffset
};
*/
const CMachineAttribute	CMatilde::m_attrVolumeRamp=
{
	"Volume Ramp (ms)",
	0,
	5000,
	0,
};

const CMachineAttribute	CMatilde::m_attrVolumeEnvelope=
{
	"Volume Envelope Span (ticks)",
	1,
	1024,
	64,
};

const CMachineAttribute	CMatilde::m_attrMIDIChannel=
{
	"MIDI Channel",
	0,
	16,
	0,
};

const CMachineAttribute	CMatilde::m_attrMIDIVelocitySensitivity=
{
	"MIDI Velocity Sensitivity",
	0,
	256,
	0,
};

const CMachineAttribute	CMatilde::m_attrMIDIWave=
{
	"MIDI Wave",
	WAVE_NO,
	WAVE_MAX,
	WAVE_NO,
};

const CMachineAttribute	CMatilde::m_attrMIDIUsesFreeTracks=
{
	"MIDI Uses Free Tracks",
	0,
	1,
	0,
};

const CMachineAttribute	CMatilde::m_attrFilterMode=
{
	"Filter Mode",
	0,
	2,
	2,
};

const CMachineAttribute	CMatilde::m_attrPitchEnvelopeDepth=
{
	"Pitch Envelope Depth (semitones)",
	0,
	24,
	2,
};

const CMachineAttribute	CMatilde::m_attrVirtualChannels=
{
	"Enable Virtual Channels",
	0,
	1,
	0,
};

const CMachineAttribute	CMatilde::m_attrLongLoopFit=
{
	"Long loop fit factor",
	1,							// min
	16384,						// max
	128,						// default
};

const CMachineAttribute	CMatilde::m_attrOffsetGain=
{
	"Offset volume gain",
	0,							// min
	100,						// max
	10,							// default
};

const CMachineAttribute	CMatilde::m_attrTuningRange=
{
	"Tuning range (0=free)",
	0,							// min
	12,							// max
	5,							// default
};

// new by kibibu - tuning attributes
const CMachineAttribute CMatilde::m_attrOffsetC = { "C offset (cents/10)",0,24000,12000 };
const CMachineAttribute CMatilde::m_attrOffsetCs = { "C# offset (cents/10)",0,24000,12000 };
const CMachineAttribute CMatilde::m_attrOffsetD = { "D offset (cents/10)",0,24000,12000 };
const CMachineAttribute CMatilde::m_attrOffsetDs = { "D# offset (cents/10)",0,24000,12000 };
const CMachineAttribute CMatilde::m_attrOffsetE = { "E offset (cents/10)",0,24000,12000 };
const CMachineAttribute CMatilde::m_attrOffsetF = { "F offset (cents/10)",0,24000,12000 };
const CMachineAttribute CMatilde::m_attrOffsetFs = { "F# offset (cents/10)",0,24000,12000 };
const CMachineAttribute CMatilde::m_attrOffsetG = { "G offset (cents/10)",0,24000,12000 };
const CMachineAttribute CMatilde::m_attrOffsetGs = { "G# offset (cents/10)",0,24000,12000 };
const CMachineAttribute CMatilde::m_attrOffsetA = { "A offset (cents/10)",0,24000,12000 };
const CMachineAttribute CMatilde::m_attrOffsetAs = { "A# offset (cents/10)",0,24000,12000 };
const CMachineAttribute CMatilde::m_attrOffsetB = { "B offset (cents/10)",0,24000,12000 };

const CMachineAttribute	*	CMatilde::m_pAttributes[]=
{
	&m_attrVolumeRamp,
	&m_attrVolumeEnvelope,
	&m_attrMIDIChannel,
	&m_attrMIDIVelocitySensitivity,
	&m_attrMIDIWave,
	&m_attrMIDIUsesFreeTracks,
	&m_attrFilterMode,
	&m_attrPitchEnvelopeDepth,
	&m_attrVirtualChannels,
	&m_attrLongLoopFit,
	&m_attrOffsetGain,
	&m_attrTuningRange,
	// tuning
	&m_attrOffsetC ,
	&m_attrOffsetCs,
	&m_attrOffsetD ,
	&m_attrOffsetDs,
	&m_attrOffsetE ,
	&m_attrOffsetF ,
	&m_attrOffsetFs,
	&m_attrOffsetG ,
	&m_attrOffsetGs,
	&m_attrOffsetA ,
	&m_attrOffsetAs,
	&m_attrOffsetB ,	// 12
	NULL
};

const CMachineInfo	CMatilde::m_MachineInfo =
{
	MT_GENERATOR,							// type
	MI_VERSION,
	MIF_PLAYS_WAVES|MIF_MULTI_IO,			// flags
	1,										// min tracks
	MAX_TRACKS,								// max tracks
	NumberOfGlobalParameters,				// numGlobalParameters
	7,										// numTrackParameters
	m_pParameters,
	24,
	m_pAttributes,
	"Matilde Tracker3",
	"MTrk",									// short name
	"Carsten Sørensen, Joachim, Anders",	// author
	"About..."
};

const CEnvelopeInfo	CMatilde::m_VolumeEnvelope=
{
	"Volume",
	EIF_SUSTAIN|EIF_LOOP
};

const CEnvelopeInfo	CMatilde::m_PanningEnvelope=
{
	"Panning",
	EIF_SUSTAIN|EIF_LOOP
};

const CEnvelopeInfo	CMatilde::m_PitchEnvelope=
{
	"Pitch",
	EIF_SUSTAIN|EIF_LOOP
};

const CEnvelopeInfo	*	CMatilde::m_Envelopes[4]=
{
	&m_VolumeEnvelope,
	&m_PanningEnvelope,
	&m_PitchEnvelope,
	NULL
};

extern "C" {
__declspec(dllexport) CMachineInfo const * __cdecl GetInfo()
{
	return &CMatilde::m_MachineInfo;
}

__declspec(dllexport) CMachineInterface * __cdecl CreateMachine()
{
	return new CMatilde;
}
}

char const *CMatilde::DescribeParam(int const param)
{	
	return NULL;
}

void CMatilde::SetDeletedState(bool deleted)
{
	if(deleted)
	{
		if(m_hDlg)
			DestroyWindow(m_hDlg);
	}
}

CMatilde::CMatilde()//: outs(&oi)
{
	GlobalVals = m_GlobalValues;		// NULL - by jm
	TrackVals = m_TrackValues;
	AttrVals = (int *)&m_Attributes;
	numTracks=0;
	m_iNextMIDITrack=0;
	m_iWaveTrack=-1;
	m_oSustainAllReleases=false;
	m_Wavetable.SetTracker( this );
}

CMatilde::~CMatilde()
{
	if(m_hDlg)
		DestroyWindow(m_hDlg);
}

void CMatilde::Init(CMachineDataInput * const pi)
{
//	TRACKER_LOCK(pCB);

	int	c;
	for( c=0; c<MAX_TRACKS; c++ )
	{
		m_Tracks[c].m_pMachine=this;
		m_Tracks[c].m_pChannel=NULL;
		m_Tracks[c].m_Output = 0;
		m_Tracks[c].Reset();
	}

	for( c=0; c<MAX_CHANNELS; c++ )
	{
		m_Channels[c].m_pMachine=this;
		m_Channels[c].m_pOwner=NULL;
		m_Channels[c].m_oFree=true;
	}

	for( c=0; c<MAX_TRACKS; c+=1 )
		m_Tracks[c].Stop();

	m_iWaveTrack=-1;
	m_oSustainAllReleases=false;

	m_Wavetable.Stop();
	m_oVirtualChannels=false;
	m_hDlg=NULL;

	m_pBuzzMachine=pCB->GetThisMachine();
	pCB->SetMachineInterfaceEx(this);

	if (pi!=NULL)
	{	 
	}
}

CChannel	*	CMatilde::AllocChannel(MTRKOUTPUT* outputchannel)
{
	int	i;
	for( i=0; i<MAX_CHANNELS; i+=1 )
	{
		if( m_Channels[i].m_oFree )
		{
			m_Channels[i].m_oFree=false;
			m_Channels[i].m_Output = outputchannel;
			return &m_Channels[i];
		}
	}

	i=(m_iNextFreeChannel++)&(MAX_CHANNELS-1);
	m_Channels[i].m_oFree=false;
	m_Channels[i].m_Output = outputchannel;

	return &m_Channels[i];
}

void CMatilde::Tick()
{
	for (int c = 0; c < numTracks; c++)
		m_Tracks[c].Tick(m_TrackValues[c],m_GlobalValues[0]);
}

void CMatilde::Stop()
{
	for( int c=0; c<MAX_TRACKS; c+=1 )
		m_Tracks[c].Stop();

	m_iWaveTrack=-1;
	m_oSustainAllReleases=false;

	m_Wavetable.Stop();
}

char const *CMatilde::GetChannelName(bool input, int index) {
	if (input) return 0;

	if (index == 0) return "Mix";

	static char name[256];
	sprintf(name, "Track %i", index);
	return name;
}

void CMatilde::MultiWork(float const * const *inputs, float **outputs, int numsamples)
{
	bool	gotsomething=false;

	for (int c = 0; c < numTracks + 1; c++)
		if (outputs[c] != 0)
			SurfDSPLib::ZeroFloat( outputs[c], numsamples*2 );

	m_iUsedChannels = 0;

	for( int c=0; c<MAX_TRACKS; c++ ) {
		if (m_Tracks[c].m_Output != 0) {
			m_Tracks[c].m_Output->m_oOutGotSomething = false;
			m_Tracks[c].m_Output->m_oMixed = false;
			m_Tracks[c].m_Output->m_Buffer = m_Tracks[c].m_Output->m_OrigBuffer;
		}
	}

	for( int c=0; c<MAX_CHANNELS; c++ )
	{
		bool	newgotsomething=gotsomething;

		if (!m_Channels[c].m_Output) continue;

		bool oNewOutGotSomething = m_Channels[c].m_Output->m_oOutGotSomething;
		m_Channels[c].m_Output->m_iOutLastSample = 0;
		m_Channels[c].m_Output->m_Buffer = m_Channels[c].m_Output->m_OrigBuffer; //m_Output->channelL;

		if( !m_Channels[c].m_oFree )
			m_iUsedChannels += 1;

		if( m_Channels[c].m_pOwner )
		{
			CTrack	*pTrack=m_Channels[c].m_pOwner;

			if( pMasterInfo->PosInTick==0 )
			{
				pTrack->m_iLastTick=0;
				pTrack->m_iLastSample=0;
			}

			int	lastsample=pTrack->m_iLastSample+numsamples;

			while( pTrack->m_iLastSample<lastsample )
			{
				int nextticksample;
				nextticksample=(pTrack->m_iLastTick+1)*pMasterInfo->SamplesPerTick/pTrack->m_iSubDivide;

				if( (nextticksample>=pTrack->m_iLastSample) && (nextticksample<lastsample) )
				{
					if( nextticksample>pTrack->m_iLastSample )
					{
						if (m_Channels[c].m_Output->m_OrigBuffer) {
							if (!m_Channels[c].m_Output->m_oOutGotSomething) {
								oNewOutGotSomething =
									m_Channels[c].Generate_Move(m_Channels[c].m_Output->m_Buffer, nextticksample-pTrack->m_iLastSample );
							} else {
								m_Channels[c].Generate_Add(m_Channels[c].m_Output->m_Buffer, nextticksample-pTrack->m_iLastSample );
							}
							//if( !gotsomething )
							//	newgotsomething=m_Channels[c].Generate_Move( p, nextticksample-pTrack->m_iLastSample );
							//else
							//	m_Channels[c].Generate_Add( p, nextticksample-pTrack->m_iLastSample );
							//p+=(nextticksample-pTrack->m_iLastSample)<<1;
							m_Channels[c].m_Output->m_Buffer+=(nextticksample-pTrack->m_iLastSample)<<1;
						}
					}

					pTrack->m_iLastTick+=1;
					pTrack->Process( pTrack->m_iLastTick );
					pTrack->m_iLastSample=nextticksample;
				}
				else
				{
					if (m_Channels[c].m_Output && m_Channels[c].m_Output->m_OrigBuffer) {
						if (!m_Channels[c].m_Output->m_oOutGotSomething) {
							oNewOutGotSomething|=m_Channels[c].Generate_Move(m_Channels[c].m_Output->m_Buffer, lastsample-pTrack->m_iLastSample );
						} else {
							m_Channels[c].Generate_Add(m_Channels[c].m_Output->m_Buffer, lastsample-pTrack->m_iLastSample );
						}
					}
					//if( !gotsomething )
					//	newgotsomething|=m_Channels[c].Generate_Move( p, lastsample-pTrack->m_iLastSample );
					//else
					//	m_Channels[c].Generate_Add( p, lastsample-pTrack->m_iLastSample );

					pTrack->m_iLastSample=lastsample;
				}
			}
		}
		else
		{
			if (m_Channels[c].m_Output && m_Channels[c].m_Output->m_OrigBuffer) {
				if (!m_Channels[c].m_Output->m_oOutGotSomething) {
					oNewOutGotSomething|=m_Channels[c].Generate_Move(m_Channels[c].m_Output->m_Buffer, numsamples );
				} else {
					m_Channels[c].Generate_Add(m_Channels[c].m_Output->m_Buffer, numsamples );
				}
			}
			//if( !gotsomething )
			//	newgotsomething|=m_Channels[c].Generate_Move( p, numsamples );
			//else
			//	m_Channels[c].Generate_Add( p, numsamples );
		}

		if (m_Channels[c].m_Output)
			m_Channels[c].m_Output->m_oOutGotSomething = oNewOutGotSomething;
		gotsomething|=oNewOutGotSomething;//newgotsomething;
	}

	//if (gotsomething) {
	gotsomething = false;
	bool mixgotsomething = false;
	for( int c=0; c<MAX_TRACKS; c++ ) {
		if (m_Tracks[c].m_Output != 0) {
			if (!m_Tracks[c].m_Output->m_oMixed) {
				if (m_Tracks[c].m_Output->m_oOutGotSomething) {

					float* pdest = outputs[c + 1];
					float* mixdest = outputs[0];

					if (pdest) {
						float* src = m_Tracks[c].m_Output->m_OrigBuffer;//m_Output->channelL;
						for (int i = 0; i < numsamples; i++) {
							*pdest++ = *src++;
							*pdest++ = *src++;
						}
					}
					if (!gotsomething) {
						gotsomething = true;
						if (mixdest) {
							float* src = m_Tracks[c].m_Output->m_OrigBuffer;//m_Output->channelL;
							for (int i = 0; i < numsamples; i++) {
								*mixdest++ = *src++;
								*mixdest++ = *src++;
							}
						}
					} else {
						if (mixdest) {
							float* src = m_Tracks[c].m_Output->m_OrigBuffer;//m_Output->channelL;
							for (int i = 0; i < numsamples; i++) {
								*mixdest++ += *src++;
								*mixdest++ += *src++;
							}
						}
					}
				} else {
					memset(m_Tracks[c].m_Output->m_OrigBuffer, 0, sizeof(float) * 2 * numsamples);
					outputs[c + 1] = 0;
				}
				m_Tracks[c].m_Output->m_oMixed = true;
			}
		}
	}

	if (!gotsomething)
		outputs[0] = 0;

	if( m_hDlg )
	{
		PostMessage(m_hDlg, WM_USER, 0, 0);
	}
}

void CMatilde::Save(CMachineDataOutput * const po)
{
}

void CMatilde::AttributesChanged()
{
	m_oVirtualChannels=m_Attributes.oVirtualChannels?true:false;
	// update the built-in pitches
	for(int i = 0; i < 12; ++i)
	{
		// set up the note pitch table
		m_fNotePitches[i] = float(i) + (float(m_Attributes.iNoteOffsets[i]) / 1000.0f) - 12.0f;
	}
}

BOOL CALLBACK AboutDialog( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
	CMatilde *pMac = (CMatilde*)GetWindowLong(hDlg,GWL_USERDATA);
	char	temp[80];
	switch( msg )
	{
		case WM_USER:
			sprintf( temp, "%d", pMac->m_iUsedChannels);
			Static_SetText( GetDlgItem(hDlg, IDC_CHANNELCOUNT), temp );
			sprintf( temp, "%d", pMac->m_Wavetable.GetUsedSamples() );
			Static_SetText( GetDlgItem(hDlg, IDC_SAMPLECOUNT), temp );
			break;
		case WM_INITDIALOG:
		{
			pMac = (CMatilde *)lParam;
			SetWindowLong(hDlg,GWL_USERDATA,lParam);
			pMac->m_hDlg = hDlg;
#ifdef	MONO
			Static_SetText( hDlg, "About Matilde Tracker (Mono) 1.10rc1" );
#else
			Static_SetText( hDlg, "About Matilde Tracker 1.10rc1" );
#endif
			Edit_SetText( GetDlgItem(hDlg,IDC_EDIT1),
"Matilde Tracker v1.10rc1 is a tracker machine for Buzz which behaves more like "
"Protracker than Jeskola Tracker.\015\012\015\012"
"All Protracker effects that make sense in Buzz are implemented and "
"behave in a similar fashion to their Protracker cousins, so you'll feel right at home.\015\012\015\012"
"Implemented effects:\015\012\015\012"
"00/xy - Arpeggio\015\012\015\012"
"01/xx - Slide up\015\012\015\012"
"02/xx - Slide down\015\012\015\012"
"03/xx - Tone portamento\015\012"
"  If <xx> is zero, keep portamento'ing.\015\012\015\012"
"04/xy - Vibrato\015\012"
"  x=speed, y=depth. If zero, use previous value\015\012\015\012"
"05/xy - Slide panning\015\012"
"  x=amount to slide panning left\015\012"
"  y=amount to slide panning right\015\012\015\012"
"06/xy - Autopan\015\012"
"  x=speed, y=depth. If zero, use previous value\015\012\015\012"
"07/xy - Tremolo\015\012"
"  x=speed, y=depth. If zero, use previous value\015\012\015\012"
"08/xx - Set panning position\015\012"
"  0=left, 80=middle, FF=right\015\012\015\012"
"09/xx - Sample offset\015\012"
"  xx=offset into sample. Unlike Protracker this is not an absolute offset"
" but scales to the whole length of the sample, ie a value of 80 will"
" start from the middle of the sample. If there's no argument, the sample"
" offset will be set right at the end, useful for E8/01.\015\012\015\012"
"0A/xy - Volume slide\015\012"
"  x=amount to slide volume up\015\012"
"  y=amount to slide volume down\015\012\015\012"
"0F/xx - Subdivide amount\015\012"
"  Subdivide amount. This is the same as the Protracker Fxx command, except"
" it doesn't actually change the speed of the song, only the speed of the"
" effects. If the subdivide amount is higher, effects will be updated more"
" often, making them run faster. The default value is 6.\015\012\015\012"
"10/xx - Probability\015\012"
"  Probability for sample being played. 01=will almost certainly not be played,"
" 80=50%, FF=almost certain\015\012"
"  See also command 30.\015\012\015\012"
"11/xx - Loop fit\015\012"
"  Number of ticks the waveform's should take to complete. Changes the frequency of the waveform.\015\012\015\012"
"12/xy - Loop fit with tracking\015\012"
"  Same as 11 but tracks the speed of the song\015\012\015\012"
"13/xy - Auto shuffle\015\012"
"  x=Ticks to shuffle. 2 shuffles every other step.\015\012  y=Shuffle amount. 0=none, F=a full tick\015\012\015\012"
"14/xx - Randomize volume\015\012"
"  xx=Maximum amount the volume will be randomized\015\012\015\012"
"15/xx - Random delay\015\012"
"  xx=Maximum number of subdivision steps the note will be delayed\015\012\015\012"
"16/xx - Randomize pitch\015\012"
"  xx=Maximum number of notches the pitch will be randomized\015\012\015\012"
"17/xx - Harmonic play\015\012"
"  xx=The base frequency will be multiplied by xx\015\012\015\012"
"18/xy - Combined note delay and cut\015\012"
"  x=The subdivision step to trigger the note\015\012"
"  y=The subdivision step to release the note\015\012\015\012"
"19/xy - Sustain pedal\015\012"
"  y=The subdivision step to trigger the command\015\012"
"  x=1 - Depress sustain pedal\015\012"
"  x=2 - Release sustain pedal\015\012\015\012"
"2F/xx - Long loop fit with tracking\015\012"
"  Same as 11 but tracks the speed of the song\015\012"
"  Length is multiplied by 128 for fitting long stuff\015\012\015\012"
"30/xx - Probability, like 10 by without note off\015\012\015\012"
"DC/xx - Note release\015\012"
"  x=subdivision count at which sample is released\015\012\015\012"
"E1/xx - Fine slide up\015\012\015\012"
"E2/xx - Fine slide down\015\012\015\012"
"E4/0x - Set vibrato type\015\012"
"  x=0 - sine, retrig waveform at samplestart\015\012"
"  x=1 - saw, retrig waveform at samplestart\015\012"
"  x=2 - square, retrig waveform at samplestart\015\012"
"  x=4 - sine, don't retrig waveform at samplestart\015\012"
"  x=5 - saw, don't retrig waveform at samplestart\015\012"
"  x=6 - square, don't retrig waveform at samplestart\015\012\015\012"
"E5/xx - Set finetune\015\012"
"  00 = -1/2 halfnote, 80 = 0, FF = ~+1/2 halfnote\015\012\015\012"
"E6/0x - Set panning type\015\012"
"  see E4x for parameter\015\012\015\012"
"E7/0x - Set tremolo type\015\012"
"  see E4x for parameter\015\012\015\012"
"E8/01 - Reverse direction of sample being played\015\012\015\012"
"E9/xx - Retrig sample\015\012"
"  x=subdivision count at which sample is retriggered\015\012\015\012"
"EA/xx - Fine volume slide up\015\012\015\012"
"EB/xx - Fine volume slide down\015\012\015\012"
"EC/xx - Note cutoff\015\012"
"  x=subdivision count at which sample is cut\015\012\015\012"
"ED/xx - Note delay\015\012"
"  Delay samplestart for <x> subdivision steps\015\012\015\012"
"EE/xx - Fine panning slide left\015\012\015\012"
"EF/xx - Fine panning slide right\015\012\015\012"
 );
			ShowWindow(hDlg, TRUE);
			return( TRUE );
		}

		case WM_NCDESTROY :
			pMac->m_hDlg = NULL;
			return FALSE;

        case WM_COMMAND:
		{
            switch( LOWORD( wParam ))
			{
				case IDOK:
				case IDCANCEL:
					DestroyWindow(hDlg);
					return( TRUE );
					break;
			}
            break;
		}
		default:
		{
			return( FALSE );
        }
	}

	return( FALSE );
}



void CMatilde::Command(int const i)
{
	if( i==0 )
	{
		//IX - Changed to modeless dialog, always on top
		CreateDialogParam(
			dllInstance,
			MAKEINTRESOURCE(IDD_DIALOG1),
			GetForegroundWindow(),
			(DLGPROC)AboutDialog,
			(LPARAM)this
			);
	}
}

void CMatilde::MuteTrack(int const i)
{
}

bool CMatilde::IsTrackMuted(int const i) const
{
	return false;
}

void CMatilde::MidiNote(int const channel, int const value, int const velocity)
{
	if( m_Attributes.iMIDIChannel==0 || channel!=m_Attributes.iMIDIChannel-1 )
		return;

	int v2;
	v2=value-24;	// + aval.MIDITranspose-24;

	if( v2/12>9 )
		return;

	int n=((v2/12)<<4) | ((v2%12)+1);
	if( velocity>0 )
	{
		if( m_iNextMIDITrack>=MAX_TRACKS )
			m_iNextMIDITrack=m_Attributes.iMIDIUsesFreeTracks?numTracks:0;

		if( m_Attributes.iMIDIUsesFreeTracks && m_iNextMIDITrack<numTracks )
			m_iNextMIDITrack=numTracks;

		if( m_iNextMIDITrack>=MAX_TRACKS )
			return;

		if( m_Tracks[m_iNextMIDITrack].m_oAvailableForMIDI )
		{
			CTrackVals	tv;
			CGlobalVals	gv;

			tv.note=n;
			tv.instrument=m_Attributes.iMIDIWave;
			tv.effects[0].command=0;
			tv.effects[0].argument=0;
			tv.effects[1].command=0;
			tv.effects[1].argument=0;

			gv.ampdecay = 0xff;
			gv.percoffset = 0xff;
			gv.percquantize = 0xff;
			gv.tuning = 0xff;

			tv.volume = ((velocity*m_Attributes.iMIDIVelocity)>>8)+((256-m_Attributes.iMIDIVelocity)>>1);
			m_Tracks[m_iNextMIDITrack].Tick( tv, gv );
			m_Tracks[m_iNextMIDITrack].m_oAvailableForMIDI=false;
			m_iNextMIDITrack+=1;
		}
	}
	else
	{
		for( int c=m_Attributes.iMIDIUsesFreeTracks?numTracks:0; c<MAX_TRACKS; c++ )
		{
			if( m_Tracks[c].m_iBaseNote==n )
			{
				CTrackVals	tv;
				CGlobalVals	gv;

				tv.note=NOTE_OFF;
				tv.instrument=WAVE_NO;
				tv.volume=0xFF;
				tv.effects[0].command=0;
				tv.effects[0].argument=0;
				tv.effects[1].command=0;
				tv.effects[1].argument=0;

				gv.ampdecay = 0xff;
				gv.percoffset = 0xff;
				gv.percquantize = 0xff;
				gv.tuning = 0xff;

				m_Tracks[c].Tick( tv, gv );
				m_Tracks[c].m_oAvailableForMIDI=true;
				//return;
			}
		}
	}
}

void CMatilde::Event(dword const data)
{
}

char const *CMatilde::DescribeValue(int const param, int const value)
{
	static char txt[20];

	if( param==2 )
	{
		if(value>1)   sprintf(txt, "%d pieces", value);
		if(value<=1)  sprintf(txt, "perc");
		return txt;
	}

	if( param==3 )
	{
		sprintf(txt, "%+d", value-127 );
		return txt;
	}

	if( (param==3+NumberOfGlobalParameters) || (param==5+NumberOfGlobalParameters) )
	{
		switch( value )
		{
			case	0:
				return "Arpeggio";
				break;
			case	1:
				return "Slide up";
				break;
			case	2:
				return "Slide down";
				break;
			case	3:
				return "Portamento";
				break;
			case	4:
				return "Vibrato";
				break;
			case	5:
				return "Slide panning";
				break;
			case	6:
				return "Autopanning";
				break;
			case	7:
				return "Tremolo";
				break;
			case	8:
				return "Panning";
				break;
			case	9:
				return "Offset";
				break;
			case	0xA:
				return "Volume slide";
				break;
			case	0x0F:
				return "Subdivide";
				break;
			case	0x10:
				return "Probability w. note off";
				break;
			case	0x11:
				return "Loop fit";
				break;
			case	0x12:
				return "Loop fit w/tracking";
				break;
			case	0x13:
				return "Auto shuffle";
				break;
			case	0x14:
				return "Randomize volume";
				break;
			case	0x15:
				return "Random delay";
				break;
			case	0x16:
				return "Randomize pitch";
				break;
			case	0x17:
				return "Harmonic";
				break;
			case	0x18:
				return "Note delay and cut";
				break;
			case	0x19:
				return "Sustain pedal";
				break;
			case	0x20:
				return "Set filter cutoff";
				break;
			case	0x21:
				return "Slide cutoff up";
				break;
			case	0x22:
				return "Slide cutoff down";
				break;
			case	0x23:
				return "Set cutoff LFO";
				break;
			case	0x24:
				return "Cutoff LFO";
				break;
			case	0x25:
				return "Fine slide cutoff up";
				break;
			case	0x26:
				return "Fine slide cutoff down";
				break;
			case	0x28:
				return "Set filter resonance";
				break;
			case	0x29:
				return "Slide resonance up";
				break;
			case	0x2A:
				return "Slide resonance down";
				break;
			case	0x2B:
				return "Set resonance LFO";
				break;
			case	0x2C:
				return "Resonance LFO";
				break;
			case	0x2D:
				return "Fine slide rez up";
				break;
			case	0x2E:
				return "Fine slide rez down";
				break;
			case	0x2F:
				return "Long loop fit x 128 (see attributes)";
				break;
			case	0x30:
				return "Probability";
				break;
			case	0xDC:
				return "Note release";
				break;
			case	0xE0:
				return "Set filter type";
				break;
			case	0xE1:
				return "Fine slide up";
				break;
			case	0xE2:
				return "Fine slide down";
				break;
			case	0xE4:
				return "Vibrato type";
				break;
			case	0xE5:
				return "Finetune";
				break;
			case	0xE6:
				return "Panning type";
				break;
			case	0xE7:
				return "Tremolo type";
				break;
			case	0xE8:
				return "Sample direction";
				break;
			case	0xE9:
				return "Retrig";
				break;
			case	0xEA:
				return "Fine volume up";
				break;
			case	0xEB:
				return "Fine volume down";
				break;
			case	0xEC:
				return "Note cut";
				break;
			case	0xED:
				return "Note delay";
				break;
			case	0xEE:
				return "Fine panning left";
				break;
			case	0xEF:
				return "Fine panning right";
				break;
		}
	}
	
	if( param==5)
	{
		const char * wn = pCB->GetWaveName(value);
		sprintf(txt, "%02X.%s", value, wn ? wn : "<empty>");
		return txt;
	}
	
	return NULL;
}

CEnvelopeInfo const **CMatilde::GetEnvelopeInfos()
{
	return &m_Envelopes[0];
}

bool CMatilde::PlayWave(int const wave, int const note, float const volume)
{
	if( m_iNextMIDITrack>=MAX_TRACKS )
		m_iNextMIDITrack=m_Attributes.iMIDIUsesFreeTracks?numTracks:0;

	if( m_Attributes.iMIDIUsesFreeTracks && m_iNextMIDITrack<numTracks )
		m_iNextMIDITrack=numTracks;

	if( m_iNextMIDITrack>=MAX_TRACKS )
		return false;

	if( m_Tracks[m_iNextMIDITrack].m_oAvailableForMIDI )
	{
		CTrackVals	tv;
		CGlobalVals	gv;

		tv.note=note;
		tv.instrument=wave;
		tv.effects[0].command=0;
		tv.effects[0].argument=0;
		tv.effects[1].command=0;
		tv.effects[1].argument=0;

		tv.volume = int(volume*128.0f);

		gv.ampdecay = 0xff;
		gv.percoffset = 0xff;
		gv.percquantize = 0xff;
		gv.tuning = 0xff;

		m_Tracks[m_iNextMIDITrack].Tick( tv, gv );
		m_Tracks[m_iNextMIDITrack].m_oAvailableForMIDI=false;

		m_iWaveTrack=m_iNextMIDITrack;

		m_iNextMIDITrack+=1;

		return true;
	}
	return false;
}

void CMatilde::StopWave()
{
	if( m_iWaveTrack!=-1 )
	{
		CTrackVals	tv;
		CGlobalVals	gv;

		tv.note=NOTE_OFF;
		tv.instrument=WAVE_NO;
		tv.volume=0;
		tv.effects[0].command=0;
		tv.effects[0].argument=0;
		tv.effects[1].command=0;
		tv.effects[1].argument=0;

		gv.ampdecay = 0xff;
		gv.percoffset = 0xff;
		gv.percquantize = 0xff;
		gv.tuning = 0xff;

		m_Tracks[m_iWaveTrack].Tick( tv, gv );
		m_Tracks[m_iWaveTrack].m_oAvailableForMIDI=true;

		m_iWaveTrack=-1;
	}
}

int CMatilde::GetWaveEnvPlayPos(int const env)
{
	if( m_iWaveTrack!=-1 )
	{
		return m_Tracks[m_iWaveTrack].GetWaveEnvPlayPos( env );
	}
	return -1;
}

void CMatilde::SetNumTracks(int const n)
{
//	TRACKER_LOCK(pCB);

	if( n>numTracks )
	{
		int i;
		for( i=numTracks; i<n; i+=1 )
		{
			m_Tracks[i].Reset();
			static char pc[128];
			sprintf(pc, "Track %i", i + 1);
			MTRKOUTPUT* mo = new MTRKOUTPUT();
			//mo->m_Output = outs.AddOutput(pc, POUT_STEREO);
			m_Tracks[i].m_Output = mo;
		}
	} else if (n < numTracks)
	{
		for (int i = 0; i < numTracks - n; i++)
		{
			int tracknum = numTracks - 1 - i;
			for( int c=0; c<MAX_CHANNELS; c++ ) {
				if (m_Channels[c].m_Output == m_Tracks[tracknum].m_Output) {
					m_Channels[c].Reset();
					break;
				}
			}
			delete m_Tracks[tracknum].m_Output;
			m_Tracks[tracknum].m_Output = 0;
		}
	}		

	numTracks = n;
	pCB->SetOutputChannelCount(n + 1);

	// wrap outputs on unused tracks when playing midi notes on tracks > numTracks
	for (int i = numTracks; i < MAX_TRACKS; i++)
		m_Tracks[i].m_Output = m_Tracks[i % numTracks].m_Output;

}

BOOL WINAPI DllMain( HANDLE hModule, DWORD fdwreason, LPVOID lpReserved )
{
	if( fdwreason==DLL_PROCESS_ATTACH )
		dllInstance=(HINSTANCE)hModule;

	return TRUE;
}
