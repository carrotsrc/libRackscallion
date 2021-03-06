/* Copyright 2015 Charlie Fyvie-Gauld
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published 
 *  by the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "RuChannelMixer.h"
#define MIXER_C1_BUF 1
#define MIXER_C2_BUF 2

#define MIXER_C1_ACT 4
#define MIXER_C2_ACT 8

#define MIXER_BUFFER 64

#define C1_FULL (mixerState&MIXER_C1_BUF)
#define C2_FULL (mixerState&MIXER_C2_BUF)
#define MIXER_FULL (mixerState&MIXER_BUFFER)

using namespace RackoonIO;
RuChannelMixer::RuChannelMixer()
: RackUnit(std::string("RuChannelMixer")) {
	addJack("channel_1", JACK_SEQ);
	addJack("channel_2", JACK_SEQ);
	addPlug("audio_out");
	mixedPeriod = periodC1 = periodC2 = nullptr;
	gainC1 = gainC2 = 1.0;
	mixerState = MIXER_C2_ACT|MIXER_C1_ACT;
	MidiExport("channelFade", RuChannelMixer::midiFade);
}


FeedState RuChannelMixer::feed(Jack *jack) {
	PcmSample *period;

	Jack *out = getPlug("audio_out")->jack;
	out->frames = jack->frames;

	if(MIXER_FULL) {
		if(out->feed(mixedPeriod) == FEED_WAIT)
			return FEED_WAIT;
		mixerState ^= MIXER_BUFFER;
	}

	// could be stale data here
	if(jack->name == "channel_1") {
		if(mixerState&MIXER_C1_ACT)
			mixerState^=MIXER_C1_ACT;

		if(mixerState&MIXER_C2_ACT) {
			jack->flush(&period);
			return out->feed(period);
		}

		if( C1_FULL ) {
			return FEED_WAIT;
		} else {
			jack->flush(&periodC1);
			mixerState ^= MIXER_C1_BUF;
		}

	} else {

		if(mixerState&MIXER_C2_ACT)
			mixerState^=MIXER_C2_ACT;

		if(mixerState&MIXER_C1_ACT) {
			jack->flush(&period);
			return out->feed(period);
		}

		if( C2_FULL ) {
			return FEED_WAIT;
		} else {
			jack->flush(&periodC2);
			mixerState ^= MIXER_C2_BUF;
		}
	}

	if(!C1_FULL || !C2_FULL) {
		return FEED_OK;
	} else {
		mixedPeriod = cacheAlloc(1);
		for(int i = 0; i < jack->frames; i++) {
			mixedPeriod[i] = ((periodC1[i]) * gainC1) +
					 ((periodC2[i]) * gainC2);
		}

		cacheFree(periodC1);
		cacheFree(periodC2);
		mixerState ^= (MIXER_BUFFER ^ MIXER_C1_BUF ^ MIXER_C2_BUF );

		if(out->feed(mixedPeriod) == FEED_OK) {
			mixerState ^= MIXER_BUFFER;
		}
	}

	return FEED_OK;
}
RackState RuChannelMixer::init() {
	workState = READY;
	UnitMsg("Initialised");
	return RACK_UNIT_OK;
}

RackState RuChannelMixer::cycle() {
	return RACK_UNIT_OK;
}

void RuChannelMixer::setConfig(string config, string value) {

}

void RuChannelMixer::block(Jack *jack) {
	Jack *out = getPlug("audio_out")->jack;
	UnitMsg("Block from " << jack->name);
	if(jack->name == "channel_1")
		mixerState ^= MIXER_C1_ACT;
	else
		mixerState ^= MIXER_C2_ACT;

	if(mixerState&MIXER_C1_ACT && mixerState&MIXER_C2_ACT)
		out->block();

}

void RuChannelMixer::midiFade(int value) {
	if(value == 64) {
		gainC1 = gainC2 = 1.0;
	}
	else
	if(value > 64) {
		// right channel open
		// left channel closing
		gainC1 = 1.0;
		gainC2 = (1-(100-((127-(float)value)/63)*100)/100);
	} else
	if(value < 64) {
		// right channel closing
		// left channel open
		gainC1 = (((float)value/64)*100)/100;
		gainC2 = 1.0;
	}
}
