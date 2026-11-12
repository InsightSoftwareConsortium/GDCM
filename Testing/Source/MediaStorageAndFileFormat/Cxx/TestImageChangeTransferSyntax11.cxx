/*=========================================================================

  Program: GDCM (Grassroots DICOM). A DICOM library

  Copyright (c) 2006-2011 Mathieu Malaterre
  All rights reserved.
  See Copyright.txt or http://gdcm.sourceforge.net/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "gdcmImageChangeTransferSyntax.h"
#include "gdcmImage.h"
#include "gdcmJPEG2000Codec.h"
#include "gdcmSequenceOfFragments.h"
#include "gdcmSmartPointer.h"

#include <cstring>
#include <iostream>
#include <sstream>
#include <vector>

// Pixel values with fewer stored bits than allocated bits must survive a
// JPEG 2000 lossless round trip, and 8 and 16-bit containers keep their size.
static int TestJ2KStoredBits(unsigned short bitsallocated, unsigned short bitsstored)
{
  const unsigned int dims[2] = { 32, 16 };
  const size_t npixels = dims[0] * dims[1];
  const size_t pixelsize = bitsallocated / 8;
  gdcm::PixelFormat pf;
  pf.SetSamplesPerPixel( 1 );
  pf.SetBitsAllocated( bitsallocated );
  pf.SetBitsStored( bitsstored );
  pf.SetHighBit( (unsigned short)(bitsstored - 1) );
  pf.SetPixelRepresentation( 0 );

  gdcm::SmartPointer<gdcm::Image> image = new gdcm::Image;
  image->SetNumberOfDimensions( 2 );
  image->SetDimensions( dims );
  image->SetPixelFormat( pf );
  image->SetPhotometricInterpretation( gdcm::PhotometricInterpretation::MONOCHROME2 );
  image->SetTransferSyntax( gdcm::TransferSyntax::ExplicitVRLittleEndian );

  std::vector<uint32_t> values( npixels );
  std::vector<char> pixels( npixels * pixelsize );
  for( size_t i = 0; i < npixels; ++i )
    {
    values[i] = (uint32_t)( i * 37 % ( 1u << bitsstored ) );
    for( size_t b = 0; b < pixelsize; ++b )
      {
      pixels[i * pixelsize + b] = (char)( values[i] >> ( 8 * b ) );
      }
    }
  gdcm::DataElement pixeldata( gdcm::Tag(0x7fe0,0x0010) );
  pixeldata.SetByteValue( pixels.data(), (uint32_t)pixels.size() );
  image->SetDataElement( pixeldata );

  gdcm::ImageChangeTransferSyntax change;
  change.SetTransferSyntax( gdcm::TransferSyntax::JPEG2000Lossless );
  change.SetInput( *image );
  if( !change.Change() )
    {
    std::cerr << "Could not compress" << std::endl;
    return 1;
    }
  const gdcm::DataElement &compressed = change.GetOutput().GetDataElement();
  const gdcm::SequenceOfFragments *sqf = compressed.GetSequenceOfFragments();
  if( !sqf || sqf->GetNumberOfFragments() != 1 || !sqf->GetFragment(0).GetByteValue() )
    {
    std::cerr << "Expected a single fragment" << std::endl;
    return 1;
    }
  const gdcm::ByteValue *codestream = sqf->GetFragment(0).GetByteValue();
  std::stringstream ss;
  ss.write( codestream->GetPointer(), codestream->GetLength() );
  gdcm::JPEG2000Codec codec;
  gdcm::TransferSyntax ts;
  if( !codec.GetHeaderInfo( ss, ts ) )
    {
    std::cerr << "Could not parse the code stream" << std::endl;
    return 1;
    }
  const gdcm::PixelFormat decodedpf = codec.GetPixelFormat();
  if( bitsallocated <= 16 && decodedpf.GetBitsAllocated() != bitsallocated )
    {
    std::cerr << "BitsAllocated changed to " << decodedpf.GetBitsAllocated() << std::endl;
    return 1;
    }

  codec.SetNumberOfDimensions( 2 );
  codec.SetDimensions( dims );
  gdcm::DataElement decoded;
  if( !codec.Decode( compressed, decoded ) || !decoded.GetByteValue() )
    {
    std::cerr << "Could not decompress" << std::endl;
    return 1;
    }
  const size_t decodedsize = decodedpf.GetBitsAllocated() / 8;
  const gdcm::ByteValue *bv = decoded.GetByteValue();
  if( bv->GetLength() != npixels * decodedsize )
    {
    std::cerr << "Unexpected decompressed length " << bv->GetLength() << std::endl;
    return 1;
    }
  const unsigned char *p = (const unsigned char*)bv->GetPointer();
  for( size_t i = 0; i < npixels; ++i )
    {
    uint32_t v = 0;
    for( size_t b = 0; b < decodedsize; ++b )
      {
      v |= (uint32_t)p[i * decodedsize + b] << ( 8 * b );
      }
    if( v != values[i] )
      {
      std::cerr << "Pixel " << i << " is " << v << ", expected " << values[i] << std::endl;
      return 1;
      }
    }
  return 0;
}

int TestImageChangeTransferSyntax11(int , char *[])
{
  static const unsigned short cases[][2] = {
    { 8, 8 }, { 8, 7 }, { 16, 8 }, { 16, 5 }, { 16, 12 }, { 16, 16 },
    { 32, 8 }, { 32, 12 }, { 32, 20 }
  };
  int r = 0;
  for( size_t i = 0; i < sizeof( cases ) / sizeof( *cases ); ++i )
    {
    if( TestJ2KStoredBits( cases[i][0], cases[i][1] ) )
      {
      std::cerr << "Failed for BitsAllocated=" << cases[i][0]
        << " BitsStored=" << cases[i][1] << std::endl;
      ++r;
      }
    }
  return r;
}
