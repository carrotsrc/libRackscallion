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
#ifndef RUPITCHBENDER_H
#define RUPITCHBENDER_H
#include "pconfig.h"
#include "framework/rack/RackUnit.h"
#include <libresample.h>
class RuPitchBender : public RackoonIO::RackUnit {
	enum WorkState {
		IDLE,
		INIT,
		READY,
		WAITING,
		RESAMPLING,
		FLUSH,
		FLUSH_REMAINDER,
		REMAINDER_WAITING
	};
	WorkState workState;
	int nResampled, nFrames, nRemainder, nNormal;
	PcmSample *convPeriod, *releasePeriod;
	float *framesIn, *framesOut, *remainder, *remRead;
	double ratio;
	void *resampler;
	void actionResample();

	void overwritePeriod(PcmSample *, int, int);

	std::mutex bufLock;

	void midiBend(int);
	bool dd;
public:
	RuPitchBender();
	~RuPitchBender();
	RackoonIO::FeedState feed(RackoonIO::Jack*);
	void setConfig(string,string);

	RackoonIO::RackState init();
	RackoonIO::RackState cycle();
	void block(RackoonIO::Jack*);
};
#endif
