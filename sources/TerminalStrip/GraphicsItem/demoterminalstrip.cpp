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
#include "demoterminalstrip.h"

namespace TerminalStripDrawer
{

/*========= DemoBridge =========*/
	class DemoBridge : public AbstractBridgeInterface
	{
		public:
			DemoBridge(const QUuid &uuid) :
				m_uuid { uuid } {}

			QUuid uuid() const override {
				return m_uuid;
			}

		private:
			const QUuid m_uuid;
	};

    class DemoRealTerminal : public AbstractRealTerminalInterface
    {
        public:
			DemoRealTerminal(const QString &label, const QString &xref, const QUuid &bridge) :
				m_label { label },
				m_xref{ xref },
				m_bridge { bridge }
            {}

			QString label() const override {
				return m_label;
			}

			bool isBridged() const override {
				return true;
			}

			DemoBridge *bridge() const override {
				return new DemoBridge { m_bridge };
			}

			QString xref() const override {
				return m_xref;
			}

			QString cable() const override {
				if (m_xref.startsWith(QLatin1Char('1'))) return QStringLiteral("-12M2");
				if (m_xref.startsWith(QLatin1Char('2'))) return QStringLiteral("-76-AU");
				return QString();
			}

			QString cableDestination() const override {
				if (m_xref.startsWith(QLatin1Char('1'))) return QStringLiteral("GH");
				if (m_xref.startsWith(QLatin1Char('2'))) return QStringLiteral("PUPITRE");
				return QString();
			}

			QString cableWire() const override {
				static const QStringList colors{QStringLiteral("BN"), QStringLiteral("BK"),
												QStringLiteral("GY"), QStringLiteral("GNYE")};
				const auto n{m_xref.right(1).toInt()};
				if (m_xref.startsWith(QLatin1Char('1'))) return colors.value(n - 1);
				if (m_xref.startsWith(QLatin1Char('2'))) return QString::number(n);
				return QString();
			}

			QString arrival() const override {
				if (m_xref.startsWith(QLatin1Char('1'))) {
					static const QStringList pins{QStringLiteral("U"), QStringLiteral("V"),
												  QStringLiteral("W"), QStringLiteral("PE")};
					return QStringLiteral("-12M2:") + pins.value(m_xref.right(1).toInt() - 1);
				}
				if (m_xref.startsWith(QLatin1Char('2'))) return QStringLiteral("X-AU:") + m_xref.right(1);
				return QString();
			}

			QString conductor() const override {
				return QStringLiteral("W") + m_xref.right(1) + QStringLiteral("12M2");
			}

        private:
			QString m_label, m_xref;
            QUuid m_bridge;
    };

	class DemoPhysicalTerminal : public AbstractPhysicalTerminalInterface
	{
		public:
			DemoPhysicalTerminal(QVector<QSharedPointer<AbstractRealTerminalInterface>> real_terminals) :
				m_real_terminals { real_terminals}
			{}

			QVector<QSharedPointer<AbstractRealTerminalInterface>> realTerminals() const override {
				return m_real_terminals;
			}

		private:
			QVector<QSharedPointer<AbstractRealTerminalInterface>> m_real_terminals;
	};



/*========= DemoTerminalStrip =========*/

	/**
	 * @brief DemoTerminalStrip::DemoTerminalStrip
	 */
	DemoTerminalStrip::DemoTerminalStrip()
	{
		build();
	}

	QVector<QSharedPointer<AbstractPhysicalTerminalInterface> > DemoTerminalStrip::physicalTerminal() const
	{
		return m_physical_terminal;
	}

	void DemoTerminalStrip::build()
	{
		QUuid lvl_1 = QUuid::createUuid();
		QUuid lvl_2 = QUuid::createUuid();
		QUuid lvl_3 = QUuid::createUuid();
		QUuid lvl_4 = QUuid::createUuid();

		QVector <QSharedPointer<AbstractRealTerminalInterface>> real_terminals_vector;

		real_terminals_vector << QSharedPointer<AbstractRealTerminalInterface> {
																			   new DemoRealTerminal( QStringLiteral("24vdc"),
																									QStringLiteral("1_A1"),
																									lvl_1)};
		real_terminals_vector << QSharedPointer<AbstractRealTerminalInterface> {
																			   new DemoRealTerminal( QStringLiteral("0vdc"),
																									QStringLiteral("1_A2"),
																									lvl_2)};
		real_terminals_vector << QSharedPointer<AbstractRealTerminalInterface> {
																			   new DemoRealTerminal( QStringLiteral("signal"),
																									QStringLiteral("1_A3"),
																									lvl_3)};
		real_terminals_vector << QSharedPointer<AbstractRealTerminalInterface> {
																			   new DemoRealTerminal( QStringLiteral("teach"),
																									QStringLiteral("1_A4"),
																									lvl_4)};
		m_physical_terminal << QSharedPointer<AbstractPhysicalTerminalInterface> {
																				 new DemoPhysicalTerminal {real_terminals_vector}};

        real_terminals_vector.clear();
		real_terminals_vector << QSharedPointer<AbstractRealTerminalInterface> {
																			   new DemoRealTerminal( QStringLiteral("24vdc"),
																									QStringLiteral("2_A1"),
																									lvl_1)};
		real_terminals_vector << QSharedPointer<AbstractRealTerminalInterface> {
																			   new DemoRealTerminal( QStringLiteral("0vdc"),
																									QStringLiteral("2_A2"),
																									lvl_2)};
		real_terminals_vector << QSharedPointer<AbstractRealTerminalInterface> {
																			   new DemoRealTerminal( QStringLiteral("signal"),
																									QStringLiteral("2_A3"),
																									lvl_3)};
		real_terminals_vector << QSharedPointer<AbstractRealTerminalInterface> {
																			   new DemoRealTerminal( QStringLiteral("teach"),
																									QStringLiteral("2_A4"),
																									lvl_4)};
		m_physical_terminal << QSharedPointer<AbstractPhysicalTerminalInterface> {
																				 new DemoPhysicalTerminal {real_terminals_vector}};


        real_terminals_vector.clear();
		real_terminals_vector << QSharedPointer<AbstractRealTerminalInterface> {
																			   new DemoRealTerminal( QStringLiteral("24vdc"),
																									QStringLiteral("3_A1"),
																									lvl_1)};
		real_terminals_vector << QSharedPointer<AbstractRealTerminalInterface> {
																			   new DemoRealTerminal( QStringLiteral("0vdc"),
																									QStringLiteral("3_A2"),
																									lvl_2)};
		real_terminals_vector << QSharedPointer<AbstractRealTerminalInterface> {
																			   new DemoRealTerminal( QStringLiteral("signal"),
																									QStringLiteral("3_A3"),
																									lvl_3)};
		real_terminals_vector << QSharedPointer<AbstractRealTerminalInterface> {
																			   new DemoRealTerminal( QStringLiteral("teach"),
																									QStringLiteral("3_A4"),
																									lvl_4)};
		m_physical_terminal << QSharedPointer<AbstractPhysicalTerminalInterface> {
																				 new DemoPhysicalTerminal {real_terminals_vector}};
    }
}
