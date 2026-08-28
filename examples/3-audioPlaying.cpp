#include "Z-Vultiplier.hpp"
int main(){
    auto& engine=Audio::Engine::instance();

    auto it1=engine.playSound(Audio::Sound("explosion.wav"));
    
    auto it2=engine.playSound(Audio::Sound(
        Audio::WaveForm::SINE,440.0f,0.3f,2.0
    ));
    
    auto it3=engine.playSound(Audio::Sound(
        Audio::NoiseColor::WHITE,0.2f,1.0
    ));
    
    while(true){
        engine.update();
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
    return 0;
}