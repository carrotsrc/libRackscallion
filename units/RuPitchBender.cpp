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
#include "RuPitchBender.h"

using namespace RackoonIO;

RuPitchBender::RuPitchBender()
: RackUnit(std::string("RuPitchBender")) { 
	addJack("audio", JACK_SEQ);
	addPlug("audio_out"); 
	workState = IDLE; 
	framesIn = framesOut = nullptr;
	ratio = 1.00; 
	convPeriod = nullptr;
	resampler = nullptr; 
	releasePeriod = nullptr;
	nRemainder = 0;
	MidiExport("pitchBend", RuPitchBender::midiBend); 
}

RuPitchBender::~RuPitchBender() {

	if(framesIn != nullptr) { 
		free(framesIn); 
		free(framesOut); 
	} 
}

void RuPitchBender::overwritePeriod(PcmSample *dst, int value, int count) {
	for(int i = 0; i < count; i++)
		dst[i] = value;
}

void RuPitchBender::actionResample() {
	bufLock.lock();
	int usedFrames;
	convPeriod = cacheAlloc(1);
	//convPeriod = (short*)calloc(nNormal, sizeof(short));

	if(nRemainder) {
		if(nRemainder <= nNormal) {
			if(nFrames) {
				int i;
				for(i = 0; i < nRemainder; i++)
					*(convPeriod+i) = (PcmSample)*(remRead+i);
				remRead = remainder;
			}
		}
		else {
			for(int i = 0; i < nNormal; i++)
				*(convPeriod+i) = (PcmSample)*(remRead+i);

			remRead += nNormal;
			nRemainder -= nNormal;
			bufLock.unlock();
			workState = FLUSH_REMAINDER;
			return;
		}
	}

	if(nRemainder == nNormal) {
		nRemainder = 0;
		bufLock.unlock();
		workState = FLUSH_REMAINDER;
		return;
	}

	nResampled = resample_process(resampler, ratio, framesIn, nFrames, 0, &usedFrames,
					framesOut, nFrames<<1);
	if((nResampled+nRemainder) >= nNormal) {
		// get normalized period and store the remainder
		int i;
		for(i = 0; i < nNormal-nRemainder; i++)
			*(convPeriod+nRemainder+i) = (PcmSample)*(framesOut+i);

		int oldRem = nRemainder;
		nRemainder = (nResampled+nRemainder-nNormal);
		memcpy(remainder, framesOut+(nNormal-oldRem), nRemainder*sizeof(float));
		workState = FLUSH;

	} else {
		memcpy(remainder+nRemainder, framesOut, nResampled*sizeof(float));
		nRemainder = nResampled+nRemainder;
		workState = WAITING;
	}

	bufLock.unlock();

}

FeedState RuPitchBender::feed(Jack *jack) {
	if(!bufLock.try_lock())
		return FEED_WAIT;

	if(workState != READY && workState != WAITING) {
		bufLock.unlock();
		return FEED_WAIT;
	}

	nFrames = jack->frames;
	PcmSample *period;

	jack->flush(&period);
	if(ratio == 1) {
		Jack *out = getPlug("audio_out")->jack;
		out->frames = jack->frames;
		bufLock.unlock();
		return out->feed(period);
	}

	
	if(framesOut == nullptr) {
		framesOut = (float*)malloc(sizeof(float)*(nFrames<<1));
		framesIn = (float*)malloc(sizeof(float)*(nFrames));
		remainder = (float*)malloc(sizeof(float)*(nFrames)<<4);
		nNormal = jack->frames;
		remRead = remainder;
	}
	dd = false;
	if(workState == WAITING){
		dd = true;
	}
	memcpy(framesIn, period, nFrames*sizeof(PcmSample));
	cacheFree(period);
	bufLock.unlock();
	ConcurrentTask(RuPitchBender::actionResample);
	workState = RESAMPLING;

	return FEED_OK;
}

RackState RuPitchBender::init() {
	resampler = resample_open(1, 0.92, 1.08);
	int fwidth = resample_get_filter_width(resampler);
	UnitMsg("Initialised");
	workState = READY;
	return RACK_UNIT_OK;
}

RackState RuPitchBender::cycle() {
	if(workState == FLUSH || workState == FLUSH_REMAINDER) {
		Jack *out = getPlug("audio_out")->jack;
		out->frames = nNormal;
		if(out->feed(convPeriod) == FEED_OK) {
			if(workState == FLUSH_REMAINDER) {
				ConcurrentTask(RuPitchBender::actionResample);
				workState = REMAINDER_WAITING;
			}
			else
				workState = READY;
		}
	}


	return RACK_UNIT_OK;
}

void RuPitchBender::setConfig(string config, string value) {

}

void RuPitchBender::block(Jack *jack) {
	Jack *out = getPlug("audio_out")->jack;
	out->block();
}

void RuPitchBender::midiBend(int value) {
	if(value == 64) {
		ratio = 1;
	}

	if(value < 64) {
		ratio = 1-((64-(double)value)*.00125);
	} else {
		ratio = 1+((64-(127-(double)value))*.00125);
	}
}
