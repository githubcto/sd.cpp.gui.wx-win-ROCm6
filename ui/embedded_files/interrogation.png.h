#ifndef INTERROGATION_PNG_H
#define INTERROGATION_PNG_H

#include <wx/mstream.h>
#include <wx/image.h>
#include <wx/bitmap.h>

static const unsigned char interrogation_png[] =
{
	0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A, 0x00, 0x00, 
	0x00, 0x0D, 0x49, 0x48, 0x44, 0x52, 0x00, 0x00, 0x00, 0x10, 
	0x00, 0x00, 0x00, 0x10, 0x08, 0x06, 0x00, 0x00, 0x00, 0x1F, 
	0xF3, 0xFF, 0x61, 0x00, 0x00, 0x00, 0x04, 0x73, 0x42, 0x49, 
	0x54, 0x08, 0x08, 0x08, 0x08, 0x7C, 0x08, 0x64, 0x88, 0x00, 
	0x00, 0x00, 0x09, 0x70, 0x48, 0x59, 0x73, 0x00, 0x00, 0x00, 
	0x76, 0x00, 0x00, 0x00, 0x76, 0x01, 0x4E, 0x7B, 0x26, 0x08, 
	0x00, 0x00, 0x00, 0x19, 0x74, 0x45, 0x58, 0x74, 0x53, 0x6F, 
	0x66, 0x74, 0x77, 0x61, 0x72, 0x65, 0x00, 0x77, 0x77, 0x77, 
	0x2E, 0x69, 0x6E, 0x6B, 0x73, 0x63, 0x61, 0x70, 0x65, 0x2E, 
	0x6F, 0x72, 0x67, 0x9B, 0xEE, 0x3C, 0x1A, 0x00, 0x00, 0x01, 
	0x4D, 0x49, 0x44, 0x41, 0x54, 0x38, 0x8D, 0x7D, 0xD3, 0xBD, 
	0x4A, 0x5C, 0x61, 0x10, 0xC6, 0xF1, 0x5F, 0xD6, 0x35, 0x6C, 
	0xEE, 0xC2, 0xCA, 0x22, 0xB0, 0xAE, 0x5A, 0x84, 0x14, 0x81, 
	0x80, 0x7D, 0x2E, 0x20, 0x84, 0xE0, 0x17, 0xF8, 0x81, 0x60, 
	0x91, 0x8B, 0x48, 0x1A, 0x2D, 0x64, 0xA3, 0xEB, 0x65, 0xA4, 
	0x4A, 0x11, 0x41, 0xAC, 0xED, 0x54, 0x4C, 0x14, 0x2C, 0xD4, 
	0x1B, 0x58, 0x0C, 0x11, 0x23, 0xEE, 0x6E, 0x52, 0xBC, 0x73, 
	0xC2, 0xBB, 0xBB, 0x27, 0xFE, 0xE1, 0xC0, 0x61, 0x66, 0xDE, 
	0x39, 0xF3, 0xCC, 0xFB, 0x1C, 0x86, 0x69, 0x60, 0x07, 0x67, 
	0xB8, 0xC7, 0x03, 0x7E, 0xA0, 0x89, 0x7A, 0x49, 0xFD, 0x3F, 
	0x6A, 0xD8, 0x45, 0x0F, 0x17, 0xD8, 0xC4, 0x32, 0xE6, 0xF0, 
	0x11, 0xDF, 0xD1, 0x45, 0x2B, 0x6A, 0x87, 0x0E, 0xEF, 0xE3, 
	0x17, 0x66, 0x51, 0x2D, 0xF9, 0xC0, 0x13, 0xBC, 0xC3, 0x4D, 
	0xD4, 0xF6, 0x35, 0xD9, 0x45, 0x1B, 0x2F, 0xB2, 0xD8, 0x74, 
	0xC4, 0x5B, 0x21, 0xAB, 0x60, 0x22, 0x9A, 0x6C, 0xE7, 0x9A, 
	0x7B, 0x58, 0x19, 0x38, 0xFC, 0x1B, 0xC7, 0x38, 0xC1, 0x1D, 
	0x26, 0xB3, 0xFC, 0xFB, 0x90, 0x53, 0x27, 0x2D, 0xEC, 0x1C, 
	0x95, 0xAC, 0xA0, 0x85, 0xA3, 0x88, 0x8D, 0x84, 0xFE, 0xCF, 
	0x59, 0xBE, 0x22, 0x2D, 0xB9, 0x59, 0xC5, 0x0C, 0xBE, 0xC5, 
	0x14, 0x05, 0xAB, 0xD9, 0xFB, 0x68, 0xE8, 0xCF, 0xE9, 0xE1, 
	0x0B, 0xDE, 0x88, 0x51, 0x17, 0x94, 0x53, 0xC3, 0xD7, 0x90, 
	0xD0, 0x18, 0xC8, 0x2D, 0xE0, 0xB6, 0x82, 0x3F, 0x4A, 0xAE, 
	0x25, 0x58, 0xC7, 0x2B, 0xBC, 0x0E, 0x49, 0x39, 0x4F, 0xD1, 
	0xA9, 0xE0, 0x12, 0x63, 0xFF, 0x69, 0x30, 0x85, 0x03, 0x1C, 
	0x96, 0xE4, 0xC6, 0x71, 0x45, 0x72, 0xD8, 0x95, 0x61, 0x9D, 
	0x8F, 0x31, 0x22, 0x99, 0x6D, 0x83, 0x74, 0x15, 0x5D, 0x2C, 
	0x96, 0x14, 0x7E, 0x88, 0x67, 0x90, 0x79, 0x74, 0xF0, 0xBC, 
	0x08, 0xEC, 0xE0, 0xD6, 0xF0, 0xA2, 0xCA, 0x1A, 0xBC, 0x8C, 
	0xDA, 0xAD, 0x3C, 0xF8, 0x4C, 0xB2, 0x67, 0x1B, 0x6F, 0xF5, 
	0x7B, 0xA2, 0xA0, 0xB0, 0x72, 0x1B, 0x7B, 0xD2, 0xF5, 0xF6, 
	0x51, 0x93, 0xEC, 0xD9, 0xC5, 0xB5, 0xF4, 0x33, 0xAD, 0x61, 
	0x09, 0x9F, 0x70, 0x1A, 0x63, 0x6F, 0x95, 0x1D, 0xCE, 0xA9, 
	0x4B, 0x8B, 0x3D, 0x8D, 0x51, 0x7F, 0x4A, 0x96, 0xDE, 0x90, 
	0x69, 0x2E, 0xF8, 0x0B, 0x0A, 0xE4, 0x4D, 0x89, 0x83, 0x92, 
	0xC6, 0xE0, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4E, 0x44, 
	0xAE, 0x42, 0x60, 0x82, 
};

wxBitmap& interrogation_png_to_wx_bitmap()
{
	static wxMemoryInputStream memIStream( interrogation_png, sizeof( interrogation_png ) );
	static wxImage image( memIStream, wxBITMAP_TYPE_PNG );
	static wxBitmap bmp( image );
	return bmp;
}


#endif //INTERROGATION_PNG_H
