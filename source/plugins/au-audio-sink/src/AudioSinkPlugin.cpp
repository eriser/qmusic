#include "AudioSinkPlugin.h"
#include "AudioSink.h"

AudioSinkPlugin::AudioSinkPlugin(QObject *pParent)
    : AudioUnitPlugin(pParent)
{
}

QIcon AudioSinkPlugin::icon() const
{
    return QIcon(":/au-audio-sink/icon.png");
}

AudioUnit* AudioSinkPlugin::createInstance()
{
    return new AudioSink(this);
}
