#pragma once

#if USE_CAIRO
struct _cairo_surface;
#endif

namespace Render
{
    class Context;

    class Surface
    {
    public:
        virtual ~Surface() {}

    public:
        virtual void SetSource(PBYTE pData, ULONG uLen, LONG iWidth, LONG iHeight, UINT iFormat) = 0;
        virtual void InitContext(Context& context) = 0;
        virtual void Flush() = 0;
        //! pRect right/ bottom means width and height
        virtual void GetSurfaceRect(PRECT pRect) const = 0;
    };

#if USE_GDI

    class RenderSurface : public Surface
    {
    private:
        //! mirror
        PBYTE m_pMirrorData;
        HBITMAP m_mirrorBitmap;
        BITMAPINFO m_mirrorBitmapInfo;

        PBYTE m_pOutData;

    public:
        RenderSurface();
        virtual ~RenderSurface();

    public:
        virtual void SetSource(PBYTE pData, ULONG uLen, LONG iWidth, LONG iHeight, UINT iFormat);
        virtual void InitContext(Context& context);
        virtual void Flush();
        virtual void GetSurfaceRect(PRECT pRect) const;
    };

#elif USE_CAIRO

    class RenderSurface : public Surface
    {
    private:
        struct _cairo_surface *m_cairo_surface;

    public:
        RenderSurface();
        virtual ~RenderSurface();

    public:
        virtual void SetSource(PBYTE pData, ULONG uLen, LONG iWidth, LONG iHeight, UINT iFormat);
        virtual void InitContext(Context& context);
        virtual void Flush();
        virtual void GetSurfaceRect(PRECT pRect) const;
    };

#endif
}