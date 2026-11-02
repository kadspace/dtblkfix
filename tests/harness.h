// DtBlkFix tests: drive the engine directly, without a host or JUCE.
//
// This file builds against the current engine and against the original one (see
// tests/CMakeLists.txt), so it only uses what both have: the DtBlkFx class, the shim's
// AudioEffectHost, and the globals filled in by DtBlkFxMain.cpp.

#ifndef _DTBLKFIX_TESTS_HARNESS_H_
#define _DTBLKFIX_TESTS_HARNESS_H_

// as every engine source does: windows.h, the plugin base class, VSTGUI, using namespace std
#include <StdAfx.h>

#include <iostream>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>

#include "DtBlkFx.hpp"
#include "fftw_support.h"
#include "rfftw_float.h"

extern std::vector<VstProgram<BlkFxParam::TOTAL_NUM>> g_blk_fx_presets;

// random number state shared by all instances (Smear's random phase), from FxRun1_0.cpp
extern long g_rand_i;

namespace harness {

const double TWO_PI = 6.283185307179586;

// load libfftw3f-3.dll from dir and build the FFT plans, read the factory presets
inline bool init(const char* fftw_dir, const char* presets_file)
{
  std::vector<std::string> paths;
  std::string dir = fftw_dir;
  if (!dir.empty() && dir.back() != '\\' && dir.back() != '/')
    dir += "\\";
  paths.push_back(dir);
  if (!LoadFFTWfDll(paths, &std::cerr))
    return false;
  CreateFFTWfPlans();

  FILE* f = fopen(presets_file, "r");
  if (!f) {
    fprintf(stderr, "can't open %s\n", presets_file);
    return false;
  }
  char line[8192];
  while (fgets(line, sizeof(line), f))
    g_blk_fx_presets.push_back(VstProgram<BlkFxParam::TOTAL_NUM>(line));
  fclose(f);
  return true;
}

// a song transport: tempo, start position and loops/jumps, in samples
struct Transport : public AudioEffectHost {
  bool enabled;
  bool playing;
  double sample_rate;
  double tempo;     // bpm
  double ppq_start; // song position at sample 0
  struct Jump {
    long at;       // absolute sample (engine time) where the song position jumps
    double to_ppq; // new song position (or < 0 to keep going)
    double tempo;  // new tempo (or 0 to keep it)
  };
  std::vector<Jump> jumps; // in order of "at"

  VstTimeInfo ti;
  long pos; // engine sample position of the current buffer

  Transport()
      : enabled(false)
      , playing(true)
      , sample_rate(44100.0)
      , tempo(120.0)
      , ppq_start(0.0)
      , pos(0)
  {
    memset(&ti, 0, sizeof(ti));
  }

  // tempo, song position at the last jump and where that was, for engine sample s
  void segAt(long s, double& seg_tempo, double& seg_ppq, long& seg_start) const
  {
    seg_start = 0;
    seg_ppq = ppq_start;
    seg_tempo = tempo;
    for (size_t i = 0; i < jumps.size(); i++) {
      if (jumps[i].at > s)
        break;
      double spb = 60.0 * sample_rate / seg_tempo;
      double ppq_here = seg_ppq + (double)(jumps[i].at - seg_start) / spb;
      seg_ppq = jumps[i].to_ppq >= 0 ? jumps[i].to_ppq : ppq_here;
      if (jumps[i].tempo > 0)
        seg_tempo = jumps[i].tempo;
      seg_start = jumps[i].at;
    }
  }

  double tempoAt(long s) const
  {
    double t, p;
    long st;
    segAt(s, t, p, st);
    return t;
  }

  // song position (ppq) at engine sample s
  double ppqAt(long s) const
  {
    double t, p;
    long st;
    segAt(s, t, p, st);
    return p + (double)(s - st) * t / (60.0 * sample_rate);
  }

  // engine samples where a beat falls (rounded), in [from, to)
  std::vector<long> beats(long from, long to) const
  {
    std::vector<long> r;
    std::vector<long> starts(1, 0);
    for (size_t i = 0; i < jumps.size(); i++)
      starts.push_back(jumps[i].at);
    starts.push_back(to);
    for (size_t i = 0; i + 1 < starts.size(); i++) {
      double t, p;
      long st;
      segAt(starts[i], t, p, st);
      double spb = 60.0 * sample_rate / t;
      for (double k = ceil(p);; k += 1.0) {
        long b = (long)floor(st + (k - p) * spb + 0.5);
        if (b >= starts[i + 1])
          break;
        if (b >= from && b >= starts[i])
          r.push_back(b);
      }
    }
    return r;
  }

  // a host splits its buffers at a loop point, return how many samples to process from pos
  long clip(long n) const
  {
    for (size_t i = 0; i < jumps.size(); i++)
      if (jumps[i].at > pos && jumps[i].at < pos + n)
        return jumps[i].at - pos;
    return n;
  }

  virtual VstTimeInfo* getTimeInfo(VstInt32 filter)
  {
    if (!enabled)
      return NULL;
    double ppq = ppqAt(pos);
    ti.sampleRate = sample_rate;
    ti.tempo = tempoAt(pos);
    ti.ppqPos = ppq;
    ti.samplePos = ppq * 60.0 * sample_rate / ti.tempo;
    ti.timeSigNumerator = 4;
    ti.timeSigDenominator = 4;
    ti.flags =
        kVstTempoValid | kVstPpqPosValid | kVstTimeSigValid | (playing ? kVstTransportPlaying : 0);
    return &ti;
  }
};

// deterministic stereo test signal: a few partials with vibrato, a chord, clicks and noise
struct Signal {
  uint32_t rnd;
  double t;
  Signal()
      : rnd(12345)
      , t(0)
  {
  }
  float noise()
  {
    rnd = rnd * 1664525u + 1013904223u;
    return (float)((int32_t)rnd) * (1.0f / 2147483648.0f);
  }
  void next(float& l, float& r)
  {
    const double sr = 44100.0;
    double f0 = 110.0 * pow(2.0, 2.0 * sin(t * 0.37));
    double s = 0;
    for (int h = 1; h <= 12; h++)
      s += sin(TWO_PI * f0 * h * t + h) / h;
    double chord =
        sin(TWO_PI * 261.63 * t) + sin(TWO_PI * 329.63 * t + 1) + sin(TWO_PI * 392.0 * t + 2);
    long n = (long)(t * sr + 0.5);
    double click = (n % 11025) < 20 ? 0.8 : 0.0;
    l = (float)(0.25 * s + 0.1 * chord + click + 0.03 * noise());
    r = (float)(0.2 * s - 0.12 * chord + 0.5 * click + 0.03 * noise());
    t += 1.0 / sr;
  }
};

// render n samples of input through fx with the given buffer sizes (cycled)
inline void render(DtBlkFx* fx, Transport* transport, const std::vector<float>& in_l,
                   const std::vector<float>& in_r, std::vector<float>& out_l,
                   std::vector<float>& out_r, const std::vector<long>& buf_sizes,
                   std::vector<long>* blk_starts = NULL)
{
  long n = (long)in_l.size();
  out_l.assign(n, 0.0f);
  out_r.assign(n, 0.0f);
  long pos = 0;
  size_t bi = 0;
  long prev_blk = -1;
  while (pos < n) {
    long len = buf_sizes[bi++ % buf_sizes.size()];
    if (len > n - pos)
      len = n - pos;
    if (transport) {
      transport->pos = pos;
      len = transport->clip(len);
    }
    float* in[2] = {const_cast<float*>(&in_l[pos]), const_cast<float*>(&in_r[pos])};
    float* out[2] = {&out_l[pos], &out_r[pos]};
    fx->processReplacing(in, out, len);
    if (blk_starts && fx->_blk_samp_abs != prev_blk) {
      prev_blk = fx->_blk_samp_abs;
      blk_starts->push_back(prev_blk);
    }
    pos += len;
  }
}

inline DtBlkFx* create(AudioEffectHost* host, long block_size)
{
  DtBlkFx* fx = new DtBlkFx(NULL);

  // The engine never clears its FFT buffers, and WarpMix can read a bin past the end of the
  // spectrum, so its output depends on whatever the allocator left there. Clear them so that
  // two builds see the same memory. (Sizes as allocated in DtBlkFx::DtBlkFx.)
  for (int ch = 0; ch < DtBlkFx::AUDIO_CHANNELS; ch++) {
    memset((float*)fx->_chan[ch].x0, 0, (fx->_x0_sz + MAX_FFT_SZ) * sizeof(float));
    memset((cplxf*)fx->_chan[ch].x1, 0, (MAX_FFT_SZ / 2 + 32 * 2) * sizeof(cplxf));
    memset((float*)fx->_chan[ch].x2, 0, (MAX_FFT_SZ + 32 * 2) * sizeof(float));
  }

  // Smear draws its random phases from one generator shared by every instance, so a render
  // depends on everything rendered before it in the process. Start each one from the same
  // state (as at load).
  g_rand_i = 1;

  fx->setHost(host);
  fx->setSampleRate(44100.0f);
  fx->setBlockSize(block_size);
  fx->resume();
  return fx;
}

// overlap param value with sync turned off and the same amount of overlap
inline float syncOff(float overlap)
{
  return overlap > 0.5f ? 1.0f - overlap : overlap;
}

} // namespace harness

#endif
