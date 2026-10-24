#ifndef SCREENCAPTURE_H
#define SCREENCAPTURE_H

#include <QObject>
#include <QByteArray>
#include <QImage>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <wrl/client.h>   // 微软 ComPtr，零开销智能指针

class ScreenCapture : public QObject
{
    Q_OBJECT

public:
    explicit ScreenCapture(QObject *parent = nullptr);
    ~ScreenCapture();

    // Capture a desktop image; a null image means no frame was available.
    QImage captureFrame();

    // Encode a captured image as JPEG. quality is in the range 0-100.
    static QByteArray encodeJpeg(const QImage &image, int quality = 100);

private:
    bool initializeDXGI();   // 只在首次调用时执行初始化
    void cleanupDXGI();

    // DXGI 核心对象
    Microsoft::WRL::ComPtr<ID3D11Device>        m_d3dDevice;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> m_d3dContext;
    Microsoft::WRL::ComPtr<IDXGIOutputDuplication> m_dupl;
    Microsoft::WRL::ComPtr<ID3D11Texture2D>     m_stagingTex;   // 用于 CPU 回读的 staging 纹理

    // 捕获参数
    UINT m_width = 0;
    UINT m_height = 0;
    bool m_initialized = false;
};

#endif // SCREENCAPTURE_H
