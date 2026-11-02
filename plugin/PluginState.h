// DtBlkFix plugin: plugin state.
//
// The state is the original DtBlkFx chunk (current settings and all programs, as the VST2
// version saves it) followed by a small trailer with the editor's size and skin:
//
//   chunk | "view" fields | uint32 fields length | "DBfx" (4 bytes, at the very end)
//
// view fields: uint8 version (1), uint16 size in percent, uint8 skin name length, skin name.
// All little endian. A state without the trailer (a bare chunk) loads with the defaults.

#pragma once

#include <stddef.h>
#include <stdint.h>
#include <string>
#include <vector>

namespace dtblkfix {

struct ViewSettings {
  int scale_pct = 100;
  std::string skin = "Original";
};

inline std::vector<uint8_t> encodeState(const std::vector<uint8_t>& chunk, const ViewSettings& view)
{
  std::vector<uint8_t> out(chunk);
  std::vector<uint8_t> f;
  f.push_back(1);
  f.push_back((uint8_t)(view.scale_pct & 0xff));
  f.push_back((uint8_t)((view.scale_pct >> 8) & 0xff));
  std::string skin = view.skin.substr(0, 255);
  f.push_back((uint8_t)skin.size());
  f.insert(f.end(), skin.begin(), skin.end());
  out.insert(out.end(), f.begin(), f.end());
  uint32_t n = (uint32_t)f.size();
  for (int i = 0; i < 4; i++)
    out.push_back((uint8_t)(n >> (8 * i)));
  const char magic[4] = {'D', 'B', 'f', 'x'};
  out.insert(out.end(), magic, magic + 4);
  return out;
}

// split a state into the chunk (returns its length) and view settings
inline size_t decodeState(const uint8_t* data, size_t size, ViewSettings* view)
{
  if (size < 8 || data[size - 4] != 'D' || data[size - 3] != 'B' || data[size - 2] != 'f' ||
      data[size - 1] != 'x')
    return size;
  uint32_t n = 0;
  for (int i = 0; i < 4; i++)
    n |= (uint32_t)data[size - 8 + i] << (8 * i);
  if (n < 4 || n > size - 8)
    return size;
  const uint8_t* f = data + size - 8 - n;
  if (f[0] >= 1) {
    view->scale_pct = f[1] | (f[2] << 8);
    size_t len = f[3];
    if (4 + len <= n)
      view->skin.assign((const char*)f + 4, len);
  }
  return size - 8 - n;
}

} // namespace dtblkfix
