#pragma once

#include <SDL2/SDL.h>
#include <mutex>
#include <functional>

#define AudioSampleRate 48000
#define AudioOutChannelCount 2
#define FrameSizeBytes 8 // 32 bit samples * 2 channels

// Based on 60Hz screen refresh rate
#define AudioBufferSize ((AudioSampleRate * AudioOutChannelCount)) // 1 sec TODO: This is pretty big
#define CyclesPerFrame 70224 // TODO: Can we import this?
#define CyclesPerSecond 4213440 // CyclesPerFrame * 60

#define MaxHarmonicsCount 52

class APU : public IMemoryUnit
{
public:
    APU();
    ~APU();

    void Step(unsigned long cycles);
    void Channel1Callback(Uint8* pStream, int length);
    void Channel2Callback(Uint8* pStream, int length);
    void Channel3Callback(Uint8* pStream, int length);
    void Channel4Callback(Uint8* pStream, int length);

    // IMemoryUnit
    byte ReadByte(const ushort& address);
    bool WriteByte(const ushort& address, const byte val);

private:
    void LoadChannel(int index, SDL_AudioCallback callback);

    typedef enum {
        UP, DOWN
    } EnvelopeDirection;

    // Ring buffer for streaming audio data
    class Buffer {
        public:
            Buffer(size_t element_count, size_t element_size);
            ~Buffer();

            void Reset();
            void Put(Uint8* element);
            Uint8* Get();

        private:
            size_t m_ElementCount;
            size_t m_ElementSize;
            size_t m_BufferSize;
            Uint8* m_Bytes;
            Uint8* m_DefaultBytes;
            size_t m_ReadIndex;
            size_t m_WriteIndex;
            std::mutex mutex;

            void AdvanceReadIndex();
            void AdvanceWriteIndex();
    };

    class AdditiveSquareWaveGenerator {
        public:
            AdditiveSquareWaveGenerator();
            ~AdditiveSquareWaveGenerator() = default;

            void SetFrequency(double frequency_hz);
            void SetDutyCycle(double duty_cycle);
            void SetCounterModeEnabled(bool is_enabled);
            void SetSoundLength(double sound_length_seconds);
            void SetSweepDirection(EnvelopeDirection direction);
            void SetSweepShiftFrequencyExponent(uint exponent);
            void SetSweepStepLength(double sweep_step_seconds);
            void SetEnvelopeStartVolume(double envelope_start_volume);
            void SetEnvelopeDirection(EnvelopeDirection direction);
            void SetEnvelopeStepLength(double envelope_step_seconds);
            void SetOnChannelOn(std::function<void()> callback);
            void SetOnChannelOff(std::function<void()> callback);
            
            void RestartSound();
            
            float NextSample();

            void DebugLog();

        private:
            double m_FrequencyHz;
            double m_DutyCycle;
            bool m_CounterModeEnabled;
            double m_SoundLengthSeconds;
            bool m_SweepModeEnabled;
            double m_SweepDirection;
            double m_SweepStartFrequency; // TODO: Is it needed?
            int m_SweepShiftFrequencyExponent;
            double m_SweepStepLengthSeconds;
            bool m_EnvelopeModeEnabled;
            double m_EnvelopeDirection;
            double m_EnvelopeStartVolume;
            double m_EnvelopeStepLengthSeconds; 
            int m_HarmonicsCount;
            double m_Coefficients[MaxHarmonicsCount];
            double m_Phase;
            double m_SoundLengthTimerSeconds;
            bool m_ChannelIsPlaying;
            std::function<void()> m_OnChannelOn;
            std::function<void()> m_OnChannelOff;

            void RegenerateCoefficients();
    };

    class NoiseGenerator {
        public:
            NoiseGenerator();
            ~NoiseGenerator() = default;

            void SetFrequency(double frequency_hz);
            void SetCounterModeEnabled(bool is_enabled);
            void SetSoundLength(double sound_length_seconds);
            void SetEnvelopeStartVolume(double envelope_start_volume);
            void SetEnvelopeDirection(EnvelopeDirection direction);
            void SetEnvelopeStepLength(double envelope_step_seconds);

            void RestartSound();
            
            float NextSample();

            void DebugLog();

        private:
            double m_FrequencyHz;
            bool m_CounterModeEnabled;
            double m_SoundLengthSeconds;
            bool m_EnvelopeModeEnabled;
            double m_EnvelopeDirection;
            double m_EnvelopeStartVolume;
            double m_EnvelopeStepLengthSeconds;
            double m_Phase;
            double m_Signal;
            double m_SoundLengthTimerSeconds;
    };

    class WaveformGenerator
    {
    public:
        WaveformGenerator();
        ~WaveformGenerator() = default;

        void SetFrequency(double frequency_hz);
        void SetOutputLevel(double level);
        void SetCounterModeEnabled(bool is_enabled);
        void SetSoundLength(double sound_length_seconds);
        void SetWaveRamLocation(byte* wave_buffer);
        
        void Enable();
        void Disable();
        void RestartSound();

        float NextSample();

        void DebugLog();

    private:
        bool m_Enabled;
        double m_FrequencyHz;
        bool m_CounterModeEnabled;
        double m_SoundLengthSeconds;
        double m_OutputLevel;
        double m_Phase;
        double m_SoundLengthTimerSeconds;
        byte* m_WaveBuffer;
    };
    
private:
    byte m_Channel1Sweep;
    byte m_Channel1SoundLength;
    byte m_Channel1VolumeEnvelope;
    byte m_Channel1FrequencyLo;
    byte m_Channel1FrequencyHi;

    byte m_Channel2SoundLength;
    byte m_Channel2VolumeEnvelope;
    byte m_Channel2FrequencyLo;
    byte m_Channel2FrequencyHi;

    byte m_Channel3SoundOnOff;
    byte m_Channel3SoundLength;
    byte m_Channel3SelectOutputLevel;
    byte m_Channel3FrequencyLo;
    byte m_Channel3FrequencyHi;
    byte m_WavePatternRAM[0x0F + 1];

    byte m_Channel4SoundLength;
    byte m_Channel4VolumeEnvelope;
    byte m_Channel4PolynomialCounter;
    byte m_Channel4Counter;

    byte m_ChannelControlOnOffVolume;
    byte m_OutputTerminal;

    byte m_SoundOnOff;

    // Synthesis
    AdditiveSquareWaveGenerator m_Channel1SoundGenerator;
    AdditiveSquareWaveGenerator m_Channel2SoundGenerator;
    WaveformGenerator m_Channel3SoundGenerator;
    NoiseGenerator m_Channel4SoundGenerator;

    // Output
    bool m_Initialized[4];
    SDL_AudioDeviceID m_DeviceChannel[4];
    double m_AudioFrameRemainder;
    Buffer m_OutputBuffer;

    // For testing...
    byte PrevChannel1SweepTime;
    byte PrevChannel1SweepDirection;
    byte PrevChannel1SweepNumber;
    byte PrevChannel1WavePatternDuty;
    byte PrevChannel1SoundLength;
    byte PrevChannel1VolumeEnvelopeStart;
    byte PrevChannel1VolumeEnvelopeDirection;
    byte PrevChannel1VolumeEnvelopeSweepNumber;
    byte PrevChannel1Initial;
    byte PrevChannel1CounterConsecutive;
    int PrevChannel1Frequency;

    byte PrevChannel2WavePatternDuty;
    byte PrevChannel2SoundLength;
    byte PrevChannel2VolumeEnvelopeStart;
    byte PrevChannel2VolumeEnvelopeDirection;
    byte PrevChannel2VolumeEnvelopeSweepNumber;
    byte PrevChannel2Initial;
    byte PrevChannel2CounterConsecutive;
    int PrevChannel2Frequency;

    byte PrevChannel3SoundOnOff;
    byte PrevChannel3SoundLength;
    byte PrevChannel3SelectOutputLevel;
    byte PrevChannel3Initial;
    byte PrevChannel3CounterConsecutive;
    int PrevChannel3Frequency;
    byte PrevWavePatternRAM[0x0F + 1];

    byte PrevChannel4SoundLength;
    byte PrevChannel4VolumeEnvelopeStart;
    byte PrevChannel4VolumeEnvelopeDirection;
    byte PrevChannel4VolumeEnvelopeSweepNumber;
    byte PrevChannel4ShiftClockFrequency;
    byte PrevChannel4CounterStep;
    byte PrevChannel4FrequencyDivideRatio;
    byte PrevChannel4Initial;
    byte PrevChannel4CounterConsecutive;
};
