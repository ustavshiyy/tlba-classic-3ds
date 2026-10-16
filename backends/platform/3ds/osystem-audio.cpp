/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "backends/platform/3ds/osystem.h"
#include "audio/mixer.h"

namespace N3DS {

static bool hasAudio = false;
static volatile bool audioExitRequested = false;
static volatile bool audioSleeping = false;
static Thread audioThreadForExit = 0;

void setAudioSleeping(bool sleeping) {
	audioSleeping = sleeping;
}

void requestAudioExit() {
	audioExitRequested = true;
}

void shutdownAudioForExit() {
	requestAudioExit();
	if (hasAudio && audioThreadForExit) {
		// APTHOOK_ONEXIT must not return while the worker still owns a stack:
		// HOME -> X may unmap that stack before the normal backend destructor.
		threadJoin(audioThreadForExit, U64_MAX);
		threadFree(audioThreadForExit);
		audioThreadForExit = 0;
		ndspExit();
		hasAudio = false;
	}
}

static void audioThreadFunc(void *arg) {
	Audio::MixerImpl *mixer = (Audio::MixerImpl *)arg;

	const int channel = 0;
	int bufferIndex = 0;
	// Keep a little more decoded audio queued on 3DS. Scene transitions perform
	// heavy synchronous SD reads; two 1024-sample buffers can underrun briefly
	// while compressed music is competing for the card. A third buffer costs only
	// a few extra KiB and gives the mixer more headroom without adding the
	// latency of a much deeper queue.
	const int bufferCount = 3;
	const int sampleRate = mixer->getOutputRate();
	const int bufferSamples = 1024;
	const int bufferSize = bufferSamples * 4;

	ndspWaveBuf buffers[bufferCount];

	for (int i = 0; i < bufferCount; ++i) {
		memset(&buffers[i], 0, sizeof(ndspWaveBuf));
		buffers[i].data_vaddr = linearAlloc(bufferSize);
		buffers[i].looping = false;
		buffers[i].status = NDSP_WBUF_FREE;
	}

	ndspChnReset(channel);
	ndspChnSetInterp(channel, NDSP_INTERP_LINEAR);
	ndspChnSetRate(channel, sampleRate);
	ndspChnSetFormat(channel, NDSP_FORMAT_STEREO_PCM16);

	while (!audioExitRequested) {
		svcSleepThread(5000 * 1000); // Wake up the thread every 5 ms

		// HOME -> X can start backend teardown while this worker is asleep.
		// Do not dereference OSystem_3DS from the audio thread.
		if (audioExitRequested) {
			break;
		}

		if (audioSleeping) {
			continue;
		}

		ndspWaveBuf *buf = &buffers[bufferIndex];
		if (buf->status == NDSP_WBUF_FREE || buf->status == NDSP_WBUF_DONE) {
			buf->nsamples = mixer->mixCallback(buf->data_adpcm, bufferSize);
			if (buf->nsamples > 0) {
				DSP_FlushDataCache(buf->data_vaddr, bufferSize);
				ndspChnWaveBufAdd(channel, buf);
			}

			bufferIndex++;
			bufferIndex %= bufferCount;
		}
	}

	for (int i = 0; i < bufferCount; ++i)
		linearFree(buffers[i].data_pcm16);
}

void OSystem_3DS::initAudio() {
	audioExitRequested = false;
	audioSleeping = false;

	_mixer = new Audio::MixerImpl(22050);

	hasAudio = R_SUCCEEDED(ndspInit());
	_mixer->setReady(false);

	if (hasAudio) {
		s32 prio = 0;
		svcGetThreadPriority(&prio, CUR_THREAD_HANDLE);
		audioThread = threadCreate(&audioThreadFunc, _mixer, 32 * 1048, prio - 1, -2, false);
		audioThreadForExit = audioThread;
	}
}

void OSystem_3DS::destroyAudio() {
	requestAudioExit();

	if (hasAudio) {
		threadJoin(audioThread, U64_MAX);
		threadFree(audioThread);
		audioThreadForExit = 0;
		ndspExit();
		hasAudio = false;
	}

	delete _mixer;
	_mixer = 0;
}

Audio::Mixer *OSystem_3DS::getMixer() {
	assert(_mixer);
	return _mixer;
}

} // namespace N3DS
