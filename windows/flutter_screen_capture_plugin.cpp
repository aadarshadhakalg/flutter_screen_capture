#include "flutter_screen_capture_plugin.h"

// This must be included before many other Windows headers.
#include <windows.h>

#include <flutter/method_channel.h>
#include <flutter/plugin_registrar_windows.h>
#include <flutter/standard_method_codec.h>

#include <memory>
#include <sstream>

namespace flutter_screen_capture {

flutter::EncodableMap CapturedScreenArea::asMap()
{
    flutter::EncodableMap dict;
    dict[flutter::EncodableValue("buffer")] = flutter::EncodableValue(buffer);
    dict[flutter::EncodableValue("width")] = flutter::EncodableValue(width);
    dict[flutter::EncodableValue("height")] = flutter::EncodableValue(height);
    dict[flutter::EncodableValue("bitsPerPixel")] = flutter::EncodableValue(bitsPerPixel);
    dict[flutter::EncodableValue("bytesPerPixel")] = flutter::EncodableValue(bytesPerPixel);
    return dict;
}

// static
void FlutterScreenCapturePlugin::RegisterWithRegistrar(
        flutter::PluginRegistrarWindows* registrar)
{
    auto channel = std::make_unique<flutter::MethodChannel<flutter::EncodableValue >>(
            registrar->messenger(),
            "flutter_screen_capture",
            &flutter::StandardMethodCodec::GetInstance()
    );

    auto plugin = std::make_unique<FlutterScreenCapturePlugin>();

    channel->SetMethodCallHandler(
            [plugin_pointer = plugin.get()](const auto& call, auto result) {
              HandleMethodCall(call, std::move(result));
            }
    );

    registrar->AddPlugin(std::move(plugin));
}

FlutterScreenCapturePlugin::FlutterScreenCapturePlugin() = default;

FlutterScreenCapturePlugin::~FlutterScreenCapturePlugin() = default;

void FlutterScreenCapturePlugin::HandleMethodCall(
        const flutter::MethodCall<flutter::EncodableValue>& method_call,
        std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result)
{
    if (method_call.method_name()=="captureScreenArea") {
        const auto& args = std::get<flutter::EncodableMap>(*method_call.arguments());
        auto x = std::get<int>(args.at(flutter::EncodableValue("x")));
        auto y = std::get<int>(args.at(flutter::EncodableValue("y")));
        auto width = std::get<int>(args.at(flutter::EncodableValue("width")));
        auto height = std::get<int>(args.at(flutter::EncodableValue("height")));

        auto capturedScreenArea = CaptureScreenArea(x, y, width, height);

        result->Success(capturedScreenArea.asMap());
    }
    else {
        result->NotImplemented();
    }
}

CapturedScreenArea FlutterScreenCapturePlugin::CaptureScreenArea(
        int x,
        int y,
        int width,
        int height)
{
    // Get entire screen
    HDC screen = GetDC(nullptr);
    HDC screenMem = CreateCompatibleDC(screen);
    HBITMAP dib = CreateCompatibleBitmap(screen, width, height);
    SelectObject(screenMem, dib);
    BitBlt(screenMem, 0, 0, width, height, screen, x, y, SRCCOPY);

    BITMAPINFO bi;
    bi.bmiHeader.biSize = sizeof(bi.bmiHeader);
    bi.bmiHeader.biWidth = width;
    bi.bmiHeader.biHeight = -height; // Non-cartesian
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;
    bi.bmiHeader.biSizeImage = (4*width*height);
    bi.bmiHeader.biXPelsPerMeter = 0;
    bi.bmiHeader.biYPelsPerMeter = 0;
    bi.bmiHeader.biClrUsed = 0;
    bi.bmiHeader.biClrImportant = 0;

    CapturedScreenArea capturedScreenArea;
    capturedScreenArea.width = width;
    capturedScreenArea.height = height;
    capturedScreenArea.bitsPerPixel = 32;
    capturedScreenArea.bytesPerPixel = 4;
    capturedScreenArea.buffer = std::vector<uint8_t>(4*width*height);

    GetDIBits(screenMem, dib, 0, height, capturedScreenArea.buffer.data(), &bi, DIB_RGB_COLORS);

    ReleaseDC(nullptr, screen);
    DeleteObject(dib);
    DeleteDC(screenMem);

    return capturedScreenArea;
}

}  // namespace flutter_screen_capture
