/**
 * File name: general_envelope.h
 * Project: Geonkick (A kick synthesizer)
 *
 * Copyright (C) 2017 Iurie Nistor (http://geontime.com)
 *
 * This file is part of Geonkick.
 *
 * GeonKick is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include "general_envelope.h"
#include "oscillator.h"

GeneralEnvelope::GeneralEnvelope(GeonkickApi *api, const RkRect &area)
        : Envelope(nullptr, area),
          geonkickApi(api)
{
        removeSupportedType(Envelope::Type::Frequency);
        connect(geonkickApi, SIGNAL(kickLengthUpdated(double)), this, SIGNAL(envelopeUpdated()));
        connect(geonkickApi, SIGNAL(kickAmplitudeUpdated(double)), this, SIGNAL(envelopeUpdated()));
        setType(Envelope::Type::Amplitude);
        setPoints(geonkickApi->getKickEnvelopePoints());
}

GeneralEnvelope::~GeneralEnvelope()
{
}

void GeneralEnvelope::pointAddedEvent(double x, double y)
{
        geonkickApi->addKickEnvelopePoint(x, y);
}

void GeneralEnvelope::pointUpdatedEvent(unsigned int index, double x, double y)
{
        geonkickApi->updateKickEnvelopePoint(index, x, y);
}

void GeneralEnvelope::pointRemovedEvent(unsigned int index)
{
        geonkickApi->removeKickEnvelopePoint(index);
}

double GeneralEnvelope::envelopeLengh(void) const
{
        return geonkickApi->kickLength();
}

void GeneralEnvelope::setEnvelopeLengh(double len)
{
        geonkickApi->setKickLength(len);
        emit envelopeLengthUpdated(len);
}

double GeneralEnvelope::envelopeAmplitude(void) const
{
        return geonkickApi->kickAmplitude();
}

void GeneralEnvelope::updatePoints()
{
        setPoints(geonkickApi->getKickEnvelopePoints());
}
