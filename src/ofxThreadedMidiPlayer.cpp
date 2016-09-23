#include "ofxThreadedMidiPlayer.h"


ofxThreadedMidiPlayer::ofxThreadedMidiPlayer() {
    count = 0;
    midiFileName = "";
    midiPort = 0;
    currentTime = 0.0;
    nextEventTime = 0.0;

    musicDurationInSeconds = 0;
    maxTime = 0;
    myTime = 0;
    //        lastTimedBigMessage = new MIDITimedBigMessage();
    tracks = NULL;
    sequencer = NULL;
    midiOut = NULL;
    doLoop = true;
    isReady = true;
    isInited = false;
}


ofxThreadedMidiPlayer::~ofxThreadedMidiPlayer() {
    ofLogVerbose("ofxThreadedMidiPlayer::~ofxThreadedMidiPlayer") << "Function Called.";
    stop();
    clean();
}


void ofxThreadedMidiPlayer::start() {
    startThread();
}


void ofxThreadedMidiPlayer::stop() {
    ofLogVerbose("ofxThreadedMidiPlayer::stop") << "Function Called.";
    stopThread();
    waitForThread();
    // clean();

}


void ofxThreadedMidiPlayer::DumpMIDITimedBigMessage( const jdksmidi::MIDITimedBigMessage& msg ) {
    char msgbuf[1024];
    lastTimedBigMessage.Copy(msg);

    // note that Sequencer generate SERVICE_BEAT_MARKER in files dump,
    // but files themselves not contain this meta event...
    // see MIDISequencer::beat_marker_msg.SetBeatMarker()
    /*
    if ( msg.IsBeatMarker() )
    {
        ofLog(OF_LOG_VERBOSE, "%8ld : %s <------------------>", msg.GetTime(), msg.MsgToText ( msgbuf ) );
    }
    else
    {
        ofLog(OF_LOG_VERBOSE, "%8ld : %s", msg.GetTime(), msg.MsgToText ( msgbuf ) );
    }

    if ( msg.IsSystemExclusive() )
    {
        ofLog(OF_LOG_VERBOSE, "SYSEX length: %d", msg.GetSysEx()->GetLengthSE() );
    }
    //*/
}


void ofxThreadedMidiPlayer::setup(string fileName, int portNumber, bool shouldLoop) {
    midiFileName = fileName;
    midiPort = portNumber;
    doLoop = shouldLoop;
    clean();
}


void ofxThreadedMidiPlayer::dispatchMidiEvent( float currentTime, float timeDelta, vector<unsigned char>& message) {
    ofxMidiMessage midiMessage(&message);

    if ((message.at(0)) >= MIDI_SYSEX) {
        midiMessage.status = (MidiStatus)(message.at(0) & 0xFF);
        midiMessage.channel = 0;
    } else {
        midiMessage.status = (MidiStatus) (message.at(0) & 0xF0);
        midiMessage.channel = (int) (message.at(0) & 0x0F) + 1;
    }

    midiMessage.deltatime = timeDelta;// deltatime;// * 1000; // convert s to ms
    midiMessage.portNum = midiPort;
    // midiMessage.portName = portName;

    switch (midiMessage.status) {
    case MIDI_NOTE_ON :
    case MIDI_NOTE_OFF:
        midiMessage.pitch = (int) message.at(1);
        midiMessage.velocity = (int) message.at(2);
        break;
    case MIDI_CONTROL_CHANGE:
        midiMessage.control = (int) message.at(1);
        midiMessage.value = (int) message.at(2);
        break;
    case MIDI_PROGRAM_CHANGE:
    case MIDI_AFTERTOUCH:
        midiMessage.value = (int) message.at(1);
        break;
    case MIDI_PITCH_BEND:
        midiMessage.value = (int) (message.at(2) << 7) +
                            (int) message.at(1); // msb + lsb
        break;
    case MIDI_POLY_AFTERTOUCH:
        midiMessage.pitch = (int) message.at(1);
        midiMessage.value = (int) message.at(2);
        break;
    default:
        break;
    }

    ofNotifyEvent(midiEvent, midiMessage, this);

}


void ofxThreadedMidiPlayer::correctMessage(vector<unsigned char> &message) {
    vector<unsigned char> correctMessage;
    
    // reset message or meta event
    if (message[0] == 0xFF) {
        correctMessage.push_back(0xFF);
    } else {

        for (unsigned char c : message) {
            if (c == 0x04) {
                if (correctMessage.size() > 0)
                    correctMessage.pop_back();
                break;
            }
            correctMessage.push_back(c);
        }

    }
    message = correctMessage;
}


bool ofxThreadedMidiPlayer::isValid(vector<unsigned char> &message) {
    for (unsigned char c : message) {
        if (c == 0x04) return false;
    }
    return true;
}


void ofxThreadedMidiPlayer::threadedFunction() {
    while (isThreadRunning()) {

        init();

        myTime = 0;

        jdksmidi::MIDITimedBigMessage event;
        int eventTrack = 0;

        for ( ; currentTime < maxTime && isThreadRunning(); currentTime += 10.) {
            // find all events that came before or at the current time
            updateCurrentTime();
            while ( nextEventTime <= currentTime ) {
                
                // wait myTime reach currentTime ------------------------------
                myTime++;
                // manage read speed by sleep
                if (myTime < currentTime) {
                    sleep(1);
                    continue;
                }
                // ------------------------------------------------------------
                
                // myTime > currentTime
                // find all events that came before or at the current time
                if (sequencer) {
                    if ( sequencer->GetNextEvent ( &eventTrack, &event ) ) {

                        ofLog ( OF_LOG_VERBOSE,
                              "currentTime=%06.0f : nextEventTime=%06.0f : eventTrack=%02d",
                            currentTime, currentTime, eventTrack );
                        jdksmidi::MIDITimedBigMessage *msg = &event;
                        if (msg->GetLength() > 0) {
                            vector<unsigned char> message;
                            message.push_back(msg->GetStatus());
                            if (msg->GetLength() > 0) message.push_back(msg->GetByte1());
                            if (msg->GetLength() > 1) message.push_back(msg->GetByte2());
                            if (msg->GetLength() > 2) message.push_back(msg->GetByte3());
                            if (msg->GetLength() > 3) message.push_back(msg->GetByte4());
                            if (msg->GetLength() > 4) message.push_back(msg->GetByte5());
                            message.resize(msg->GetLength());
                            correctMessage(message);
                            
                            if (isValid(message)) {
                                midiOut->sendMessage(&message);
                                dispatchMidiEvent(currentTime, currentTime - nextEventTime, message);
                                DumpMIDITimedBigMessage ( event );
                            }
                        }
                        if ( !sequencer->GetNextEventTimeMs ( &nextEventTime ) ) {
                            ofLogVerbose("NO MORE EVENTS FOR SEQUENCE, LAST TIME CHECKED IS: " ,   ofToString(nextEventTime) );
                        }
                    }
                }
            }
        }
        count++;

        if (doLoop) {
            isReady = true;
        } else {
            isReady = false;
        }
    }
}


/* Update currentTime if updatedCurrentTime is differ from currentTime. */
void ofxThreadedMidiPlayer::updateCurrentTime() {
    if (abs(currentTime - updatedCurrentTime) >= 1000) {
        sequencer->GoToZero();
        myTime = updatedCurrentTime;
        currentTime = updatedCurrentTime;
        nextEventTime = updatedCurrentTime;
        sequencer->GoToTimeMs(updatedCurrentTime);
    }
}


void ofxThreadedMidiPlayer::clean() {
    ofLogVerbose("ofxThreadedMidiPlayer::clean") << "Function Called.";
    if (tracks) {
        delete tracks;
        tracks = NULL;
    }
    if (sequencer) {
        delete sequencer;
        sequencer = NULL;
    }
    //        if(lastTimedBigMessage) {
    //            delete lastTimedBigMessage;
    //            lastTimedBigMessage = NULL;
    //        }
    if (midiOut) {
        delete midiOut;
        midiOut = NULL;
    }
    isInited = false;
}


void ofxThreadedMidiPlayer::init() {
    if (!isInited) {
        isReady = false;
        string filePath = ofToDataPath(midiFileName, true);

        jdksmidi::MIDIFileReadStreamFile rs ( filePath.c_str() );

        if ( !rs.IsValid() ) {
            ofLogError( "ERROR OPENING FILE AT: ",  filePath);

        }

        tracks = new jdksmidi::MIDIMultiTrack();
        jdksmidi::MIDIFileReadMultiTrack track_loader ( tracks );
        jdksmidi::MIDIFileRead reader ( &rs, &track_loader );

        int numMidiTracks = reader.ReadNumTracks();
        int midiFormat = reader.GetFormat();

        tracks->ClearAndResize( numMidiTracks );
        ofLogVerbose("ofxThreadedMidiPlayer::init") << "numMidiTracks: " << numMidiTracks;
        ofLogVerbose("ofxThreadedMidiPlayer::init") << "midiFormat: " << midiFormat;

        if ( reader.Parse() ) {
            ofLogVerbose("ofxThreadedMidiPlayer::init") << "reader parsed!: ";
        }

        //MIDISequencer seq( &tracks );
        sequencer = new jdksmidi::MIDISequencer ( tracks );//&seq;
        musicDurationInSeconds = sequencer->GetMusicDurationInSeconds();

        ofLogVerbose( "musicDurationInSeconds is ", ofToString(musicDurationInSeconds));

        midiOut = new RtMidiOut();
        if (midiOut->getPortCount()) {
            midiOut->openPort(midiPort);
            ofLogVerbose("Using Port name: " ,   ofToString(midiOut->getPortName(0)) );
        }

        currentTime = 0.0;
        nextEventTime = 0.0;
        updatedCurrentTime = 0.0;
        if (!sequencer->GoToTimeMs ( currentTime )) {
            ofLogError("Couldn't go to time in sequence: " ,   ofToString(currentTime) );
        }
        if ( !sequencer->GetNextEventTimeMs ( &nextEventTime ) ) {
            ofLogVerbose("No next events for sequence", ofToString(nextEventTime));
        }
        maxTime = (musicDurationInSeconds * 1000.);
        isInited = true;
    }
}


void ofxThreadedMidiPlayer::setCurrentTimeMs(float new_current_time) {
    updatedCurrentTime = new_current_time;
}
