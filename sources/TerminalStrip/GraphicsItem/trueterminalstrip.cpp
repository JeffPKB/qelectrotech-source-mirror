/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.
	
	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.
	
	QElectroTech is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.
	
	You should have received a copy of the GNU General Public License
	along with QElectroTech.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "trueterminalstrip.h"
#include "../physicalterminal.h"
#include "../realterminal.h"
#include "../terminalstrip.h"
#include "../terminalstripbridge.h"
#include "../../autoNum/assignvariables.h"

#include "terminalstripdrawer.h"

namespace TerminalStripDrawer
{
	/**
	 * @brief TrueTerminalStrip::TrueTerminalStrip
	 * Constructor, this class don't take ownership of @a strip
	 * @param strip
	 */
	TrueTerminalStrip::TrueTerminalStrip(TerminalStrip *strip) :
		m_strip { strip }
	{}


	QString TrueTerminalStrip::installation() const
	{
		if (m_strip) {
			return m_strip->installation();
		} else {
			return QString();
		}
	}

	QString TrueTerminalStrip::location() const
	{
		if (m_strip) {
			return m_strip->location();
		} else {
			return QString();
		}
	}

	QString TrueTerminalStrip::name() const
	{
		if (m_strip) {
			return m_strip->name();
		} else {
			return QString();
		}
	}

	QVector<QSharedPointer<AbstractPhysicalTerminalInterface>> TrueTerminalStrip::physicalTerminal() const
	{
		QVector<QSharedPointer<AbstractPhysicalTerminalInterface>> vector_;
		if (m_strip) {
			for (const auto &phy : m_strip->physicalTerminal()) {
				vector_.append(QSharedPointer<AbstractPhysicalTerminalInterface>{ new TruePhysicalTerminal(phy) });
			}
		}

		return vector_;
	}

	TruePhysicalTerminal::TruePhysicalTerminal(QSharedPointer<PhysicalTerminal> physical) :
		m_physical { physical }
	{}

	QVector<QSharedPointer<AbstractRealTerminalInterface>> TruePhysicalTerminal::realTerminals() const
	{
		QVector<QSharedPointer<AbstractRealTerminalInterface>> vector_;
		if (m_physical) {
			for (const auto &real_ : m_physical->realTerminals()) {
				vector_.append(QSharedPointer<AbstractRealTerminalInterface> { new TrueRealTerminal{ real_ }});
			}
		}

		return vector_;
	}

	TrueRealTerminal::TrueRealTerminal(QSharedPointer<RealTerminal> real) :
		m_real { real }
	{}

	QString TrueRealTerminal::label() const
	{
		if (m_real) {
			return m_real->label();
		} else {
			return QString();
		}
	}

	bool TrueRealTerminal::isBridged() const
	{
		if (m_real) {
			return m_real->isBridged();
		} else {
			return false;
		}
	}

		//Return a raw pointer, the pointer is not managed by this class
	AbstractBridgeInterface* TrueRealTerminal::bridge() const
	{
		return new TrueBridge(m_real->bridge());
	}

	QString TrueRealTerminal::xref() const
	{
		if (m_real && m_real->isElement())
		{
			auto xref_{autonum::AssignVariables::genericXref(m_real->element())};
				//The folio of the title block is often "%id/%total" (1/2):
				//keep only the number of the page, "1/2-E2" become "1-E2"
			const auto dash_{xref_.lastIndexOf(QLatin1Char('-'))};
			const auto slash_{xref_.indexOf(QLatin1Char('/'))};
			if (slash_ > 0 && dash_ > slash_) {
				xref_.remove(slash_, dash_ - slash_);
			}
			return xref_;
		} else {
			return QString{};
		}
	}

	QString TrueRealTerminal::conductor() const
	{
			//The wire number written under the strip is the one of the
			//bottom side (it can be different from the top one for a
			//fuse or a sectional terminal)
		return m_real ? m_real->conductorOnSide(true) : QString{};
	}

	QString TrueRealTerminal::conductorTop() const
	{
		return m_real ? m_real->conductorOnSide(false) : QString{};
	}

	QString TrueRealTerminal::arrivalTop() const
	{
			//The element reached by the wire is not written on the strip
			//any more (its name is too long): the wire number and the
			//position of the terminal are enough. Still in the editor table.
		return QString{};
	}

	QString TrueRealTerminal::cable() const
	{
		return m_real ? m_real->cable() : QString{};
	}

	QString TrueRealTerminal::cableWire() const
	{
		return m_real ? m_real->cableWire() : QString{};
	}

	QString TrueRealTerminal::cableDestination() const
	{
		return m_real ? m_real->cableDestination() : QString{};
	}

	QString TrueRealTerminal::arrival() const
	{
			//The element reached by the wire is not written on the strip
			//any more (its name is too long): the wire number and the
			//position of the terminal are enough. Still in the editor table.
		return QString{};
	}

	QString TrueRealTerminal::cableType() const
	{
		return m_real ? m_real->cableType() : QString{};
	}

	TerminalSymbol TrueRealTerminal::symbol() const
	{
		if (!m_real) {
			return TerminalSymbol::Generic;
		}
		switch (m_real->type()) {
			case ElementData::TTFuse:      return TerminalSymbol::Fuse;
			case ElementData::TTSectional: return TerminalSymbol::Sectional;
			case ElementData::TTDiode:     return TerminalSymbol::Diode;
			case ElementData::TTGround:    return TerminalSymbol::Ground;
			default:                       return TerminalSymbol::Generic;
		}
	}

    TrueBridge::TrueBridge(QSharedPointer<TerminalStripBridge> bridge) :
        m_bridge { bridge }
    {}

	QUuid TrueBridge::uuid() const
	{
		if (m_bridge) {
			return m_bridge->uuid();
		} else {
			return QUuid();
		}
	}
}
