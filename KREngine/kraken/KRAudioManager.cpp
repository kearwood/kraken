//
//  FileManager.cpp
//  KREngine
//
//  Copyright 2012 Kearwood Gilbert. All rights reserved.
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

#include "KRAudioManager.h"
#include "KRAudioSample.h"
#include "KREngine-common.h"
#include "KRDataBlock.h"
#include "KRAudioBuffer.h"
#include "KRContext.h"
#include "KRVector2.h"
#include "KRCollider.h"
#include <Accelerate/Accelerate.h>

OSStatus  alcASASetListenerProc(const ALuint property, ALvoid *data, ALuint dataSize);
ALvoid  alcMacOSXRenderingQualityProc(const ALint value);

KRAudioManager::KRAudioManager(KRContext &context) : KRContextObject(context)
{
    m_enable_audio = true;
    m_enable_hrtf = true;
    m_enable_reverb = true;
    m_reverb_max_length = 8.0f;
    
    m_anticlick_block = true;
    mach_timebase_info(&m_timebase_info);
    
    m_audio_engine = KRAKEN_AUDIO_SIREN;
    m_high_quality_hrtf = false;
    
    m_listener_scene = NULL;
    
    m_global_gain = 0.20f;
    m_global_reverb_send_level = 1.0f;
    m_global_ambient_gain = 1.0f;
   
    
    // OpenAL
    m_alDevice = 0;
    m_alContext = 0;
    
    // Siren
    m_auGraph = NULL;
    m_auMixer = NULL;
    
    m_audio_frame = 0;
    

    m_output_sample = 0;
    for(int i=KRENGINE_AUDIO_BLOCK_LOG2N; i <= KRENGINE_REVERB_MAX_FFT_LOG2; i++) {
        m_fft_setup[i - KRENGINE_AUDIO_BLOCK_LOG2N] = NULL;
    }
    
    m_reverb_input_samples = NULL;
    m_reverb_input_next_sample = 0;
    
    m_workspace_data = NULL;
    m_reverb_sequence = 0;
    
    m_hrtf_data = NULL;

    for(int i=0; i < KRENGINE_MAX_REVERB_IMPULSE_MIX; i++) {
        m_reverb_impulse_responses[i] = NULL;
        m_reverb_impulse_responses_weight[i] = 0.0f;
    }
    m_output_accumulation = NULL;
}

void KRAudioManager::initAudio()
{
    switch(m_audio_engine) {
        case KRAKEN_AUDIO_OPENAL:
            initOpenAL();
            break;
        case KRAKEN_AUDIO_SIREN:
            initSiren();
            break;
        case KRAKEN_AUDIO_NONE:
            break;
    }
}

unordered_map<std::string, KRAudioSample *> &KRAudioManager::getSounds()
{
    return m_sounds;
}

bool KRAudioManager::getEnableAudio()
{
    return m_enable_audio;
}
void KRAudioManager::setEnableAudio(bool enable)
{
    m_enable_audio = enable;
}

bool KRAudioManager::getEnableHRTF()
{
    return m_enable_hrtf;
}
void KRAudioManager::setEnableHRTF(bool enable)
{
    m_enable_hrtf = enable;
}

bool KRAudioManager::getEnableReverb()
{
    return m_enable_reverb;
}

float KRAudioManager::getReverbMaxLength()
{
    return m_reverb_max_length;
}

void KRAudioManager::setEnableReverb(bool enable)
{
    m_enable_reverb = enable;
}

void KRAudioManager::setReverbMaxLength(float max_length)
{
    m_reverb_max_length = max_length;
}

KRScene *KRAudioManager::getListenerScene()
{
    return m_listener_scene;
}

void KRAudioManager::setListenerScene(KRScene *scene)
{
    m_listener_scene = scene;
}

void KRAudioManager::renderAudio(UInt32 inNumberFrames, AudioBufferList *ioData)
{
    uint64_t start_time = mach_absolute_time();
    
    
	AudioUnitSampleType *outA = (AudioUnitSampleType *)ioData->mBuffers[0].mData;
    AudioUnitSampleType *outB = (AudioUnitSampleType *)ioData->mBuffers[1].mData; // Non-Interleaved only
    
    int output_frame = 0;
    
    while(output_frame < inNumberFrames) {
        int frames_ready = KRENGINE_AUDIO_BLOCK_LENGTH - m_output_sample;
        if(frames_ready == 0) {
            renderBlock();
            m_output_sample = 0;
            frames_ready = KRENGINE_AUDIO_BLOCK_LENGTH;
        }
        
        int frames_processed = inNumberFrames - output_frame;
        if(frames_processed > frames_ready) frames_processed = frames_ready;
        
        float *block_data = getBlockAddress(0);
        
        for(int i=0; i < frames_processed; i++) {
            
            float left_channel = block_data[m_output_sample * KRENGINE_MAX_OUTPUT_CHANNELS];
            float right_channel = block_data[m_output_sample * KRENGINE_MAX_OUTPUT_CHANNELS + 1];
            m_output_sample++;
            
#if CA_PREFER_FIXED_POINT
            // Interleaved
            //        outA[i*2] = (SInt16)(left_channel * 32767.0f);
            //        outA[i*2 + 1] = (SInt16)(right_channel * 32767.0f);
            
            // Non-Interleaved
            outA[output_frame] = (SInt32)(left_channel * 0x1000000f);
            outB[output_frame] = (SInt32)(right_channel * 0x1000000f);
#else
            
            // Interleaved
            //        outA[i*2] = (Float32)left_channel;
            //        outA[i*2 + 1] = (Float32)right_channel;
            
            // Non-Interleaved
            outA[output_frame] = (Float32)left_channel;
            outB[output_frame] = (Float32)right_channel;
#endif
            output_frame++;
            
        }
    }
    
    
    uint64_t end_time = mach_absolute_time();
//    uint64_t duration = (end_time - start_time) * m_timebase_info.numer / m_timebase_info.denom; // Nanoseconds
//    uint64_t max_duration = (uint64_t)inNumberFrames * 1000000000 / 44100;
//    fprintf(stderr, "audio load: %5.1f%% hrtf channels: %li\n", (float)(duration * 1000 / max_duration) / 10.0f, m_mapped_sources.size());
}

float *KRAudioManager::getBlockAddress(int block_offset)
{
    return m_output_accumulation + (m_output_accumulation_block_start + block_offset * KRENGINE_AUDIO_BLOCK_LENGTH * KRENGINE_MAX_OUTPUT_CHANNELS) % (KRENGINE_REVERB_MAX_SAMPLES * KRENGINE_MAX_OUTPUT_CHANNELS);
}

void KRAudioManager::renderReverbImpulseResponse(int impulse_response_offset, int frame_count_log2)
{
    
    int frame_count = 1 << frame_count_log2;
    int fft_size = frame_count * 2;
    int fft_size_log2 = frame_count_log2 + 1;
    
    DSPSplitComplex reverb_sample_data_complex = m_workspace[0];
    DSPSplitComplex impulse_block_data_complex = m_workspace[1];
    DSPSplitComplex conv_data_complex = m_workspace[2];
    
    int reverb_offset = (m_reverb_input_next_sample + KRENGINE_AUDIO_BLOCK_LENGTH - frame_count);
    if(reverb_offset < 0) {
        reverb_offset += KRENGINE_REVERB_MAX_SAMPLES;
    } else {
        reverb_offset = reverb_offset % KRENGINE_REVERB_MAX_SAMPLES;
    }
    
    int frames_left = frame_count;
    while(frames_left) {
        int frames_to_process = KRENGINE_REVERB_MAX_SAMPLES - reverb_offset;
        if(frames_to_process > frames_left) frames_to_process = frames_left;
        memcpy(reverb_sample_data_complex.realp + frame_count - frames_left, m_reverb_input_samples + reverb_offset, frames_to_process * sizeof(float));
        
        frames_left -= frames_to_process;
        reverb_offset = (reverb_offset + frames_to_process) % KRENGINE_REVERB_MAX_SAMPLES;
    }
    
    memset(reverb_sample_data_complex.realp + frame_count, 0, frame_count * sizeof(float));
    memset(reverb_sample_data_complex.imagp, 0, fft_size * sizeof(float));

    vDSP_fft_zip(m_fft_setup[fft_size_log2 - KRENGINE_AUDIO_BLOCK_LOG2N], &reverb_sample_data_complex, 1, fft_size_log2, kFFTDirection_Forward);
    
    float scale = 0.5f / fft_size;
    
    int impulse_response_channels = 2;
    for(int channel=0; channel < impulse_response_channels; channel++) {
        bool first_sample = true;
        for(unordered_map<std::string, siren_reverb_zone_weight_info>::iterator zone_itr=m_reverb_zone_weights.begin(); zone_itr != m_reverb_zone_weights.end(); zone_itr++) {
            siren_reverb_zone_weight_info zi = (*zone_itr).second;
            if(zi.reverb_sample) {
                if(impulse_response_offset < KRMIN(zi.reverb_sample->getFrameCount(), m_reverb_max_length * 44100)) { // Optimization - when mixing multiple impulse responses (i.e. fading between reverb zones), do not process blocks past the end of a shorter impulse response sample when they differ in length
                    if(first_sample) {
                        // If this is the first or only sample, write directly to the first half of the FFT input buffer
                        first_sample = false;
                        zi.reverb_sample->sample(impulse_response_offset, frame_count, channel, impulse_block_data_complex.realp, zi.weight, false);
                    } else {
                        // Subsequent samples write to the second half of the FFT input buffer, which is then added to the first half (the second half will be zero'ed out anyways and works as a convenient temporary buffer)
                        zi.reverb_sample->sample(impulse_response_offset, frame_count, channel, impulse_block_data_complex.realp + frame_count, zi.weight, false);
                        vDSP_vadd(impulse_block_data_complex.realp, 1, impulse_block_data_complex.realp + frame_count, 1, impulse_block_data_complex.realp, 1, frame_count);
                    }
                }
                
            }
        }
        memset(impulse_block_data_complex.realp + frame_count, 0, frame_count * sizeof(float));
        memset(impulse_block_data_complex.imagp, 0, fft_size * sizeof(float));
        

        
        vDSP_fft_zip(m_fft_setup[fft_size_log2 - KRENGINE_AUDIO_BLOCK_LOG2N], &impulse_block_data_complex, 1, fft_size_log2, kFFTDirection_Forward);
        vDSP_zvmul(&reverb_sample_data_complex, 1, &impulse_block_data_complex, 1, &conv_data_complex, 1, fft_size, 1);
        vDSP_fft_zip(m_fft_setup[fft_size_log2 - KRENGINE_AUDIO_BLOCK_LOG2N], &conv_data_complex, 1, fft_size_log2, kFFTDirection_Inverse);
        vDSP_vsmul(conv_data_complex.realp, 1, &scale, conv_data_complex.realp, 1, fft_size);
        
        
        int output_offset = (m_output_accumulation_block_start + impulse_response_offset * KRENGINE_MAX_OUTPUT_CHANNELS) % (KRENGINE_REVERB_MAX_SAMPLES * KRENGINE_MAX_OUTPUT_CHANNELS);
        frames_left = fft_size;
        while(frames_left) {
            int frames_to_process = (KRENGINE_REVERB_MAX_SAMPLES * KRENGINE_MAX_OUTPUT_CHANNELS - output_offset) / KRENGINE_MAX_OUTPUT_CHANNELS;
            if(frames_to_process > frames_left) frames_to_process = frames_left;
            vDSP_vadd(m_output_accumulation + output_offset + channel, KRENGINE_MAX_OUTPUT_CHANNELS, conv_data_complex.realp + fft_size - frames_left, 1, m_output_accumulation + output_offset + channel, KRENGINE_MAX_OUTPUT_CHANNELS, frames_to_process);
            frames_left -= frames_to_process;
            output_offset = (output_offset + frames_to_process * KRENGINE_MAX_OUTPUT_CHANNELS) % (KRENGINE_REVERB_MAX_SAMPLES * KRENGINE_MAX_OUTPUT_CHANNELS);
        }
    }
}

void KRAudioManager::renderReverb()
{
    float reverb_data[KRENGINE_AUDIO_BLOCK_LENGTH];
    
    float *reverb_accum = m_reverb_input_samples + m_reverb_input_next_sample;
    memset(reverb_accum, 0, sizeof(float) * KRENGINE_AUDIO_BLOCK_LENGTH);
    
    std::set<KRAudioSource *> active_sources = m_activeAudioSources;
    
    for(std::set<KRAudioSource *>::iterator itr=active_sources.begin(); itr != active_sources.end(); itr++) {
        KRAudioSource *source = *itr;
        if(&source->getScene() == m_listener_scene) {
            float containment_factor = 0.0f;
            
            for(unordered_map<std::string, siren_reverb_zone_weight_info>::iterator zone_itr=m_reverb_zone_weights.begin(); zone_itr != m_reverb_zone_weights.end(); zone_itr++) {
                siren_reverb_zone_weight_info zi = (*zone_itr).second;
                float gain = zi.weight * zi.reverb_zone->getReverbGain() * zi.reverb_zone->getContainment(source->getWorldTranslation());
                if(gain > containment_factor) containment_factor = gain;
            }
            
            
            float reverb_send_level = m_global_reverb_send_level * m_global_gain * source->getReverb() * containment_factor;
            if(reverb_send_level > 0.0f) {
                source->sample(KRENGINE_AUDIO_BLOCK_LENGTH, 0, reverb_data, reverb_send_level);
                vDSP_vadd(reverb_accum, 1, reverb_data, 1, reverb_accum, 1, KRENGINE_AUDIO_BLOCK_LENGTH);
            }
        }
    }
    
    // Apply impulse response reverb
    //    KRAudioSample *impulse_response = getContext().getAudioManager()->get("hrtf_kemar_H10e040a");
    
    int impulse_response_blocks = 0;
    for(unordered_map<std::string, siren_reverb_zone_weight_info>::iterator zone_itr=m_reverb_zone_weights.begin(); zone_itr != m_reverb_zone_weights.end(); zone_itr++) {
        siren_reverb_zone_weight_info zi = (*zone_itr).second;
        if(zi.reverb_sample) {
            int zone_sample_blocks = KRMIN(zi.reverb_sample->getFrameCount(), m_reverb_max_length * 44100) / KRENGINE_AUDIO_BLOCK_LENGTH + 1;
            impulse_response_blocks = KRMAX(impulse_response_blocks, zone_sample_blocks);
        }
    }
    
//    KRAudioSample *impulse_response_sample = getContext().getAudioManager()->get("test_reverb");
    if(impulse_response_blocks) {
//        int impulse_response_blocks = impulse_response_sample->getFrameCount() / KRENGINE_AUDIO_BLOCK_LENGTH + 1;
        int period_log2 = 0;
        int impulse_response_block=0;
        int period_count = 0;
        
        while(impulse_response_block < impulse_response_blocks) {
            int period = 1 << period_log2;
            
            if((m_reverb_sequence + period - period_count) % period == 0) {
                renderReverbImpulseResponse(impulse_response_block * KRENGINE_AUDIO_BLOCK_LENGTH, KRENGINE_AUDIO_BLOCK_LOG2N + period_log2);
            }
            
            impulse_response_block += period;
            
            period_count++;
            if(period_count >= period) {
                period_count = 0;
                if(KRENGINE_AUDIO_BLOCK_LOG2N + period_log2 + 1 < KRENGINE_REVERB_MAX_FFT_LOG2) { // FFT Size is double the number of frames in the period
                    period_log2++;
                }
            }
        }
    }
    
    m_reverb_sequence = (m_reverb_sequence + 1) % 0x1000000;
    
    // Rotate reverb buffer
    m_reverb_input_next_sample = (m_reverb_input_next_sample + KRENGINE_AUDIO_BLOCK_LENGTH) % KRENGINE_REVERB_MAX_SAMPLES;
}

void KRAudioManager::renderBlock()
{
    m_mutex.lock();
    
    // ----====---- Advance to next block in accumulation buffer ----====----
    
    // Zero out block that was last used, so it will be ready for the next pass through the circular buffer
    float *block_data = getBlockAddress(0);
    memset(block_data, 0, KRENGINE_AUDIO_BLOCK_LENGTH * KRENGINE_MAX_OUTPUT_CHANNELS * sizeof(float));
    
    // Advance to the next block, and wrap around
    m_output_accumulation_block_start = (m_output_accumulation_block_start + KRENGINE_AUDIO_BLOCK_LENGTH * KRENGINE_MAX_OUTPUT_CHANNELS) % (KRENGINE_REVERB_MAX_SAMPLES * KRENGINE_MAX_OUTPUT_CHANNELS);
    
    if(m_enable_audio) {
        // ----====---- Render Direct / HRTF audio ----====----
        if(m_enable_hrtf) {
            renderHRTF();
        } else {
            renderITD();
        }
        
        // ----====---- Render Indirect / Reverb channel ----====----
        if(m_enable_reverb) {
            renderReverb();
        }
        
        // ----====---- Render Ambient Sound ----====----
        renderAmbient();
    }
    
    // ----====---- Advance audio sources ----====----
    m_audio_frame += KRENGINE_AUDIO_BLOCK_LENGTH;
    
    m_anticlick_block = false;
    m_mutex.unlock();
}

// audio render procedure, don't allocate memory, don't take any locks, don't waste time
OSStatus KRAudioManager::renderInput(void *inRefCon, AudioUnitRenderActionFlags *ioActionFlags, const AudioTimeStamp *inTimeStamp, UInt32 inBusNumber, UInt32 inNumberFrames, AudioBufferList *ioData)
{
    ((KRAudioManager*)inRefCon)->renderAudio(inNumberFrames, ioData);
	return noErr;
}


void KRSetAUCanonical(AudioStreamBasicDescription &desc, UInt32 nChannels, bool interleaved)
{
    desc.mFormatID = kAudioFormatLinearPCM;
#if CA_PREFER_FIXED_POINT
    desc.mFormatFlags = kAudioFormatFlagsCanonical | (kAudioUnitSampleFractionBits << kLinearPCMFormatFlagsSampleFractionShift);
#else
    desc.mFormatFlags = kAudioFormatFlagsCanonical;
#endif
    desc.mChannelsPerFrame = nChannels;
    desc.mFramesPerPacket = 1;
    desc.mBitsPerChannel = 8 * sizeof(AudioUnitSampleType);
    if (interleaved)
        desc.mBytesPerPacket = desc.mBytesPerFrame = nChannels * sizeof(AudioUnitSampleType);
    else {
        desc.mBytesPerPacket = desc.mBytesPerFrame = sizeof(AudioUnitSampleType);
        desc.mFormatFlags |= kAudioFormatFlagIsNonInterleaved;
    }
}

void KRAudioManager::initHRTF()
{
    m_hrtf_sample_locations.push_back(KRVector2(-10.0f,000.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-10.0f,005.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-10.0f,010.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-10.0f,015.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-10.0f,020.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-10.0f,025.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-10.0f,030.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-10.0f,035.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-10.0f,040.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-10.0f,045.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-10.0f,050.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-10.0f,055.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-10.0f,060.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-10.0f,065.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-10.0f,070.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-10.0f,075.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-10.0f,080.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-10.0f,085.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-10.0f,090.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-10.0f,095.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-10.0f,100.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-10.0f,105.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-10.0f,110.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-10.0f,115.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-10.0f,120.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-10.0f,125.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-10.0f,130.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-10.0f,135.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-10.0f,140.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-10.0f,145.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-10.0f,150.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-10.0f,155.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-10.0f,160.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-10.0f,165.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-10.0f,170.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-10.0f,175.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-10.0f,180.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-20.0f,000.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-20.0f,005.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-20.0f,010.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-20.0f,015.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-20.0f,020.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-20.0f,025.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-20.0f,030.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-20.0f,035.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-20.0f,040.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-20.0f,045.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-20.0f,050.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-20.0f,055.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-20.0f,060.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-20.0f,065.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-20.0f,070.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-20.0f,075.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-20.0f,080.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-20.0f,085.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-20.0f,090.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-20.0f,095.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-20.0f,100.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-20.0f,105.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-20.0f,110.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-20.0f,115.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-20.0f,120.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-20.0f,125.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-20.0f,130.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-20.0f,135.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-20.0f,140.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-20.0f,145.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-20.0f,150.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-20.0f,155.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-20.0f,160.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-20.0f,165.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-20.0f,170.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-20.0f,175.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-20.0f,180.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-30.0f,000.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-30.0f,006.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-30.0f,012.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-30.0f,018.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-30.0f,024.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-30.0f,030.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-30.0f,036.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-30.0f,042.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-30.0f,048.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-30.0f,054.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-30.0f,060.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-30.0f,066.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-30.0f,072.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-30.0f,078.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-30.0f,084.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-30.0f,090.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-30.0f,096.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-30.0f,102.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-30.0f,108.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-30.0f,114.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-30.0f,120.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-30.0f,126.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-30.0f,132.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-30.0f,138.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-30.0f,144.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-30.0f,150.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-30.0f,156.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-30.0f,162.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-30.0f,168.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-30.0f,174.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-30.0f,180.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-40.0f,000.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-40.0f,006.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-40.0f,013.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-40.0f,019.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-40.0f,026.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-40.0f,032.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-40.0f,039.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-40.0f,045.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-40.0f,051.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-40.0f,058.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-40.0f,064.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-40.0f,071.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-40.0f,077.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-40.0f,084.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-40.0f,090.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-40.0f,096.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-40.0f,103.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-40.0f,109.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-40.0f,116.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-40.0f,122.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-40.0f,129.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-40.0f,135.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-40.0f,141.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-40.0f,148.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-40.0f,154.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-40.0f,161.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-40.0f,167.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-40.0f,174.0f));
    m_hrtf_sample_locations.push_back(KRVector2(-40.0f,180.0f));
    m_hrtf_sample_locations.push_back(KRVector2(0.0f,000.0f));
    m_hrtf_sample_locations.push_back(KRVector2(0.0f,005.0f));
    m_hrtf_sample_locations.push_back(KRVector2(0.0f,010.0f));
    m_hrtf_sample_locations.push_back(KRVector2(0.0f,015.0f));
    m_hrtf_sample_locations.push_back(KRVector2(0.0f,020.0f));
    m_hrtf_sample_locations.push_back(KRVector2(0.0f,025.0f));
    m_hrtf_sample_locations.push_back(KRVector2(0.0f,030.0f));
    m_hrtf_sample_locations.push_back(KRVector2(0.0f,035.0f));
    m_hrtf_sample_locations.push_back(KRVector2(0.0f,040.0f));
    m_hrtf_sample_locations.push_back(KRVector2(0.0f,045.0f));
    m_hrtf_sample_locations.push_back(KRVector2(0.0f,050.0f));
    m_hrtf_sample_locations.push_back(KRVector2(0.0f,055.0f));
    m_hrtf_sample_locations.push_back(KRVector2(0.0f,060.0f));
    m_hrtf_sample_locations.push_back(KRVector2(0.0f,065.0f));
    m_hrtf_sample_locations.push_back(KRVector2(0.0f,070.0f));
    m_hrtf_sample_locations.push_back(KRVector2(0.0f,075.0f));
    m_hrtf_sample_locations.push_back(KRVector2(0.0f,080.0f));
    m_hrtf_sample_locations.push_back(KRVector2(0.0f,085.0f));
    m_hrtf_sample_locations.push_back(KRVector2(0.0f,090.0f));
    m_hrtf_sample_locations.push_back(KRVector2(0.0f,095.0f));
    m_hrtf_sample_locations.push_back(KRVector2(0.0f,100.0f));
    m_hrtf_sample_locations.push_back(KRVector2(0.0f,105.0f));
    m_hrtf_sample_locations.push_back(KRVector2(0.0f,110.0f));
    m_hrtf_sample_locations.push_back(KRVector2(0.0f,115.0f));
    m_hrtf_sample_locations.push_back(KRVector2(0.0f,120.0f));
    m_hrtf_sample_locations.push_back(KRVector2(0.0f,125.0f));
    m_hrtf_sample_locations.push_back(KRVector2(0.0f,130.0f));
    m_hrtf_sample_locations.push_back(KRVector2(0.0f,135.0f));
    m_hrtf_sample_locations.push_back(KRVector2(0.0f,140.0f));
    m_hrtf_sample_locations.push_back(KRVector2(0.0f,145.0f));
    m_hrtf_sample_locations.push_back(KRVector2(0.0f,150.0f));
    m_hrtf_sample_locations.push_back(KRVector2(0.0f,155.0f));
    m_hrtf_sample_locations.push_back(KRVector2(0.0f,160.0f));
    m_hrtf_sample_locations.push_back(KRVector2(0.0f,165.0f));
    m_hrtf_sample_locations.push_back(KRVector2(0.0f,170.0f));
    m_hrtf_sample_locations.push_back(KRVector2(0.0f,175.0f));
    m_hrtf_sample_locations.push_back(KRVector2(0.0f,180.0f));
    m_hrtf_sample_locations.push_back(KRVector2(10.0f,000.0f));
    m_hrtf_sample_locations.push_back(KRVector2(10.0f,005.0f));
    m_hrtf_sample_locations.push_back(KRVector2(10.0f,010.0f));
    m_hrtf_sample_locations.push_back(KRVector2(10.0f,015.0f));
    m_hrtf_sample_locations.push_back(KRVector2(10.0f,020.0f));
    m_hrtf_sample_locations.push_back(KRVector2(10.0f,025.0f));
    m_hrtf_sample_locations.push_back(KRVector2(10.0f,030.0f));
    m_hrtf_sample_locations.push_back(KRVector2(10.0f,035.0f));
    m_hrtf_sample_locations.push_back(KRVector2(10.0f,040.0f));
    m_hrtf_sample_locations.push_back(KRVector2(10.0f,045.0f));
    m_hrtf_sample_locations.push_back(KRVector2(10.0f,050.0f));
    m_hrtf_sample_locations.push_back(KRVector2(10.0f,055.0f));
    m_hrtf_sample_locations.push_back(KRVector2(10.0f,060.0f));
    m_hrtf_sample_locations.push_back(KRVector2(10.0f,065.0f));
    m_hrtf_sample_locations.push_back(KRVector2(10.0f,070.0f));
    m_hrtf_sample_locations.push_back(KRVector2(10.0f,075.0f));
    m_hrtf_sample_locations.push_back(KRVector2(10.0f,080.0f));
    m_hrtf_sample_locations.push_back(KRVector2(10.0f,085.0f));
    m_hrtf_sample_locations.push_back(KRVector2(10.0f,090.0f));
    m_hrtf_sample_locations.push_back(KRVector2(10.0f,095.0f));
    m_hrtf_sample_locations.push_back(KRVector2(10.0f,100.0f));
    m_hrtf_sample_locations.push_back(KRVector2(10.0f,105.0f));
    m_hrtf_sample_locations.push_back(KRVector2(10.0f,110.0f));
    m_hrtf_sample_locations.push_back(KRVector2(10.0f,115.0f));
    m_hrtf_sample_locations.push_back(KRVector2(10.0f,120.0f));
    m_hrtf_sample_locations.push_back(KRVector2(10.0f,125.0f));
    m_hrtf_sample_locations.push_back(KRVector2(10.0f,130.0f));
    m_hrtf_sample_locations.push_back(KRVector2(10.0f,135.0f));
    m_hrtf_sample_locations.push_back(KRVector2(10.0f,140.0f));
    m_hrtf_sample_locations.push_back(KRVector2(10.0f,145.0f));
    m_hrtf_sample_locations.push_back(KRVector2(10.0f,150.0f));
    m_hrtf_sample_locations.push_back(KRVector2(10.0f,155.0f));
    m_hrtf_sample_locations.push_back(KRVector2(10.0f,160.0f));
    m_hrtf_sample_locations.push_back(KRVector2(10.0f,165.0f));
    m_hrtf_sample_locations.push_back(KRVector2(10.0f,170.0f));
    m_hrtf_sample_locations.push_back(KRVector2(10.0f,175.0f));
    m_hrtf_sample_locations.push_back(KRVector2(10.0f,180.0f));
    m_hrtf_sample_locations.push_back(KRVector2(20.0f,000.0f));
    m_hrtf_sample_locations.push_back(KRVector2(20.0f,005.0f));
    m_hrtf_sample_locations.push_back(KRVector2(20.0f,010.0f));
    m_hrtf_sample_locations.push_back(KRVector2(20.0f,015.0f));
    m_hrtf_sample_locations.push_back(KRVector2(20.0f,020.0f));
    m_hrtf_sample_locations.push_back(KRVector2(20.0f,025.0f));
    m_hrtf_sample_locations.push_back(KRVector2(20.0f,030.0f));
    m_hrtf_sample_locations.push_back(KRVector2(20.0f,035.0f));
    m_hrtf_sample_locations.push_back(KRVector2(20.0f,040.0f));
    m_hrtf_sample_locations.push_back(KRVector2(20.0f,045.0f));
    m_hrtf_sample_locations.push_back(KRVector2(20.0f,050.0f));
    m_hrtf_sample_locations.push_back(KRVector2(20.0f,055.0f));
    m_hrtf_sample_locations.push_back(KRVector2(20.0f,060.0f));
    m_hrtf_sample_locations.push_back(KRVector2(20.0f,065.0f));
    m_hrtf_sample_locations.push_back(KRVector2(20.0f,070.0f));
    m_hrtf_sample_locations.push_back(KRVector2(20.0f,075.0f));
    m_hrtf_sample_locations.push_back(KRVector2(20.0f,080.0f));
    m_hrtf_sample_locations.push_back(KRVector2(20.0f,085.0f));
    m_hrtf_sample_locations.push_back(KRVector2(20.0f,090.0f));
    m_hrtf_sample_locations.push_back(KRVector2(20.0f,095.0f));
    m_hrtf_sample_locations.push_back(KRVector2(20.0f,100.0f));
    m_hrtf_sample_locations.push_back(KRVector2(20.0f,105.0f));
    m_hrtf_sample_locations.push_back(KRVector2(20.0f,110.0f));
    m_hrtf_sample_locations.push_back(KRVector2(20.0f,115.0f));
    m_hrtf_sample_locations.push_back(KRVector2(20.0f,120.0f));
    m_hrtf_sample_locations.push_back(KRVector2(20.0f,125.0f));
    m_hrtf_sample_locations.push_back(KRVector2(20.0f,130.0f));
    m_hrtf_sample_locations.push_back(KRVector2(20.0f,135.0f));
    m_hrtf_sample_locations.push_back(KRVector2(20.0f,140.0f));
    m_hrtf_sample_locations.push_back(KRVector2(20.0f,145.0f));
    m_hrtf_sample_locations.push_back(KRVector2(20.0f,150.0f));
    m_hrtf_sample_locations.push_back(KRVector2(20.0f,155.0f));
    m_hrtf_sample_locations.push_back(KRVector2(20.0f,160.0f));
    m_hrtf_sample_locations.push_back(KRVector2(20.0f,165.0f));
    m_hrtf_sample_locations.push_back(KRVector2(20.0f,170.0f));
    m_hrtf_sample_locations.push_back(KRVector2(20.0f,175.0f));
    m_hrtf_sample_locations.push_back(KRVector2(20.0f,180.0f));
    m_hrtf_sample_locations.push_back(KRVector2(30.0f,000.0f));
    m_hrtf_sample_locations.push_back(KRVector2(30.0f,006.0f));
    m_hrtf_sample_locations.push_back(KRVector2(30.0f,012.0f));
    m_hrtf_sample_locations.push_back(KRVector2(30.0f,018.0f));
    m_hrtf_sample_locations.push_back(KRVector2(30.0f,024.0f));
    m_hrtf_sample_locations.push_back(KRVector2(30.0f,030.0f));
    m_hrtf_sample_locations.push_back(KRVector2(30.0f,036.0f));
    m_hrtf_sample_locations.push_back(KRVector2(30.0f,042.0f));
    m_hrtf_sample_locations.push_back(KRVector2(30.0f,048.0f));
    m_hrtf_sample_locations.push_back(KRVector2(30.0f,054.0f));
    m_hrtf_sample_locations.push_back(KRVector2(30.0f,060.0f));
    m_hrtf_sample_locations.push_back(KRVector2(30.0f,066.0f));
    m_hrtf_sample_locations.push_back(KRVector2(30.0f,072.0f));
    m_hrtf_sample_locations.push_back(KRVector2(30.0f,078.0f));
    m_hrtf_sample_locations.push_back(KRVector2(30.0f,084.0f));
    m_hrtf_sample_locations.push_back(KRVector2(30.0f,090.0f));
    m_hrtf_sample_locations.push_back(KRVector2(30.0f,096.0f));
    m_hrtf_sample_locations.push_back(KRVector2(30.0f,102.0f));
    m_hrtf_sample_locations.push_back(KRVector2(30.0f,108.0f));
    m_hrtf_sample_locations.push_back(KRVector2(30.0f,114.0f));
    m_hrtf_sample_locations.push_back(KRVector2(30.0f,120.0f));
    m_hrtf_sample_locations.push_back(KRVector2(30.0f,126.0f));
    m_hrtf_sample_locations.push_back(KRVector2(30.0f,132.0f));
    m_hrtf_sample_locations.push_back(KRVector2(30.0f,138.0f));
    m_hrtf_sample_locations.push_back(KRVector2(30.0f,144.0f));
    m_hrtf_sample_locations.push_back(KRVector2(30.0f,150.0f));
    m_hrtf_sample_locations.push_back(KRVector2(30.0f,156.0f));
    m_hrtf_sample_locations.push_back(KRVector2(30.0f,162.0f));
    m_hrtf_sample_locations.push_back(KRVector2(30.0f,168.0f));
    m_hrtf_sample_locations.push_back(KRVector2(30.0f,174.0f));
    m_hrtf_sample_locations.push_back(KRVector2(30.0f,180.0f));
    m_hrtf_sample_locations.push_back(KRVector2(40.0f,000.0f));
    m_hrtf_sample_locations.push_back(KRVector2(40.0f,006.0f));
    m_hrtf_sample_locations.push_back(KRVector2(40.0f,013.0f));
    m_hrtf_sample_locations.push_back(KRVector2(40.0f,019.0f));
    m_hrtf_sample_locations.push_back(KRVector2(40.0f,026.0f));
    m_hrtf_sample_locations.push_back(KRVector2(40.0f,032.0f));
    m_hrtf_sample_locations.push_back(KRVector2(40.0f,039.0f));
    m_hrtf_sample_locations.push_back(KRVector2(40.0f,045.0f));
    m_hrtf_sample_locations.push_back(KRVector2(40.0f,051.0f));
    m_hrtf_sample_locations.push_back(KRVector2(40.0f,058.0f));
    m_hrtf_sample_locations.push_back(KRVector2(40.0f,064.0f));
    m_hrtf_sample_locations.push_back(KRVector2(40.0f,071.0f));
    m_hrtf_sample_locations.push_back(KRVector2(40.0f,077.0f));
    m_hrtf_sample_locations.push_back(KRVector2(40.0f,084.0f));
    m_hrtf_sample_locations.push_back(KRVector2(40.0f,090.0f));
    m_hrtf_sample_locations.push_back(KRVector2(40.0f,096.0f));
    m_hrtf_sample_locations.push_back(KRVector2(40.0f,103.0f));
    m_hrtf_sample_locations.push_back(KRVector2(40.0f,109.0f));
    m_hrtf_sample_locations.push_back(KRVector2(40.0f,116.0f));
    m_hrtf_sample_locations.push_back(KRVector2(40.0f,122.0f));
    m_hrtf_sample_locations.push_back(KRVector2(40.0f,129.0f));
    m_hrtf_sample_locations.push_back(KRVector2(40.0f,135.0f));
    m_hrtf_sample_locations.push_back(KRVector2(40.0f,141.0f));
    m_hrtf_sample_locations.push_back(KRVector2(40.0f,148.0f));
    m_hrtf_sample_locations.push_back(KRVector2(40.0f,154.0f));
    m_hrtf_sample_locations.push_back(KRVector2(40.0f,161.0f));
    m_hrtf_sample_locations.push_back(KRVector2(40.0f,167.0f));
    m_hrtf_sample_locations.push_back(KRVector2(40.0f,174.0f));
    m_hrtf_sample_locations.push_back(KRVector2(40.0f,180.0f));
    m_hrtf_sample_locations.push_back(KRVector2(50.0f,000.0f));
    m_hrtf_sample_locations.push_back(KRVector2(50.0f,008.0f));
    m_hrtf_sample_locations.push_back(KRVector2(50.0f,016.0f));
    m_hrtf_sample_locations.push_back(KRVector2(50.0f,024.0f));
    m_hrtf_sample_locations.push_back(KRVector2(50.0f,032.0f));
    m_hrtf_sample_locations.push_back(KRVector2(50.0f,040.0f));
    m_hrtf_sample_locations.push_back(KRVector2(50.0f,048.0f));
    m_hrtf_sample_locations.push_back(KRVector2(50.0f,056.0f));
    m_hrtf_sample_locations.push_back(KRVector2(50.0f,064.0f));
    m_hrtf_sample_locations.push_back(KRVector2(50.0f,072.0f));
    m_hrtf_sample_locations.push_back(KRVector2(50.0f,080.0f));
    m_hrtf_sample_locations.push_back(KRVector2(50.0f,088.0f));
    m_hrtf_sample_locations.push_back(KRVector2(50.0f,096.0f));
    m_hrtf_sample_locations.push_back(KRVector2(50.0f,104.0f));
    m_hrtf_sample_locations.push_back(KRVector2(50.0f,112.0f));
    m_hrtf_sample_locations.push_back(KRVector2(50.0f,120.0f));
    m_hrtf_sample_locations.push_back(KRVector2(50.0f,128.0f));
    m_hrtf_sample_locations.push_back(KRVector2(50.0f,136.0f));
    m_hrtf_sample_locations.push_back(KRVector2(50.0f,144.0f));
    m_hrtf_sample_locations.push_back(KRVector2(50.0f,152.0f));
    m_hrtf_sample_locations.push_back(KRVector2(50.0f,160.0f));
    m_hrtf_sample_locations.push_back(KRVector2(50.0f,168.0f));
    m_hrtf_sample_locations.push_back(KRVector2(50.0f,176.0f));
    m_hrtf_sample_locations.push_back(KRVector2(60.0f,000.0f));
    m_hrtf_sample_locations.push_back(KRVector2(60.0f,010.0f));
    m_hrtf_sample_locations.push_back(KRVector2(60.0f,020.0f));
    m_hrtf_sample_locations.push_back(KRVector2(60.0f,030.0f));
    m_hrtf_sample_locations.push_back(KRVector2(60.0f,040.0f));
    m_hrtf_sample_locations.push_back(KRVector2(60.0f,050.0f));
    m_hrtf_sample_locations.push_back(KRVector2(60.0f,060.0f));
    m_hrtf_sample_locations.push_back(KRVector2(60.0f,070.0f));
    m_hrtf_sample_locations.push_back(KRVector2(60.0f,080.0f));
    m_hrtf_sample_locations.push_back(KRVector2(60.0f,090.0f));
    m_hrtf_sample_locations.push_back(KRVector2(60.0f,100.0f));
    m_hrtf_sample_locations.push_back(KRVector2(60.0f,110.0f));
    m_hrtf_sample_locations.push_back(KRVector2(60.0f,120.0f));
    m_hrtf_sample_locations.push_back(KRVector2(60.0f,130.0f));
    m_hrtf_sample_locations.push_back(KRVector2(60.0f,140.0f));
    m_hrtf_sample_locations.push_back(KRVector2(60.0f,150.0f));
    m_hrtf_sample_locations.push_back(KRVector2(60.0f,160.0f));
    m_hrtf_sample_locations.push_back(KRVector2(60.0f,170.0f));
    m_hrtf_sample_locations.push_back(KRVector2(60.0f,180.0f));
    m_hrtf_sample_locations.push_back(KRVector2(70.0f,000.0f));
    m_hrtf_sample_locations.push_back(KRVector2(70.0f,015.0f));
    m_hrtf_sample_locations.push_back(KRVector2(70.0f,030.0f));
    m_hrtf_sample_locations.push_back(KRVector2(70.0f,045.0f));
    m_hrtf_sample_locations.push_back(KRVector2(70.0f,060.0f));
    m_hrtf_sample_locations.push_back(KRVector2(70.0f,075.0f));
    m_hrtf_sample_locations.push_back(KRVector2(70.0f,090.0f));
    m_hrtf_sample_locations.push_back(KRVector2(70.0f,105.0f));
    m_hrtf_sample_locations.push_back(KRVector2(70.0f,120.0f));
    m_hrtf_sample_locations.push_back(KRVector2(70.0f,135.0f));
    m_hrtf_sample_locations.push_back(KRVector2(70.0f,150.0f));
    m_hrtf_sample_locations.push_back(KRVector2(70.0f,165.0f));
    m_hrtf_sample_locations.push_back(KRVector2(70.0f,180.0f));
    m_hrtf_sample_locations.push_back(KRVector2(80.0f,000.0f));
    m_hrtf_sample_locations.push_back(KRVector2(80.0f,030.0f));
    m_hrtf_sample_locations.push_back(KRVector2(80.0f,060.0f));
    m_hrtf_sample_locations.push_back(KRVector2(80.0f,090.0f));
    m_hrtf_sample_locations.push_back(KRVector2(80.0f,120.0f));
    m_hrtf_sample_locations.push_back(KRVector2(80.0f,150.0f));
    m_hrtf_sample_locations.push_back(KRVector2(80.0f,180.0f));
    m_hrtf_sample_locations.push_back(KRVector2(90.0f,000.0f));
    
    if(m_hrtf_data) {
        delete m_hrtf_data;
    }
    m_hrtf_data = (float *)malloc(m_hrtf_sample_locations.size() * sizeof(float) * 128 * 4 * 2);
    
    for(int channel=0; channel < 2; channel++) {
        m_hrtf_spectral[channel].clear();
    }
    
    int sample_index=0;
    for(std::vector<KRVector2>::iterator itr=m_hrtf_sample_locations.begin(); itr != m_hrtf_sample_locations.end(); itr++) {
        KRVector2 pos = *itr;
        KRAudioSample *sample = getHRTFSample(pos);
        for(int channel=0; channel < 2; channel++) {
            DSPSplitComplex spectral;
            spectral.realp = m_hrtf_data + sample_index * 1024 + channel * 512;
            spectral.imagp = m_hrtf_data + sample_index * 1024 + channel * 512 + 256;
            sample->sample(0, 128, channel, spectral.realp, 1.0f, false);
            memset(spectral.realp + 128, 0, sizeof(float) * 128);
            memset(spectral.imagp, 0, sizeof(float) * 256);
            vDSP_fft_zip(m_fft_setup[8 - KRENGINE_AUDIO_BLOCK_LOG2N], &spectral, 1, 8, kFFTDirection_Forward);
            m_hrtf_spectral[channel][pos] = spectral;
        }
        sample_index++;
    }
    
}

KRAudioSample *KRAudioManager::getHRTFSample(const KRVector2 &hrtf_dir)
{
    //hrtf_kemar_H-10e000a.wav
    char szName[64];
    sprintf(szName, "hrtf_kemar_H%de%03da", int(hrtf_dir.x), abs(int(hrtf_dir.y)));
    
    return get(szName);
}

DSPSplitComplex KRAudioManager::getHRTFSpectral(const KRVector2 &hrtf_dir, const int channel)
{
    KRVector2 dir = hrtf_dir;
    int sample_channel = channel;
    if(dir.y < 0) {
        dir.y = -dir.y;
        sample_channel = (channel + 1) % 2;
    }
    return m_hrtf_spectral[sample_channel][dir];
}

KRVector2 KRAudioManager::getNearestHRTFSample(const KRVector2 &dir)
{
    float elev_gran = 10.0f;

    
    KRVector2 dir_deg = dir * (180.0f / M_PI);
    float elevation = floor(dir_deg.x / elev_gran + 0.5f) * elev_gran;
    if(elevation < -40.0f) {
        elevation = -40.0f;
    }
    
    KRVector2 min_direction;
    bool first = true;
    float min_distance;
    for(std::vector<KRVector2>::iterator itr = m_hrtf_sample_locations.begin(); itr != m_hrtf_sample_locations.end(); itr++) {
        if(first) {
            first = false;
            min_direction = (*itr);
            min_distance = 360.0f;
        } else if((*itr).x == elevation) {
            float distance = fabs(dir_deg.y - (*itr).y);
            if(min_distance > distance) {
                min_direction = (*itr);
                min_distance = distance;
            }
            distance = fabs(dir_deg.y - -(*itr).y);
            if(min_distance > distance) {
                min_direction = (*itr);
                min_direction.y = -min_direction.y;
                min_distance = distance;
            }
        }
    }
    return min_direction;
}

void KRAudioManager::getHRTFMix(const KRVector2 &dir, KRVector2 &dir1, KRVector2 &dir2, KRVector2 &dir3, KRVector2 &dir4, float &mix1, float &mix2, float &mix3, float &mix4)
{
    float elev_gran = 10.0f;
    
    float elevation = dir.x * 180.0f / M_PI;
    float azimuth = dir.y * 180.0f / M_PI;
    
    float elev1 = floor(elevation / elev_gran) * elev_gran;
    float elev2 = ceil(elevation / elev_gran) * elev_gran;
    float elev_blend = (elevation - elev1) / elev_gran;
    if(elev1 < -40.0f) {
        elev1 = -40.0f;
        elev2 = -40.0f;
        elev_blend = 0.0f;
    }
    
    dir1.x = elev1;
    dir2.x = elev1;
    dir3.x = elev2;
    dir4.x = elev2;
    
    bool first1 = true;
    bool first2 = true;
    bool first3 = true;
    bool first4 = true;
    for(std::vector<KRVector2>::iterator itr = m_hrtf_sample_locations.begin(); itr != m_hrtf_sample_locations.end(); itr++) {
        KRVector2 sample_pos = *itr;
        
        if(sample_pos.x == elev1) {
            if(sample_pos.y <= azimuth) {
                if(first1) {
                    first1 = false;
                    dir1.y = sample_pos.y;
                } else if(sample_pos.y > dir1.y) {
                    dir1.y = sample_pos.y;
                }
            }
            if(-sample_pos.y <= azimuth) {
                if(first1) {
                    first1 = false;
                    dir1.y = -sample_pos.y;
                } else if(-sample_pos.y > dir1.y) {
                    dir1.y = -sample_pos.y;
                }
            }

            if(sample_pos.y > azimuth) {
                if(first2) {
                    first2 = false;
                    dir2.y = sample_pos.y;
                } else if(sample_pos.y < dir2.y) {
                    dir2.y = sample_pos.y;
                }
            }
            if(-sample_pos.y > azimuth) {
                if(first2) {
                    first2 = false;
                    dir2.y = -sample_pos.y;
                } else if(-sample_pos.y < dir2.y) {
                    dir2.y = -sample_pos.y;
                }
            }
        }
        if(sample_pos.x == elev2) {
            if(sample_pos.y <= azimuth) {
                if(first3) {
                    first3 = false;
                    dir3.y = sample_pos.y;
                } else if(sample_pos.y > dir3.y) {
                    dir3.y = sample_pos.y;
                }
            }
            if(-sample_pos.y <= azimuth) {
                if(first3) {
                    first3 = false;
                    dir3.y = -sample_pos.y;
                } else if(-sample_pos.y > dir3.y) {
                    dir3.y = -sample_pos.y;
                }
            }
            
            if(sample_pos.y > azimuth) {
                if(first4) {
                    first4 = false;
                    dir4.y = sample_pos.y;
                } else if(sample_pos.y < dir4.y) {
                    dir4.y = sample_pos.y;
                }
            }
            if(-sample_pos.y > azimuth) {
                if(first4) {
                    first4 = false;
                    dir4.y = -sample_pos.y;
                } else if(-sample_pos.y < dir4.y) {
                    dir4.y = -sample_pos.y;
                }
            }
        }
    }
    
    
    float azim_blend1 = 0.0f;
    if(dir2.y > dir1.y) {
        azim_blend1 = (azimuth - dir1.y) / (dir2.y - dir1.y);
    }
    
    float azim_blend2 = 0.0f;
    if(dir4.y > dir3.y) {
        azim_blend2 = (azimuth - dir3.y) / (dir4.y - dir3.y);
    }
    
    mix1 = (1.0f - azim_blend1)  *  (1.0f - elev_blend);
    mix2 = azim_blend1           *  (1.0f - elev_blend);
    mix3 = (1.0f - azim_blend2)  *  elev_blend;
    mix4 = azim_blend2           *  elev_blend;
}

void KRAudioManager::initSiren()
{
    if(m_auGraph == NULL) {
        
        m_output_sample = KRENGINE_AUDIO_BLOCK_LENGTH;
        
        // initialize double-buffer for reverb input
        int buffer_size = sizeof(float) * KRENGINE_REVERB_MAX_SAMPLES; // Reverb input is a single channel, circular buffered
        m_reverb_input_samples = (float *)malloc(buffer_size);
        memset(m_reverb_input_samples, 0, buffer_size);
        m_reverb_input_next_sample = 0;
        
        m_output_accumulation = (float *)malloc(buffer_size * 2); // 2 channels
        memset(m_output_accumulation, 0, buffer_size * 2);
        m_output_accumulation_block_start = 0;
        
        m_workspace_data = (float *)malloc(KRENGINE_REVERB_WORKSPACE_SIZE * 6 * sizeof(float));
        m_workspace[0].realp = m_workspace_data + KRENGINE_REVERB_WORKSPACE_SIZE * 0;
        m_workspace[0].imagp = m_workspace_data + KRENGINE_REVERB_WORKSPACE_SIZE * 1;
        m_workspace[1].realp = m_workspace_data + KRENGINE_REVERB_WORKSPACE_SIZE * 2;
        m_workspace[1].imagp = m_workspace_data + KRENGINE_REVERB_WORKSPACE_SIZE * 3;
        m_workspace[2].realp = m_workspace_data + KRENGINE_REVERB_WORKSPACE_SIZE * 4;
        m_workspace[2].imagp = m_workspace_data + KRENGINE_REVERB_WORKSPACE_SIZE * 5;
        
        m_reverb_sequence = 0;
        for(int i=KRENGINE_AUDIO_BLOCK_LOG2N; i <= KRENGINE_REVERB_MAX_FFT_LOG2; i++) {
            m_fft_setup[i - KRENGINE_AUDIO_BLOCK_LOG2N] = vDSP_create_fftsetup( KRENGINE_REVERB_MAX_FFT_LOG2, kFFTRadix2);
        }
    
        // ----====---- Initialize Core Audio Objects ----====----
        OSDEBUG(NewAUGraph(&m_auGraph));
        
        // ---- Create output node ----
        AudioComponentDescription output_desc;
        output_desc.componentType = kAudioUnitType_Output;
#if TARGET_OS_IPHONE
        output_desc.componentSubType = kAudioUnitSubType_RemoteIO;
#else
        output_desc.componentSubType = kAudioUnitSubType_DefaultOutput;
#endif
        output_desc.componentFlags = 0;
        output_desc.componentFlagsMask = 0;
        output_desc.componentManufacturer = kAudioUnitManufacturer_Apple;
        AUNode outputNode = 0;
        OSDEBUG(AUGraphAddNode(m_auGraph, &output_desc, &outputNode));
        
        // ---- Create mixer node ----
        AudioComponentDescription mixer_desc;
        mixer_desc.componentType = kAudioUnitType_Mixer;
        mixer_desc.componentSubType = kAudioUnitSubType_MultiChannelMixer;
        mixer_desc.componentFlags = 0;
        mixer_desc.componentFlagsMask = 0;
        mixer_desc.componentManufacturer = kAudioUnitManufacturer_Apple;
        AUNode mixerNode = 0;
        OSDEBUG(AUGraphAddNode(m_auGraph, &mixer_desc, &mixerNode ));
        
        // ---- Connect mixer to output node ----
        OSDEBUG(AUGraphConnectNodeInput(m_auGraph, mixerNode, 0, outputNode, 0));
        
        // ---- Open the audio graph ----
        OSDEBUG(AUGraphOpen(m_auGraph));
        
        // ---- Get a handle to the mixer ----
        OSDEBUG(AUGraphNodeInfo(m_auGraph, mixerNode, NULL, &m_auMixer));
        
        // ---- Add output channel to mixer ----
        UInt32 bus_count = 1;
        OSDEBUG(AudioUnitSetProperty(m_auMixer, kAudioUnitProperty_ElementCount, kAudioUnitScope_Input, 0, &bus_count, sizeof(bus_count)));
        
        // ---- Attach render function to channel ----
        AURenderCallbackStruct renderCallbackStruct;
		renderCallbackStruct.inputProc = &renderInput;
		renderCallbackStruct.inputProcRefCon = this;
        OSDEBUG(AUGraphSetNodeInputCallback(m_auGraph, mixerNode, 0, &renderCallbackStruct)); // 0 = mixer input number
        
        AudioStreamBasicDescription desc;
        memset(&desc, 0, sizeof(desc));
        
        UInt32 size = sizeof(desc);
        memset(&desc, 0, sizeof(desc));
		OSDEBUG(AudioUnitGetProperty(  m_auMixer,
                                      kAudioUnitProperty_StreamFormat,
                                      kAudioUnitScope_Input,
                                      0, // 0 = mixer input number
                                      &desc,
                                      &size));

        KRSetAUCanonical(desc, 2, false);
        desc.mSampleRate = 44100.0f;

        OSDEBUG(AudioUnitSetProperty(m_auMixer,
                             kAudioUnitProperty_StreamFormat,
                             kAudioUnitScope_Input,
                             0, // 0 == mixer input number
                             &desc,
                            sizeof(desc)));
        
        // ---- Apply properties to mixer output ----
        OSDEBUG(AudioUnitSetProperty(m_auMixer,
                             kAudioUnitProperty_StreamFormat,
                             kAudioUnitScope_Output,
                             0, // Always 0 for output bus
                             &desc,
                             sizeof(desc)));
        
        
        memset(&desc, 0, sizeof(desc));
        size = sizeof(desc);
        OSDEBUG(AudioUnitGetProperty(m_auMixer,
                             kAudioUnitProperty_StreamFormat,
                             kAudioUnitScope_Output,
                             0,
                             &desc,
                             &size));
        
        // ----
        // AUCanonical on the iPhone is the 8.24 integer format that is native to the iPhone.

        KRSetAUCanonical(desc, 2, false);
        desc.mSampleRate = 44100.0f;

        
        // ----

        OSDEBUG(AudioUnitSetProperty(m_auMixer,
                                      kAudioUnitProperty_StreamFormat,
                                      kAudioUnitScope_Output,
                                      0,
                                      &desc,
                                      sizeof(desc)));
        
        
        OSDEBUG(AudioUnitSetParameter(m_auMixer, kMultiChannelMixerParam_Volume, kAudioUnitScope_Input, 0, 1.0, 0));
        OSDEBUG(AudioUnitSetParameter(m_auMixer, kMultiChannelMixerParam_Volume, kAudioUnitScope_Output, 0, 1.0, 0));
        
        OSDEBUG(AUGraphInitialize(m_auGraph));
        
        // ----====---- Initialize HRTF Engine ----====----
        
        initHRTF();
        
        // ----====---- Start the audio system ----====---- 
        OSDEBUG(AUGraphStart(m_auGraph));
        
//        CAShow(m_auGraph);
    }
}


void KRAudioManager::cleanupSiren()
{
    if(m_auGraph) {
        OSDEBUG(AUGraphStop(m_auGraph));
        OSDEBUG(DisposeAUGraph(m_auGraph));
        m_auGraph = NULL;
        m_auMixer = NULL;
    }
    
    if(m_reverb_input_samples) {
        free(m_reverb_input_samples);
        m_reverb_input_samples = NULL;
    }
    
    if(m_output_accumulation) {
        free(m_output_accumulation);
        m_output_accumulation = NULL;
    }
    
    if(m_workspace_data) {
        free(m_workspace_data);
        m_workspace_data = NULL;
    }
    
    if(m_hrtf_data) {
        free(m_hrtf_data);
        m_hrtf_data = NULL;
    }
    
    for(int i=0; i < KRENGINE_MAX_REVERB_IMPULSE_MIX; i++) {
        m_reverb_impulse_responses[i] = NULL;
        m_reverb_impulse_responses_weight[i] = 0.0f;
    }
    
    for(int i=KRENGINE_AUDIO_BLOCK_LOG2N; i <= KRENGINE_REVERB_MAX_FFT_LOG2; i++) {
        if(m_fft_setup[i - KRENGINE_AUDIO_BLOCK_LOG2N]) {
            vDSP_destroy_fftsetup(m_fft_setup[i - KRENGINE_AUDIO_BLOCK_LOG2N]);
            m_fft_setup[i - KRENGINE_AUDIO_BLOCK_LOG2N] = NULL;
        }
    }
}

void KRAudioManager::initOpenAL()
{
    if(m_alDevice == 0) {
        // ----- Initialize OpenAL -----
        ALDEBUG(m_alDevice = alcOpenDevice(NULL));
        ALDEBUG(m_alContext=alcCreateContext(m_alDevice,NULL));
        ALDEBUG(alcMakeContextCurrent(m_alContext));
        
        // ----- Configure listener -----
        ALDEBUG(alDistanceModel(AL_EXPONENT_DISTANCE));
        ALDEBUG(alSpeedOfSound(1116.43701f)); // 1116.43701 feet per second
        
         /*
          // BROKEN IN IOS 6!!
    #if TARGET_OS_IPHONE
        ALDEBUG(alcMacOSXRenderingQualityProc(ALC_IPHONE_SPATIAL_RENDERING_QUALITY_HEADPHONES));
    #else
        ALDEBUG(alcMacOSXRenderingQualityProc(ALC_MAC_OSX_SPATIAL_RENDERING_QUALITY_HIGH));
    #endif
          */
        bool enable_reverb = false;
        if(enable_reverb) {
            UInt32 setting = 1;
            ALDEBUG(alcASASetListenerProc(ALC_ASA_REVERB_ON, &setting, sizeof(setting)));
            ALfloat global_reverb_level = -5.0f;
            ALDEBUG(alcASASetListenerProc(ALC_ASA_REVERB_GLOBAL_LEVEL, &global_reverb_level, sizeof(global_reverb_level)));
            
            setting = ALC_ASA_REVERB_ROOM_TYPE_SmallRoom; // ALC_ASA_REVERB_ROOM_TYPE_MediumHall2;
            ALDEBUG(alcASASetListenerProc(ALC_ASA_REVERB_ROOM_TYPE, &setting, sizeof(setting)));
            
            
            ALfloat global_reverb_eq_gain = 0.0f;
            ALDEBUG(alcASASetListenerProc(ALC_ASA_REVERB_EQ_GAIN, &global_reverb_eq_gain, sizeof(global_reverb_eq_gain)));
            
            ALfloat global_reverb_eq_bandwidth = 0.0f;
            ALDEBUG(alcASASetListenerProc(ALC_ASA_REVERB_EQ_BANDWITH, &global_reverb_eq_bandwidth, sizeof(global_reverb_eq_bandwidth)));
            
            ALfloat global_reverb_eq_freq = 0.0f;
            ALDEBUG(alcASASetListenerProc(ALC_ASA_REVERB_EQ_FREQ, &global_reverb_eq_freq, sizeof(global_reverb_eq_freq)));
        }
    }
}

void KRAudioManager::cleanupAudio()
{
    cleanupOpenAL();
    cleanupSiren();
}

void KRAudioManager::cleanupOpenAL()
{
    if(m_alContext) {
        ALDEBUG(alcDestroyContext(m_alContext));
        m_alContext = 0;
    }
    if(m_alDevice) {
        ALDEBUG(alcCloseDevice(m_alDevice));
        m_alDevice = 0;
    }
}

KRAudioManager::~KRAudioManager()
{
    for(unordered_map<std::string, KRAudioSample *>::iterator name_itr=m_sounds.begin(); name_itr != m_sounds.end(); name_itr++) {
        delete (*name_itr).second;
    }
    
    cleanupAudio();
    
    for(std::vector<KRDataBlock *>::iterator itr = m_bufferPoolIdle.begin(); itr != m_bufferPoolIdle.end(); itr++) {
        delete *itr;
    }
}

void KRAudioManager::makeCurrentContext()
{
    initAudio();
    if(m_audio_engine == KRAKEN_AUDIO_OPENAL) {
        if(m_alContext != 0) {
            ALDEBUG(alcMakeContextCurrent(m_alContext));
        }
    }
}

void KRAudioManager::setListenerOrientationFromModelMatrix(const KRMat4 &modelMatrix)
{
    setListenerOrientation(
        KRMat4::Dot(modelMatrix, KRVector3(0.0, 0.0, 0.0)),
        KRVector3::Normalize(KRMat4::Dot(modelMatrix, KRVector3(0.0, 0.0, -1.0)) - m_listener_position),
        KRVector3::Normalize(KRMat4::Dot(modelMatrix, KRVector3(0.0, 1.0, 0.0)) - m_listener_position)
    );
}

KRVector3 &KRAudioManager::getListenerForward()
{
    return m_listener_forward;
}

KRVector3 &KRAudioManager::getListenerPosition()
{
    return m_listener_position;
}

KRVector3 &KRAudioManager::getListenerUp()
{
    return m_listener_up;
}

void KRAudioManager::setListenerOrientation(const KRVector3 &position, const KRVector3 &forward, const KRVector3 &up)
{
    m_listener_position = position;
    m_listener_forward = forward;
    m_listener_up = up;

    makeCurrentContext();
    if(m_audio_engine == KRAKEN_AUDIO_OPENAL) {
        ALDEBUG(alListener3f(AL_POSITION, m_listener_position.x, m_listener_position.y, m_listener_position.z));
        ALfloat orientation[] = {m_listener_forward.x, m_listener_forward.y, m_listener_forward.z, m_listener_up.x, m_listener_up.y, m_listener_up.z};
        ALDEBUG(alListenerfv(AL_ORIENTATION, orientation));
    }
}

void KRAudioManager::add(KRAudioSample *sound)
{
    std::string lower_name = sound->getName();
    std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(), ::tolower);
    
    unordered_map<std::string, KRAudioSample *>::iterator name_itr = m_sounds.find(lower_name);
    if(name_itr != m_sounds.end()) {
        delete (*name_itr).second;
        (*name_itr).second = sound;
    } else {
        m_sounds[lower_name] = sound;
    }
}

KRAudioSample *KRAudioManager::load(const std::string &name, const std::string &extension, KRDataBlock *data)
{
    KRAudioSample *Sound = new KRAudioSample(getContext(), name, extension, data);
    if(Sound) add(Sound);
    return Sound;
}

KRAudioSample *KRAudioManager::get(const std::string &name)
{
    std::string lower_name = name;
    std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(), ::tolower);
    return m_sounds[lower_name];
}

KRDataBlock *KRAudioManager::getBufferData(int size)
{
    KRDataBlock *data;
    // Note: We only store and recycle buffers with a size of CIRCA_AUDIO_MAX_BUFFER_SIZE
    if(size == KRENGINE_AUDIO_MAX_BUFFER_SIZE && m_bufferPoolIdle.size() > 0) {
        // Recycle a buffer from the pool
        data = m_bufferPoolIdle.back();
        m_bufferPoolIdle.pop_back();
    } else {
        data = new KRDataBlock();
        data->expand(size);
    }
    data->lock();
    return data;
}

void KRAudioManager::recycleBufferData(KRDataBlock *data)
{
    if(data != NULL) {
        data->unlock();
        if(data->getSize() == KRENGINE_AUDIO_MAX_BUFFER_SIZE && m_bufferPoolIdle.size() < KRENGINE_AUDIO_MAX_POOL_SIZE) {
            m_bufferPoolIdle.push_back(data);
        } else {
            delete data;
        }
    }
}

OSStatus alcASASetListenerProc(const ALuint property, ALvoid *data, ALuint dataSize)
{
    OSStatus    err = noErr;
    static  alcASASetListenerProcPtr    proc = NULL;
    
    if (proc == NULL) {
        proc = (alcASASetListenerProcPtr) alcGetProcAddress(NULL, "alcASASetListener");
    }
    
    if (proc)
        err = proc(property, data, dataSize);
    return (err);
}

ALvoid alcMacOSXRenderingQualityProc(const ALint value)
{
    static  alcMacOSXRenderingQualityProcPtr    proc = NULL;
    
    if (proc == NULL) {
        proc = (alcMacOSXRenderingQualityProcPtr) alcGetProcAddress(NULL, (const ALCchar*) "alcMacOSXRenderingQuality");
    }
    
    if (proc)
        proc(value);
    
    return;
}

KRAudioManager::audio_engine_t KRAudioManager::getAudioEngine()
{
    return m_audio_engine;
}

void KRAudioManager::activateAudioSource(KRAudioSource *audioSource)
{
    m_activeAudioSources.insert(audioSource);
}

void KRAudioManager::deactivateAudioSource(KRAudioSource *audioSource)
{
    m_activeAudioSources.erase(audioSource);
}

__int64_t KRAudioManager::getAudioFrame()
{
    return m_audio_frame;
}

KRAudioBuffer *KRAudioManager::getBuffer(KRAudioSample &audio_sample, int buffer_index)
{
    // ----====---- Try to find the buffer in the cache ----====----
    for(std::vector<KRAudioBuffer *>::iterator itr=m_bufferCache.begin(); itr != m_bufferCache.end(); itr++) {
        KRAudioBuffer *cache_buffer = *itr;
        if(cache_buffer->getAudioSample() == &audio_sample && cache_buffer->getIndex() == buffer_index) {
            return cache_buffer;
        }
    }
    
    // ----====---- Make room in the cache for a new buffer ----====----
    if(m_bufferCache.size() >= KRENGINE_AUDIO_MAX_POOL_SIZE) {
        // delete a random entry from the cache
        int index_to_delete = arc4random() % m_bufferCache.size();
        std::vector<KRAudioBuffer *>::iterator itr_to_delete = m_bufferCache.begin() + index_to_delete;
        delete *itr_to_delete;
        m_bufferCache.erase(itr_to_delete);
    }
    
    // ----====---- Request new buffer, add to cache, and return it ----====----
    KRAudioBuffer *buffer = audio_sample.getBuffer(buffer_index);
    m_bufferCache.push_back(buffer);
    return buffer;
}

float KRAudioManager::getGlobalReverbSendLevel()
{
    return m_global_reverb_send_level;
}

void KRAudioManager::setGlobalReverbSendLevel(float send_level)
{
    m_global_reverb_send_level = send_level;
}

float KRAudioManager::getGlobalGain()
{
    return m_global_gain;
}

void KRAudioManager::setGlobalGain(float gain)
{
    m_global_gain = gain;
}

float KRAudioManager::getGlobalAmbientGain()
{
    return m_global_ambient_gain;
}

void KRAudioManager::setGlobalAmbientGain(float gain)
{
    m_global_ambient_gain = gain;
}

void KRAudioManager::startFrame(float deltaTime)
{
    m_mutex.lock();
    
    // ----====---- Determine Ambient Zone Contributions ----====----
    m_ambient_zone_weights.clear();
    m_ambient_zone_total_weight = 0.0f; // For normalizing zone weights
    if(m_listener_scene) {
        std::set<KRAmbientZone *> ambient_zones = m_listener_scene->getAmbientZones();
        
        for(std::set<KRAmbientZone *>::iterator itr=ambient_zones.begin(); itr != ambient_zones.end(); itr++) {
            KRAmbientZone *sphere = *itr;
            siren_ambient_zone_weight_info zi;
            
            zi.weight = sphere->getContainment(m_listener_position);
            if(zi.weight > 0.0f) {
                if(m_ambient_zone_weights.find(sphere->getZone()) == m_ambient_zone_weights.end()) {
                    zi.ambient_zone = sphere;
                    zi.ambient_sample = get(sphere->getAmbient());
                    m_ambient_zone_weights[sphere->getZone()] = zi;
                    m_ambient_zone_total_weight += zi.weight;
                } else {
                    float prev_weight = m_ambient_zone_weights[sphere->getZone()].weight;
                    if(zi.weight > prev_weight) {
                        m_ambient_zone_total_weight -= prev_weight;
                        m_ambient_zone_weights[sphere->getZone()].weight = zi.weight;
                        m_ambient_zone_total_weight += zi.weight;
                    }
                }
            }
        }
    }
    
    // ----====---- Determine Reverb Zone Contributions ----====----
    m_reverb_zone_weights.clear();
    m_reverb_zone_total_weight = 0.0f; // For normalizing zone weights
    if(m_listener_scene) {
        std::set<KRReverbZone *> reverb_zones = m_listener_scene->getReverbZones();
        
        for(std::set<KRReverbZone *>::iterator itr=reverb_zones.begin(); itr != reverb_zones.end(); itr++) {
            KRReverbZone *sphere = *itr;
            siren_reverb_zone_weight_info zi;
            
            zi.weight = sphere->getContainment(m_listener_position);
            if(zi.weight > 0.0f) {
                if(m_reverb_zone_weights.find(sphere->getZone()) == m_reverb_zone_weights.end()) {
                    zi.reverb_zone = sphere;
                    zi.reverb_sample = get(sphere->getReverb());
                    m_reverb_zone_weights[sphere->getZone()] = zi;
                    m_reverb_zone_total_weight += zi.weight;
                } else {
                    float prev_weight = m_reverb_zone_weights[sphere->getZone()].weight;
                    if(zi.weight > prev_weight) {
                        m_reverb_zone_total_weight -= prev_weight;
                        m_reverb_zone_weights[sphere->getZone()].weight = zi.weight;
                        m_reverb_zone_total_weight += zi.weight;
                    }
                }
            }
        }
    }
    
    
    
    // ----====---- Map Source Directions and Gains ----====----
    m_prev_mapped_sources.clear();
    m_mapped_sources.swap(m_prev_mapped_sources);
    
    KRVector3 listener_right = KRVector3::Cross(m_listener_forward, m_listener_up);
    std::set<KRAudioSource *> active_sources = m_activeAudioSources;
    
    
    for(std::set<KRAudioSource *>::iterator itr=active_sources.begin(); itr != active_sources.end(); itr++) {
        KRAudioSource *source = *itr;
        KRVector3 source_world_position = source->getWorldTranslation();
        KRVector3 diff = source_world_position - m_listener_position;
        float distance = diff.magnitude();
        float gain = source->getGain() * m_global_gain / pow(KRMAX(distance / source->getReferenceDistance(), 1.0f), source->getRolloffFactor());
        
        // apply minimum-cutoff so that we don't waste cycles processing very quiet / distant sound sources
        gain = KRMAX(gain - KRENGINE_AUDIO_CUTOFF, 0.0f) / (1.0f - KRENGINE_AUDIO_CUTOFF);
        
        if(gain > 0.0f) {
            
            KRVector3 source_listener_space = KRVector3(
                                                        KRVector3::Dot(listener_right, diff),
                                                        KRVector3::Dot(m_listener_up, diff),
                                                        KRVector3::Dot(m_listener_forward, diff)
                                                        );
            
            
            KRVector3 source_dir = KRVector3::Normalize(source_listener_space);
            
            
            
            if(source->getEnableOcclusion() && false) {
                KRHitInfo hitinfo;
                if(source->getScene().lineCast(m_listener_position, source_world_position, hitinfo, KRAKEN_COLLIDER_AUDIO)) {
                    gain = 0.0f;
                }
            }
            
            KRVector2 source_dir2 = KRVector2::Normalize(KRVector2(source_dir.x, source_dir.z));
            float azimuth = -atan2(source_dir2.x, -source_dir2.y);
            float elevation = atan( source_dir.y / sqrt(source_dir.x * source_dir.x + source_dir.z * source_dir.z));
            
            KRVector2 adjusted_source_dir = KRVector2(elevation, azimuth);
            
            if(!m_high_quality_hrtf) {
                adjusted_source_dir = getNearestHRTFSample(adjusted_source_dir);
            }
            
            // Click Removal - Add ramping of gain changes for audio sources that are continuing to play
            float gain_anticlick = 0.0f;
            std::pair<unordered_multimap<KRVector2, std::pair<KRAudioSource *, std::pair<float, float> > >::iterator, unordered_multimap<KRVector2, std::pair<KRAudioSource *, std::pair<float, float> > >::iterator> prev_range = m_prev_mapped_sources.equal_range(adjusted_source_dir);
            for(unordered_multimap<KRVector2, std::pair<KRAudioSource *, std::pair<float, float> > >::iterator prev_itr=prev_range.first; prev_itr != prev_range.second; prev_itr++) {
                if( (*prev_itr).second.first == source) {
                    gain_anticlick = (*prev_itr).second.second.second;
                    break;
                }
            }
            
            m_mapped_sources.insert(std::pair<KRVector2, std::pair<KRAudioSource *, std::pair<float, float> > >(adjusted_source_dir, std::pair<KRAudioSource *, std::pair<float, float> >(source, std::pair<float, float>(gain_anticlick, gain))));
        }
    }
    
    // Click Removal - Map audio sources for ramp-down of gain for audio sources that have been squelched by attenuation
    for(unordered_multimap<KRVector2, std::pair<KRAudioSource *, std::pair<float, float> > >::iterator itr=m_prev_mapped_sources.begin(); itr != m_prev_mapped_sources.end(); itr++) {

        KRAudioSource *source = (*itr).second.first;
        float source_prev_gain = (*itr).second.second.second;
        if(source->isPlaying() && source_prev_gain > 0.0f) {
            // Only create ramp-down channels for 3d sources that have been squelched by attenuation; this is not necessary if the sample has completed playing
            KRVector2 source_position = (*itr).first;
            
            std::pair<unordered_multimap<KRVector2, std::pair<KRAudioSource *, std::pair<float, float> > >::iterator, unordered_multimap<KRVector2, std::pair<KRAudioSource *, std::pair<float, float> > >::iterator> new_range = m_mapped_sources.equal_range(source_position);
            bool already_merged = false;
            for(unordered_multimap<KRVector2, std::pair<KRAudioSource *, std::pair<float, float> > >::iterator new_itr=new_range.first; new_itr != new_range.second; new_itr++) {
                if( (*new_itr).second.first == source) {
                    already_merged = true;
                    break;
                }
            }
            if(!already_merged) {
                
                // source gain becomes anticlick gain and gain becomes 0 for anti-click ramp-down.
                m_mapped_sources.insert(std::pair<KRVector2, std::pair<KRAudioSource *, std::pair<float, float> > >(source_position, std::pair<KRAudioSource *, std::pair<float, float> >(source, std::pair<float, float>(source_prev_gain, 0.0f))));
            }
        }
    }
    
    m_anticlick_block = true;
    m_mutex.unlock();
}

void KRAudioManager::renderAmbient()
{
    if(m_listener_scene) {

        int output_offset = (m_output_accumulation_block_start) % (KRENGINE_REVERB_MAX_SAMPLES * KRENGINE_MAX_OUTPUT_CHANNELS);
        float *buffer = m_workspace[0].realp;
        
        for(unordered_map<std::string, siren_ambient_zone_weight_info>::iterator zone_itr=m_ambient_zone_weights.begin(); zone_itr != m_ambient_zone_weights.end(); zone_itr++) {
            siren_ambient_zone_weight_info zi = (*zone_itr).second;
            float gain = (*zone_itr).second.weight * zi.ambient_zone->getAmbientGain() * m_global_ambient_gain * m_global_gain;
            
            KRAudioSample *source_sample = zi.ambient_sample;
            if(source_sample) {
                for(int channel=0; channel < KRENGINE_MAX_OUTPUT_CHANNELS; channel++) {
                    source_sample->sample(getContext().getAudioManager()->getAudioFrame(), KRENGINE_AUDIO_BLOCK_LENGTH, channel, buffer, gain, true);            
                    vDSP_vadd(m_output_accumulation + output_offset + channel, KRENGINE_MAX_OUTPUT_CHANNELS, buffer, 1, m_output_accumulation + output_offset + channel, KRENGINE_MAX_OUTPUT_CHANNELS, KRENGINE_AUDIO_BLOCK_LENGTH);
                }
            }
        }
    }
}

void KRAudioManager::renderHRTF()
{
    DSPSplitComplex *hrtf_accum = m_workspace + 0;
    DSPSplitComplex *hrtf_impulse = m_workspace + 1;
    DSPSplitComplex *hrtf_convolved = m_workspace + 1; // We only need hrtf_impulse or hrtf_convolved at once; we can recycle the buffer
    DSPSplitComplex *hrtf_sample = m_workspace + 2;
    
    int impulse_response_channels = 2;
    int hrtf_frames = 128;
    int fft_size = 256;
    int fft_size_log2 = 8;
    
    for(int channel=0; channel<impulse_response_channels; channel++) {
        
        bool first_source = true;
        unordered_multimap<KRVector2, std::pair<KRAudioSource *, std::pair<float, float> > >::iterator itr=m_mapped_sources.begin();
        while(itr != m_mapped_sources.end()) {
            // Batch together sound sources that are emitted from the same direction
            KRVector2 source_direction = (*itr).first;
            KRAudioSource *source = (*itr).second.first;
            float gain_anticlick = (*itr).second.second.first;
            float gain = (*itr).second.second.second;

            
            // If this is the first or only sample, write directly to the first half of the FFT input buffer
            // Subsequent samples write to the second half of the FFT input buffer, which is then added to the first half (the second half will be zero'ed out anyways and works as a convenient temporary buffer)
            float *sample_buffer = first_source ? hrtf_sample->realp : hrtf_sample->realp + KRENGINE_AUDIO_BLOCK_LENGTH;
            
            if(gain != gain_anticlick && m_anticlick_block) {
                // Sample and perform anti-click filtering
                source->sample(KRENGINE_AUDIO_BLOCK_LENGTH, 0, sample_buffer, 1.0);
                float ramp_gain = gain_anticlick;
                float ramp_step = (gain - gain_anticlick) / KRENGINE_AUDIO_ANTICLICK_SAMPLES;
                vDSP_vrampmul(sample_buffer, 1, &ramp_gain, &ramp_step, sample_buffer, 1, KRENGINE_AUDIO_ANTICLICK_SAMPLES);
                if(KRENGINE_AUDIO_BLOCK_LENGTH > KRENGINE_AUDIO_ANTICLICK_SAMPLES) {
                    vDSP_vsmul(sample_buffer + KRENGINE_AUDIO_ANTICLICK_SAMPLES, 1, &gain, sample_buffer + KRENGINE_AUDIO_ANTICLICK_SAMPLES, 1, KRENGINE_AUDIO_BLOCK_LENGTH - KRENGINE_AUDIO_ANTICLICK_SAMPLES);
                }
            } else {
                // Don't need to perform anti-click filtering, so just sample
                source->sample(KRENGINE_AUDIO_BLOCK_LENGTH, 0, sample_buffer, gain);
            }

            if(first_source) {
                first_source = false;
            } else {
                // Accumulate samples on subsequent sources
                vDSP_vadd(hrtf_sample->realp, 1, sample_buffer, 1, hrtf_sample->realp, 1, KRENGINE_AUDIO_BLOCK_LENGTH);
            }
            
            itr++;

            bool end_of_group = false;
            if(itr == m_mapped_sources.end()) {
                end_of_group = true;
            } else {
                KRVector2 next_direction = (*itr).first;
                end_of_group = next_direction != source_direction;
            }

            if(end_of_group) {
                // ----====---- We have reached the end of a batch; convolve with HRTF impulse response and add to output accumulation buffer ----====----                
                first_source = true;
                
                memset(hrtf_sample->realp + hrtf_frames, 0, sizeof(float) * hrtf_frames);
                memset(hrtf_sample->imagp, 0, sizeof(float) * fft_size);
                
                DSPSplitComplex hrtf_spectral;
                
                if(m_high_quality_hrtf) {
                    // High quality, interpolated HRTF
                    hrtf_spectral= *hrtf_accum;
                    
                    float mix[4];
                    KRVector2 dir[4];
                    
                    getHRTFMix(source_direction, dir[0], dir[1], dir[2], dir[3], mix[0], mix[1], mix[2], mix[3]);
                
                    
                    memset(hrtf_accum->realp, 0, sizeof(float) * fft_size);
                    memset(hrtf_accum->imagp, 0, sizeof(float) * fft_size);
                    
                    for(int i=0; i < 1 /*4 */; i++) {
                        if(mix[i] > 0.0f) {
                            DSPSplitComplex hrtf_impulse_sample = getHRTFSpectral(dir[i], channel);
                            vDSP_vsmul(hrtf_impulse_sample.realp, 1, mix+i, hrtf_impulse->realp, 1, fft_size);
                            vDSP_vsmul(hrtf_impulse_sample.imagp, 1, mix+i, hrtf_impulse->imagp, 1, fft_size);
                            vDSP_zvadd(hrtf_impulse, 1, hrtf_accum, 1, hrtf_accum, 1, fft_size);
                        }
                    }
                } else {
                    // Low quality, non-interpolated HRTF
                    hrtf_spectral = getHRTFSpectral(source_direction, channel);
                }
                
                float scale = 0.5f / fft_size;
                
                vDSP_fft_zip(m_fft_setup[fft_size_log2 - KRENGINE_AUDIO_BLOCK_LOG2N], hrtf_sample, 1, fft_size_log2, kFFTDirection_Forward);
                vDSP_zvmul(hrtf_sample, 1, &hrtf_spectral, 1, hrtf_convolved, 1, fft_size, 1);
                vDSP_fft_zip(m_fft_setup[fft_size_log2 - KRENGINE_AUDIO_BLOCK_LOG2N], hrtf_convolved, 1, fft_size_log2, kFFTDirection_Inverse);
                vDSP_vsmul(hrtf_convolved->realp, 1, &scale, hrtf_convolved->realp, 1, fft_size);
                
                int output_offset = (m_output_accumulation_block_start) % (KRENGINE_REVERB_MAX_SAMPLES * KRENGINE_MAX_OUTPUT_CHANNELS);
                int frames_left = fft_size;
                while(frames_left) {
                    int frames_to_process = (KRENGINE_REVERB_MAX_SAMPLES * KRENGINE_MAX_OUTPUT_CHANNELS - output_offset) / KRENGINE_MAX_OUTPUT_CHANNELS;
                    if(frames_to_process > frames_left) frames_to_process = frames_left;
                    vDSP_vadd(m_output_accumulation + output_offset + channel, KRENGINE_MAX_OUTPUT_CHANNELS, hrtf_convolved->realp + fft_size - frames_left, 1, m_output_accumulation + output_offset + channel, KRENGINE_MAX_OUTPUT_CHANNELS, frames_to_process);
                    frames_left -= frames_to_process;
                    output_offset = (output_offset + frames_to_process * KRENGINE_MAX_OUTPUT_CHANNELS) % (KRENGINE_REVERB_MAX_SAMPLES * KRENGINE_MAX_OUTPUT_CHANNELS);
                }
            }
        }
    }
}

void KRAudioManager::renderITD()
{
    // FINDME, TODO - Need Inter-Temperal based phase shifting to support 3-d spatialized audio without headphones
    
    /*
     
     
     // hrtf_kemar_H-10e000a.wav
     
     float speed_of_sound = 1126.0f; // feed per second FINDME - TODO - This needs to be configurable for scenes with different units
     float head_radius = 0.7431f; // 0.74ft = 22cm
     float half_max_itd_time = head_radius / speed_of_sound / 2.0f; // half of ITD time (Interaural time difference) when audio source is directly 90 degrees azimuth to one ear.
     
     //    KRVector3 m_listener_position;
     //    KRVector3 m_listener_forward;
     //    KRVector3 m_listener_up;
     
     KRVector3 listener_right = KRVector3::Cross(m_listener_forward, m_listener_up);
     KRVector3 listener_right_ear = m_listener_position + listener_right * head_radius / 2.0f;
     KRVector3 listener_left_ear = m_listener_position - listener_right * head_radius / 2.0f;
     
     // Get a pointer to the dataBuffer of the AudioBufferList
     
     AudioUnitSampleType *outA = (AudioUnitSampleType *)ioData->mBuffers[0].mData;
     AudioUnitSampleType *outB = (AudioUnitSampleType *)ioData->mBuffers[1].mData; // Non-Interleaved only
     
     
     // ----====---- Zero out accumulation / output buffer ----====----
     for (UInt32 i = 0; i < inNumberFrames; ++i) {
     
     #if CA_PREFER_FIXED_POINT
     // Interleaved
     //        outA[i*2] = (SInt16)(left_channel * 32767.0f);
     //        outA[i*2 + 1] = (SInt16)(right_channel * 32767.0f);
     
     // Non-Interleaved
     outA[i] = (SInt32)(0x1000000f);
     outB[i] = (SInt32)(0x1000000f);
     #else
     
     // Interleaved
     //        outA[i*2] = (Float32)left_channel;
     //        outA[i*2 + 1] = (Float32)right_channel;
     
     // Non-Interleaved
     outA[i] = (Float32)0.0f;
     outB[i] = (Float32)0.0f;
     #endif
     }
     
     // ----====---- Render direct / HRTF audio ----====----
     for(std::set<KRAudioSource *>::iterator itr=m_activeAudioSources.begin(); itr != m_activeAudioSources.end(); itr++) {
     KRAudioSource *source = *itr;
     KRVector3 listener_to_source = source->getWorldTranslation() - m_listener_position;
     KRVector3 right_ear_to_source = source->getWorldTranslation() - listener_right_ear;
     KRVector3 left_ear_to_source = source->getWorldTranslation() - listener_left_ear;
     KRVector3 source_direction = KRVector3::Normalize(listener_to_source);
     float right_ear_distance = right_ear_to_source.magnitude();
     float left_ear_distance = left_ear_to_source.magnitude();
     float right_itd_time = right_ear_distance / speed_of_sound;
     float left_itd_time = left_ear_distance / speed_of_sound;
     
     float rolloff_factor = source->getRolloffFactor();
     float left_gain = 1.0f / pow(left_ear_distance / rolloff_factor, 2.0f);
     float right_gain = 1.0f / pow(left_ear_distance / rolloff_factor, 2.0f);
     if(left_gain > 1.0f) left_gain = 1.0f;
     if(right_gain > 1.0f) right_gain = 1.0f;
     left_gain *= source->getGain();
     right_gain *= source->getGain();
     int left_itd_offset = (int)(left_itd_time * 44100.0f);
     int right_itd_offset = (int)(right_itd_time * 44100.0f);
     KRAudioSample *sample = source->getAudioSample();
     if(sample) {
     __int64_t source_start_frame = source->getStartAudioFrame();
     int sample_frame = (int)(m_audio_frame - source_start_frame);
     for (UInt32 i = 0; i < inNumberFrames; ++i) {
     float left_channel=sample->sample(sample_frame + left_itd_offset, 44100, 0) * left_gain;
     float right_channel = sample->sample(sample_frame + right_itd_offset, 44100, 0) * right_gain;
     
     //                left_channel = 0.0f;
     //                right_channel = 0.0f;
     
     #if CA_PREFER_FIXED_POINT
     // Interleaved
     //        outA[i*2] = (SInt16)(left_channel * 32767.0f);
     //        outA[i*2 + 1] = (SInt16)(right_channel * 32767.0f);
     
     // Non-Interleaved
     outA[i] += (SInt32)(left_channel * 0x1000000f);
     outB[i] += (SInt32)(right_channel * 0x1000000f);
     #else
     
     // Interleaved
     //        outA[i*2] = (Float32)left_channel;
     //        outA[i*2 + 1] = (Float32)right_channel;
     
     // Non-Interleaved
     outA[i] += (Float32)left_channel;
     outB[i] += (Float32)right_channel;
     #endif
     sample_frame++;
     }
     }
     }
     
     KRAudioSample *reverb_impulse_response = getContext().getAudioManager()->get("test_reverb");
     
     // ----====---- Render Indirect / Reverb channel ----====----
     int buffer_frame_start=0;
     int remaining_frames = inNumberFrames - buffer_frame_start;
     while(remaining_frames) {
     int frames_processed = remaining_frames;
     if(frames_processed > KRENGINE_REVERB_FILTER_LENGTH) {
     frames_processed = KRENGINE_REVERB_FILTER_LENGTH;
     }
     bool first_source = true;
     for(std::set<KRAudioSource *>::iterator itr=m_activeAudioSources.begin(); itr != m_activeAudioSources.end(); itr++) {
     KRAudioSource *source = *itr;
     KRAudioSample *sample = source->getAudioSample();
     if(sample) {
     __int64_t source_start_frame = source->getStartAudioFrame();
     int sample_frame = (int)(m_audio_frame - source_start_frame + buffer_frame_start);
     
     
     
     
     for (UInt32 i = 0; i < inNumberFrames; ++i) {
     if(first_source) {
     sample->sample(sample_frame, 44100, 0, m_reverb_input_next_sample);
     }
     sample->sampleAdditive(sample_frame, 44100, 0, m_reverb_input_next_sample);
     if(m_reverb_input_next_sample >= KRENGINE_REVERB_MAX_SAMPLES) {
     m_reverb_input_next_sample -= KRENGINE_REVERB_MAX_SAMPLES;
     }
     }
     }
     first_source = false;
     }
     
     
     if(reverb_impulse_response) {
     // Perform FFT Convolution for Impulse-response based reverb
     
     memcpy(m_reverb_accumulation, m_reverb_accumulation + KRENGINE_REVERB_FILTER_LENGTH, KRENGINE_REVERB_FILTER_LENGTH * sizeof(float));
     memset(m_reverb_accumulation + KRENGINE_REVERB_FILTER_LENGTH, 0, KRENGINE_REVERB_FILTER_LENGTH * sizeof(float));
     }
     
     buffer_frame_start += frames_processed;
     remaining_frames = inNumberFrames - buffer_frame_start;
     }
     
     
     */
}