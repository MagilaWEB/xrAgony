#pragma once
#include "xrCore/xrCore.h"
#include "xrCore/FS.h"

namespace XRay
{
namespace Media
{
enum class ImageFormat : u32
{
	Unknown = 0,
	RGB8 = 1,
	RGBA8 = 2,
};

class XRCORE_API Image final
{
private:
#pragma pack(push, 1)
	struct TGAHeader
	{
		u8 DescSize;
		u8 MapType;
		u8 ImageType;
		u16 MapStart;
		u16 MapEntries;
		u8 MapBits;
		u16 XOffset;
		u16 YOffset;
		u16 Width;
		u16 Height;
		u8 BPP;
		u8 ImageDesc;
	};
#pragma pack(pop)

	ImageFormat format;
	int channelCount;
	u32 width, height;
	void* data;

public:
	Image()
	{
		format = ImageFormat::Unknown;
		channelCount = 0;
		width = height = 0;
		data = nullptr;
	}

	~Image() {};
	Image(u32 width, u32 height, void* data, ImageFormat format)
	{
		Create(width, height, data, format);
	};
	Image& Create(u32 width, u32 height, void* data, ImageFormat format);
	void SaveTGA(IWriter& writer, bool align);
	void SaveTGA(IWriter& writer, ImageFormat format, bool align);
	void SaveTGA(const char* name, bool align);
	void SaveTGA(const char* name, ImageFormat format, bool align);

private:
	template <typename TWriter>
	void SaveTGA(TWriter& writer, ImageFormat format, bool align);
};
}
}
