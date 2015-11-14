#include "Surface.h"

#include <Amvideo.h>

#include "Context.h"

#if USE_CAIRO
#include "cairo.h"
#endif

namespace Render
{

#if USE_GDI

    RenderSurface::RenderSurface()
        : m_pMirrorData(NULL)
        , m_mirrorBitmap(NULL)
        , m_mirrorBitmapInfo()
        , m_pOutData(NULL)
    {
    }

    RenderSurface::~RenderSurface()
    {
        if (m_mirrorBitmap)
            ::DeleteObject(m_mirrorBitmap);
    }

    void RenderSurface::SetSource(PBYTE pData, ULONG uLen, LONG iWidth, LONG iHeight, UINT iFormat)
    {
        if (m_mirrorBitmapInfo.bmiHeader.biWidth != iWidth ||
            -m_mirrorBitmapInfo.bmiHeader.biHeight != iHeight ||
            m_mirrorBitmapInfo.bmiHeader.biBitCount != iFormat)
        {
            BITMAPINFO bitmapInfo;
            memset(&bitmapInfo, 0, sizeof(BITMAPINFO));

            bitmapInfo.bmiHeader.biCompression = BI_RGB;
            bitmapInfo.bmiHeader.biBitCount = iFormat;
            bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
            bitmapInfo.bmiHeader.biWidth = iWidth;
            bitmapInfo.bmiHeader.biHeight = -iHeight;
            bitmapInfo.bmiHeader.biPlanes = 1;

            PBYTE pData = NULL;
            HBITMAP newMirror = CreateDIBSection(NULL, &bitmapInfo, DIB_RGB_COLORS, (LPVOID *)&pData, NULL, 0);

            if (m_mirrorBitmap)
            {
                ::DeleteObject(m_mirrorBitmap);
            }

            m_mirrorBitmap = newMirror;
            m_pMirrorData = pData;
            ::memcpy(&m_mirrorBitmapInfo, &bitmapInfo, sizeof(BITMAPINFO));
        }

        m_pOutData = pData;
    }

    void RenderSurface::InitContext(Context& context)
    {
        if (m_mirrorBitmap)
        {
            HDC dc = ::CreateCompatibleDC(NULL);
            auto has = ::SelectObject(dc, m_mirrorBitmap);

            context.OnAttached(dc, this);
        }
    }

    void RenderSurface::Flush()
    {
        if (m_pOutData)
        {
            ::memcpy(m_pOutData, m_pMirrorData, DIBSIZE(m_mirrorBitmapInfo.bmiHeader));
        }
    }

    void RenderSurface::GetSurfaceRect(PRECT pRect) const
    {
		RECT tmp = { 0, 0, m_mirrorBitmapInfo.bmiHeader.biWidth, -m_mirrorBitmapInfo.bmiHeader.biHeight };
        *pRect = tmp;
    }

#elif USE_CAIRO

    RenderSurface::RenderSurface()
        : m_cairo_surface(NULL)
    {

    }

    RenderSurface::~RenderSurface()
    {
        if (m_cairo_surface)
        {
            ::cairo_surface_destroy(m_cairo_surface);
        }
    }

    void RenderSurface::SetSource(PBYTE pData, ULONG uLen, LONG iWidth, LONG iHeight, UINT iFormat)
    {
        //! must be 32bit
        if (32 != iFormat)
            throw 0;

        if (m_cairo_surface)
        {
            int w = cairo_image_surface_get_width(m_cairo_surface);
            int h = cairo_image_surface_get_height(m_cairo_surface);

            if (w != iWidth || h != iHeight)
            {
                ::cairo_surface_destroy(m_cairo_surface);
            }
        }

        m_cairo_surface = ::cairo_image_surface_create_for_data(pData, CAIRO_FORMAT_ARGB32, iWidth, iHeight, ::cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, iWidth));
    }

    void RenderSurface::InitContext(Context& context)
    {
        if (m_cairo_surface)
        {
            cairo_t *dc = ::cairo_create(m_cairo_surface);
            context.OnAttached(dc, this);
        }
    }

    void RenderSurface::Flush()
    {
        if (m_cairo_surface)
        {
            cairo_surface_flush(m_cairo_surface);
        }
    }

    void RenderSurface::GetSurfaceRect(PRECT pRect) const
    {
        if (m_cairo_surface)
        {
            int w = cairo_image_surface_get_width(m_cairo_surface);
            int h = cairo_image_surface_get_height(m_cairo_surface);

            pRect->left = 0;
            pRect->top = 0;
            pRect->right = w;
            pRect->bottom = h;
        }
    }

#endif

}