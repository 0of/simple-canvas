#pragma once

#include <vector>

namespace Render
{
    class Image
    {
    public:
        static bool Allocate(Image *nullImage, const SIZE& size);
        static bool ReAllocate(Image *image, const SIZE& size) throw();
        static bool Initialize(Image *image, PBYTE pData, HBITMAP bitmap, const BITMAPINFO& bitmapInfo);

    private:
        PBYTE m_pData;
        BITMAPINFO m_bitmapInfo;

    public:
        HBITMAP m_bitmap;

    public:
        Image();
        virtual~Image();

    public:
        void Scale(const SIZE& size);
        void Clean();

    public:
        PBYTE GetOffset(UINT w, UINT h);
        const PBYTE GetOffset(UINT w, UINT h) const;

    public:
        SIZE GetSize() const;
        INT GetWidth() const;
        INT GetHeight() const;
        INT GetDepth() const;   //! bit count / 8
        INT GetStride() const;  //! line width in bytes

    public:
        bool IsNull() const { return !m_bitmap; }
    };

    class RefImageResource : public Image
    {
    private:
        SIZE_T m_RefCount;

    public:
        static RefImageResource *Create(const String& path);
        static RefImageResource *Create(const SIZE& size);

    public:
        RefImageResource();
        virtual ~RefImageResource();

    public:
        void AddRef();
        BOOL Release();
    };

    class AnimationImageSet
    {
    private:
        struct Frame
        {
            Image image;
            UINT delay;
        };
        typedef std::vector<Frame> Frames;
        Frames m_frames;

        SIZE_T m_RefCount;

    public:
        static AnimationImageSet *Create(const String& filePath);

    protected:
        AnimationImageSet();

    public:
        ~AnimationImageSet();

    public:
        void AddRef();
        BOOL Release();

    public:
        void Scale(const SIZE& size);

    public:
        INT GetFramesCount() const;
        INT GetFrameDelay(INT frameIndex) const;

        //! NOT RECOMMANDED save the return image, unless u know what ur doing
        //! when scale or destory, the return image will be invalid
        Image GetFrameAt(INT frameIndex) const;
    };
}