#pragma warning(disable:4189 4244)
#include "window_utils_plugin.h"

// This must be included before many other Windows headers.
#include <windows.h>

// For getPlatformVersion; remove unless needed for your plugin implementation.
#include <VersionHelpers.h>

#include <flutter/method_channel.h>
#include <flutter/plugin_registrar_windows.h>
#include <flutter/standard_method_codec.h>

#include <map>
#include <memory>
#include <sstream>
#include <unordered_map>

namespace
{

class WindowUtilsPlugin : public flutter::Plugin
{
  RECT normalRect;
  LONG oldStyle;

public:
  static void RegisterWithRegistrar(flutter::PluginRegistrarWindows *registrar);

  // Creates a plugin that communicates on the given channel.
  WindowUtilsPlugin(
      std::unique_ptr<flutter::MethodChannel<flutter::EncodableValue>> channel);

  virtual ~WindowUtilsPlugin();

private:
  // Called when a method is called on |channel_|;
  void HandleMethodCall(
      const flutter::MethodCall<flutter::EncodableValue> &method_call,
      std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result);

  // The MethodChannel used for communication with the Flutter engine.
  std::unique_ptr<flutter::MethodChannel<flutter::EncodableValue>> channel_;
};

// static
void WindowUtilsPlugin::RegisterWithRegistrar(flutter::PluginRegistrarWindows *registrar)
{
  auto channel =
      std::make_unique<flutter::MethodChannel<flutter::EncodableValue>>(
          registrar->messenger(), "window_utils",
          &flutter::StandardMethodCodec::GetInstance());
  auto *channel_pointer = channel.get();

  auto plugin = std::make_unique<WindowUtilsPlugin>(std::move(channel));

  channel_pointer->SetMethodCallHandler(
      [plugin_pointer = plugin.get()](const auto &call, auto result) {
        plugin_pointer->HandleMethodCall(call, std::move(result));
      });

  registrar->AddPlugin(std::move(plugin));
}

WindowUtilsPlugin::WindowUtilsPlugin(
    std::unique_ptr<flutter::MethodChannel<flutter::EncodableValue>> channel)
    : channel_(std::move(channel)) {}

WindowUtilsPlugin::~WindowUtilsPlugin(){};

void WindowUtilsPlugin::HandleMethodCall(
    const flutter::MethodCall<flutter::EncodableValue> &method_call,
    std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result)
{
  std::string method = method_call.method_name();
  // printf("Method is %s \n", method.c_str());
  if (method.compare("hideTitleBar") == 0)
  {
    HWND hWnd = GetActiveWindow();
    SetMenu(hWnd, NULL);
    LONG lStyle = GetWindowLong(hWnd, GWL_STYLE);
    oldStyle = lStyle;
    lStyle &= ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU | WS_DLGFRAME);
    SetWindowLong(hWnd, GWL_STYLE, lStyle);
    LONG flags = SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER;
    SetWindowPos(hWnd, NULL, 0, 0, 0, 0, flags);
    flutter::EncodableValue response(true);
    result->Success(&response);
  }
  else if (method.compare("showTitleBar") == 0)
  {
    HWND hWnd = GetActiveWindow();
    SetWindowLong(hWnd, GWL_STYLE, oldStyle);
    flutter::EncodableValue response(true);
    result->Success(&response);
  }
  else if (method.compare("getScreenSize") == 0)
  {
    flutter::EncodableMap umap;
    HWND hWnd = GetDesktopWindow();
    RECT rect;
    if (GetWindowRect(hWnd, &rect))
    {
      double width = rect.right;
      double height = rect.bottom;
      umap[flutter::EncodableValue("width")] = flutter::EncodableValue(width);
      umap[flutter::EncodableValue("height")] = flutter::EncodableValue(height);
    }
    flutter::EncodableValue response(umap);
    result->Success(&response);
  }
  else if (method.compare("startResize") == 0)
  {
    const flutter::EncodableValue *args = method_call.arguments();
    const flutter::EncodableMap &map = args->MapValue();
    bool top = map.at(flutter::EncodableValue("top")).BoolValue();
    bool bottom = map.at(flutter::EncodableValue("bottom")).BoolValue();
    bool left = map.at(flutter::EncodableValue("left")).BoolValue();
    bool right = map.at(flutter::EncodableValue("right")).BoolValue();
    HWND hWnd = GetActiveWindow();
    ReleaseCapture();
    LONG command = SC_SIZE;
    if (top && !bottom && !right && !left)
    {
      command |= WMSZ_TOP;
    }
    else if (top && left && !bottom && !right)
    {
      command |= WMSZ_TOPLEFT;
    }
    else if (left && !top && !bottom && !right)
    {
      command |= WMSZ_LEFT;
    }
    else if (right && !top && !left && !bottom)
    {
      command |= WMSZ_RIGHT;
    }
    else if (top && right && !left && !bottom)
    {
      command |= WMSZ_TOPRIGHT;
    }
    else if (bottom && !top && !right && !left)
    {
      command |= WMSZ_BOTTOM;
    }
    else if (bottom && left && !top && !right)
    {
      command |= WMSZ_BOTTOMLEFT;
    }
    else if (bottom && right && !top && !left)
    {
      command |= WMSZ_BOTTOMRIGHT;
    }
    SendMessage(hWnd, WM_SYSCOMMAND, command, 0);
    flutter::EncodableValue response(true);
    result->Success(&response);
  }
  else if (method.compare("startDrag") == 0)
  {
    HWND hWnd = GetActiveWindow();
    ReleaseCapture();
    SendMessage(hWnd, WM_SYSCOMMAND, SC_MOVE | HTCAPTION, 0);
    flutter::EncodableValue response(true);
    result->Success(&response);
  }
  else if (method.compare("closeWindow") == 0)
  {
    HWND hWnd = GetActiveWindow();
    SendMessage(hWnd, WM_CLOSE, 0, NULL);
    flutter::EncodableValue response(true);
    result->Success(&response);
  }
  else if (method.compare("minWindow") == 0)
  {
    HWND hWnd = GetActiveWindow();
    ShowWindow(hWnd, SW_MINIMIZE);
    flutter::EncodableValue response(true);
    result->Success(&response);
  }
  else if (method.compare("windowTitleDoubleTap") == 0)
  {
    HWND hWnd = GetActiveWindow();
    HWND hWndScreen = GetDesktopWindow();
    RECT rectScreen;
    GetWindowRect(hWndScreen, &rectScreen);
    double padding = 5.0;
    bool isMaximized = false;
    RECT activeRect;
    GetWindowRect(hWnd, &activeRect);
    if (activeRect.top <= rectScreen.top + padding)
    {
      isMaximized = true;
    }
    if (activeRect.bottom >= rectScreen.bottom - padding)
    {
      isMaximized = true;
    }
    if (activeRect.left <= rectScreen.left + padding)
    {
      isMaximized = true;
    }
    if (activeRect.right >= rectScreen.right - padding)
    {
      isMaximized = true;
    }
    if (!isMaximized)
    {
      GetWindowRect(hWnd, &normalRect);
    }
    if (isMaximized)
    {
      RECT rect = normalRect;
      double width = rect.right - rect.left;
      double height = rect.bottom - rect.top;
      int x = rect.left;
      int y = rect.top;
      MoveWindow(hWnd, x, y, width, height, true);
    }
    else
    {
      RECT rect = rectScreen;
      double width = rect.right - rect.left;
      double height = rect.bottom - rect.top;
      int x = rect.left;
      int y = rect.top;
      MoveWindow(hWnd, x, y, width, height, true);
    }
    //  ReleaseCapture();
    // // InvalidateRect(hWnd, NULL, TRUE);
    // UpdateWindow(hWnd);
    flutter::EncodableValue response(true);
    result->Success(&response);
  }
  else if (method.compare("maxWindow") == 0)
  {
    HWND hWnd = GetActiveWindow();
    ShowWindow(hWnd, SW_SHOWMAXIMIZED);
    flutter::EncodableValue response(true);
    result->Success(&response);
  }
  else if (method.compare("centerWindow") == 0)
  {
    HWND hWnd = GetActiveWindow();
    RECT rect;
    bool success = false;
    HWND hWndScreen = GetDesktopWindow();
    RECT rectScreen;
    if (GetWindowRect(hWndScreen, &rectScreen))
    {
      double screenWidth = rectScreen.right;
      double screenHeight = rectScreen.bottom;
      double centerX = screenWidth / 2;
      double centerY = screenHeight / 2;

      if (GetWindowRect(hWnd, &rect))
      {
        double width = rect.right - rect.left;
        double height = rect.bottom - rect.top;
        int x = ((rectScreen.right - rectScreen.left) / 2 - width / 2);
        int y = ((rectScreen.bottom - rectScreen.top) / 2 - height / 2);
        success = MoveWindow(hWnd, x, y, width, height, true);
      }
    }
    flutter::EncodableValue response(success);
    result->Success(&response);
  }
  else if (method.compare("setSize") == 0)
  {
    const flutter::EncodableValue *args = method_call.arguments();
    const flutter::EncodableMap &map = args->MapValue();
    double width = map.at(flutter::EncodableValue("width")).DoubleValue();
    double height = map.at(flutter::EncodableValue("height")).DoubleValue();
    HWND hWnd = GetActiveWindow();
    RECT rect;
    bool success = false;
    if (GetWindowRect(hWnd, &rect))
    {
      double x = rect.left;
      double y = rect.top;
      success = MoveWindow(hWnd, x, y, width, height, true);
    }
    flutter::EncodableValue response(success);
    result->Success(&response);
  }
  else if (method.compare("setPosition") == 0)
  {
    const flutter::EncodableValue *args = method_call.arguments();
    const flutter::EncodableMap &map = args->MapValue();
    double x = map.at(flutter::EncodableValue("x")).DoubleValue();
    double y = map.at(flutter::EncodableValue("y")).DoubleValue();
    HWND hWnd = GetActiveWindow();
    RECT rect;
    bool success = false;
    if (GetWindowRect(hWnd, &rect))
    {
      double width = rect.right - rect.left;
      double height = rect.bottom - rect.top;
      success = MoveWindow(hWnd, x, y, width, height, true);
    }
    flutter::EncodableValue response(success);
    result->Success(&response);
  }
  else if (method.compare("getWindowSize") == 0)
  {
    flutter::EncodableMap umap;
    HWND hWnd = GetActiveWindow();
    RECT rect;
    if (GetWindowRect(hWnd, &rect))
    {
      double width = rect.right - rect.left;
      double height = rect.bottom - rect.top;
      umap[flutter::EncodableValue("width")] = flutter::EncodableValue(width);
      umap[flutter::EncodableValue("height")] = flutter::EncodableValue(height);
    }
    flutter::EncodableValue response(umap);
    result->Success(&response);
  }
  else if (method.compare("getWindowOffset") == 0)
  {
    flutter::EncodableMap umap;
    HWND hWnd = GetActiveWindow();
    RECT rect;
    if (GetWindowRect(hWnd, &rect))
    {
      double offsetX = rect.left;
      double offsetY = rect.top;
      umap[flutter::EncodableValue("offsetX")] = flutter::EncodableValue(offsetX);
      umap[flutter::EncodableValue("offsetY")] = flutter::EncodableValue(offsetY);
    }
    flutter::EncodableValue response(umap);
    result->Success(&response);
  }
  else if (method.compare("hideCursor") == 0)
  {
    while (ShowCursor(false) >= 0)
    {
    }
    flutter::EncodableValue response(true);
    result->Success(&response);
  }
  else if (method.compare("showCursor") == 0)
  {
    while (ShowCursor(true) >= 0)
    {
    }
    flutter::EncodableValue response(true);
    result->Success(&response);
  }
  else if (method.compare("setCursor") == 0)
  {
    const flutter::EncodableValue *args = method_call.arguments();
    const flutter::EncodableMap &map = args->MapValue();
    bool update = map.at(flutter::EncodableValue("update")).BoolValue();
    std::string type = map.at(flutter::EncodableValue("type")).StringValue();
    HCURSOR cursor;
    if (type.compare("appStart") == 0)
    {
      cursor = LoadCursor(0, IDC_APPSTARTING);
    }
    else if (type.compare("arrow") == 0)
    {
      cursor = LoadCursor(0, IDC_ARROW);
    }
    else if (type.compare("cross") == 0)
    {
      cursor = LoadCursor(0, IDC_CROSS);
    }
    else if (type.compare("hand") == 0)
    {
      cursor = LoadCursor(0, IDC_HAND);
    }
    else if (type.compare("help") == 0)
    {
      cursor = LoadCursor(0, IDC_HELP);
    }
    else if (type.compare("iBeam") == 0)
    {
      cursor = LoadCursor(0, IDC_IBEAM);
    }
    else if (type.compare("no") == 0)
    {
      cursor = LoadCursor(0, IDC_NO);
    }
    else if (type.compare("resizeAll") == 0)
    {
      cursor = LoadCursor(0, IDC_SIZEALL);
    }
    else if (type.compare("resizeNESW") == 0)
    {
      cursor = LoadCursor(0, IDC_SIZENESW);
    }
    else if (type.compare("resizeNS") == 0)
    {
      cursor = LoadCursor(0, IDC_SIZENS);
    }
    else if (type.compare("resizeNWSE") == 0)
    {
      cursor = LoadCursor(0, IDC_SIZENWSE);
    }
    else if (type.compare("resizeWE") == 0)
    {
      cursor = LoadCursor(0, IDC_SIZEWE);
    }
    else if (type.compare("upArrow") == 0)
    {
      cursor = LoadCursor(0, IDC_UPARROW);
    }
    else if (type.compare("wait") == 0)
    {
      cursor = LoadCursor(0, IDC_WAIT);
    }
    else
    {
      cursor = LoadCursor(0, IDC_ARROW);
    }
    HWND hWnd = GetActiveWindow();
    HWND hWndDesktop = GetDesktopWindow();
    SetClassLongPtrA(hWnd, -12, (LONG_PTR)cursor);
    SetClassLongPtrA(hWndDesktop, -12, (LONG_PTR)cursor);
    // if (update)
    // {
    //   SetCursor(cursor);
    // }
    // else
    // {
    //   SetCursor(cursor);
    // }
    SetCursor(cursor);
    flutter::EncodableValue response(true);
    result->Success(&response);
  }
  else
  {
    result->NotImplemented();
  }
}

} // namespace

void WindowUtilsPluginRegisterWithRegistrar(
    FlutterDesktopPluginRegistrarRef registrar) {
  // The plugin registrar wrappers owns the plugins, registered callbacks, etc.,
  // so must remain valid for the life of the application.
  static auto *plugin_registrars =
      new std::map<FlutterDesktopPluginRegistrarRef,
                   std::unique_ptr<flutter::PluginRegistrarWindows>>;
  auto insert_result = plugin_registrars->emplace(
      registrar, std::make_unique<flutter::PluginRegistrarWindows>(registrar));

  WindowUtilsPlugin::RegisterWithRegistrar(insert_result.first->second.get());
}
