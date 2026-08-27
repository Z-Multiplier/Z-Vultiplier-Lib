//MIT License

//Copyright (c) 2026 Z-Multiplier
#ifndef AUDIO_HPP
#define AUDIO_HPP

#include "miniaudio.h"
#include <variant>
#include <string>
#include <chrono>
#include <list>

namespace Audio{
    enum class WaveForm{
        SINE,
        SQUARE,
        TRIANGLE,
        SAWTOOTH
    };
    enum class NoiseColor{
        WHITE,
        PINK,
        BROWN
    };
    struct SoundDeleter{
        void operator()(ma_sound* ptr)const{
            if(ptr){
                ma_sound_uninit(ptr);
                delete ptr;
            }
        }
    };
    using SoundPtr=std::unique_ptr<ma_sound,SoundDeleter>;
    struct Sound{
        std::variant<ma_decoder,ma_waveform,ma_noise> source;
        std::chrono::time_point<std::chrono::steady_clock> beginTime;
        SoundPtr sound;
        long double duration;
        Sound(std::string filePath);
        Sound(WaveForm wave,float hz,float amplitude,long double duration);
        Sound(NoiseColor noise,float amplitude,long double duration);
        ~Sound()=default;

        Sound(const Sound&)=delete;
        Sound& operator=(const Sound&)=delete;
        Sound(Sound&&)noexcept;
        Sound& operator=(Sound&&)noexcept;

        bool soundEnded(){
            auto now=std::chrono::steady_clock::now();
            auto elapsed=std::chrono::duration<long double>(now-beginTime).count();
            return elapsed>=duration;
        }

        bool setAttribute(float volume,float pitch,float pan);
    };
    class Engine{
        public:
            static Engine& instance();

            Engine(const Engine&)=delete;
            Engine& operator=(const Engine&)=delete;
            Engine(Engine&&)=delete;
            Engine& operator=(Engine&&)=delete;

            std::list<Sound>::iterator playSound(Sound&& sound);
            bool stopSound(std::list<Sound>::iterator index);
            bool isSoundPlaying(std::list<Sound>::iterator index)const;
            size_t update();

        private:
            Engine();
            ~Engine();

            ma_engine engine;
            std::list<Sound> storedSounds;
    };
}

#endif