//
//  wide
//
//  Created by @pd3v_
//

#include "instrument.h"
#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <RtMidi.h>

using namespace std::chrono;

//short Instrument::syncOffset = 0; // microseconds

Instrument::Instrument(std::string _id, int _ch)
:id(_id), ch(_ch), midiout(std::unique_ptr<RtMidiOut>(new RtMidiOut)), _generator(Generator()) {
  midiout->openPort(0);
  
  noteMessage.push_back(0);
  noteMessage.push_back(0);
  noteMessage.push_back(0);
  
  //cc
  /*ccMessage.push_back(0);
   ccMessage.push_back(0);
   ccMessage.push_back(0);
  */
  //playIt();
};

Instrument::~Instrument(){
  std::cout << "instrument " << id << " quit." << std::endl;
  noteMessage[0] = 128+ch;
  midiout->sendMessage(&noteMessage);
};

void Instrument::bpm(int bpm) {
  _generator.bpm(bpm);
}

std::vector<int> Instrument::scale() {
  return _generator.scale();
}

void Instrument::scale(std::vector<int> scale) {
  _generator.scale(scale);
}

int Instrument::scaleSize() {
  return (int)_generator.scale().size();
}

unsigned long Instrument::step() {
  return _generator.step;
}

void Instrument::playbar(std::function<Notes(void)> f) {
  std::thread t([&](){
    while (true) {
      if (stepTimer.bar_start()) {
        _generator.f(id,f);
        metaNotes = _generator.metaNotes; // notes before octaves calculus
        break;
      }
    }
  });
  t.detach();
}

void Instrument::play(std::function<Notes(void)> f) {
  _generator.f(id,f);
  //metaNotes = _generator.metaNotes;
}

void Instrument::cc(std::vector<cc_t> ccs) {
  for(auto& cc : ccs)
    std::cout << cc.ch << " " << &cc.f << " " << cc.f() << std::endl;
  
  _generator.cc(ccs);
}

void Instrument::cc() {
  std::vector<cc_t> ccVct {};
  _generator.cc(ccVct);
}

void Instrument::mute() {
  isMuted = true;
}

void Instrument::unmute() {
  isMuted = false;
}

void Instrument::playIt() {
//  Notes tskPoolNote;
  
//  while (true) {
//    Generator::tskPool.mtx.lock();
//    tskPoolNote = Generator::tskPool.jobs.front();
//    Generator::tskPool.jobs.pop_front();
//    Generator::tskPool.mtx.unlock();
//    std::cout << "pop from Jobs:" << tskPoolNote.notes[0] << std::endl;
    
    _startTime = time_point_cast<nanoseconds>(steady_clock::now()).time_since_epoch().count()-(Instrument::syncOffset*1000);
    _future = std::async(std::launch::async,[&](){
      playing = true;
      
      // Notes off
      for (auto& pitch : n.notes) {
        noteMessage[0] = 128+ch ;
        noteMessage[1] = pitch;
        noteMessage[2] = 0;
        midiout->sendMessage(&noteMessage);
      }
      
      n = _generator.notes();
//      n = _generator.notesTskPool();
      
      for (auto& pitch : n.notes) {
        noteMessage[0] = 144+ch;
        noteMessage[1] = pitch;
        noteMessage[2] = isMuted == true ? 0 : n.amp;
        midiout->sendMessage(&noteMessage);
      }
      
      // cc
      for (auto &cc : _generator.ccVct) {
       _ccCompute = _generator.midicc(cc);
       ccMessage[0] = 176+ch;
       ccMessage[1] = _ccCompute[0];
       ccMessage[2] = _ccCompute[1];
       midiout->sendMessage(&ccMessage);
      }
     
      _elapsedTime = time_point_cast<nanoseconds>(steady_clock::now()).time_since_epoch().count();
      
      std::this_thread::sleep_for(nanoseconds(n.dur-(_elapsedTime-_startTime)));
      
      playing = false;
    });
  //}
}