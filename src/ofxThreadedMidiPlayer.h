#pragma once

#include "ofMain.h"
#include <jdksmidi/world.h>
#include <jdksmidi/track.h>
#include <jdksmidi/multitrack.h>
#include <jdksmidi/filereadmultitrack.h>
#include <jdksmidi/fileread.h>
#include <jdksmidi/fileshow.h>
#include <jdksmidi/filewritemultitrack.h>
#include <jdksmidi/sequencer.h>
#include "ofxMidi.h"


class ofxThreadedMidiPlayer: public ofThread{
public:
    int count;
    bool isReady;
    string midiFileName;
    int midiPort;
    float currentTime;
    float nextEventTime;

    double musicDurationInSeconds;
    float max_time;
    long myTime;
    bool doLoop;

    jdksmidi::MIDIMultiTrack *tracks;
    jdksmidi::MIDISequencer *sequencer;
    jdksmidi::MIDITimedBigMessage lastTimedBigMessage;

    RtMidiOut *midiout;

    ofxThreadedMidiPlayer();
    ~ofxThreadedMidiPlayer();
    void stop();
    void DumpMIDITimedBigMessage( const jdksmidi::MIDITimedBigMessage& msg );
    void start();

    void setup(string fileName, int portNumber, bool shouldLoop = true);
    void threadedFunction();
    void clean();

    ofxMidiEvent midiEvent;

protected:
    void dispatchMidiEvent(float currentTime, float timeDelta, vector<unsigned char>& message);
    bool bIsInited;

    void init();
};