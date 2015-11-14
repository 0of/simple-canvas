#pragma once

#include <stack>

#if USE_CAIRO
struct _cairo;
#endif

#define RGBA(r,g,b,a) ((COLORREF)(((BYTE)(r)|((WORD)((BYTE)(g))<<8))|(((DWORD)(BYTE)(b))<<16)|(((DWORD)(BYTE)(a))<<24)))
#define GetAValue(rgba) (LOBYTE((rgba)>>24))
#define MAKE_COLOR(rgb) ((COLORREF)(rgb)|(((DWORD)(BYTE)(255))<<24))

namespace Render
{
    class Surface;
    class Image;

    class Font
    {
    public:
        LOGFONTW m_font;    //! direct access

    public:
        Font();
        Font(const String& family, int px);
        Font(const String& family, int px, int weight, bool italic = false);

    public:
        void SetFamily(const String& family);
        void SetPixelSize(int px);

    public:
        String GetFamily() const { return String(m_font.lfFaceName); }
        int GetPixelSize() const { return m_font.lfWidth; }
    };

    class Pen
    {
    public:
        enum Style
        {
            Null = 0, Solid, Dash, Dot,
        };

    private:
        Style m_style;
        float m_width;
        COLORREF m_color;

    public:
        Pen();
        Pen(Style style, float width, COLORREF color);
        ~Pen();

    public:
        Style GetStyle() const { return m_style; }
        float GetWidth() const { return m_width; }
        COLORREF GetColor() const { return m_color; }

    public:
        bool IsNull() const { return m_style == Null; }
    };

    class Brush
    {
    public:
        enum Style
        {
            Null = 0, Solid
        };

    private:
        Style m_style;
        COLORREF m_color;

    public:
        Brush();
        Brush(Style st, COLORREF color);
        ~Brush();

    public:
        Style GetStyle() const { return m_style; }
        COLORREF GetColor() const { return m_color; }

    public:
        bool IsNull() const { return m_style == Null; }
    };

#if USE_GDI

    class TextMetric
    {
    private:
        Font m_font;

    public:
        TextMetric();
        explicit TextMetric(const Font& f);
        ~TextMetric();

    public:
        //! bounding size
        SIZE GetMeasureSize(const String& text) const;
    };

    class AffineMaxtrix
    {
    public:
        XFORM m_maxtrix;

    public:
        AffineMaxtrix();
        AffineMaxtrix(float m11, float m12,
            float m21, float m22,
            float dx, float dy);

    public:
        void Rotate(float degrees);
        void Scale(float xFactor, float yFactor);
        void Sheer(float xFactor, float yFactor);
        void Translate(float dx, float dy);
    };

    class Context
    {
        friend class RenderSurface;

        class ContextStatus
        {
        public:
            Font font;
            Pen pen;
            Brush brush;
            AffineMaxtrix transform;
            UINT opaque;         //! max 255

        public:
            ContextStatus() : opaque(255) {}
        };

    private:
        typedef std::stack<ContextStatus> ContextStatusStack;
        ContextStatusStack m_status;

        HDC m_DC;
        Surface *m_surface;

    public:
        Context();
        ~Context();

    public:
        void Reset();

    public:
        void SetFont(const Font& f);
        void SetPen(const Pen& p);
        void SetBrush(const Brush& b);
        void SetTransform(const AffineMaxtrix& matrix);
        void SetOpaque(BYTE opaque);

    public:
        void DrawImage(Image *);
        void DrawImageAt(Image *, const POINT& pos);
        //! 
        void DrawClippedImage(Image *, const RECT& region);
        void DrawClippedImageAt(Image *, const RECT& region, const POINT& pos);

        void DrawScaledImage(Image *, const RECT& region);

        void DrawText(const String& text);
        void DrawTextAt(POINT where, const String& text);

    public:
        void Save();
        void Restore();

    public:
        TextMetric GetTextMetric() const;

        //! right is width, bottom is height
        RECT GetBoundingRegion() const;

    public:
        bool IsValid() const { return NULL != m_DC; }

    protected:
        void OnAttached(HDC dc, Surface *surface);

    private:
        Context(const Context&);
        Context& operator = (const Context&);
    };

#elif USE_CAIRO
    
    class TextMetric
    {
    private:
        struct _cairo *m_boundDC;

    public:
        TextMetric();
        explicit TextMetric(struct _cairo *boundDC);
        ~TextMetric();

    public:
        //! bounding size
        SIZE GetMeasureSize(const String& text) const;
    };

    class Context
    {
        friend class RenderSurface;

        class ContextStatus
        {
        public:
            UINT opaque;         //! max 255
            Pen pen;
            Brush brush;

        public:
            ContextStatus() : opaque(255) {}
        };

    private:
        typedef std::stack<ContextStatus> ContextStatusStack;
        ContextStatusStack m_status;

        Surface *m_surface;
        struct _cairo *m_DC;

    public:
        Context();
        ~Context();

    public:
        void Reset();

    public:
        void SetFont(const Font& f);
        void SetPen(const Pen& p);
        void SetBrush(const Brush& b);
        // void SetTransform(const AffineMaxtrix& matrix);
        void SetOpaque(BYTE opaque);

    public:
        void DrawImage(Image *);
        void DrawImageAt(Image *, const POINT& pos);
        //! 
        void DrawClippedImage(Image *, const RECT& region);
        void DrawClippedImageAt(Image *, const RECT& region, const POINT& pos);

        void DrawText(const String& text);
        void DrawTextAt(const POINT& where, const String& text);

    public:
        void Save();
        void Restore();

    public:
        TextMetric GetTextMetric() const;

        //! right is width, bottom is height
        RECT GetBoundingRegion() const;

    public:
        bool IsValid() const { return NULL != m_DC; }

    protected:
        void OnAttached(struct _cairo *dc, Surface *surface);

    private:
        Context(const Context&);
        Context& operator = (const Context&);
    };

#endif
}