// DtBlkFix tests: compare two dtblkfx_render outputs bit for bit.
//
// dtblkfx_compare <reference> <new> [--allow-sync]
//
// Fails if any preset differs. With --allow-sync, presets that have Sync on are reported but
// allowed to differ (DtBlkFix changes how Sync follows the host).

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <vector>

struct Preset {
  char name[33];
  int32_t sync;
  std::vector<float> data;
};

static bool load(const char* file, std::vector<Preset>& presets)
{
  FILE* f = fopen(file, "rb");
  if (!f) {
    fprintf(stderr, "can't open %s\n", file);
    return false;
  }
  while (1) {
    Preset p;
    memset(p.name, 0, sizeof(p.name));
    int32_t hdr[2];
    if (fread(p.name, 1, 32, f) != 32 || fread(hdr, 4, 2, f) != 2)
      break;
    p.sync = hdr[0];
    p.data.resize(2 * (size_t)hdr[1]);
    if (fread(&p.data[0], 4, p.data.size(), f) != p.data.size())
      break;
    presets.push_back(p);
  }
  fclose(f);
  return !presets.empty();
}

int main(int argc, char** argv)
{
  if (argc < 3) {
    fprintf(stderr, "usage: %s <reference> <new> [--allow-sync]\n", argv[0]);
    return 2;
  }
  bool allow_sync = argc > 3 && !strcmp(argv[3], "--allow-sync");
  std::vector<Preset> a, b;
  if (!load(argv[1], a) || !load(argv[2], b))
    return 1;
  if (a.size() != b.size()) {
    printf("FAIL: %d presets vs %d\n", (int)a.size(), (int)b.size());
    return 1;
  }
  int failed = 0, identical = 0, sync_diff = 0;
  for (size_t i = 0; i < a.size(); i++) {
    bool same = a[i].data.size() == b[i].data.size() &&
                memcmp(&a[i].data[0], &b[i].data[0], a[i].data.size() * 4) == 0;
    const char* result = "identical";
    if (same)
      identical++;
    else if (allow_sync && a[i].sync) {
      result = "differs (Sync on)";
      sync_diff++;
    }
    else {
      result = "DIFFERS";
      failed++;
    }
    double max_diff = 0;
    size_t first = 0;
    if (!same) {
      for (size_t j = 0; j < a[i].data.size() && j < b[i].data.size(); j++) {
        double d = fabs((double)a[i].data[j] - b[i].data[j]);
        if (d > max_diff)
          max_diff = d;
        if (d > 0 && !first)
          first = j + 1;
      }
      printf("%2d %-24s sync=%d %s (max diff %g, first at sample %d)\n", (int)i, a[i].name,
             a[i].sync, result, max_diff, (int)((first - 1) % (a[i].data.size() / 2)));
    }
    else
      printf("%2d %-24s sync=%d %s\n", (int)i, a[i].name, a[i].sync, result);
  }
  printf("%d presets: %d identical, %d differ with Sync on, %d failed\n", (int)a.size(),
         identical, sync_diff, failed);
  return failed ? 1 : 0;
}
