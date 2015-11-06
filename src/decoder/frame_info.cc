#include "frame_info.hh"
#include "file.hh"

FrameInfo::FrameInfo()
  : ivf_filename_(), offset_( 0 ), length_( 0 ), source_hash_(),
    target_hash_()
{}

FrameInfo::FrameInfo( const std::string & ivf_filename, const std::string & frame_name,
                      const size_t & offset, const size_t & length )
  : ivf_filename_( ivf_filename ),
    offset_( offset ),
    length_( length ),
    source_hash_( frame_name ),
    target_hash_( frame_name )
{}

FrameInfo::FrameInfo( const std::string & ivf_filename, const size_t & offset, const size_t & length,
                      const SourceHash & source_hash, const TargetHash & target_hash )
  : ivf_filename_( ivf_filename ),
    offset_( offset ),
    length_( length ),
    source_hash_( source_hash ),
    target_hash_( target_hash )
{}

bool FrameInfo::validate_source( const DecoderHash & decoder_hash ) const
{
  return decoder_hash.can_decode( source_hash_ );
}

bool FrameInfo::validate_target( const DecoderHash & decoder_hash ) const
{
  return target_hash_.continuation_hash == decoder_hash.continuation_hash();
}

bool FrameInfo::shown() const
{
  return target_hash_.shown;
}

Chunk FrameInfo::chunk() const
{
  File file( ivf_filename_ );

  return file( offset_, length_ );
}

std::string build_frame_name( const SourceHash & source_hash,
  const TargetHash & target_hash )
{
  return source_hash.str() + "#" + target_hash.str();
}
