/*
Copyright (C) 2000, 2001  Ryan Nunn

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

// Midi Player

#ifndef RANDGEN_WIN_MIDIOUT_H
#define RANDGEN_WIN_MIDIOUT_H

#include "xmidi.h"
#include <windows.h>
#include <mmsystem.h>
#include <vector>

class note_data
{
public:
	note_data();
	void clear();
	void play(HMIDIOUT midi_port);
	void show(int tempo);
	void handle_event(midi_event *e);

private:
	unsigned char volume[16];
	unsigned char pan[16];
    unsigned char notechr[16][127];
    int notecol[16][127];
    int first[16][127];
	bool outed;
	double outnext;
};

struct mid_data
{
	midi_event *list;
	int ppqn;
	bool repeat;
	int tempo;
	double ippqn;

	void reset();
	void deleteList();
};

struct play_data
{
	double tick;
	double last_tick;
	double last_time;
	double aim;
	double diff;
	
	midi_event *event;

	// Microsecond Clock
	unsigned int start;

	void wmoInitClock();
	double wmoGetTime();
	void wmoDelay(double mcs_delay);

	// Xmidi Looping
	midi_event *loop_event[XMIDI_MAX_FOR_LOOP_COUNT];
	int loop_count[XMIDI_MAX_FOR_LOOP_COUNT];
	int loop_ticks[XMIDI_MAX_FOR_LOOP_COUNT];
	int loop_num;

	void reset();
	void at_end(mid_data &mid);
	void end_loop_delay();
	bool play_event(HMIDIOUT midi_port, mid_data &current, note_data &nd);
};

class Windows_MidiOut
{
public:
	enum PlayerState
	{
		NotAvailable = 0,
		Starting,
		InitializationFailed,
		Available,
		Playing
	};

	virtual void add_track(midi_event *evntlist, int ppqn, bool repeat);
	virtual PlayerState get_state();
	virtual void wait_state(PlayerState waitState);
	virtual PlayerState wait_any_state(PlayerState *waitStates, int count);

	Windows_MidiOut();
	virtual ~Windows_MidiOut();

private:
	HMIDIOUT midi_port;
	
	HANDLE *thread_handle;
	DWORD thread_id;

	// Thread communications
	PlayerState state;
	CRITICAL_SECTION stateLock;
	CONDITION_VARIABLE stateCond;
	std::vector<mid_data> partList;
	bool partListClosed;
	CONDITION_VARIABLE partListCond;

	// Methods
	static DWORD __stdcall thread_start(void *data);
	bool start_play_thread();
	void set_state(PlayerState newState);
	bool dequeue_part(mid_data *part);
	DWORD thread_main();
	void thread_play();
	void play_part(mid_data &part);
};

#endif // RANDGEN_WIN_MIDIOUT_H