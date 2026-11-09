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
#include "realterminal.h"
#include "terminalstrip.h"
#include "../qetgraphicsitem/terminalelement.h"
#include "physicalterminal.h"
#include "../qetgraphicsitem/conductor.h"
#include "../qetgraphicsitem/terminal.h"

/**
 * @brief RealTerminal
 * @param parent_strip : parent terminal strip
 * @param terminal : terminal element (if any) in a folio
 */
RealTerminal::RealTerminal(Element *terminal) :
	m_element(terminal)
{}

RealTerminal::~RealTerminal()
{
	if (m_physical_terminal) {
		m_physical_terminal->removeTerminal(sharedRef());
	}
}

/**
 * @brief RealTerminal::sharedRef
 * @return a QSharedPointer of this
 */
QSharedPointer<RealTerminal> RealTerminal::sharedRef()
{
	QSharedPointer<RealTerminal> this_shared(this->weakRef());
	if (this_shared.isNull())
	{
		this_shared = QSharedPointer<RealTerminal>(this);
		m_this_weak = this_shared.toWeakRef();
	}

	return this_shared;
}

/**
 * @brief RealTerminal::sharedRef
 * @return a shared reference of this, not that because
 * this method is const, the shared reference can be null if not already
 * used in another part of the code.
 */
QSharedPointer<RealTerminal> RealTerminal::sharedRef() const {
	return QSharedPointer<RealTerminal>(m_this_weak);
}

/**
 * @brief RealTerminal::weakRef
 * @return a QWeakPointer of this, weak pointer can be bull
 */
QWeakPointer<RealTerminal> RealTerminal::weakRef() {
	return m_this_weak;
}

/**
 * @brief toXml
 * @param parent_document
 * @return this real terminal to xml
 */
QDomElement RealTerminal::toXml(QDomDocument &parent_document) const
{
	auto root_elmt = parent_document.createElement(this->xmlTagName());
	if (m_element)
		root_elmt.setAttribute(QStringLiteral("element_uuid"), m_element->uuid().toString());

	return root_elmt;
}

/**
 * @brief RealTerminal::setPhysicalTerminal
 * Set the parent physical terminal of this real terminal
 * @param phy_t
 */
void RealTerminal::setPhysicalTerminal(const QSharedPointer<PhysicalTerminal> &phy_t) {
	m_physical_terminal = phy_t;
}

/**
* @brief parentStrip
* @return parent terminal strip or nullptr
*/
TerminalStrip *RealTerminal::parentStrip() const noexcept {
	if (m_physical_terminal) {
		return m_physical_terminal->terminalStrip();
	} else {
		return nullptr;
	}
}

/**
 * @brief RealTerminal::physicalTerminal
 * @return The parent physical terminal of this terminal.
 * The returned QSharedPointer can be null
 */
QSharedPointer<PhysicalTerminal> RealTerminal::physicalTerminal() const noexcept{
	return m_physical_terminal;
}

/**
 * @brief RealTerminal::level
 * @return
 */
int RealTerminal::level() const
{
	if (m_physical_terminal &&
		sharedRef()) {
		return m_physical_terminal->levelOf(sharedRef());
	}

	return -1;
}

/**
 * @brief label
 * @return the label of this real terminal
 */
QString RealTerminal::label() const {
	if (!m_element.isNull()) {
		return m_element->actualLabel();
	} else {
		return QLatin1String();
	}
}

/**
 * @brief RealTerminal::Xref
 * @return Convenient method to get the XRef
 * formatted to string
 */
QString RealTerminal::Xref() const
{
	if (!m_element.isNull()) {
		return autonum::AssignVariables::genericXref(m_element.data());
	} else {
		return QString();
	}
}

/**
 * @brief RealTerminal::cable
 * @return
 */
QString RealTerminal::cable() const {
	if (m_element) {
		return m_element->elementInformations().value(cableKey()).toString();
	}
	return QString();
}

/**
 * @brief RealTerminal::cableWire
 * @return
 */
QString RealTerminal::cableWire() const {
	if (m_element) {
		return m_element->elementInformations().value(cableWireKey()).toString();
	}
	return QString();
}

/**
 * @brief RealTerminal::cableDestination
 * @return where the cable of this terminal goes (a box, a desk...)
 */
QString RealTerminal::cableDestination() const {
	if (m_element) {
		return m_element->elementInformations().value(cableDestinationKey()).toString();
	}
	return QString();
}

/**
 * @brief RealTerminal::cableType
 * @return the type of the cable of this terminal (4G2,5...)
 */
QString RealTerminal::cableType() const {
	if (m_element) {
		return m_element->elementInformations().value(cableTypeKey()).toString();
	}
	return QString();
}

namespace {
		/**
		 * Return "label:terminal" of the element at the other end of
		 * @a conductor, seen from @a from. A folio report is crossed to reach
		 * the element on the other folio.
		 */
	QString farEnd(Conductor *conductor, Terminal *from, int depth = 0)
	{
		if (!conductor || depth > 3) {
			return QString();
		}
		auto far_terminal{conductor->terminal1 == from ? conductor->terminal2
													  : conductor->terminal1};
		if (!far_terminal) {
			return QString();
		}
		auto far_element{far_terminal->parentElement()};
		if (!far_element) {
			return QString();
		}

			//Cross the folio report
		if (far_element->linkType() & Element::AllReport)
		{
			const auto linked_{far_element->linkedElements()};
			if (!linked_.isEmpty())
			{
				auto other_report{linked_.first()};
				for (const auto &t : other_report->terminals()) {
					for (const auto &c : t->conductors()) {
						const auto str_{farEnd(c, t, depth + 1)};
						if (!str_.isEmpty()) {
							return str_;
						}
					}
				}
			}
			return QString();
		}

		auto label_{far_element->actualLabel()};
		if (label_.isEmpty()) {
			label_ = far_element->name();
		}
		const auto terminal_name{far_terminal->name()};
		if (terminal_name.isEmpty()) {
			return label_;
		}
		return label_ + QLatin1Char(':') + terminal_name;
	}
}

/**
 * @brief RealTerminal::sideTerminal
 * @param bottom : true for the field side, false for the cabinet side
 * @return the connection point of the terminal element the lowest on the
 * folio (the rightmost one at the same height) if @a bottom is true, the
 * highest one (the leftmost one at the same height) otherwise.
 * Return nullptr if the element has less than two connection points and
 * @a bottom is false, so a single point is only seen from the bottom.
 */
Terminal *RealTerminal::sideTerminal(bool bottom) const
{
	if (!m_element) {
		return nullptr;
	}
	const auto terminals_{m_element->terminals()};
	if (terminals_.isEmpty() || (!bottom && terminals_.size() < 2)) {
		return nullptr;
	}

	Terminal *best_terminal{nullptr};
	for (const auto &t : terminals_)
	{
		if (!best_terminal) {
			best_terminal = t;
			continue;
		}
		const auto p{t->dockConductor()};
		const auto best{best_terminal->dockConductor()};
		const bool lower_{p.y() > best.y() + 0.5 ||
						  (qAbs(p.y() - best.y()) <= 0.5 && p.x() > best.x())};
		if (lower_ == bottom) {
			best_terminal = t;
		}
	}
	return best_terminal;
}

/**
 * @brief RealTerminal::arrival
 * @return the element (and its terminal) reached by the wire of the
 * field side (bottom) of this terminal, for example "-12M2:U".
 */
QString RealTerminal::arrival() const {
	return arrivalOnSide(true);
}

/**
 * @brief RealTerminal::arrivalTop
 * @return the element reached by the wire of the cabinet side (top)
 */
QString RealTerminal::arrivalTop() const {
	return arrivalOnSide(false);
}

QString RealTerminal::arrivalOnSide(bool bottom) const
{
	const auto side_terminal{sideTerminal(bottom)};
	if (!side_terminal) {
		return QString();
	}
	for (const auto &c : side_terminal->conductors())
	{
		const auto str_{farEnd(c, side_terminal)};
		if (!str_.isEmpty()) {
			return str_;
		}
	}
	return QString();
}

/**
 * @brief RealTerminal::conductorOnSide
 * @return the number of the wire connected on the bottom (field) side
 * or on the top (cabinet) side of the terminal. They can be different
 * for a fuse or a sectional terminal.
 */
QString RealTerminal::conductorOnSide(bool bottom) const
{
	const auto side_terminal{sideTerminal(bottom)};
	if (!side_terminal) {
		return QString();
	}
	for (const auto &c : side_terminal->conductors()) {
		const auto text_{c->properties().text};
		if (!text_.isEmpty()) {
			return text_;
		}
	}
	return QString();
}

/**
 * @brief RealTerminal::conductor
 * @return
 */
QString RealTerminal::conductor() const {
	if (m_element)
	{
		const auto conductors_{m_element->conductors()};
		if (conductors_.size()) {
			return conductors_.first()->properties().text;
		}
	}
	return QString();
}

/**
 * @brief RealTerminal::type
 * @return
 */
ElementData::TerminalType RealTerminal::type() const {
	if (m_element) {
		return m_element->elementData().terminalType();
	} else {
		return ElementData::TTGeneric;
	}
}

/**
 * @brief RealTerminal::function
 * @return
 */
ElementData::TerminalFunction RealTerminal::function() const {
	if (m_element) {
		return m_element->elementData().terminalFunction();
	} else {
		return ElementData::TFGeneric;
	}
}

/**
 * @brief RealTerminal::isLed
 * @return
 */
bool RealTerminal::isLed() const {
	if (m_element) {
		return m_element->elementData().terminalLed();
	} else {
		return false;
	}
}

/**
 * @brief isElement
 * @return true if this real terminal is linked to a terminal element
 */
bool RealTerminal::isElement() const {
	return m_element.isNull() ? false : true;
}

/**
 * @brief RealTerminal::isBridged
 * @return true if is bridged.
 * @sa TerminalStrip::isBridged
 */
bool RealTerminal::isBridged() const
{
	if (parentStrip()) {
		return !parentStrip()->isBridged(m_this_weak.toStrongRef()).isNull();
	} else {
		return false;
	}
}

/**
 * @brief RealTerminal::bridge
 * @return
 */
QSharedPointer<TerminalStripBridge> RealTerminal::bridge() const
{
	if (parentStrip()) {
		return parentStrip()->isBridged(m_this_weak.toStrongRef());
	} else {
		return QSharedPointer<TerminalStripBridge>();
	}
}

/**
 * @brief element
 * @return the element linked to this real terminal
 * or nullptr if not linked to an Element.
 */
Element *RealTerminal::element() const {
	return m_element.data();
}

/**
 * @brief elementUuid
 * @return if this real terminal is an element
 * in a folio, return the uuid of the element
 * else return a null uuid.
 */
QUuid RealTerminal::elementUuid() const {
	if (!m_element.isNull()) {
		return m_element->uuid();
	} else {
		return QUuid();
	}
}

/**
 * @brief RealTerminal::RealTerminal::xmlTagName
 * @return
 */
QString RealTerminal::RealTerminal::xmlTagName() {
	return QStringLiteral("real_terminal");
}
