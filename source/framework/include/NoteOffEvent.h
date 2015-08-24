/*
                          qmusic

    Copyright (C) 2015 Arthur Benilov,
    arthur.benilov@gmail.com

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This software is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
    Lesser General Public License for more details.
*/

#ifndef NOTEOFFEVENT_H
#define NOTEOFFEVENT_H

#include "FrameworkApi.h"
#include "SignalChainEvent.h"

class QMUSIC_FRAMEWORK_API NoteOffEvent : public SignalChainEvent
{
public:
    NoteOffEvent(int number = -1, int velocity = 0);
    NoteOffEvent(const NoteOffEvent &evt);
    NoteOffEvent& operator =(const NoteOffEvent &evt);
    SignalChainEvent* clone() const;

    QString toString() const;

    int noteNumber() const { return m_number; }
    int velocity() const { return m_velocity; }
    float frequency() const;
    float normalizedVelocity() const;

private:
    int m_number;       ///< Note number.
    int m_velocity;     ///< Note velocity (amplitude).
};

#endif // NOTEOFFEVENT_H
