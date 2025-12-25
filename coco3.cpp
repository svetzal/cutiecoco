/*
Copyright 2015 by Joseph Forgione
This file is part of VCC (Virtual Color Computer).

    VCC (Virtual Color Computer) is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    VCC (Virtual Color Computer) is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with VCC (Virtual Color Computer).  If not, see <http://www.gnu.org/licenses/>.
*/

#include <cstdio>
#include <cmath>
#include <cstring>
#include <string>
#include <iostream>

#include "defines.h"
#include "dream/stubs.h"
#include "BuildConfig.h"
#include "tcc1014graphics.h"
#include "tcc1014registers.h"
#include "mc6821.h"
#include "hd6309.h"
#include "mc6809.h"
// pakinterface.h removed - stubs provide PakTimer
#include "dream/audio.h"
#include "coco3.h"
#include "tcc1014mmu.h"
#include "vcc/utils/logger.h"

// Debug audio tape support disabled for Qt port
#define USE_DEBUG_AUDIOTAPE 0

constexpr auto RENDERS_PER_BLINK_TOGGLE = 16u;

//****************************************
	static double SoundInterupt=0;
	static double NanosToSoundSample=SoundInterupt;
	static double NanosToAudioSample = SoundInterupt;
	static double CyclesPerSecord=(COLORBURST/4)*(TARGETFRAMERATE/FRAMESPERSECORD);
	static double LinesPerSecond= TARGETFRAMERATE * LINESPERSCREEN;
	static double NanosPerLine = NANOSECOND / LinesPerSecond;
	static double HSYNCWidthInNanos = 5000;
	static double CyclesPerLine = CyclesPerSecord / LinesPerSecond;
	static double CycleDrift=0;
	static double CyclesThisLine=0;
	static unsigned int StateSwitch=0;
	unsigned int SoundRate=0;
//*****************************************************

static unsigned char HorzInteruptEnabled=0,VertInteruptEnabled=0;
static unsigned char TopBoarder=0,BottomBoarder=0,TopOffScreen=0,BottomOffScreen=0;
static unsigned char LinesperScreen;
static unsigned char TimerInteruptEnabled=0;
static int MasterTimer=0; 
static unsigned int TimerClockRate=0;
static int TimerCycleCount=0;
static double MasterTickCounter=0,UnxlatedTickCounter=0,OldMaster=0;
static double NanosThisLine=0;
static unsigned char BlinkPhase=1;
static unsigned int AudioBuffer[16384];
static unsigned char CassBuffer[8192];
static unsigned int AudioIndex = 0;
static unsigned int CassIndex = 0;
static unsigned int CassBufferSize = 0;
double NanosToInterrupt=0;
static int IntEnable=0;
static int SndEnable=1;
static int OverClock=1;
static unsigned char SoundOutputMode=0;	//Default to Speaker 1= Cassette
static double emulatedCycles;
double TimeToHSYNCLow = 0;
double TimeToHSYNCHigh = 0;
static unsigned char LastMotorState;
static int AudioFreeBlockCount;

static int clipcycle = 1, cyclewait=2000;
bool codepaste, PasteWithNew = false; 
void AudioOut();
void CassOut();
void CassIn();
void (*AudioEvent)()=AudioOut;
void SetMasterTickCounter();
void (*DrawTopBoarder[4]) (SystemState *)={DrawTopBoarder8,DrawTopBoarder16,DrawTopBoarder24,DrawTopBoarder32};
void (*DrawBottomBoarder[4]) (SystemState *)={DrawBottomBoarder8,DrawBottomBoarder16,DrawBottomBoarder24,DrawBottomBoarder32};
void (*UpdateScreen[4]) (SystemState *)={UpdateScreen8,UpdateScreen16,UpdateScreen24,UpdateScreen32};
void HLINE();
void VSYNC(unsigned char level);
void HSYNC(unsigned char level);

using namespace std;

inline void CPUCycle(double nanoseconds);

void UpdateAudio()
{
#if USE_DEBUG_AUDIOTAPE
	if (CassIndex < CassBufferSize)
	{
		uint8_t sample = LastMotorState ? CassBuffer[CassIndex] : CAS_SILENCE;
		auto& data = gAudioHistory[AudioHistorySize - 1];
		if (gAudioHistoryCount == 0)
		{
			data.inputMin = 0;
			data.inputMax = 0;
		}
		if (gAudioHistoryCount < 5)
		{
			data.inputMin += sample;
			data.inputMax += sample;
		}
		else if (gAudioHistoryCount == 5)
		{
			// begin with average sample (last 6)
			data.inputMin += sample;
			data.inputMax += sample;
			data.inputMin /= 6;
			data.inputMax /= 6;
		}
		else
		{
			// now, keep track of peak to peak volume
			if (sample > data.inputMax) data.inputMax = sample;
			if (sample < data.inputMin) data.inputMin = sample;
		}
		++gAudioHistoryCount;
	}
#endif // USE_DEBUG_AUDIOTAPE

	// keep audio system full by tiny expansion of sound
	if (AudioFreeBlockCount > 1 && (AudioIndex & 63) == 1)
	{
		unsigned int last = AudioBuffer[AudioIndex - 1];
		AudioBuffer[AudioIndex++] = last;
	}
}

void DebugDrawAudio()
{
#if USE_DEBUG_AUDIOTAPE
	static VCC::Pixel col(255, 255, 255);
	col.a = 240;
	for (int i = 0; i < 734; ++i)
	{
		DebugDrawLine(i, 256 - ((AudioBuffer[i] & 0xFFFF) >> 7), i + 1, 256 - ((AudioBuffer[i + 1] & 0xFFFF) >> 7), col);
		DebugDrawLine(750+i, 256 - ((AudioBuffer[i] & 0xFFFF0000) >> 23), 750+i + 1, 256 - ((AudioBuffer[i + 1] & 0xFFFF00000) >> 23), col);
	}

	auto& history = gAudioHistory[AudioHistorySize - 1];
	history.motorState = LastMotorState;
	history.audioState = GetMuxState() == PIA_MUX_CASSETTE;
	gAudioHistoryCount = 0;
	for (int i = 0; i < AudioHistorySize - 1; ++i)
	{
		auto& a = gAudioHistory[i];
		auto& b = gAudioHistory[i + 1];
		DebugDrawLine(i, 500 - a.inputMin, i + 1, 500 - a.inputMax, col);
		DebugDrawLine(i, 520 - 20 * a.motorState, i + 1, 520 - 20 * b.motorState, col);
		DebugDrawLine(i, 550 - 20 * a.audioState, i + 1, 550 - 20 * b.audioState, col);
	}
	memcpy(&gAudioHistory[0], &gAudioHistory[1], sizeof(gAudioHistory) - sizeof(*gAudioHistory));
#endif // USE_DEBUG_AUDIOTAPE
}


float RenderFrame (SystemState *RFState)
{
	static unsigned int FrameCounter=0;
	static bool firstFrame = true;

	if (firstFrame) fprintf(stderr, "RenderFrame: starting first frame\n");

	// once per frame
	LastMotorState = GetMotorState();
	AudioFreeBlockCount = GetFreeBlockCount();

//********************************Start of frame Render*****************************************************

	// Blink state toggle
	if (BlinkPhase++ > RENDERS_PER_BLINK_TOGGLE) {
		TogBlinkState();
		BlinkPhase = 0;
	}

	if (firstFrame) fprintf(stderr, "RenderFrame: VSYNC(0)\n");
	// VSYNC goes Low
	VSYNC(0);

	if (firstFrame) fprintf(stderr, "RenderFrame: first HLINE loop\n");
	// Four lines of blank during VSYNC low
	for (RFState->LineCounter = 0; RFState->LineCounter < 4; RFState->LineCounter++)
	{
		if (firstFrame) fprintf(stderr, "RenderFrame: HLINE %d\n", RFState->LineCounter);
		HLINE();
	}

	if (firstFrame) { fprintf(stderr, "RenderFrame: VSYNC(1)\n"); fflush(stderr); }
	// VSYNC goes High
	VSYNC(1);

	if (firstFrame) { fprintf(stderr, "RenderFrame: second HLINE loop (3 lines)\n"); fflush(stderr); }
	// Three lines of blank after VSYNC goes high
	for (RFState->LineCounter = 0; RFState->LineCounter < 3; RFState->LineCounter++)
	{
		if (firstFrame) fprintf(stderr, "RenderFrame: HLINE2 %d\n", RFState->LineCounter);
		HLINE();
	}
	if (firstFrame) { fprintf(stderr, "RenderFrame: second loop done\n"); fflush(stderr); }

	if (firstFrame) { fprintf(stderr, "RenderFrame: offscreen loop (%d lines)\n", TopOffScreen); fflush(stderr); }
	// Top Border actually begins here, but is offscreen
	for (RFState->LineCounter = 0; RFState->LineCounter < TopOffScreen; RFState->LineCounter++)
	{
		HLINE();
	}
	if (firstFrame) { fprintf(stderr, "RenderFrame: offscreen loop done\n"); fflush(stderr); }

	if (!(FrameCounter % RFState->FrameSkip))
	{
		if (firstFrame) { fprintf(stderr, "RenderFrame: LockScreen\n"); fflush(stderr); }
		if (LockScreen())
			return 0;
	}

	if (firstFrame) { fprintf(stderr, "RenderFrame: top border loop (%d lines), BitDepth=%d\n", TopBoarder, RFState->BitDepth); fflush(stderr); }
	// Visible Top Border begins here. (Remove 4 lines for centering)
	RFState->Debugger.TraceCaptureScreenEvent(VCC::TraceEvent::ScreenTopBorder, 0);
	for (RFState->LineCounter = 0; RFState->LineCounter < TopBoarder; RFState->LineCounter++)
	{
		HLINE();
		if (!(FrameCounter % RFState->FrameSkip))
		{
			if (firstFrame && RFState->LineCounter == 0) { fprintf(stderr, "RenderFrame: DrawTopBoarder[%d] = %p\n", RFState->BitDepth, (void*)DrawTopBoarder[RFState->BitDepth]); fflush(stderr); }
			DrawTopBoarder[RFState->BitDepth](RFState);
		}
	}
	if (firstFrame) { fprintf(stderr, "RenderFrame: top border done\n"); fflush(stderr); }

	// Main Screen begins here: LPF = 192, 200 (actually 199), 225
	RFState->Debugger.TraceCaptureScreenEvent(VCC::TraceEvent::ScreenRender, 0);
	for (RFState->LineCounter = 0; RFState->LineCounter < LinesperScreen; RFState->LineCounter++)		
	{
		HLINE();
		if (!(FrameCounter % RFState->FrameSkip))
			UpdateScreen[RFState->BitDepth](RFState);
	}

	// Bottom Border begins here.
	RFState->Debugger.TraceCaptureScreenEvent(VCC::TraceEvent::ScreenBottomBorder, 0);
	for (RFState->LineCounter=0;RFState->LineCounter < BottomBoarder;RFState->LineCounter++)
	{
		HLINE();
		if (!(FrameCounter % RFState->FrameSkip))
			DrawBottomBoarder[RFState->BitDepth](RFState);
	}

	if (!(FrameCounter % RFState->FrameSkip))
	{
		DrawBottomBoarder[RFState->BitDepth](RFState);
		UnlockScreen(RFState);
		SetBoarderChange();
	}

	// Bottom Border continues but is offscreen
	for (RFState->LineCounter = 0; RFState->LineCounter < BottomOffScreen; RFState->LineCounter++)
	{
		HLINE();
	}

	switch (SoundOutputMode)
	{
	case 1:
		FlushCassetteBuffer(CassBuffer,&CassIndex);
		break;
	}
	FlushAudioBuffer(AudioBuffer, AudioIndex << 2);
	AudioIndex=0;

	DebugDrawAudio();

	// Only affect frame rate if a debug window is open.
	RFState->Debugger.Update();

	return(CalculateFPS());
}

void VSYNC(unsigned char level)
{
	if (level == 0)
	{
		EmuState.Debugger.TraceCaptureScreenEvent(VCC::TraceEvent::ScreenVSYNCLow, 0);
		irq_fs(0);
		if (VertInteruptEnabled)
			GimeAssertVertInterupt();
	}
	else
	{
		EmuState.Debugger.TraceCaptureScreenEvent(VCC::TraceEvent::ScreenVSYNCHigh, 0);
		irq_fs(1);
	}
}

void HSYNC(unsigned char level)
{
	if (level == 0)
	{
		EmuState.Debugger.TraceCaptureScreenEvent(VCC::TraceEvent::ScreenHSYNCLow, 0);
		if (HorzInteruptEnabled)
			GimeAssertHorzInterupt();
		irq_hs(0);
	}
	else
	{
		EmuState.Debugger.TraceCaptureScreenEvent(VCC::TraceEvent::ScreenHSYNCHigh, 0);
		irq_hs(1);
	}
}

void SetClockSpeed(unsigned int Cycles)
{
	OverClock=Cycles;
	return;
}

void SetHorzInteruptState(unsigned char State)
{
	HorzInteruptEnabled= !!State;
	return;
}

void SetVertInteruptState(unsigned char State)
{
	VertInteruptEnabled= !!State;
	return;
}

void SetLinesperScreen (unsigned char Lines)
{
	Lines = (Lines & 3);
	LinesperScreen=Lpf[Lines];
	TopBoarder=VcenterTable[Lines];
	BottomBoarder = 239 - (TopBoarder + LinesperScreen);
	TopOffScreen = TopOffScreenTable[Lines];
	BottomOffScreen = BottomOffScreenTable[Lines];
	return;
}


DisplayDetails GetDisplayDetails(const int clientWidth, const int clientHeight)
{
	const float pixelsPerLine = GetDisplayedPixelsPerLine();
	const float horizontalBorderSize = GetHorizontalBorderSize();
	const float activeLines = 192.0f;	//	FIXME: Needs a symbolic

	DisplayDetails details;

	const auto extraBorderPadding = GetForcedAspectBorderPadding();

	// calcuate the complete screen size including its borders in device coords
	float deviceScreenWidth = (float)clientWidth - extraBorderPadding.x * 2;
	float deviceScreenHeight = (float)clientHeight - extraBorderPadding.y * 2;

	// calculate the content size including the borders in surface coords
	float contentWidth = pixelsPerLine + horizontalBorderSize * 2;
	float contentHeight = activeLines + TopBoarder + BottomBoarder;

	// now get scale difference between both previous equivalent boxes
	float horizontalScale = deviceScreenWidth / contentWidth;
	float verticalScale = deviceScreenHeight / contentHeight;

	// fill in details by scalling the coco screen into device coords
	details.contentRows = static_cast<int>(LinesperScreen * verticalScale);
	details.topBorderRows = static_cast<int>(TopBoarder * verticalScale) + extraBorderPadding.y;
	details.bottomBorderRows = static_cast<int>(BottomBoarder * verticalScale) + extraBorderPadding.y;

	details.contentColumns = static_cast<int>(pixelsPerLine * horizontalScale);
	details.leftBorderColumns = static_cast<int>(horizontalBorderSize * horizontalScale) + extraBorderPadding.x;
	details.rightBorderColumns = static_cast<int>(horizontalBorderSize * horizontalScale) + extraBorderPadding.x;
	
	return details;
}

inline void HLINE()
{
	UpdateAudio();

	// First part of the line
	CPUCycle(NanosPerLine - HSYNCWidthInNanos);

	// HSYNC going low.
	HSYNC(0);
	PakTimer();

	// Run for a bit.
	CPUCycle(HSYNCWidthInNanos);

	// HSYNC goes high
	HSYNC(1);
}

inline void CPUCycle(double NanosToRun)
{
	// CPU is in a halted state.
	if (EmuState.Debugger.IsHalted())
	{
		return;
	}

	EmuState.Debugger.TraceEmulatorCycle(VCC::TraceEvent::EmulatorCycle, 10, NanosToRun, 0, 0, 0, 0);
	NanosThisLine += NanosToRun;
	double emulationCycles = 0, emulationDrift = 0;
	while (NanosThisLine >= 1)
	{
		StateSwitch = 0;
		if ((NanosToInterrupt <= NanosThisLine) & IntEnable)	//Does this iteration need to Timer Interupt
			StateSwitch = 1;
		if ((NanosToSoundSample <= NanosThisLine) & SndEnable)//Does it need to collect an Audio sample
			StateSwitch += 2;
		switch (StateSwitch)
		{
		case 0:		//No interupts this line
			CyclesThisLine = CycleDrift + (NanosThisLine * CyclesPerLine * OverClock / NanosPerLine);
			if (CyclesThisLine >= 1)	//Avoid un-needed CPU engine calls
				CycleDrift = CPUExec((int)floor(CyclesThisLine)) + (CyclesThisLine - floor(CyclesThisLine));
			else
				CycleDrift = CyclesThisLine;
			EmuState.Debugger.TraceEmulatorCycle(VCC::TraceEvent::EmulatorCycle, StateSwitch, NanosThisLine, NanosToInterrupt, NanosToSoundSample, CyclesThisLine, CycleDrift);
			emulationCycles += CyclesThisLine;
			emulationDrift += CycleDrift;
			NanosToInterrupt -= NanosThisLine;
			NanosToSoundSample -= NanosThisLine;
			NanosThisLine = 0;
			break;

		case 1:		//Only Interupting
			NanosThisLine -= NanosToInterrupt;
			CyclesThisLine = CycleDrift + (NanosToInterrupt * CyclesPerLine * OverClock / NanosPerLine);
			if (CyclesThisLine >= 1)
				CycleDrift = CPUExec((int)floor(CyclesThisLine)) + (CyclesThisLine - floor(CyclesThisLine));
			else
				CycleDrift = CyclesThisLine;
			EmuState.Debugger.TraceEmulatorCycle(VCC::TraceEvent::EmulatorCycle, StateSwitch, NanosThisLine, NanosToInterrupt, NanosToSoundSample, CyclesThisLine, CycleDrift);
			emulationCycles += CyclesThisLine;
			emulationDrift += CycleDrift;
			GimeAssertTimerInterupt();
			NanosToSoundSample -= NanosToInterrupt;
			NanosToInterrupt = MasterTickCounter;
			break;

		case 2:		//Only Sampling
			NanosThisLine -= NanosToSoundSample;
			CyclesThisLine = CycleDrift + (NanosToSoundSample * CyclesPerLine * OverClock / NanosPerLine);
			if (CyclesThisLine >= 1)
				CycleDrift = CPUExec((int)floor(CyclesThisLine)) + (CyclesThisLine - floor(CyclesThisLine));
			else
				CycleDrift = CyclesThisLine;
			EmuState.Debugger.TraceEmulatorCycle(VCC::TraceEvent::EmulatorCycle, StateSwitch, NanosThisLine, NanosToInterrupt, NanosToSoundSample, CyclesThisLine, CycleDrift);
			emulationCycles += CyclesThisLine;
			emulationDrift += CycleDrift;
			AudioEvent();
			NanosToInterrupt -= NanosToSoundSample;
			NanosToSoundSample = SoundInterupt;
			break;

		case 3:		//Interupting and Sampling
			if (NanosToSoundSample < NanosToInterrupt)
			{
				NanosThisLine -= NanosToSoundSample;
				CyclesThisLine = CycleDrift + (NanosToSoundSample * CyclesPerLine * OverClock / NanosPerLine);
				if (CyclesThisLine >= 1)
					CycleDrift = CPUExec((int)floor(CyclesThisLine)) + (CyclesThisLine - floor(CyclesThisLine));
				else
					CycleDrift = CyclesThisLine;
				EmuState.Debugger.TraceEmulatorCycle(VCC::TraceEvent::EmulatorCycle, 3, NanosThisLine, NanosToInterrupt, NanosToSoundSample, CyclesThisLine, CycleDrift);
				emulationCycles += CyclesThisLine;
				emulationDrift += CycleDrift;
				AudioEvent();
				NanosToInterrupt -= NanosToSoundSample;
				NanosToSoundSample = SoundInterupt;
				NanosThisLine -= NanosToInterrupt;

				CyclesThisLine = CycleDrift + (NanosToInterrupt * CyclesPerLine * OverClock / NanosPerLine);
				if (CyclesThisLine >= 1)
					CycleDrift = CPUExec((int)floor(CyclesThisLine)) + (CyclesThisLine - floor(CyclesThisLine));
				else
					CycleDrift = CyclesThisLine;
				EmuState.Debugger.TraceEmulatorCycle(VCC::TraceEvent::EmulatorCycle, 4, NanosThisLine, NanosToInterrupt, NanosToSoundSample, CyclesThisLine, CycleDrift);
				emulationCycles += CyclesThisLine;
				emulationDrift += CycleDrift;
				GimeAssertTimerInterupt();
				NanosToSoundSample -= NanosToInterrupt;
				NanosToInterrupt = MasterTickCounter;
				break;
			}

			if (NanosToSoundSample > NanosToInterrupt)
			{
				NanosThisLine -= NanosToInterrupt;
				CyclesThisLine = CycleDrift + (NanosToInterrupt * CyclesPerLine * OverClock / NanosPerLine);
				if (CyclesThisLine >= 1)
					CycleDrift = CPUExec((int)floor(CyclesThisLine)) + (CyclesThisLine - floor(CyclesThisLine));
				else
					CycleDrift = CyclesThisLine;
				EmuState.Debugger.TraceEmulatorCycle(VCC::TraceEvent::EmulatorCycle, 5, NanosThisLine, NanosToInterrupt, NanosToSoundSample, CyclesThisLine, CycleDrift);
				emulationCycles += CyclesThisLine;
				emulationDrift += CycleDrift;
				GimeAssertTimerInterupt();
				NanosToSoundSample -= NanosToInterrupt;
				NanosToInterrupt = MasterTickCounter;
				NanosThisLine -= NanosToSoundSample;
				CyclesThisLine = CycleDrift + (NanosToSoundSample * CyclesPerLine * OverClock / NanosPerLine);
				if (CyclesThisLine >= 1)
					CycleDrift = CPUExec((int)floor(CyclesThisLine)) + (CyclesThisLine - floor(CyclesThisLine));
				else
					CycleDrift = CyclesThisLine;
				EmuState.Debugger.TraceEmulatorCycle(VCC::TraceEvent::EmulatorCycle, 6, NanosThisLine, NanosToInterrupt, NanosToSoundSample, CyclesThisLine, CycleDrift);
				emulationCycles += CyclesThisLine;
				emulationDrift += CycleDrift;
				AudioEvent();
				NanosToInterrupt -= NanosToSoundSample;
				NanosToSoundSample = SoundInterupt;
				break;
			}
			//They are the same (rare)
			NanosThisLine -= NanosToInterrupt;
			CyclesThisLine = CycleDrift + (NanosToSoundSample * CyclesPerLine * OverClock / NanosPerLine);
			if (CyclesThisLine > 1)
				CycleDrift = CPUExec((int)floor(CyclesThisLine)) + (CyclesThisLine - floor(CyclesThisLine));
			else
				CycleDrift = CyclesThisLine;
			EmuState.Debugger.TraceEmulatorCycle(VCC::TraceEvent::EmulatorCycle, 7, NanosThisLine, NanosToInterrupt, NanosToSoundSample, CyclesThisLine, CycleDrift);
			emulationCycles += CyclesThisLine;
			emulationDrift += CycleDrift;
			GimeAssertTimerInterupt();
			AudioEvent();
			NanosToInterrupt = MasterTickCounter;
			NanosToSoundSample = SoundInterupt;
		}
	}
	EmuState.Debugger.TraceEmulatorCycle(VCC::TraceEvent::EmulatorCycle, 20, 0, 0, 0, emulationCycles, emulationDrift);
}

void SetTimerInteruptState(unsigned char State)
{
	TimerInteruptEnabled=State;
	return;
}

void SetInteruptTimer(unsigned int Timer)
{
	UnxlatedTickCounter=(Timer & 0xFFF);
	SetMasterTickCounter();
	IntEnable = 1;  // Gime always sets timer flag when timer expires.  EJJ 25oct24
	return;
}

void SetTimerClockRate (unsigned char Tmp)	//1= 279.265nS (1/ColorBurst) 
{											//0= 63.695uS  (1/60*262)  1 scanline time
	TimerClockRate=!!Tmp;
	SetMasterTickCounter();
	return;
}

void SetMasterTickCounter()
{
	// Rate = { 63613.2315, 279.265 };
	double Rate[2]={NANOSECOND/(TARGETFRAMERATE*LINESPERSCREEN),NANOSECOND/COLORBURST};
	// Master count contains at least one tick. EJJ 10mar25
	MasterTickCounter = (UnxlatedTickCounter+1) * Rate[TimerClockRate];
	if (MasterTickCounter != OldMaster)  
	{
		OldMaster=MasterTickCounter;
		NanosToInterrupt=MasterTickCounter;
	}
	return;
}

void MiscReset()
{
	HorzInteruptEnabled=0;
	VertInteruptEnabled=0;
	TimerInteruptEnabled=0;
	MasterTimer=0; 
	TimerClockRate=0;
	MasterTickCounter=0;
	UnxlatedTickCounter=0;
	OldMaster=0;
//*************************
	SoundInterupt=0;//PICOSECOND/44100;
	NanosToSoundSample=SoundInterupt;
	NanosToAudioSample = SoundInterupt;
	CycleDrift=0;
	CyclesThisLine=0;
	NanosThisLine=0;
	IntEnable=0;
	AudioIndex=0;
	ResetAudio();
	return;
}

unsigned int SetAudioRate (unsigned int Rate)
{

	SndEnable=1;
	SoundInterupt=0;
	CycleDrift=0;

	if (Rate==0)
		SndEnable=0;
	else
	{
		SoundInterupt=NANOSECOND/Rate;
		NanosToSoundSample=SoundInterupt;
		NanosToAudioSample = NANOSECOND/AUDIO_RATE;
	}
	SoundRate=Rate;
	return 0;
}

void AudioOut()
{

	AudioBuffer[AudioIndex++]=GetDACSample();
	return;
}

void CassOut()
{
	if (LastMotorState && CassIndex < sizeof(CassBuffer)/sizeof(*CassBuffer))
		CassBuffer[CassIndex++]=GetCasSample();
	return;
}

//
// Reads the next byte from cassette data stream until end of the tape
// either fast mode, normal or wave data.
//
uint8_t CassInByteStream()
{
	if (CassIndex >= CassBufferSize)
	{
		LoadCassetteBuffer(CassBuffer, &CassBufferSize);
		CassIndex = 0;
	}
	return LastMotorState ? CassBuffer[CassIndex++] : CAS_SILENCE;
}

//
// In fast load mode, byte stream is two samples per bit high & low,
// the type of bit (0 or 1) depends on the period. based on this pass
// the period to basic at address $83.
//
uint8_t CassInBitStream()
{
	uint8_t nextHalfBit = CassInByteStream();
	// set counter time for one lo-hi cycle, basic checks for >18 (0bit) or <18 (1bit)
	MemWrite8(nextHalfBit & 1 ? 10 : 20, 0x83);
	return nextHalfBit >> 1;
}

void CassIn()
{
	// fade ramp state
	static unsigned int fadeTo = 0;
	static unsigned int fade = 0;

	// extract left channel
	auto getLeft = [](auto sample) { return (unsigned long)(sample & 0xFFFF); };
	// extract right channel
	auto getRight = [](auto sample) { return (unsigned long)((sample >> 16) & 0xFFFF); };
	// convert 8 bit to 16 bit stereo (like dac)
	auto monoToStereo = [](uint8_t sample) { return ((uint32_t)sample << 23) | ((uint32_t)sample << 7); };

	// read next sample or same as last if motor is off
	auto casSample =  CassInByteStream();
	SetCassetteSample(casSample);

	// mix two channels dependant on mux (2 x 16bit)
	auto casChannel = monoToStereo(casSample);
	auto dacChannel = GetDACSample();

	// fade time of 125ms, note: this is slow enough to eliminate switching pop but
	// it must be quick too because some games have a nasty habit of toggling the 
	// mux on and off, such as Tuts Tomb.
	const int FADE_TIME = SoundRate / 8;

	// update ramp, always moving towards correct channel  
	fade = fade < fadeTo ? fade + 1 : fade > fadeTo ? fade - 1 : fade;

	// if mux changed, start transition
	fadeTo = FADE_TIME * (GetMuxState() == PIA_MUX_CASSETTE ? 1 : 0);

	// mix audio level between device channels
	auto left = (getLeft(casChannel) * fade + getLeft(dacChannel) * (FADE_TIME - fade)) / FADE_TIME;
	auto right = (getRight(casChannel) * fade + getRight(dacChannel) * (FADE_TIME - fade)) / FADE_TIME;
	auto sample = (unsigned int)(left + (right << 16));

	while (NanosToAudioSample > 0)
	{
		AudioBuffer[AudioIndex++] = sample;
		NanosToAudioSample -= NANOSECOND / AUDIO_RATE;
	}
	NanosToAudioSample += SoundInterupt;
}



void SetSndOutMode(unsigned char Mode)  //0 = Speaker 1= Cassette Out 2=Cassette In
{
	static unsigned char LastMode=0;

	if (Mode == LastMode) return;

	switch (Mode)
	{
	case 0:
		if (LastMode==1)	//Send the last bits to be encoded
			FlushCassetteBuffer(CassBuffer,&CassIndex);

		AudioEvent=AudioOut;
		SetAudioRate(SoundRate);
		break;

	case 1:
		AudioEvent=CassOut;
		SetAudioRate(GetTapeRate());
		break;

	case 2:
		AudioEvent=CassIn;
		SetAudioRate(GetTapeRate());
		break;
	}

	LastMode=Mode;
	SoundOutputMode=Mode;
}

// Clipboard paste - stubbed for Qt port (will use Qt clipboard API)
void PasteText() {
	// TODO: Implement with Qt clipboard
}

void QueueText(const char * text) {
	// TODO: Implement keyboard queue for Qt port
	(void)text;
}

// Clipboard/copy/paste functions stubbed for Qt port
// TODO: Implement with Qt clipboard API

void CopyText() {
	// TODO: Implement with Qt clipboard
}

void PasteBASIC() {
	// TODO: Implement with Qt clipboard
}

void PasteBASICWithNew() {
	// TODO: Implement with Qt clipboard
}
