#include "Image.h"

#include <Amvideo.h>
#include <gdiplus.h>

#define BITMAP_BITCOUNT 32

namespace Render
{
    static HBITMAP CreateBitmap(const SIZE& s, LPVOID *ppData, BITMAPINFO *bitmapInfo)
    {
        memset(bitmapInfo, 0, sizeof(BITMAPINFO));

        bitmapInfo->bmiHeader.biCompression = BI_RGB;
        bitmapInfo->bmiHeader.biBitCount = BITMAP_BITCOUNT;
        bitmapInfo->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bitmapInfo->bmiHeader.biWidth = s.cx;
        bitmapInfo->bmiHeader.biHeight = -s.cy;
        bitmapInfo->bmiHeader.biPlanes = 1;

        return CreateDIBSection(NULL, bitmapInfo, DIB_RGB_COLORS, ppData, NULL, 0);
    }

    bool Image::Allocate(Image *nullImage, const SIZE& size)
    {
        if (!nullImage->IsNull())
            return false;

        if (size.cx <= 0 || size.cy <= 0)
            return false;

        BITMAPINFO desBitmapInfo;
        PBYTE pDesData = NULL;

        HBITMAP bitmap = CreateBitmap(size, (LPVOID *)&pDesData, &desBitmapInfo);

        if (bitmap)
        {
            ::memcpy(&nullImage->m_bitmapInfo, &desBitmapInfo, sizeof(BITMAPINFO));
            nullImage->m_pData = pDesData;
            nullImage->m_bitmap = bitmap;

            return true;
        }

        return false;
    }

    bool Image::ReAllocate(Image *image, const SIZE& size) throw()
    {
        if (size.cx <= 0 || size.cy <= 0)
            return false;

        if (!image->IsNull())
            image->Clean();

        return Allocate(image, size);
    }

    bool Image::Initialize(Image *nullImage, PBYTE pData, HBITMAP bitmap, const BITMAPINFO& bitmapInfo)
    {
        if (!nullImage->IsNull())
            return false;

        if (32 != bitmapInfo.bmiHeader.biBitCount)
            return false;

        nullImage->m_pData = pData;
        nullImage->m_bitmap = bitmap;

        ::memcpy(&nullImage->m_bitmapInfo, &bitmapInfo, sizeof(BITMAPINFO));

        return true;
    }

    static HBITMAP CreateBitmapFrom(Gdiplus::Image *srcBitmap, LPVOID *ppData, BITMAPINFO *bitmapInfo)
    {
        SIZE imageSize = { srcBitmap->GetWidth(), srcBitmap->GetHeight() };
        HBITMAP desBitmap = CreateBitmap(imageSize, ppData, bitmapInfo);

        if (desBitmap)
        {
            //! draw the bitmap
            HDC desDC = ::CreateCompatibleDC(NULL);
            ::SelectObject(desDC, desBitmap);

            Gdiplus::Graphics g(desDC);
            if (Gdiplus::Ok != g.DrawImage(srcBitmap, 0, 0))
            {
                //! when failed, delete the bitmap
                ::DeleteObject(desBitmap);

                desBitmap = NULL;
                *ppData = NULL;
            }

            ::DeleteObject(desDC);
        }

        return desBitmap;
    }

    Image::Image()
        : m_pData(NULL)
        , m_bitmap(NULL)
        , m_bitmapInfo()
    {

    }

    Image::~Image()
    {

    }

    void Image::Clean()
    {
        if (m_bitmap)
        {
            ::DeleteObject(m_bitmap);
            m_bitmap = NULL;
            m_pData = NULL;
        }
    }

    PBYTE Image::GetOffset(UINT w, UINT h)
    {
        if (h > static_cast<UINT>(-m_bitmapInfo.bmiHeader.biHeight) || w > static_cast<UINT>(m_bitmapInfo.bmiHeader.biWidth))
            return NULL;

        return m_pData + h * DIBWIDTHBYTES(m_bitmapInfo.bmiHeader) + w * (m_bitmapInfo.bmiHeader.biBitCount / 8);
    }

    const PBYTE Image::GetOffset(UINT w, UINT h) const
    {
        if (h > static_cast<UINT>(-m_bitmapInfo.bmiHeader.biHeight) || w > static_cast<UINT>(m_bitmapInfo.bmiHeader.biWidth))
            return NULL;

        return m_pData + h * DIBWIDTHBYTES(m_bitmapInfo.bmiHeader) + w * (m_bitmapInfo.bmiHeader.biBitCount / 8);
    }

    SIZE Image::GetSize() const
    {
        SIZE s = { m_bitmapInfo.bmiHeader.biWidth, -m_bitmapInfo.bmiHeader.biHeight };
        return s;
    }

    INT Image::GetWidth() const
    {
        return m_bitmapInfo.bmiHeader.biWidth;
    }

    INT Image::GetHeight() const
    {
        return -m_bitmapInfo.bmiHeader.biHeight;
    }

    INT Image::GetDepth() const
    {
        return m_bitmapInfo.bmiHeader.biBitCount / 8;
    }

    INT Image::GetStride() const
    {
        return DIBWIDTHBYTES(m_bitmapInfo.bmiHeader);
    }

    void Image::Scale(const SIZE& size)
    {
        if (!m_bitmap)
            return;

        if (size.cx <= 0 || size.cy <= 0)
            return;

        if (size.cx == m_bitmapInfo.bmiHeader.biWidth && size.cy == -m_bitmapInfo.bmiHeader.biHeight)
            return;

        HDC srcDC = ::CreateCompatibleDC(NULL);
        HDC desDC = ::CreateCompatibleDC(NULL);
        
        //! src
        HBITMAP srcBitmap = m_bitmap;
        BITMAPINFO srcBitmapInfo;
        ::memcpy(&srcBitmapInfo, &m_bitmapInfo, sizeof(BITMAPINFO));

        //! des
        BITMAPINFO desBitmapInfo;
        PBYTE pDesData = NULL;
        HBITMAP desBitmap = CreateBitmap(size, (PVOID *)&pDesData, &desBitmapInfo);

        if (desBitmap)
        {
            ::SelectObject(srcDC, srcBitmap);
            ::SelectObject(desDC, desBitmap);

            ::SetStretchBltMode(desDC, HALFTONE);
            ::StretchBlt(desDC, 0, 0, size.cx, size.cy,
                srcDC, 0, 0, srcBitmapInfo.bmiHeader.biWidth, -srcBitmapInfo.bmiHeader.biHeight, SRCCOPY);

            ::DeleteObject(srcBitmap);

            //! update 
            ::memcpy(&m_bitmapInfo, &desBitmapInfo, sizeof(BITMAPINFO));
            m_pData = pDesData;
            m_bitmap = desBitmap;
        }

        ::DeleteObject(desDC);
        ::DeleteObject(srcDC);
    }

    class GIDPlusContext
    {
    private:
        Gdiplus::Status m_status;
        ULONG_PTR m_token;

    public:
        GIDPlusContext()
        {
            Gdiplus::GdiplusStartupInput gdiplusStartupInput;
            m_status = Gdiplus::GdiplusStartup(&m_token, &gdiplusStartupInput, NULL);
        }

        ~GIDPlusContext()
        {
            if (m_token)
            {
                Gdiplus::GdiplusShutdown(m_token);
            }
        }

    public:
        bool TestOK() const 
        {
            return m_status == Gdiplus::Ok;
        }
    };

    template<typename _Pointer_Type>
    class ScopedPointer
    {
        typedef typename _Pointer_Type _Given_Pointer_Type;
        typedef typename ScopedPointer<_Pointer_Type> _Self_Type;

    public:
        ScopedPointer(_Given_Pointer_Type *_raw_ptr)
            :m_rawPtr(_raw_ptr){}

        ~ScopedPointer()
        {
            if (m_rawPtr)
                delete m_rawPtr;
        }

    private:
        ScopedPointer(const _Self_Type&);
        _Self_Type & operator = (const _Self_Type&);

    public:
        inline _Given_Pointer_Type *operator -> (){ return m_rawPtr; }
        inline const _Given_Pointer_Type *operator -> () const{ return m_rawPtr; }

        inline _Given_Pointer_Type &operator * (){ return *m_rawPtr; }
        inline const _Given_Pointer_Type &operator * () const{ return *m_rawPtr; }

        inline operator bool() { return m_rawPtr != NULL; }

    public:
        _Given_Pointer_Type *GetRaw() const { return m_rawPtr; }
        void Dismiss() { m_rawPtr = NULL; }

    private:
        _Given_Pointer_Type *m_rawPtr;
    };

    RefImageResource *RefImageResource::Create(const SIZE& size)
    {
        if (size.cx > 0 && size.cy > 0)
        {
            ScopedPointer<RefImageResource> d = new RefImageResource();

            if (Image::Allocate(d.GetRaw(), size))
            {
                RefImageResource *ret = d.GetRaw();
                d.Dismiss();

                return ret;
            }
        }

        return NULL;
    }

    RefImageResource *RefImageResource::Create(const String& path)
    {
        ScopedPointer<RefImageResource> d = new RefImageResource();
        if (d)
        {
            GIDPlusContext context;
            if (context.TestOK())
            {
                Gdiplus::Bitmap srcBitmap(path.c_str());
                if (Gdiplus::Ok == srcBitmap.GetLastStatus())
                {
                    BITMAPINFO desBitmapInfo;
                    PBYTE pDesData = NULL;

                    HBITMAP bitmap = CreateBitmapFrom(&srcBitmap, (LPVOID *)&pDesData, &desBitmapInfo);

                    RefImageResource *ret = d.GetRaw();
                    Image::Initialize(ret, pDesData, bitmap, desBitmapInfo);

                    d.Dismiss();

                    return ret;
                }
            }
        }

        return NULL;
    }

    RefImageResource::RefImageResource()
        : m_RefCount(0)
    {

    }

    RefImageResource::~RefImageResource()
    {
        if (m_bitmap)
            Clean();
    }

    void RefImageResource::AddRef()
    {
        InterlockedIncrement(&m_RefCount);
    }

    BOOL RefImageResource::Release()
    {
        if (0 == InterlockedDecrement(&m_RefCount))
        {
            delete this;
            return TRUE;
        }

        return FALSE;
    }


    AnimationImageSet *AnimationImageSet::Create(const String& filePath)
    {
        ScopedPointer<AnimationImageSet> d = new AnimationImageSet;
        if (d)
        {
            GIDPlusContext context;
            if (context.TestOK())
            {
                Gdiplus::Image srcImage(filePath.c_str());
                if (Gdiplus::Ok == srcImage.GetLastStatus())
                {
                    GUID dimensionID;
                    if (Gdiplus::Ok == srcImage.GetFrameDimensionsList(&dimensionID, 1))
                    {
                        UINT frameCount = srcImage.GetFrameCount(&dimensionID);

                        UINT propSize = srcImage.GetPropertyItemSize(PropertyTagFrameDelay);
                        Gdiplus::PropertyItem *delayProp = (Gdiplus::PropertyItem*)::malloc(propSize);
                        srcImage.GetPropertyItem(PropertyTagFrameDelay, propSize, delayProp);

                        BITMAPINFO bitmapInfo;
                        PBYTE pDesData = NULL;

                        for (UINT i = 0; i != frameCount; ++i)
                        {
                            srcImage.SelectActiveFrame(&dimensionID, i);

                            //! allocate bitmap
                            DWORD ldwPause = ((DWORD*)delayProp->value)[i];
                            if (0 == ldwPause)
                                ldwPause = 1;

                            HBITMAP bitmap = CreateBitmapFrom(&srcImage, (LPVOID *)&pDesData, &bitmapInfo);
                            if (bitmap)
                            {
                                Image loImage;
                                Image::Initialize(&loImage, pDesData, bitmap, bitmapInfo);
								Frame loFrame = {loImage,ldwPause};

								d->m_frames.push_back(loFrame);
                            }
                        }

                        ::free(delayProp);

                        AnimationImageSet *ret = d.GetRaw();
                        d.Dismiss();

                        return ret;
                    }
                }
            }
        }

        return NULL;
    }

    AnimationImageSet::AnimationImageSet()
        : m_frames()
    {
    }

    AnimationImageSet::~AnimationImageSet()
    {
        for (auto it = m_frames.begin(); it != m_frames.end(); ++it)
        {
            ::DeleteObject(it->image.m_bitmap);
        }
    }

    void AnimationImageSet::AddRef()
    {
        InterlockedIncrement(&m_RefCount);
    }

    BOOL AnimationImageSet::Release()
    {
        if (0 == InterlockedDecrement(&m_RefCount))
        {
            delete this;
            return TRUE;
        }

        return FALSE;
    }

    void AnimationImageSet::Scale(const SIZE& size)
    {
        for (auto it = m_frames.begin(); it != m_frames.end(); ++it)
        {
            it->image.Scale(size);
        }
    }

    INT AnimationImageSet::GetFramesCount() const
    {
        return m_frames.size();
    }

    INT AnimationImageSet::GetFrameDelay(INT frameIndex) const
    {
        return m_frames.at(frameIndex).delay;
    }

    Image AnimationImageSet::GetFrameAt(INT frameIndex) const
    {
        return m_frames.at(frameIndex).image;
    }
}

