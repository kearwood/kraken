//
//  KRAudioSample.h
//  Kraken Engine
//
//  Copyright 2024 Kearwood Gilbert. All rights reserved.
//
//  Redistribution and use in source and binary forms, with or without modification, are
//  permitted provided that the following conditions are met:
//
//  1. Redistributions of source code must retain the above copyright notice, this list of
//  conditions and the following disclaimer.
//
//  2. Redistributions in binary form must reproduce the above copyright notice, this list
//  of conditions and the following disclaimer in the documentation and/or other materials
//  provided with the distribution.
//
//  THIS SOFTWARE IS PROVIDED BY KEARWOOD GILBERT ''AS IS'' AND ANY EXPRESS OR IMPLIED
//  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
//  FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL KEARWOOD GILBERT OR
//  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
//  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
//  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
//  ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
//  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
//  ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//  The views and conclusions contained in the software and documentation are those of the
//  authors and should not be interpreted as representing official policies, either expressed
//  or implied, of Kearwood Gilbert.
//

#pragma once

#include "KREngine-common.h"
#include "KRContextObject.h"
#include "block.h"
#include "resources/KRResource.h"

class KRAudioBuffer;

class KRAudioSample : public KRResource
{

public:
  KRAudioSample(KRContext& context, std::string name, std::string extension);
  KRAudioSample(KRContext& context, std::string name, std::string extension, mimir::Block* data);
  virtual ~KRAudioSample();

  virtual std::string getExtension();

  virtual bool save(mimir::Block& data);

  float getDuration();
  KRAudioBuffer* getBuffer(int index);
  int getBufferCount();

  // Siren audio engine interface
  int getChannelCount();
  __int64_t getFrameCount();
  float sample(int frame_offset, int frame_rate, int channel);
  void sample(__int64_t frame_offset, int frame_count, int channel, float* buffer, float amplitude, bool loop);

  void _endFrame();
private:

  __int64_t m_last_frame_used;

  std::string m_extension;
  mimir::Block* m_pData;

#ifdef __APPLE__
  // Apple Audio Toolbox
  AudioFileID m_audio_file_id;
  ExtAudioFileRef m_fileRef;

  static OSStatus ReadProc( // AudioFile_ReadProc
    void* inClientData,
    SInt64		inPosition,
    UInt32	requestCount,
    void* buffer,
    UInt32* actualCount);

  static OSStatus WriteProc( // AudioFile_WriteProc
    void* inClientData,
    SInt64		inPosition,
    UInt32		requestCount,
    const void* buffer,
    UInt32* actualCount);

  static SInt64 GetSizeProc( // AudioFile_GetSizeProc
    void* inClientData);


  static OSStatus SetSizeProc( // AudioFile_SetSizeProc
    void* inClientData,
    SInt64		inSize);
#endif

  int m_bufferCount;

  __int64_t m_totalFrames;
  int m_frameRate;
  int m_bytesPerFrame;
  int m_channelsPerFrame;

  void openFile();
  void closeFile();
  void loadInfo();

  static void PopulateBuffer(KRAudioSample* sound, int index, void* data);
};
