
#include "ScreenCapture.h"
#include <QDebug>
#include <QImage>
#include <QBuffer>

ScreenCapture::ScreenCapture(QObject *parent)
    : QObject(parent)
{
}

ScreenCapture::~ScreenCapture()
{
    cleanupDXGI();
}

// ----------------------------------------------------------------------------
// 公开接口：返回一张屏幕截图的 JPEG 字节数组
// ----------------------------------------------------------------------------
QImage ScreenCapture::captureFrame()
{
    // 首次调用时初始化 DXGI 捕获管道
    if (!m_initialized && !initializeDXGI()) {
        qWarning() << "[DXGI] 初始化失败，无法捕获屏幕";
        return {};
    }

    // 1. 获取下一帧（等待最多 500ms）
    Microsoft::WRL::ComPtr<IDXGIResource> desktopResource;
    DXGI_OUTDUPL_FRAME_INFO frameInfo;
    HRESULT hr = m_dupl->AcquireNextFrame(16, &frameInfo, desktopResource.GetAddressOf());
    if (hr == DXGI_ERROR_WAIT_TIMEOUT) {
        return {};
    }
    if (FAILED(hr)) {
        qWarning() << "[DXGI] AcquireNextFrame 失败: 0x" << QString::number(hr, 16);
        // 设备丢失时可尝试重初始化
        cleanupDXGI();
        return {};
    }

    // 2. 将获取的桌面纹理复制到 staging 纹理（CPU 可读）
    Microsoft::WRL::ComPtr<ID3D11Texture2D> acquiredImage;
    hr = desktopResource.As(&acquiredImage);
    if (FAILED(hr)) {
        m_dupl->ReleaseFrame();
        qWarning() << "[DXGI] 纹理 QueryInterface 失败";
        return {};
    }

    m_d3dContext->CopyResource(m_stagingTex.Get(), acquiredImage.Get());

    // 3. 尽早释放帧锁
    m_dupl->ReleaseFrame();
    acquiredImage.Reset();
    desktopResource.Reset();

    // 4. 映射 staging 纹理到 CPU 内存
    D3D11_MAPPED_SUBRESOURCE mapped;
    hr = m_d3dContext->Map(m_stagingTex.Get(), 0, D3D11_MAP_READ, 0, &mapped);
    if (FAILED(hr)) {
        qWarning() << "[DXGI] Map 失败";
        return {};
    }

    // 5. 构造 QImage（格式 DXGI_FORMAT_B8G8R8A8_UNORM 与 QImage::Format_ARGB32 兼容）
    QImage image;
    // 检查是否有行填充
    if (mapped.RowPitch == m_width * 4) {
        // 行连续，直接包装并深拷贝（Unmap 后数据失效）
        image = QImage(static_cast<const uchar*>(mapped.pData),
                       m_width, m_height,
                       QImage::Format_ARGB32).copy();
    } else {
        // 逐行复制（处理对齐情况）
        image = QImage(m_width, m_height, QImage::Format_ARGB32);
        for (UINT y = 0; y < m_height; ++y) {
            memcpy(image.scanLine(y),
                   static_cast<const uchar*>(mapped.pData) + y * mapped.RowPitch,
                   m_width * 4);
        }
    }
    m_d3dContext->Unmap(m_stagingTex.Get(), 0);

    return image;
}

QByteArray ScreenCapture::encodeJpeg(const QImage &image, int quality)
{
    if (image.isNull()) {
        return {};
    }

    QByteArray jpegData;
    QBuffer buffer(&jpegData);
    buffer.open(QIODevice::WriteOnly);
    if (!image.save(&buffer, "JPG", quality)) {
        qWarning() << "[DXGI] JPEG 编码失败";
        return QByteArray();
    }

    return jpegData;
}

// ----------------------------------------------------------------------------
// 私有实现：初始化 DXGI 捕获管道
// ----------------------------------------------------------------------------
bool ScreenCapture::initializeDXGI()
{
    // 1. 创建 D3D11 设备（仅尝试硬件驱动）
    D3D_FEATURE_LEVEL featLevel;
    HRESULT hr = D3D11CreateDevice(
        nullptr,                    // 默认适配器
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        0,                          // 无设备标志
        nullptr, 0,                 // 默认特性级别
        D3D11_SDK_VERSION,
        m_d3dDevice.GetAddressOf(),
        &featLevel,
        m_d3dContext.GetAddressOf()
        );
    if (FAILED(hr)) {
        qWarning() << "[DXGI] D3D11CreateDevice 失败";
        return false;
    }

    // 2. 获取 DXGI 设备与适配器
    Microsoft::WRL::ComPtr<IDXGIDevice> dxgiDevice;
    hr = m_d3dDevice.As(&dxgiDevice);
    if (FAILED(hr)) return false;

    Microsoft::WRL::ComPtr<IDXGIAdapter> adapter;
    hr = dxgiDevice->GetParent(__uuidof(IDXGIAdapter), &adapter);
    if (FAILED(hr)) return false;

    // 3. 枚举第一个输出（主显示器）
    Microsoft::WRL::ComPtr<IDXGIOutput> output;
    hr = adapter->EnumOutputs(0, output.GetAddressOf());
    if (FAILED(hr)) {
        qWarning() << "[DXGI] EnumOutputs 失败";
        return false;
    }

    // 获取输出信息（坐标等）
    DXGI_OUTPUT_DESC outputDesc;
    output->GetDesc(&outputDesc);

    // 4. 查询 IDXGIOutput1 接口
    Microsoft::WRL::ComPtr<IDXGIOutput1> output1;
    hr = output.As(&output1);
    if (FAILED(hr)) {
        qWarning() << "[DXGI] 不支持 IDXGIOutput1";
        return false;
    }

    // 5. 创建桌面复制对象
    hr = output1->DuplicateOutput(m_d3dDevice.Get(), m_dupl.GetAddressOf());
    if (FAILED(hr)) {
        qWarning() << "[DXGI] DuplicateOutput 失败 (可能桌面已锁定或权限不足)";
        return false;
    }

    // 6. 获取桌面尺寸
    DXGI_OUTDUPL_DESC duplDesc;
    m_dupl->GetDesc(&duplDesc);
    m_width  = duplDesc.ModeDesc.Width;
    m_height = duplDesc.ModeDesc.Height;

    // 7. 创建 staging 纹理（用于 CPU 回读）
    D3D11_TEXTURE2D_DESC stagingDesc = {};
    stagingDesc.Width              = m_width;
    stagingDesc.Height             = m_height;
    stagingDesc.Format             = duplDesc.ModeDesc.Format;   // 通常为 B8G8R8A8_UNORM
    stagingDesc.ArraySize          = 1;
    stagingDesc.MipLevels          = 1;
    stagingDesc.SampleDesc.Count   = 1;
    stagingDesc.CPUAccessFlags     = D3D11_CPU_ACCESS_READ;
    stagingDesc.Usage              = D3D11_USAGE_STAGING;
    hr = m_d3dDevice->CreateTexture2D(&stagingDesc, nullptr, m_stagingTex.GetAddressOf());
    if (FAILED(hr)) {
        qWarning() << "[DXGI] 创建 staging 纹理失败";
        return false;
    }

    m_initialized = true;
    qDebug() << "[DXGI] 初始化成功，分辨率:" << m_width << "x" << m_height;
    return true;
}

void ScreenCapture::cleanupDXGI()
{
    // 显式重置 ComPtr，确保正确的释放顺序
    m_stagingTex.Reset();
    m_dupl.Reset();
    m_d3dContext.Reset();
    m_d3dDevice.Reset();
    m_initialized = false;
    m_width = m_height = 0;
}
