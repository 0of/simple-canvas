#include "Context.h"

#include "Image.h"
#include "Surface.h"
#include "wingdi.h"

#pragma comment(lib, "Msimg32.lib")

#if USE_CAIRO
#include "cairo.h"

#include <string>
#include <codecvt>
#include <locale>
#pragma comment(lib, "cairo.lib")
#endif


namespace DuiLibImport
{
    static COLORREF PixelAlpha(COLORREF clrSrc, double src_darken, COLORREF clrDest, double dest_darken)
    {
        return RGB(GetRValue(clrSrc) * src_darken + GetRValue(clrDest) * dest_darken,
            GetGValue(clrSrc) * src_darken + GetGValue(clrDest) * dest_darken,
            GetBValue(clrSrc) * src_darken + GetBValue(clrDest) * dest_darken);
    }

    static BOOL WINAPI AlphaBitBlt(HDC hDC, int nDestX, int nDestY, int dwWidth, int dwHeight, HDC hSrcDC, \
        int nSrcX, int nSrcY, int wSrc, int hSrc, BLENDFUNCTION ftn)
    {
        HDC hTempDC = ::CreateCompatibleDC(hDC);
        if (NULL == hTempDC)
            return FALSE;

        //Creates Source DIB
        LPBITMAPINFO lpbiSrc = NULL;
        // Fill in the BITMAPINFOHEADER
        lpbiSrc = (LPBITMAPINFO) new BYTE[sizeof(BITMAPINFOHEADER)];
        if (lpbiSrc == NULL)
        {
            ::DeleteDC(hTempDC);
            return FALSE;
        }
        lpbiSrc->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        lpbiSrc->bmiHeader.biWidth = dwWidth;
        lpbiSrc->bmiHeader.biHeight = dwHeight;
        lpbiSrc->bmiHeader.biPlanes = 1;
        lpbiSrc->bmiHeader.biBitCount = 32;
        lpbiSrc->bmiHeader.biCompression = BI_RGB;
        lpbiSrc->bmiHeader.biSizeImage = dwWidth * dwHeight;
        lpbiSrc->bmiHeader.biXPelsPerMeter = 0;
        lpbiSrc->bmiHeader.biYPelsPerMeter = 0;
        lpbiSrc->bmiHeader.biClrUsed = 0;
        lpbiSrc->bmiHeader.biClrImportant = 0;

        COLORREF* pSrcBits = NULL;
        HBITMAP hSrcDib = CreateDIBSection(
            hSrcDC, lpbiSrc, DIB_RGB_COLORS, (void **)&pSrcBits,
            NULL, NULL);

        if ((NULL == hSrcDib) || (NULL == pSrcBits))
        {
            delete[] lpbiSrc;
            ::DeleteDC(hTempDC);
            return FALSE;
        }

        HBITMAP hOldTempBmp = (HBITMAP)::SelectObject(hTempDC, hSrcDib);
        ::StretchBlt(hTempDC, 0, 0, dwWidth, dwHeight, hSrcDC, nSrcX, nSrcY, wSrc, hSrc, SRCCOPY);
        ::SelectObject(hTempDC, hOldTempBmp);

        //Creates Destination DIB
        LPBITMAPINFO lpbiDest = NULL;
        // Fill in the BITMAPINFOHEADER
        lpbiDest = (LPBITMAPINFO) new BYTE[sizeof(BITMAPINFOHEADER)];
        if (lpbiDest == NULL)
        {
            delete[] lpbiSrc;
            ::DeleteObject(hSrcDib);
            ::DeleteDC(hTempDC);
            return FALSE;
        }

        lpbiDest->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        lpbiDest->bmiHeader.biWidth = dwWidth;
        lpbiDest->bmiHeader.biHeight = dwHeight;
        lpbiDest->bmiHeader.biPlanes = 1;
        lpbiDest->bmiHeader.biBitCount = 32;
        lpbiDest->bmiHeader.biCompression = BI_RGB;
        lpbiDest->bmiHeader.biSizeImage = dwWidth * dwHeight;
        lpbiDest->bmiHeader.biXPelsPerMeter = 0;
        lpbiDest->bmiHeader.biYPelsPerMeter = 0;
        lpbiDest->bmiHeader.biClrUsed = 0;
        lpbiDest->bmiHeader.biClrImportant = 0;

        COLORREF* pDestBits = NULL;
        HBITMAP hDestDib = CreateDIBSection(
            hDC, lpbiDest, DIB_RGB_COLORS, (void **)&pDestBits,
            NULL, NULL);

        if ((NULL == hDestDib) || (NULL == pDestBits))
        {
            delete[] lpbiSrc;
            ::DeleteObject(hSrcDib);
            ::DeleteDC(hTempDC);
            return FALSE;
        }

        ::SelectObject(hTempDC, hDestDib);
        ::BitBlt(hTempDC, 0, 0, dwWidth, dwHeight, hDC, nDestX, nDestY, SRCCOPY);
        ::SelectObject(hTempDC, hOldTempBmp);

        double src_darken;
        BYTE nAlpha;

        for (int pixel = 0; pixel < dwWidth * dwHeight; pixel++, pSrcBits++, pDestBits++)
        {
            nAlpha = LOBYTE(*pSrcBits >> 24);
            src_darken = (double)(nAlpha * ftn.SourceConstantAlpha) / 255.0 / 255.0;
            if (src_darken < 0.0) src_darken = 0.0;
            *pDestBits = PixelAlpha(*pSrcBits, src_darken, *pDestBits, 1.0 - src_darken);
        } //for

        ::SelectObject(hTempDC, hDestDib);
        ::BitBlt(hDC, nDestX, nDestY, dwWidth, dwHeight, hTempDC, 0, 0, SRCCOPY);
        ::SelectObject(hTempDC, hOldTempBmp);

        delete[] lpbiDest;
        ::DeleteObject(hDestDib);

        delete[] lpbiSrc;
        ::DeleteObject(hSrcDib);

        ::DeleteDC(hTempDC);
        return TRUE;
    }
}

namespace Render
{
    static void IntersectRegion(LPRECT outRegion, const SIZE& s1, const RECT& r2)
    {
        outRegion->left = max(0, r2.left);
        outRegion->top = max(0, r2.top);

        outRegion->right = min(s1.cx - outRegion->left, r2.left + r2.right - outRegion->left);
        outRegion->bottom = min(s1.cy - outRegion->top, r2.top + r2.bottom - outRegion->top);
    }

    typedef BOOL(WINAPI *LPALPHABLEND)(HDC, int, int, int, int, HDC, int, int, int, int, BLENDFUNCTION);
    static LPALPHABLEND lpAlphaBlend = AlphaBlend;/*(LPALPHABLEND) ::GetProcAddress(::GetModuleHandle(_T("msimg32.dll")), "AlphaBlend");*/

    Font::Font()
        : m_font()
    {
        ::memset(&m_font, 0, sizeof(LOGFONTW));
    }

    Font::Font(const String& family, int px)
        : m_font()
    {
        ::memset(&m_font, 0, sizeof(LOGFONTW));
        ::memcpy(m_font.lfFaceName, family.c_str(), min(LF_FACESIZE, family.length()) *sizeof(WCHAR));
        m_font.lfWidth = px;
    }

    Font::Font(const String& family, int px, int weight, bool italic)
        : m_font()
    {
        ::memset(&m_font, 0, sizeof(LOGFONTW));
        ::memcpy(m_font.lfFaceName, family.c_str(), min(LF_FACESIZE, family.length()) *sizeof(WCHAR));
        m_font.lfWidth = px;
        m_font.lfWeight = weight;
        m_font.lfItalic = italic;
    }

    void Font::SetFamily(const String& family)
    {
        ::memcpy(m_font.lfFaceName, family.c_str(), min(LF_FACESIZE, family.length()) *sizeof(WCHAR));
    }

    void Font::SetPixelSize(int px)
    {
        m_font.lfWidth = px;
    }

    Pen::Pen()
        : m_style(Pen::Null)
        , m_width(1.f)
        , m_color(~0)
    {}
    
    Pen::Pen(Pen::Style style, float width, COLORREF color)
        : m_style(style)
        , m_width(width)
        , m_color(color)
    {}

    Pen::~Pen()
    {

    }


    Brush::Brush()
        : m_style(Brush::Null)
        , m_color(~0)
    {}

    Brush::Brush(Style st, COLORREF color)
        : m_style(st)
        , m_color(color)
    {}

    Brush::~Brush()
    {

    }

#if USE_GDI

    AffineMaxtrix::AffineMaxtrix()
        : m_maxtrix()
    {}

    AffineMaxtrix::AffineMaxtrix(float m11, float m12,
        float m21, float m22,
        float dx, float dy)
        : m_maxtrix()
    {

    }

    void Rotate(float degrees)
    {}
    void Scale(float xFactor, float yFactor)
    {}
    void Sheer(float xFactor, float yFactor)
    {}
    void Translate(float dx, float dy)
    {}

    TextMetric::TextMetric()
        : m_font()
    {
    }

    TextMetric::TextMetric(const Font& f)
        : m_font(f)
    {
    }

    TextMetric::~TextMetric()
    {
    }

    SIZE TextMetric::GetMeasureSize(const String& text) const
    {
        HDC memDC = ::CreateCompatibleDC(NULL);
        RECT textBoundingRect = { 0 };
        HFONT hFont = CreateFontIndirect(&m_font.m_font);

        ::SelectObject(memDC, hFont);
        ::DrawText(memDC, text.c_str(), text.length(), &textBoundingRect, DT_CALCRECT);

        ::DeleteObject(hFont);
        ::DeleteObject(memDC);

		SIZE tmp = { textBoundingRect.right - textBoundingRect.left, textBoundingRect.bottom - textBoundingRect.top };

        return tmp;
    }

    Context::Context()
        : m_DC(NULL)
        , m_surface(NULL)
    {
        
    }

    Context::~Context()
    {
        if (m_DC)
            ::DeleteObject(m_DC);
    }

    void Context::Reset()
    {
        if (m_surface)
        {
            if (m_DC)
                ::DeleteObject(m_DC);

            m_surface->InitContext(*this);
        }
    }

    void Context::SetFont(const Font& f)
    {
        m_status.top().font = f;
    }

    void Context::SetPen(const Pen& p)
    {
        m_status.top().pen = p;
    }

    void Context::SetBrush(const Brush& b)
    {
        m_status.top().brush = b;

        if (m_DC)
            ::SetBkMode(m_DC, m_status.top().brush.IsNull() ? TRANSPARENT : OPAQUE);
    }

    void Context::SetTransform(const AffineMaxtrix& matrix)
    {
        m_status.top().transform = matrix;
    }

    void Context::SetOpaque(BYTE opaque)
    {
        m_status.top().opaque = opaque;
    }

    void Context::DrawImage(Image *image)
    {
        POINT zero = { 0, 0 };
        DrawImageAt(image, zero);
    }

    void Context::DrawImageAt(Image *image, const POINT& pos)
    {
        UINT opaque = m_status.top().opaque;

        if (opaque && m_surface && m_DC && !image->IsNull())
        {
            RECT desRect = { 0 };
            m_surface->GetSurfaceRect(&desRect);

            if (pos.x > desRect.right || pos.y > desRect.bottom)
                return;

            HDC srcDC = ::CreateCompatibleDC(NULL);

            ::SelectObject(srcDC, image->m_bitmap);

            if (255 != opaque)
            {
                BLENDFUNCTION bf = { AC_SRC_OVER, 0, opaque, 0 };
                lpAlphaBlend(m_DC, pos.x, pos.y, desRect.right, desRect.bottom,
                    srcDC, 0, 0, image->GetWidth(), image->GetHeight(), bf);
            }
            else
            {
                ::BitBlt(m_DC, pos.x, pos.y, desRect.right, desRect.bottom, srcDC, 0, 0, SRCCOPY);
            }

            ::DeleteObject(srcDC);
        }
    }

    void Context::DrawClippedImage(Image *image, const RECT& region)
    {
        if (region.right <= 0 || region.bottom <= 0)
            return;

        if (m_surface && m_DC && !image->IsNull())
        {
            RECT desRect = { 0 };
            m_surface->GetSurfaceRect(&desRect);

            HDC srcDC = ::CreateCompatibleDC(NULL);

            ::SelectObject(srcDC, image->m_bitmap);
         
            ::BitBlt(m_DC, desRect.left, desRect.top, region.right, region.bottom, srcDC, region.left, region.top, SRCCOPY);

            ::DeleteObject(srcDC);
        }
    }

    void Context::DrawClippedImageAt(Image *image, const RECT& region, const POINT& pos)
    {
        if (region.right <= 0 || region.bottom <= 0)
            return;

        if (m_surface && m_DC && !image->IsNull())
        {
            RECT desRect = { 0 };
            m_surface->GetSurfaceRect(&desRect);

            if (pos.x > desRect.right || pos.y > desRect.bottom)
                return;

            HDC srcDC = ::CreateCompatibleDC(NULL);

            ::SelectObject(srcDC, image->m_bitmap);

            ::BitBlt(m_DC, pos.x, pos.y, region.right, region.bottom, srcDC, region.left, region.top, SRCCOPY);

            ::DeleteObject(srcDC);
        }
    }

    void Context::DrawScaledImage(Image *image, const RECT& region)
    {
        if (region.right <= 0 || region.bottom <= 0)
            return;

        if (m_surface && m_DC && !image->IsNull())
        {
            RECT desRect = { 0 };
            m_surface->GetSurfaceRect(&desRect);

            HDC srcDC = ::CreateCompatibleDC(NULL);
            ::SelectObject(srcDC, image->m_bitmap);

            ::SetStretchBltMode(m_DC, HALFTONE);
            ::StretchBlt(m_DC, desRect.left, desRect.top, desRect.right, desRect.bottom,
                srcDC, 0, 0, image->GetWidth(), image->GetHeight(), SRCCOPY);

            ::DeleteObject(srcDC);
        }
    }

    void Context::DrawText(const String& text)
    {
        POINT topLeft = { 0, 0 };
        DrawTextAt(topLeft, text);
    }

    void Context::DrawTextAt(POINT where, const String& text)
    {
        RECT pos = { where.x, where.y, 0, 0 };

        if (!text.empty() && m_surface && m_DC)
        {
            auto currentStatus = m_status.top();

            const Font& currentFont = currentStatus.font;
            ::SetTextColor(m_DC, currentStatus.pen.GetColor());

            HFONT fObj = ::CreateFontIndirect(&currentFont.m_font);
            HGDIOBJ fPre = ::SelectObject(m_DC, fObj);

            if (!currentStatus.brush.IsNull())
            {
                SetBkColor(m_DC, currentStatus.brush.GetColor());
            }
            ::DrawText(m_DC, text.c_str(), text.length(), &pos, DT_LEFT | DT_TOP | DT_NOCLIP);

            ::SelectObject(m_DC, fPre);
            ::DeleteObject(fObj);
        }
    }

    TextMetric Context::GetTextMetric() const
    {
        return TextMetric(m_status.top().font);
    }

    RECT Context::GetBoundingRegion() const
    {
        RECT rect = { 0 };
        if (m_surface)
        {
            m_surface->GetSurfaceRect(&rect);
        }

        return rect;
    }

    void Context::Save()
    {
        if (m_DC)
        {
            m_status.push(ContextStatus());
            ::SetBkMode(m_DC, TRANSPARENT);
        }
    }

    void Context::Restore()
    {
        if (m_status.size() > 1)
        {
            m_status.pop();

            if (m_DC)
                ::SetBkMode(m_DC, m_status.top().brush.IsNull() ? TRANSPARENT : OPAQUE);
        }
    }

    void Context::OnAttached(HDC dc, Surface *surface)
    {
        m_DC = dc;
        m_surface = surface;

        m_status = ContextStatusStack();
        m_status.push(ContextStatus());

        ::SetBkMode(m_DC, TRANSPARENT);
    }

#elif USE_CAIRO

    TextMetric::TextMetric()
        : m_boundDC(NULL)
    {

    }

    TextMetric::TextMetric(struct _cairo *boundDC)
        : m_boundDC(boundDC)
    {

    }

    TextMetric::~TextMetric()
    {

    }

    SIZE TextMetric::GetMeasureSize(const String& text) const
    {
        SIZE size = { 0 };

        if (m_boundDC)
        {
            std::wstring_convert<std::codecvt_utf8<wchar_t>> utf8_conv;
            std::string utf8_text = utf8_conv.to_bytes(text);

            cairo_text_extents_t extends = { 0 };
            ::cairo_text_extents(m_boundDC, utf8_text.c_str(), &extends);
        }
        
        return size;
    }

    Context::Context()
        : m_status()
        , m_surface(NULL)
        , m_DC(NULL)
    {

    }

    Context::~Context()
    {
        if (m_DC)
            ::cairo_destroy(m_DC);
    }

    void Context::Reset()
    {
        if (m_surface)
        {
            if (m_DC)
                ::cairo_destroy(m_DC);

            m_surface->InitContext(*this);
        }
    }

    void Context::SetFont(const Font& f)
    {
        if (m_DC)
        {
            std::wstring_convert<std::codecvt_utf8<wchar_t>> utf8_conv;
            std::string face = utf8_conv.to_bytes(f.m_font.lfFaceName);
            ::cairo_select_font_face(m_DC, face.c_str(), CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
        }
    }

    void Context::SetPen(const Pen& p)
    {
        if (m_DC)
        {
            ::cairo_set_line_width(m_DC, p.GetWidth());
            m_status.top().pen = p;
        }
    }

    void Context::SetBrush(const Brush& b)
    {
        if (m_DC)
        {
            m_status.top().brush = b;
        }
    }

    void Context::SetOpaque(BYTE opaque)
    {
        if (m_DC)
        {
            m_status.top().opaque = opaque;
        }
    }

    void Context::DrawImage(Image *image)
    {
        POINT pos = { 0, 0 };
        DrawImageAt(image, pos);
    }

    void Context::DrawImageAt(Image *image, const POINT& pos)
    {
        UINT opaque = m_status.top().opaque;

        if (opaque && m_surface && m_DC && !image->IsNull())
        {
            cairo_surface_t *srcSurface = cairo_image_surface_create_for_data(image->GetOffset(0, 0), CAIRO_FORMAT_RGB24, image->GetWidth(), image->GetHeight(), image->GetStride());
            ::cairo_set_source_surface(m_DC, srcSurface, pos.x, pos.y);

            if (255 == opaque)
            {
                ::cairo_paint(m_DC);
            }
            else
            {
                ::cairo_paint_with_alpha(m_DC, opaque / 255.);
            }

            ::cairo_surface_destroy(srcSurface);
        }
    }

    void Context::DrawClippedImage(Image *image, const RECT& region)
    {
        POINT pos = { 0, 0 };
        DrawClippedImageAt(image, region, pos);
    }

    void Context::DrawClippedImageAt(Image *image, const RECT& region, const POINT& pos)
    {
        UINT opaque = m_status.top().opaque;

        if (opaque && m_surface && m_DC && !image->IsNull())
        {
            RECT desRect = { 0 };
            m_surface->GetSurfaceRect(&desRect);

            if (pos.x > desRect.right || pos.y > desRect.bottom)
                return;

            RECT clipRegion = { 0 };

            IntersectRegion(&clipRegion, image->GetSize(), region);
            if (clipRegion.right <= 0 || clipRegion.bottom <= 0)
                return;

            cairo_surface_t *srcSurface = cairo_image_surface_create_for_data(image->GetOffset(clipRegion.left, clipRegion.top), CAIRO_FORMAT_RGB24, clipRegion.right, clipRegion.bottom, image->GetStride());
            ::cairo_set_source_surface(m_DC, srcSurface, pos.x, pos.y);

            if (255 == opaque)
            {
                ::cairo_paint(m_DC);
            }
            else
            {
                ::cairo_paint_with_alpha(m_DC, opaque / 255.);
            }

            ::cairo_surface_destroy(srcSurface);
        }
    }

    void Context::DrawText(const String& text)
    {
        POINT pos = { 0, 0 };
        DrawTextAt(pos, text);
    }

    void Context::DrawTextAt(const POINT& where, const String& text)
    {
        if (text.empty())
            return;

        const auto& currentStatus = m_status.top();

        if (currentStatus.opaque && m_surface && m_DC)
        {
            auto textColor = currentStatus.pen.GetColor();
            if (255 == currentStatus.opaque && 255 == GetAValue(textColor))
            {
                ::cairo_set_source_rgb(m_DC, GetRValue(textColor) / 255., GetGValue(textColor) / 255., GetBValue(textColor) / 255.);
            }
            else
            {
                ::cairo_set_source_rgba(m_DC, GetRValue(textColor) / 255., GetGValue(textColor) / 255., GetBValue(textColor) / 255.,
                    (currentStatus.opaque / 255.) * (GetAValue(textColor) / 255.));
            }

            std::wstring_convert<std::codecvt_utf8<wchar_t>> utf8_conv;
            std::string utf8_text = utf8_conv.to_bytes(text);

            cairo_text_extents_t extends = { 0 };
            ::cairo_text_extents(m_DC, utf8_text.c_str(), &extends);

            ::cairo_move_to(m_DC, where.x, where.y - extends.y_bearing);
            ::cairo_show_text(m_DC, utf8_text.c_str());
        }
    }

    void Context::Save()
    {
        if (m_DC)
        {
            m_status.push(ContextStatus());
            ::cairo_save(m_DC);
        }
    }

    void Context::Restore()
    {
        if (m_DC)
        {
            if (m_status.size() > 1)
            {
                m_status.pop();
            }
            ::cairo_restore(m_DC);
        }
    }

    TextMetric Context::GetTextMetric() const
    {
        return TextMetric(m_DC);
    }

    RECT Context::GetBoundingRegion() const
    {
        RECT rect = { 0 };
        if (m_surface)
        {
            m_surface->GetSurfaceRect(&rect);
        }

        return rect;
    }

    void Context::OnAttached(struct _cairo *dc, Surface *surface)
    {
        m_DC = dc;
        m_surface = surface;

        m_status = ContextStatusStack();
        m_status.push(ContextStatus());
    }

#endif

}