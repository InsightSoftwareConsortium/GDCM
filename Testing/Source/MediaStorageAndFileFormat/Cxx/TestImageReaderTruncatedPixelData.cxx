/*=========================================================================

  Program: GDCM (Grassroots DICOM). A DICOM library

  Copyright (c) 2006-2011 Mathieu Malaterre
  All rights reserved.
  See Copyright.txt or http://gdcm.sourceforge.net/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "gdcmImageReader.h"
#include "gdcmImageWriter.h"
#include "gdcmImage.h"
#include "gdcmRAWCodec.h"
#include "gdcmSystem.h"
#include "gdcmTesting.h"
#include "gdcmTrace.h"
#include "gdcmConfigure.h"

#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

namespace
{
const char Canary = 0x5a;

// True when out holds the first kept bytes of expected followed by zeros only.
bool IsZeroFilled(const std::vector<char> &out, const std::vector<char> &expected, size_t kept)
{
  if( std::memcmp(out.data(), expected.data(), kept) != 0 ) return false;
  for( size_t i = kept; i < out.size(); ++i )
    {
    if( out[i] != 0 ) return false;
    }
  return true;
}

int TestDecodeBytesShortInput(bool byteswap)
{
  const std::vector<char> in(64, 0x11);
  std::vector<char> out(256, Canary);
  gdcm::RAWCodec codec;
  codec.SetPixelFormat( gdcm::PixelFormat::UINT16 );
  codec.SetPhotometricInterpretation( gdcm::PhotometricInterpretation::MONOCHROME2 );
  codec.SetNeedByteSwap( byteswap );
  if( !codec.DecodeBytes(in.data(), in.size(), out.data(), out.size())
    || !IsZeroFilled(out, in, in.size()) )
    {
    std::cerr << "DecodeBytes(byteswap=" << byteswap << "): missing output bytes are not zero" << std::endl;
    return 1;
    }
  return 0;
}

int TestTruncatedFile()
{
#ifdef GDCM_SUPPORT_BROKEN_IMPLEMENTATION
  const unsigned int dims[2] = { 1024, 1024 };
  std::vector<char> pixels(dims[0] * dims[1] * 2);
  for( size_t i = 0; i < pixels.size(); ++i )
    {
    pixels[i] = static_cast<char>(i % 251 + 1);
    }

  const char subdir[] = "TestImageReaderTruncatedPixelData";
  const std::string tmpdir = gdcm::Testing::GetTempDirectory( subdir );
  if( !gdcm::System::FileIsDirectory( tmpdir.c_str() ) )
    {
    gdcm::System::MakeDirectory( tmpdir.c_str() );
    }
  const std::string fullname = tmpdir + "/full.dcm";
  const std::string truncname = tmpdir + "/truncated.dcm";

  gdcm::ImageWriter writer;
  gdcm::Image &image = writer.GetImage();
  image.SetNumberOfDimensions( 2 );
  image.SetDimensions( dims );
  image.SetPixelFormat( gdcm::PixelFormat::UINT16 );
  image.SetPhotometricInterpretation( gdcm::PhotometricInterpretation::MONOCHROME2 );
  image.SetTransferSyntax( gdcm::TransferSyntax::ExplicitVRLittleEndian );
  gdcm::DataElement pixeldata( gdcm::Tag(0x7fe0,0x0010) );
  pixeldata.SetByteValue( pixels.data(), static_cast<uint32_t>(pixels.size()) );
  image.SetDataElement( pixeldata );
  writer.SetFileName( fullname.c_str() );
  if( !writer.Write() )
    {
    std::cerr << "Failed to write: " << fullname << std::endl;
    return 1;
    }

  std::ifstream is( fullname.c_str(), std::ios::binary );
  const std::string content( (std::istreambuf_iterator<char>(is)), std::istreambuf_iterator<char>() );
  is.close();
  if( content.size() < pixels.size()
    || std::memcmp(content.data() + content.size() - pixels.size(), pixels.data(), pixels.size()) != 0 )
    {
    std::cerr << "Pixel Data is not the last element of: " << fullname << std::endl;
    return 1;
    }
  // The declared length (2 MiB) exceeds both 1 MiB and the truncated file.
  const size_t kept = 1500000;
  std::ofstream os( truncname.c_str(), std::ios::binary );
  os.write( content.data(), static_cast<std::streamsize>(content.size() - pixels.size() + kept) );
  os.close();

  gdcm::ImageReader reader;
  reader.SetFileName( truncname.c_str() );
  if( !reader.Read() )
    {
    std::cerr << "Failed to read: " << truncname << std::endl;
    return 1;
    }
  const gdcm::Image &truncated = reader.GetImage();
  if( truncated.GetBufferLength() != pixels.size() )
    {
    std::cerr << "Unexpected buffer length: " << truncated.GetBufferLength() << std::endl;
    return 1;
    }
  std::vector<char> out(pixels.size(), Canary);
  if( !truncated.GetBuffer( out.data() ) || !IsZeroFilled(out, pixels, kept) )
    {
    std::cerr << "Missing pixels of a truncated image are not zero" << std::endl;
    return 1;
    }
#endif
  return 0;
}
} // end anonymous namespace

int TestImageReaderTruncatedPixelData(int, char *[])
{
  gdcm::Trace::WarningOff();
  int ret = 0;
  ret += TestDecodeBytesShortInput(false);
  ret += TestDecodeBytesShortInput(true);
  ret += TestTruncatedFile();
  return ret;
}
