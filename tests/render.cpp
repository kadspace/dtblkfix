// DtBlkFix tests: render every factory preset through the engine and write the output.
//
// dtblkfix_render <fftw dir> <presets.txt> <out file> [--sync-off] [--host] [--buf <n>]
//
//   --sync-off  turn Sync off in each preset (keeping its overlap) before rendering
//   --host      run with a playing transport at 120 bpm (otherwise no host time info)
//   --buf <n>   buffer size (default 512)
//
// The output file holds, per preset: name (32 bytes), sync on (int32), sample count (int32),
// then the left and right channels as float32. dtblkfix_compare compares two of these.

#include "harness.h"

int main(int argc, char** argv)
{
  if (argc < 4) {
    fprintf(stderr, "usage: %s <fftw dir> <presets.txt> <out> [--sync-off] [--host] [--buf n]\n",
            argv[0]);
    return 2;
  }
  bool sync_off = false, host = false;
  long buf = 512;
  for (int i = 4; i < argc; i++) {
    if (!strcmp(argv[i], "--sync-off"))
      sync_off = true;
    else if (!strcmp(argv[i], "--host"))
      host = true;
    else if (!strcmp(argv[i], "--buf") && i + 1 < argc)
      buf = atol(argv[++i]);
  }
  if (!harness::init(argv[1], argv[2]))
    return 1;

  FILE* out = fopen(argv[3], "wb");
  if (!out) {
    fprintf(stderr, "can't write %s\n", argv[3]);
    return 1;
  }

  // 6 seconds of test signal
  const long n = 6 * 44100;
  std::vector<float> in_l(n), in_r(n), out_l, out_r;
  harness::Signal sig;
  for (long i = 0; i < n; i++)
    sig.next(in_l[i], in_r[i]);

  std::vector<long> bufs(1, buf);
  for (int p = 0; p < (int)g_blk_fx_presets.size(); p++) {
    harness::Transport transport;
    transport.enabled = host;
    DtBlkFx* fx = harness::create(&transport, buf);
    fx->setProgram(p);
    float overlap = fx->getParameter(BlkFxParam::OVERLAP);
    int sync = BlkFxParam::getBlkSync(overlap) ? 1 : 0;
    if (sync_off) {
      fx->setParameter(BlkFxParam::OVERLAP, harness::syncOff(overlap));
      sync = 0;
    }
    harness::render(fx, &transport, in_l, in_r, out_l, out_r, bufs);

    char name[32];
    memset(name, 0, sizeof(name));
    fx->getProgramName(name);
    int32_t hdr[2] = {sync, (int32_t)n};
    fwrite(name, 1, 32, out);
    fwrite(hdr, 4, 2, out);
    fwrite(&out_l[0], 4, n, out);
    fwrite(&out_r[0], 4, n, out);

    double pwr = 0;
    for (long i = 0; i < n; i++)
      pwr += out_l[i] * out_l[i] + out_r[i] * out_r[i];
    printf("%2d %-24s sync=%d rms=%.6f\n", p, name, sync, sqrt(pwr / (2 * n)));
    delete fx;
  }
  fclose(out);
  return 0;
}
