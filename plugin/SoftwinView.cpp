#include "SoftwinView.h"

#include "softwin_api.h"

#include <cmath>
#include <set>

namespace {

std::set<SoftwinView*>& views()
{
  static std::set<SoftwinView*> v;
  return v;
}

SoftwinView* g_active = nullptr;

juce::MouseCursor::StandardCursorType cursorType(int c)
{
  switch (c) {
    case softwin::kCursorIBeam: return juce::MouseCursor::IBeamCursor;
    case softwin::kCursorWait: return juce::MouseCursor::WaitCursor;
    case softwin::kCursorCross: return juce::MouseCursor::CrosshairCursor;
    case softwin::kCursorSizeWE: return juce::MouseCursor::LeftRightResizeCursor;
    case softwin::kCursorSizeNS: return juce::MouseCursor::UpDownResizeCursor;
    case softwin::kCursorSizeNWSE: return juce::MouseCursor::TopLeftCornerResizeCursor;
    case softwin::kCursorSizeNESW: return juce::MouseCursor::TopRightCornerResizeCursor;
    case softwin::kCursorSizeAll: return juce::MouseCursor::UpDownLeftRightResizeCursor;
    case softwin::kCursorHand: return juce::MouseCursor::PointingHandCursor;
    default: return juce::MouseCursor::NormalCursor;
  }
}

unsigned currentModifiers()
{
  auto m = juce::ModifierKeys::getCurrentModifiersRealtime();
  unsigned r = 0;
  if (m.isShiftDown())
    r |= softwin::kModShift;
  if (m.isCtrlDown())
    r |= softwin::kModControl;
  if (m.isAltDown())
    r |= softwin::kModAlt;
  if (m.isCommandDown())
    r |= softwin::kModCommand;
  if (m.isLeftButtonDown())
    r |= softwin::kModLButton;
  if (m.isRightButtonDown())
    r |= softwin::kModRButton;
  if (m.isMiddleButtonDown())
    r |= softwin::kModMButton;
  return r;
}

void addMenuItems(juce::PopupMenu& menu, void* sw_menu, std::vector<uintptr_t>& ids)
{
  int n = softwin::menuItemCount(sw_menu);
  for (int i = 0; i < n; i++) {
    unsigned flags = softwin::menuItemFlags(sw_menu, i);
    juce::String text = juce::String::fromUTF8(softwin::menuItemText(sw_menu, i)).replace("&", "");
    if (flags & softwin::kMenuSeparator) {
      menu.addSeparator();
      continue;
    }
    bool enabled = !(flags & (softwin::kMenuGrayed | softwin::kMenuDisabled));
    if (flags & softwin::kMenuPopup) {
      juce::PopupMenu sub;
      addMenuItems(sub, (void*)softwin::menuItemId(sw_menu, i), ids);
      menu.addSubMenu(text, sub, enabled);
      continue;
    }
    ids.push_back(softwin::menuItemId(sw_menu, i));
    menu.addItem((int)ids.size(), text, enabled, (flags & softwin::kMenuChecked) != 0);
  }
}

struct Host : public softwin::Host {
  void invalidate(void* root, const softwin::SwRect& r) override
  {
    if (auto* v = SoftwinView::forWindow(root))
      v->invalidatePixels(r.left, r.top, r.right, r.bottom);
  }
  void updateNow(void* root) override
  {
    if (auto* v = SoftwinView::forWindow(root))
      v->updateNow();
  }
  void setCursor(int cursor) override
  {
    if (auto* v = SoftwinView::active())
      v->setMouseCursor(cursorType(cursor));
  }
  softwin::SwPoint rootScreenPos(void* root) override
  {
    if (auto* v = SoftwinView::forWindow(root)) {
      auto p = v->screenPixels({0, 0});
      return {p.x, p.y};
    }
    return {0, 0};
  }
  softwin::SwPoint mouseScreenPos() override
  {
    auto* v = SoftwinView::active();
    float s = v ? v->pixelScale() : 1.0f;
    auto p = juce::Desktop::getInstance().getMainMouseSource().getScreenPosition();
    return {juce::roundToInt(p.x * s), juce::roundToInt(p.y * s)};
  }
  unsigned modifiers() override { return currentModifiers(); }
  int popupMenu(void* root, void* menu, int x, int y) override
  {
    auto* v = SoftwinView::forWindow(root);
    float s = v ? v->pixelScale() : 1.0f;
    juce::PopupMenu m;
    std::vector<uintptr_t> ids;
    addMenuItems(m, menu, ids);
    auto opts = juce::PopupMenu::Options().withTargetScreenArea(
        juce::Rectangle<int>(juce::roundToInt(x / s), juce::roundToInt(y / s), 1, 1));
    int r = m.showMenu(opts);
    if (r <= 0 || r > (int)ids.size())
      return 0;
    return (int)ids[(size_t)r - 1];
  }
  void showOverlay(void* wnd, int x, int y, int w, int h, const uint32_t* bgra) override
  {
    if (auto* v = SoftwinView::active())
      v->showOverlay(wnd, x, y, w, h, bgra);
  }
  void hideOverlay(void* wnd) override
  {
    for (auto* v : views())
      v->hideOverlay(wnd);
  }
  void textEditOpen(void* edit, const softwin::SwRect& r, const char* text, int limit, int font_px) override
  {
    if (auto* v = SoftwinView::active())
      v->openTextEdit(edit, juce::Rectangle<int>(r.left, r.top, r.right - r.left, r.bottom - r.top), text, limit, font_px);
  }
  void textEditClose(void* edit) override
  {
    for (auto* v : views())
      v->closeTextEdit(edit);
  }
  int textEditGet(void* edit, char* buf, int n) override
  {
    for (auto* v : views())
      if (auto* t = v->textEdit(edit)) {
        juce::String s = t->getText();
        s.copyToUTF8(buf, (size_t)n);
        return (int)strlen(buf);
      }
    if (n > 0)
      buf[0] = 0;
    return 0;
  }
  void textEditSet(void* edit, const char* text) override
  {
    for (auto* v : views())
      if (auto* t = v->textEdit(edit))
        t->setText(juce::String::fromUTF8(text), false);
  }
};

Host& host()
{
  static Host h;
  return h;
}

}

class SoftwinView::Overlay : public juce::Component {
public:
  Overlay() { setInterceptsMouseClicks(false, false); }
  void set(int x, int y, int w, int h, const uint32_t* bgra, float s)
  {
    if (w <= 0 || h <= 0)
      return;
    if (!_image.isValid() || _image.getWidth() != w || _image.getHeight() != h)
      _image = juce::Image(juce::Image::ARGB, w, h, true);
    {
      juce::Image::BitmapData d(_image, juce::Image::BitmapData::writeOnly);
      for (int yy = 0; yy < h; yy++)
        memcpy(d.getLinePointer(yy), bgra + (size_t)yy * w, (size_t)w * 4);
    }
    _scale = s;
    setBounds(juce::roundToInt(x / s), juce::roundToInt(y / s), juce::roundToInt(w / s), juce::roundToInt(h / s));
    if (!isOnDesktop()) {
      setOpaque(false);
      addToDesktop(juce::ComponentPeer::windowIsTemporary | juce::ComponentPeer::windowIgnoresMouseClicks);
      setAlwaysOnTop(true);
    }
    setVisible(true);
    repaint();
  }
  void paint(juce::Graphics& g) override
  {
    if (_image.isValid())
      g.drawImageTransformed(_image, juce::AffineTransform::scale(1.0f / _scale));
  }

private:
  juce::Image _image;
  float _scale = 1.0f;
};

SoftwinView::SoftwinView()
{
  softwin::setHost(&host());
  views().insert(this);
  setOpaque(true);
  setWantsKeyboardFocus(false);
  startTimerHz(60);
}

SoftwinView::~SoftwinView()
{
  stopTimer();
  views().erase(this);
  if (g_active == this)
    g_active = nullptr;
}

SoftwinView* SoftwinView::forWindow(void* window)
{
  for (auto* v : views())
    if (v->_window == window)
      return v;
  return nullptr;
}

SoftwinView* SoftwinView::active()
{
  if (g_active && views().count(g_active))
    return g_active;
  return views().empty() ? nullptr : *views().begin();
}

void SoftwinView::setHWND(void* window)
{
  if (_window == window)
    return;
  for (auto& e : _edits) {
    e.second->onReturnKey = nullptr;
    e.second->onEscapeKey = nullptr;
    e.second->onFocusLost = nullptr;
    e.second->setVisible(false);
    removeChildComponent(e.second.get());
    _closed_edits.push_back(std::move(e.second));
  }
  _edits.clear();
  _overlays.clear();
  if (_window)
    softwin::destroyWindow(_window);
  _window = window;
  _image = juce::Image();
  _dirty = {};
  if (_window) {
    g_active = this;
    int w = 0, h = 0;
    softwin::rootPixels(_window, &w, &h);
    _dirty = {0, 0, w, h};
  }
  repaint();
}

void SoftwinView::invalidatePixels(int left, int top, int right, int bottom)
{
  _dirty = _dirty.getUnion(juce::Rectangle<int>(left, top, right - left, bottom - top));
  float s = _pixel_scale;
  repaint(juce::Rectangle<int>((int)std::floor(left / s) - 1, (int)std::floor(top / s) - 1,
                               (int)std::ceil((right - left) / s) + 3, (int)std::ceil((bottom - top) / s) + 3));
}

void SoftwinView::updateNow()
{
  softwin::paintPending(_window);
  repaint();
}

juce::Point<int> SoftwinView::screenPixels(juce::Point<int> local_points) const
{
  auto p = localPointToGlobal(local_points);
  return {juce::roundToInt(p.x * _pixel_scale), juce::roundToInt(p.y * _pixel_scale)};
}

void SoftwinView::updateImage()
{
  if (!_window)
    return;
  softwin::paintPending(_window);
  int w = 0, h = 0;
  const uint32_t* px = softwin::rootPixels(_window, &w, &h);
  if (!px || w <= 0 || h <= 0)
    return;
  if (!_image.isValid() || _image.getWidth() != w || _image.getHeight() != h) {
    _image = juce::Image(juce::Image::ARGB, w, h, false);
    _dirty = {0, 0, w, h};
  }
  auto r = _dirty.getIntersection({0, 0, w, h});
  _dirty = {};
  if (r.isEmpty())
    return;
  juce::Image::BitmapData d(_image, r.getX(), r.getY(), r.getWidth(), r.getHeight(), juce::Image::BitmapData::writeOnly);
  for (int y = 0; y < r.getHeight(); y++) {
    const uint32_t* src = px + (size_t)(r.getY() + y) * w + r.getX();
    uint32_t* dst = (uint32_t*)d.getLinePointer(y);
    for (int x = 0; x < r.getWidth(); x++)
      dst[x] = src[x] | 0xff000000u;
  }
}

void SoftwinView::paint(juce::Graphics& g)
{
  updateImage();
  if (!_image.isValid()) {
    g.fillAll(juce::Colours::black);
    return;
  }
  g.setImageResamplingQuality(juce::Graphics::lowResamplingQuality);
  g.drawImageTransformed(_image, juce::AffineTransform::scale(1.0f / _pixel_scale));
}

void SoftwinView::timerCallback()
{
  if (!_window)
    return;
  softwin::paintPending(_window);
  for (auto it = _closed_edits.begin(); it != _closed_edits.end();)
    it = _closed_edits.erase(it);
}

unsigned SoftwinView::buttons(const juce::MouseEvent& e) const
{
  unsigned m = currentModifiers() & ~(softwin::kModLButton | softwin::kModRButton | softwin::kModMButton);
  if (e.mods.isLeftButtonDown())
    m |= _right_drag ? softwin::kModRButton : softwin::kModLButton;
  if (e.mods.isRightButtonDown())
    m |= softwin::kModRButton;
  if (e.mods.isMiddleButtonDown())
    m |= softwin::kModMButton;
  return m;
}

void SoftwinView::send(unsigned message, const juce::MouseEvent& e, int wheel)
{
  if (!_window)
    return;
  g_active = this;
  auto p = e.getPosition();
  int x = juce::roundToInt(p.x * _pixel_scale), y = juce::roundToInt(p.y * _pixel_scale);
  softwin::mouseEvent(_window, message, x, y, buttons(e), wheel);
  softwin::paintPending(_window);
}

void SoftwinView::mouseMove(const juce::MouseEvent& e)
{
  send(softwin::kMouseMove, e);
}

void SoftwinView::mouseDown(const juce::MouseEvent& e)
{
  _right_drag = e.mods.isPopupMenu();
  if (_right_drag)
    send(softwin::kRButtonDown, e);
  else if (e.mods.isMiddleButtonDown())
    send(softwin::kMButtonDown, e);
  else
    send(softwin::kLButtonDown, e);
}

void SoftwinView::mouseDoubleClick(const juce::MouseEvent& e)
{
  if (!e.mods.isPopupMenu())
    send(softwin::kLButtonDoubleClick, e);
}

void SoftwinView::mouseDrag(const juce::MouseEvent& e)
{
  send(softwin::kMouseMove, e);
}

void SoftwinView::mouseUp(const juce::MouseEvent& e)
{
  if (_right_drag)
    send(softwin::kRButtonUp, e);
  else if (e.mods.isMiddleButtonDown())
    send(softwin::kMButtonUp, e);
  else
    send(softwin::kLButtonUp, e);
  _right_drag = false;
}

void SoftwinView::mouseExit(const juce::MouseEvent& e)
{
  send(softwin::kMouseLeave, e);
}

void SoftwinView::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& w)
{
  _wheel += w.deltaY * 480.0f;
  int notch = (int)_wheel;
  if (notch == 0)
    return;
  _wheel -= (float)notch;
  send(softwin::kMouseWheel, e, notch);
}

void SoftwinView::openTextEdit(void* edit, juce::Rectangle<int> pixels, const char* text, int limit, int font_px)
{
  closeTextEdit(edit);
  float s = _pixel_scale;
  auto t = std::make_unique<juce::TextEditor>();
  t->setBounds(juce::roundToInt(pixels.getX() / s), juce::roundToInt(pixels.getY() / s),
               juce::roundToInt(pixels.getWidth() / s), juce::roundToInt(pixels.getHeight() / s));
  t->setFont(juce::Font(juce::FontOptions((float)font_px / s)));
  t->setIndents(2, 0);
  t->setJustification(juce::Justification::centredLeft);
  if (limit > 0)
    t->setInputRestrictions(limit);
  t->setText(juce::String::fromUTF8(text), false);
  t->onReturnKey = [edit] { softwin::textEditEvent(edit, softwin::kEditReturn); };
  t->onEscapeKey = [edit] { softwin::textEditEvent(edit, softwin::kEditEscape); };
  t->onFocusLost = [edit] { softwin::textEditEvent(edit, softwin::kEditFocusLost); };
  addAndMakeVisible(*t);
  t->selectAll();
  t->grabKeyboardFocus();
  _edits[edit] = std::move(t);
}

void SoftwinView::closeTextEdit(void* edit)
{
  auto it = _edits.find(edit);
  if (it == _edits.end())
    return;
  auto t = std::move(it->second);
  _edits.erase(it);
  t->onReturnKey = nullptr;
  t->onEscapeKey = nullptr;
  t->onFocusLost = nullptr;
  t->setVisible(false);
  removeChildComponent(t.get());
  _closed_edits.push_back(std::move(t));
}

juce::TextEditor* SoftwinView::textEdit(void* edit)
{
  auto it = _edits.find(edit);
  return it == _edits.end() ? nullptr : it->second.get();
}

void SoftwinView::showOverlay(void* wnd, int x, int y, int w, int h, const uint32_t* bgra)
{
  auto& o = _overlays[wnd];
  if (!o)
    o = std::make_unique<Overlay>();
  o->set(x, y, w, h, bgra, _pixel_scale);
}

void SoftwinView::hideOverlay(void* wnd)
{
  auto it = _overlays.find(wnd);
  if (it != _overlays.end())
    _overlays.erase(it);
}
