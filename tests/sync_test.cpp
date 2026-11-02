// DtBlkFix tests: song sync and latency.
//
// dtblkfix_sync_test <fftw dir> <presets.txt>
//
// 1. With Sync on and a host transport (start offsets, loops, jumps, a tempo change), the
//    output is bit-identical at every buffer size from 32 to 4096, and with mixed sizes.
// 2. After every loop or jump a block starts on each beat (blocks shorter than a beat).
// 3. The reported latency equals the delay measured with an impulse, at every block size.

#include "harness.h"

#include <algorithm>
#include <set>

static int g_failed = 0;

static void check(bool ok, const char* what)
{
  printf("%s: %s\n", ok ? "ok  " : "FAIL", what);
  if (!ok)
    g_failed++;
}

// a factory preset with Sync turned on (keeping its overlap)
static void syncOn(DtBlkFx* fx, int preset)
{
  fx->setProgram(preset);
  float overlap = fx->getParameter(BlkFxParam::OVERLAP);
  if (!BlkFxParam::getBlkSync(overlap))
    fx->setParameter(BlkFxParam::OVERLAP, 1.0f - overlap);
}

struct Case {
  const char* name;
  harness::Transport transport;
};

static std::vector<Case> transports()
{
  std::vector<Case> cases;
  const double sr = 44100.0;

  Case c;
  c.name = "128 bpm from the start";
  c.transport.enabled = true;
  c.transport.tempo = 128.0;
  cases.push_back(c);

  c.name = "128 bpm, starting at beat 3.37";
  c.transport.ppq_start = 3.37;
  cases.push_back(c);

  c.name = "93.7 bpm from beat 1.5, loop beats 4-9 twice, jump to beat 2.5, tempo 140";
  c.transport = harness::Transport();
  c.transport.enabled = true;
  c.transport.tempo = 93.7;
  c.transport.ppq_start = 1.5;
  {
    double spb = 60.0 * sr / 93.7;
    long at = (long)((9.0 - 1.5) * spb);
    harness::Transport::Jump j1 = {at, 4.0, 0.0};
    harness::Transport::Jump j2 = {at + (long)(5.0 * spb), 4.0, 0.0};
    harness::Transport::Jump j3 = {at + (long)(7.3 * spb), 2.5, 0.0};
    harness::Transport::Jump j4 = {at + (long)(10.1 * spb), -1.0, 140.0};
    c.transport.jumps.push_back(j1);
    c.transport.jumps.push_back(j2);
    c.transport.jumps.push_back(j3);
    c.transport.jumps.push_back(j4);
  }
  cases.push_back(c);

  c.name = "120 bpm, stopped at beat 7.25";
  c.transport = harness::Transport();
  c.transport.enabled = true;
  c.transport.playing = false;
  c.transport.ppq_start = 7.25;
  cases.push_back(c);
  return cases;
}

int main(int argc, char** argv)
{
  if (argc < 3) {
    fprintf(stderr, "usage: %s <fftw dir> <presets.txt>\n", argv[0]);
    return 2;
  }
  if (!harness::init(argv[1], argv[2]))
    return 1;

  const long n = 12 * 44100;
  std::vector<float> in_l(n), in_r(n), out_l, out_r, ref_l, ref_r;
  harness::Signal sig;
  for (long i = 0; i < n; i++)
    sig.next(in_l[i], in_r[i]);

  // --- 1. same output at every buffer size
  long sizes[] = {32, 37, 64, 100, 128, 256, 441, 512, 1000, 1024, 2048, 4096};
  long mixed[] = {32, 4096, 100, 777, 1, 2048, 33, 512};
  int presets[] = {0, 7, 12, 23, 26, 33};
  std::vector<Case> cases = transports();
  for (size_t ci = 0; ci < cases.size(); ci++) {
    for (size_t pi = 0; pi < sizeof(presets) / sizeof(presets[0]); pi++) {
      int differ = 0;
      for (size_t si = 0; si <= sizeof(sizes) / sizeof(sizes[0]); si++) {
        std::vector<long> bufs;
        if (si < sizeof(sizes) / sizeof(sizes[0]))
          bufs.push_back(sizes[si]);
        else
          bufs.assign(mixed, mixed + sizeof(mixed) / sizeof(mixed[0]));
        harness::Transport t = cases[ci].transport;
        DtBlkFx* fx = harness::create(&t, 4096);
        syncOn(fx, presets[pi]);
        harness::render(fx, &t, in_l, in_r, out_l, out_r, bufs);
        delete fx;
        if (si == 0) {
          ref_l = out_l;
          ref_r = out_r;
        }
        else if (memcmp(&ref_l[0], &out_l[0], n * 4) || memcmp(&ref_r[0], &out_r[0], n * 4))
          differ++;
      }
      char what[256];
      snprintf(what, sizeof(what), "Sync on, %s, preset %d: identical at buffer sizes 32-4096",
               cases[ci].name, presets[pi]);
      check(differ == 0, what);
    }
  }

  // --- 2. a block starts on every beat
  for (size_t ci = 0; ci < cases.size(); ci++) {
    harness::Transport t = cases[ci].transport;
    DtBlkFx* fx = harness::create(&t, 16);
    syncOn(fx, 0); // 4096 sample blocks, shorter than a beat
    std::vector<long> starts;
    harness::render(fx, &t, in_l, in_r, out_l, out_r, std::vector<long>(1, 16), &starts);
    delete fx;

    std::set<long> blk(starts.begin(), starts.end());
    std::vector<long> beats;
    if (t.playing)
      beats = t.beats(/*after the first block*/ 4096, n - 8192);
    else {
      // stopped: the grid keeps running from where the transport stands
      double spb = 60.0 * 44100.0 / t.tempo;
      for (double k = ceil(t.ppq_start); ; k++) {
        long b = (long)floor((k - t.ppq_start) * spb + 0.5);
        if (b >= n - 8192)
          break;
        if (b >= 4096)
          beats.push_back(b);
      }
    }
    int missed = 0;
    for (size_t i = 0; i < beats.size(); i++)
      if (!blk.count(beats[i])) {
        if (missed < 5)
          printf("     no block at beat sample %ld\n", beats[i]);
        missed++;
      }
    char what[256];
    snprintf(what, sizeof(what), "Sync on, %s: a block starts on all %d beats", cases[ci].name,
             (int)beats.size());
    check(missed == 0 && !beats.empty(), what);
  }

  // --- 3. latency = measured delay
  int plans[] = {0, 4, 9, 12, 16, 17, 20, 21, 24, 28, 33};
  for (int sync = 0; sync < 2; sync++) {
    for (size_t pi = 0; pi < sizeof(plans) / sizeof(plans[0]); pi++) {
      harness::Transport t;
      t.enabled = sync != 0;
      t.tempo = 128.0;
      DtBlkFx* fx = harness::create(&t, 512);
      fx->setProgram(0); // filter at 0 Hz: close to a straight copy
      fx->setParameter(BlkFxParam::FFT_LEN, BlkFxParam::getFFTLenParam(plans[pi]));
      fx->setParameter(BlkFxParam::OVERLAP, sync ? 0.65f : 0.35f);
      long latency = fx->getAeffect()->initialDelay;

      const long len = 6 * 44100, at = 3 * 44100 + 123;
      std::vector<float> imp_l(len, 0.0f), imp_r(len, 0.0f);
      imp_l[at] = 1.0f;
      imp_r[at] = 1.0f;
      harness::render(fx, &t, imp_l, imp_r, out_l, out_r, std::vector<long>(1, 512));
      delete fx;
      long peak = 0;
      for (long i = 0; i < len; i++)
        if (fabsf(out_l[i]) > fabsf(out_l[peak]))
          peak = i;
      char what[256];
      snprintf(what, sizeof(what),
               "Sync %s, %5d sample blocks: latency %ld, measured delay %ld (peak %.3f)",
               sync ? "on " : "off", g_fft_sz[plans[pi]], latency, peak - at, out_l[peak]);
      check(peak - at == latency && latency == g_fft_sz[plans[pi]] && out_l[peak] > 0.5f,
            what);
    }
  }

  printf("%s\n", g_failed ? "FAILED" : "all passed");
  return g_failed ? 1 : 0;
}
