#ifndef DICE_FOUR_PNG_H
#define DICE_FOUR_PNG_H

#include <wx/mstream.h>
#include <wx/image.h>
#include <wx/bitmap.h>

static const unsigned char dice_four_png[] =
{
	0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A, 0x00, 0x00, 
	0x00, 0x0D, 0x49, 0x48, 0x44, 0x52, 0x00, 0x00, 0x00, 0x10, 
	0x00, 0x00, 0x00, 0x10, 0x08, 0x06, 0x00, 0x00, 0x00, 0x1F, 
	0xF3, 0xFF, 0x61, 0x00, 0x00, 0x00, 0x09, 0x70, 0x48, 0x59, 
	0x73, 0x00, 0x00, 0x00, 0x76, 0x00, 0x00, 0x00, 0x76, 0x01, 
	0x4E, 0x7B, 0x26, 0x08, 0x00, 0x00, 0x00, 0x19, 0x74, 0x45, 
	0x58, 0x74, 0x53, 0x6F, 0x66, 0x74, 0x77, 0x61, 0x72, 0x65, 
	0x00, 0x77, 0x77, 0x77, 0x2E, 0x69, 0x6E, 0x6B, 0x73, 0x63, 
	0x61, 0x70, 0x65, 0x2E, 0x6F, 0x72, 0x67, 0x9B, 0xEE, 0x3C, 
	0x1A, 0x00, 0x00, 0x00, 0xD1, 0x49, 0x44, 0x41, 0x54, 0x38, 
	0x8D, 0xCD, 0xD3, 0xB1, 0x4A, 0x03, 0x41, 0x14, 0x05, 0xD0, 
	0xC3, 0xEE, 0x42, 0x3E, 0x45, 0xAC, 0xB4, 0x8A, 0xE0, 0x7E, 
	0x88, 0x56, 0x2A, 0xE9, 0xFC, 0x8E, 0x94, 0xA9, 0x14, 0x6C, 
	0xC4, 0x46, 0xFC, 0x17, 0x31, 0x56, 0x62, 0x88, 0xC1, 0x3F, 
	0xB1, 0xD8, 0x4D, 0x2C, 0x66, 0x8A, 0x21, 0x3C, 0xB3, 0xD1, 
	0xCA, 0x0B, 0x53, 0xBC, 0xFB, 0xDE, 0xBD, 0x5C, 0x66, 0xE6, 
	0x91, 0x30, 0xC1, 0x12, 0x1D, 0x36, 0x03, 0xA7, 0xCB, 0xB3, 
	0x57, 0xD0, 0x64, 0xF1, 0x3D, 0xEE, 0x30, 0x45, 0x6F, 0x37, 
	0x6A, 0x9C, 0xE2, 0x21, 0x1B, 0x5A, 0xE2, 0x66, 0x40, 0x14, 
	0xE1, 0x16, 0x0B, 0x39, 0xD2, 0xD9, 0x1F, 0x0C, 0xCE, 0xD1, 
	0x55, 0x39, 0xD2, 0xBA, 0x68, 0x7C, 0xE0, 0x24, 0x10, 0x8C, 
	0xB1, 0x2A, 0xEA, 0x1E, 0x75, 0x15, 0x0C, 0x4E, 0xF0, 0x16, 
	0xF0, 0xEF, 0xF2, 0xC5, 0x95, 0x68, 0x82, 0xC1, 0x97, 0x30, 
	0x30, 0x5F, 0x98, 0x6F, 0x93, 0x51, 0x82, 0x5F, 0xE1, 0x7F, 
	0x1A, 0x8C, 0x31, 0x0A, 0xF8, 0x51, 0xEE, 0x0D, 0x1A, 0x3C, 
	0xE2, 0x38, 0xE0, 0x8F, 0xF0, 0xB4, 0x4D, 0x36, 0xD2, 0x7B, 
	0x96, 0x46, 0x87, 0x81, 0x18, 0x5E, 0x71, 0x50, 0xD4, 0x35, 
	0xFA, 0x0A, 0x9F, 0x68, 0x7F, 0x10, 0xED, 0x42, 0x8B, 0x55, 
	0x83, 0x99, 0xB4, 0x18, 0xF0, 0x6C, 0xBF, 0x65, 0x6A, 0x71, 
	0x2D, 0x7D, 0x3A, 0x70, 0x29, 0x2D, 0xC6, 0xBE, 0xEB, 0xBC, 
	0xC0, 0x05, 0x7C, 0x03, 0xFA, 0x10, 0x37, 0x82, 0xEB, 0xDF, 
	0xE5, 0x7B, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4E, 0x44, 
	0xAE, 0x42, 0x60, 0x82, 
};

wxBitmap& dice_four_png_to_wx_bitmap()
{
	static wxMemoryInputStream memIStream( dice_four_png, sizeof( dice_four_png ) );
	static wxImage image( memIStream, wxBITMAP_TYPE_PNG );
	static wxBitmap bmp( image );
	return bmp;
}


#endif //DICE_FOUR_PNG_H
