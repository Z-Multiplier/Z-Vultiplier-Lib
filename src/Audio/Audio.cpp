//MIT License

//Copyright (c) 2026 Z-Multiplier
#include "Audio.hpp"

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"
#include <cstring>
#include <cmath>

namespace Audio{
    Sound::Sound(std::string filePath):source(std::in_place_index<0>){
        ma_decoder* decoder=&std::get<ma_decoder>(source);
        if(ma_decoder_init_file(filePath.c_str(),nullptr,decoder)!=MA_SUCCESS){
            throw std::runtime_error("Failed to load audio file: "+filePath);
        }

        ma_uint64 totalFrames;
        if(ma_decoder_get_length_in_pcm_frames(decoder,&totalFrames)==MA_SUCCESS){
            duration=static_cast<long double>(totalFrames)/decoder->outputSampleRate;
        }
        else{
            duration=0.0;
        }

        sound=SoundPtr(new ma_sound());
    }

    Sound::Sound(WaveForm wave,float hz,float amplitude,long double duration)
        :duration(duration),source(std::in_place_index<1>){
        ma_waveform* waveform=&std::get<ma_waveform>(source);

        ma_waveform_type type;
        switch(wave){
            case WaveForm::SINE:type=ma_waveform_type_sine;break;
            case WaveForm::SQUARE:type=ma_waveform_type_square;break;
            case WaveForm::TRIANGLE:type=ma_waveform_type_triangle;break;
            case WaveForm::SAWTOOTH:type=ma_waveform_type_sawtooth;break;
            default:type=ma_waveform_type_sine;break;
        }

        ma_waveform_config config=ma_waveform_config_init(
            ma_format_f32,
            2,
            48000,
            type,
            amplitude,
            hz
        );
        if(ma_waveform_init(&config,waveform)!=MA_SUCCESS){
            throw std::runtime_error("Failed to initialize waveform");
        }

        sound=SoundPtr(new ma_sound());
    }

    Sound::Sound(NoiseColor noise,float amplitude,long double duration)
        :duration(duration),source(std::in_place_index<2>){
        ma_noise* noiseObj=&std::get<ma_noise>(source);

        ma_noise_type type;
        switch(noise){
            case NoiseColor::WHITE:type=ma_noise_type_white;break;
            case NoiseColor::PINK:type=ma_noise_type_pink;break;
            case NoiseColor::BROWN:type=ma_noise_type_brownian;break;
            default:type=ma_noise_type_white;break;
        }

        ma_noise_config config=ma_noise_config_init(
            ma_format_f32,
            2,
            type,
            0,
            amplitude
        );
        if(ma_noise_init(&config,nullptr,noiseObj)!=MA_SUCCESS){
            throw std::runtime_error("Failed to initialize noise");
        }

        sound=SoundPtr(new ma_sound());
    }

    Sound::Sound(Sound&& other)noexcept
        :source(std::move(other.source))
        ,beginTime(other.beginTime)
        ,sound(std::move(other.sound))
        ,duration(other.duration){
        other.sound.reset();
    }

    Sound& Sound::operator=(Sound&& other)noexcept{
        if(this!=&other){
            this->~Sound();

            source=std::move(other.source);
            beginTime=other.beginTime;
            sound=std::move(other.sound);
            duration=other.duration;

            other.sound.reset();
        }
        return *this;
    }

    bool Sound::setAttribute(float volume,float pitch,float pan){
        if(!sound) return false;
        ma_sound_set_volume(sound.get(),volume);
        ma_sound_set_pitch(sound.get(),pitch);
        ma_sound_set_pan(sound.get(),pan);
        return true;
    }

    Engine::Engine(){
        if(ma_engine_init(nullptr,&engine)!=MA_SUCCESS){
            throw std::runtime_error("Failed to initialize audio engine!");
        }
    }

    Engine::~Engine(){
        storedSounds.clear();
        ma_engine_uninit(&engine);
    }

    Engine& Engine::instance(){
        static Engine instance;
        return instance;
    }

    std::list<Sound>::iterator Engine::playSound(Sound&& sound){
        std::visit([&](auto& src){
            if(!sound.sound){
                sound.sound=SoundPtr(new ma_sound());
            }
            if(ma_sound_init_from_data_source(
                    &engine,
                    reinterpret_cast<ma_data_source*>(&src),
                    0,
                    nullptr,
                    sound.sound.get())!=MA_SUCCESS){
                throw std::runtime_error("Failed to initialize ma_sound from data source");
            }
        },sound.source);

        sound.beginTime=std::chrono::steady_clock::now();

        ma_sound_start(sound.sound.get());

        storedSounds.push_back(std::move(sound));
        return std::prev(storedSounds.end());
    }

    bool Engine::stopSound(std::list<Sound>::iterator it){
        if(it==storedSounds.end()) return false;
        if(it->sound){
            ma_sound_stop(it->sound.get());
        }
        storedSounds.erase(it);
        return true;
    }

    bool Engine::isSoundPlaying(std::list<Sound>::iterator it)const{
        if(it==storedSounds.end()) return false;
        if(!it->sound) return false;
        return ma_sound_is_playing(it->sound.get())!=0;
    }

    size_t Engine::update(){
        size_t removed=0;
        auto it=storedSounds.begin();
        while(it!=storedSounds.end()){
            if(it->soundEnded()){
                it=storedSounds.erase(it);
                removed++;
            }
            else{
                ++it;
            }
        }
        return removed;
    }
}