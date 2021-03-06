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
#ifndef RULEVELS_H
#define RULEVELS_H

#include "pconfig.h"
#include "framework/rack/RackUnit.h"
class RuLevels : public RackoonIO::RackUnit
{
	enum WorkState {
		IDLE,
		INIT,
		READY
	};

	int zs;
	WorkState workState;
	snd_pcm_t *handle;
	float masterGain;
	float fadeGain;
	PcmSample *finalPeriod;
	bool processed;

	std::function<void(int)> onFadeChange;
	std::function<void(int)> onGainChange;

public:
	RuLevels();
	RackoonIO::FeedState feed(RackoonIO::Jack*);
	void setConfig(string,string);

	RackoonIO::RackState init();
	RackoonIO::RackState cycle();
	void block(RackoonIO::Jack*);

	void midiMasterGain(int);
	void midiFadeGain(int);

	void cbFadeChange(std::function<void(int)>);
	void cbGainChange(std::function<void(int)>);
};
#endif
