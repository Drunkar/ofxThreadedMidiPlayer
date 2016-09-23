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


class ofxThreadedMidiPlayer: public ofThread {
public:
    int count;
    bool isReady;
    string midiFileName;
    int midiPort;
    float currentTime;
    float nextEventTime;

    double musicDurationInSeconds;
    float maxTime;
    long myTime;
    bool doLoop;

    jdksmidi::MIDIMultiTrack *tracks;
    jdksmidi::MIDISequencer *sequencer;
    jdksmidi::MIDITimedBigMessage lastTimedBigMessage;
    RtMidiOut *midiOut;
    ofxMidiEvent midiEvent;

    ofxThreadedMidiPlayer();
    ~ofxThreadedMidiPlayer();
    void start();
    void stop();
    void clean();
    void threadedFunction();
    void setup(string fileName, int portNumber, bool shouldLoop = true);
    void DumpMIDITimedBigMessage( const jdksmidi::MIDITimedBigMessage& msg );
    void setCurrentTimeMs(float currentTime);

protected:
    bool isInited;
    float updatedCurrentTime;
    void init();
    void dispatchMidiEvent(float currentTime, float timeDelta, vector<unsigned char>& message);
    void updateCurrentTime();
    void correctMessage(vector<unsigned char>& message);
    bool isValid(vector<unsigned char>& message);
};
