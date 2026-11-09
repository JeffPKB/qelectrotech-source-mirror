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
#include "terminalstripdrawer.h"

#include <QPainter>

namespace TerminalStripDrawer {

/**
 * @brief TerminalStripDrawer::TerminalStripDrawer
 * @param strip
 * @param pattern
 */
TerminalStripDrawer::TerminalStripDrawer(QSharedPointer<AbstractTerminalStripInterface> strip,
										 QSharedPointer<TerminalStripLayoutPattern> layout) :
	m_strip { strip },
	m_pattern { layout }
{}

void TerminalStripDrawer::setStrip(QSharedPointer<AbstractTerminalStripInterface> strip)
{
	m_strip = strip;
}

/**
 * @brief TerminalStripDrawer::paint
 * @param painter
 */
void TerminalStripDrawer::paint(QPainter *painter)
{
    if (m_strip && m_pattern)
    {
		m_united_xref_text_rect = QRectF();
            //To draw text, QPainter need a Qrect. Instead of create an instance
            //for each text, we re-use the same instance of QRect.
        QRect text_rect;
        painter->save();

		auto pen_{painter->pen()};
		pen_.setColor(Qt::black);
		pen_.setWidth(1);

		auto brush_ = painter->brush();
		brush_.setColor(Qt::white);

        painter->setFont(m_pattern->font());

        painter->setPen(pen_);
        painter->setBrush(brush_);

        if (m_preview_draw)
        {
            painter->save();
            painter->setPen(Qt::blue);
            painter->drawRect(boundingRect());
            painter->drawLine(QPointF{boundingRect().left(), boundingRect().center().y()},
                              QPointF{boundingRect().right(), boundingRect().center().y()});
            painter->restore();
        }

			//Draw header
		painter->drawRect(m_pattern->m_header_rect);

			//Draw the header text
		painter->save();

		if (m_pattern->m_header_text_orientation == Qt::Horizontal)
		{
			text_rect.setRect(0,m_pattern->m_header_rect.y(),m_pattern->m_header_rect.width(),m_pattern->m_header_rect.height());
		}
		else
		{
			painter->translate(m_pattern->m_header_rect.bottomLeft());
			painter->rotate(270);
			text_rect.setRect(0,0,m_pattern->m_header_rect.height(),m_pattern->m_header_rect.width());
		}

        const auto text_{m_strip->installation() + " " + m_strip->location() + " " + m_strip->name()};
        painter->drawText(text_rect, text_, m_pattern->headerTextOption());
		painter->restore();

            //Move painter pos to next drawing
        painter->translate(m_pattern->m_header_rect.width(),0);
        qreal x_offset{m_pattern->m_header_rect.width()};

			//Draw spacer
		painter->drawRect(m_pattern->m_spacer_rect);
			//Move painter pos to next drawing
		painter->translate(m_pattern->m_spacer_rect.width(),0);
		x_offset += m_pattern->m_spacer_rect.width();

            //Draw terminals
        const auto terminals_text_orientation{m_pattern->m_terminals_text_orientation};
        const auto terminals_text_option{m_pattern->terminalsTextOption()};
        const auto terminals_text_height{m_pattern->m_terminals_text_height};
        const auto terminals_text_y{m_pattern->m_terminals_text_y};
        QRectF terminal_rect;

		const auto xref_text_orientation{m_pattern->m_xref_text_orientation};
		const auto xref_text_option{m_pattern->xrefTextOption()};
		const auto xref_text_height{m_pattern->m_xref_text_height};
		const auto xref_text_y{m_pattern->m_xref_text_y};
		QRectF xref_rect;

			//Under the strip, along the wire of each terminal: the wire
			//number, the arrival and the core of the cable. A zone is
			//reserved only if one terminal of the strip at least use it.
		const auto strip_height{stripHeight()};
		const auto zones_{belowZones()};
		const qreal wire_line_length{6};
		const auto zones_top{strip_height + wire_line_length};
		const auto zones_bottom{zones_top + zones_.wire + zones_.arrival + zones_.core};
		const auto bar_y{zones_bottom + 6};
		QTextOption below_text_option{Qt::AlignRight | Qt::AlignBottom};
		below_text_option.setWrapMode(QTextOption::NoWrap);

			//Runs of consecutive terminals sharing the same cable
		struct CableRun { QString name; QString destination; qreal x1; qreal x2; };
		QVector<CableRun> cable_runs;

		QHash<QUuid, QVector<QPointF>> bridges_anchor_points;

		m_hovered_xref = hoverTerminal{};
		int physical_index = 0;
            //Loop over physical terminals
        for (const auto &physical_t : m_strip->physicalTerminal())
        {
                //Get the good offset according to how many level have the current physical terminal
            const QVector<QSharedPointer<AbstractRealTerminalInterface>> real_terminal_vector{physical_t->realTerminals()};
            const auto real_t_count{real_terminal_vector.size()};
            const auto offset_{4 - real_t_count};

				//Loop over real terminals
			for (auto i=0 ; i<real_t_count ; ++i)
			{
				const auto index_ = offset_ + i;
				if (index_ >= 4) {
					break;
				}

                terminal_rect = m_pattern->m_terminal_rect[index_];
                    //Draw terminal rect
                painter->drawRect(terminal_rect);
                    //Draw a stronger line if the current terminal have level
                    //and the current level is the first
                if (real_t_count > 1 && i == 0)
                {
                    painter->save();
                    pen_ = painter->pen();
                    pen_.setWidth(4);
                    pen_.setCapStyle(Qt::FlatCap);
                    painter->setPen(pen_);
                    const auto p1 { terminal_rect.topLeft() };
                        //We can't use terminal_rect.bottomLeft for p2 because
                        //the returned value deviate from the true value
                        //(see Qt documentation about QRect)
                    const QPointF p2 { p1.x(), p1.y() + terminal_rect.height() };
                    painter->drawLine(p1, p2);
                    painter->restore();
                }

				if(m_preview_draw)
                {
                    painter->save();
                    painter->setPen(Qt::yellow);
                    painter->drawLine(QPointF{terminal_rect.x(), terminal_rect.y() + terminal_rect.height()/2},
                                      QPointF{terminal_rect.width(), terminal_rect.y() + terminal_rect.height()/2});
                    painter->restore();
				}

                    //Draw text
                painter->save();
                text_rect.setRect(0, terminals_text_y, terminal_rect.width(), terminals_text_height);
                if (terminals_text_orientation == Qt::Vertical)
                {
                    painter->translate(text_rect.bottomLeft());
                    painter->rotate(270);
                    text_rect.setRect(0, 0, text_rect.height(), text_rect.width());
                }

                const auto shared_real_terminal{real_terminal_vector[i]};
				painter->drawText(text_rect,
								  shared_real_terminal ? shared_real_terminal->label() : QLatin1String(),
								  terminals_text_option);

				if (m_preview_draw)
				{
					painter->setPen(Qt::blue);
					painter->drawRect(text_rect);
				}

				painter->restore();

					//Draw xref
				xref_rect.setRect(0, xref_text_y, terminal_rect.width(), xref_text_height);
				painter->save();
				if (xref_text_orientation == Qt::Vertical)
				{
					painter->translate(xref_rect.bottomLeft());
					painter->rotate(270);
					xref_rect.setRect(0, 0, xref_rect.height(), xref_rect.width());
				}

				QTransform transform;
				transform.translate(x_offset, 0);

				if (xref_text_orientation == Qt::Vertical)
				{
					transform.translate(0, xref_text_y + xref_text_height);
					transform.rotate(270);
				}

				auto xref_string = shared_real_terminal->xref();

				const auto mapped_xref_text_rect = transform.mapRect(painter->boundingRect(xref_rect, xref_string, xref_text_option));
				if (m_united_xref_text_rect.isNull()) {
					m_united_xref_text_rect = mapped_xref_text_rect;
				} else {
					m_united_xref_text_rect = m_united_xref_text_rect.united(mapped_xref_text_rect);
				}

					//if mouse hover the xref text, draw it in blue to advise user the xref is clickable.
				if (!m_mouse_hover_pos.isNull() && mapped_xref_text_rect.contains(m_mouse_hover_pos)) {
					painter->setPen(Qt::blue);
					m_hovered_xref.physical = physical_index;
					m_hovered_xref.real = i;
				}

				painter->drawText(xref_rect, xref_string, xref_text_option);

				if (m_preview_draw)
				{
					painter->setPen(Qt::blue);
					painter->drawRect(xref_rect);
				}
				painter->restore();

					//Remember the cable of this terminal
				const auto cable_name{shared_real_terminal->cable()};
				const auto wire_x{x_offset + terminal_rect.width()/2};
				if (!cable_name.isEmpty())
				{
					if (!cable_runs.isEmpty() && cable_runs.last().name == cable_name) {
						cable_runs.last().x2 = wire_x;
						if (cable_runs.last().destination.isEmpty()) {
							cable_runs.last().destination = shared_real_terminal->cableDestination();
						}
					} else {
						cable_runs.append(CableRun{cable_name,
												   shared_real_terminal->cableDestination(),
												   wire_x, wire_x});
					}
				}
				else if (!cable_runs.isEmpty() && !cable_runs.last().name.isEmpty()) {
						//A terminal without cable end the current run
					cable_runs.append(CableRun{QString(), QString(), wire_x, wire_x});
				}

					//Draw the wire under the terminal and its texts
				const auto wire_string{shared_real_terminal->conductor()};
				const auto arrival_string{shared_real_terminal->arrival()};
				const auto core_string{shared_real_terminal->cableWire()};
				if (!wire_string.isEmpty() || !arrival_string.isEmpty() ||
					!core_string.isEmpty() || !cable_name.isEmpty())
				{
					painter->save();
					const auto line_x{terminal_rect.width()/2};
					painter->drawLine(QPointF{line_x, terminal_rect.y() + terminal_rect.height()},
									  QPointF{line_x, cable_name.isEmpty() ? zones_bottom : bar_y});

					qreal y_{zones_top};
					const auto draw_zone = [&](const QString &text, qreal zone_height)
					{
						if (zone_height <= 0) {
							return;
						}
						if (!text.isEmpty())
						{
							painter->save();
							QRectF rect_{0, y_, line_x - 1, zone_height};
							painter->translate(rect_.bottomLeft());
							painter->rotate(270);
							rect_.setRect(0, 0, rect_.height(), rect_.width());
							painter->drawText(rect_, text, below_text_option);
							if (m_preview_draw) {
								painter->setPen(Qt::blue);
								painter->drawRect(rect_);
							}
							painter->restore();
						}
						y_ += zone_height;
					};
					draw_zone(wire_string, zones_.wire);
					draw_zone(arrival_string, zones_.arrival);
					draw_zone(core_string, zones_.core);
					painter->restore();
				}

                    //Add bridge anchor
                if (shared_real_terminal->isBridged())
                {
                    painter->save();
                    if (QScopedPointer<AbstractBridgeInterface> bridge_ {
                        shared_real_terminal->bridge() })
                    {
                        const auto x_anchor{terminal_rect.width()/2};
                        const auto y_anchor {m_pattern->m_bridge_point_y_offset[index_]};
                        const auto radius_anchor{m_pattern->m_bridge_point_d/2};

						painter->setBrush(Qt::SolidPattern);
						painter->drawEllipse(QPointF(x_anchor, y_anchor),
											 radius_anchor, radius_anchor);

						auto anchor_points{bridges_anchor_points.value(bridge_->uuid())};
						anchor_points.append(QPointF(x_offset + x_anchor, y_anchor));
						bridges_anchor_points.insert(bridge_->uuid(), anchor_points);
					}
					painter->restore();
				}

                    //Move painter pos to next drawing
                painter->translate(terminal_rect.width(),0);
                x_offset += terminal_rect.width();
            }
			physical_index++;
        }
        painter->restore();

			//Draw the cables: a bar joining the wires, the tag with the
			//name of the cable and its destination at the end
		painter->save();
		painter->setFont(m_pattern->font());
		const QFontMetricsF metrics{painter->font()};
		qreal last_right{-1e9};
		for (const auto &run : qAsConst(cable_runs))
		{
			if (run.name.isEmpty()) {
				continue;
			}
			painter->save();
			auto pen_{painter->pen()};
			pen_.setWidth(2);
			painter->setPen(pen_);
			painter->drawLine(QPointF{run.x1, bar_y}, QPointF{run.x2, bar_y});
			pen_.setWidth(1);
			painter->setPen(pen_);
			const auto center_x{(run.x1 + run.x2)/2};

			const auto tag_w{metrics.horizontalAdvance(run.name) + 8};
			const auto tag_h{metrics.height() + 4};
			const auto dest_w{run.destination.isEmpty() ? 0 : metrics.horizontalAdvance(run.destination)};
			const auto half_w{qMax(tag_w, dest_w)/2};
			qreal tag_y{bar_y + 8};
				//Two cables side by side: the second one goes lower
				//so the texts don't overlap
			if (center_x - half_w < last_right + 2) {
				tag_y += tag_h + 2;
				last_right = -1e9;
			} else {
				last_right = center_x + half_w;
			}
			painter->drawLine(QPointF{center_x, bar_y}, QPointF{center_x, tag_y});

			const QRectF tag{center_x - tag_w/2, tag_y, tag_w, tag_h};
			painter->setBrush(Qt::white);
			painter->drawRect(tag);
			painter->drawText(tag, Qt::AlignCenter, run.name);

			if (!run.destination.isEmpty())
			{
				painter->drawLine(QPointF{center_x, tag.bottom()}, QPointF{center_x, tag.bottom() + 8});
				const QRectF dest_rect{center_x - dest_w/2 - 2, tag.bottom() + 8, dest_w + 4, metrics.height() + 2};
				painter->drawText(dest_rect, Qt::AlignHCenter | Qt::AlignTop, run.destination);
			}
			painter->restore();
		}
		painter->restore();

			//Draw the bridges
		for (const auto &points_ : qAsConst(bridges_anchor_points))
		{
			painter->save();
			auto pen_{painter->pen()};
			pen_.setWidth(2);
			painter->setPen(pen_);
			painter->drawPolyline(QPolygonF{points_});
			painter->restore();
		}
	}
}

QRectF TerminalStripDrawer::boundingRect() const
{
    return QRectF{0, 0, width(), height()};;
}

void TerminalStripDrawer::setLayout(QSharedPointer<TerminalStripLayoutPattern> layout)
{
	m_pattern = layout;
}

bool TerminalStripDrawer::haveLayout() const
{
	return !m_pattern.isNull();
}

void TerminalStripDrawer::setPreviewDraw(bool draw) {
	m_preview_draw = draw;
}

void TerminalStripDrawer::setMouseHoverPos(const QPointF &pos)
{
	m_last_mouse_pos_in_xrefs_rect = m_united_xref_text_rect.contains(m_mouse_hover_pos);
	m_mouse_hover_pos = pos;
}

/**
 * @brief TerminalStripDrawer::mouseHoverXref
 * @return True if the mouse position (given through the function setMouseHoverPos)
 * hover the rect of a xref.
 */
bool TerminalStripDrawer::mouseHoverXref() const {
	return m_united_xref_text_rect.contains(m_mouse_hover_pos);
}

bool TerminalStripDrawer::needUpdate()
{
	if (mouseHoverXref()) {
		return true;
	} else if (m_last_mouse_pos_in_xrefs_rect) {
		return true;
	}
	return false;
}

/**
 * @brief TerminalStripDrawer::hoveredXref
 * @return the current terminal hovered by the mouse
 * in the xref bounding rectangle
 */
hoverTerminal TerminalStripDrawer::hoveredXref() const
{
	return m_hovered_xref;
}

/**
 * @brief TerminalStripDrawer::height
 * @return the height of the strip plus the room of the wire numbers
 */
qreal TerminalStripDrawer::height() const
{
	if (m_pattern) {
		const auto zones_{belowZones()};
		return stripHeight() + 6 + zones_.wire + zones_.arrival + zones_.core + zones_.cable;
	}
	return 0;
}

/**
 * @brief TerminalStripDrawer::belowZones
 * @return the height of each zone drawn under the strip, 0 for a zone
 * not used by any terminal of the strip
 */
BelowZones TerminalStripDrawer::belowZones() const
{
	BelowZones zones_;
	if (!m_pattern || !m_strip) {
		return zones_;
	}
	for (const auto &physical_t : m_strip->physicalTerminal())
	{
		for (const auto &real_t : physical_t->realTerminals())
		{
			if (!real_t) {
				continue;
			}
			if (!real_t->conductor().isEmpty()) zones_.wire    = m_pattern->m_wire_text_height;
			if (!real_t->arrival().isEmpty())   zones_.arrival = m_pattern->m_arrival_text_height;
			if (!real_t->cableWire().isEmpty()) zones_.core    = m_pattern->m_core_text_height;
			if (!real_t->cable().isEmpty())     zones_.cable   = m_pattern->m_cable_height;
		}
	}
	return zones_;
}

/**
 * @brief TerminalStripDrawer::stripHeight
 * @return the height of the strip itself (header, spacer and terminals)
 */
qreal TerminalStripDrawer::stripHeight() const
{
	if (m_pattern)
	{
		auto height_{m_pattern->m_header_rect.y() + m_pattern->m_header_rect.height()};

		height_ = std::max(height_, m_pattern->m_spacer_rect.y() + m_pattern->m_spacer_rect.height());

		for (const auto &rect : m_pattern->m_terminal_rect) {
			height_ = std::max(height_, rect.y() + rect.height());
		}

		return height_;
	}

	return 0;
}

qreal TerminalStripDrawer::width() const
{
    if (m_pattern)
    {
        qreal width_{m_pattern->m_header_rect.width() + m_pattern->m_spacer_rect.width()};

		if (m_strip)
		{
			//Loop over physical terminals
			for (const auto &physical_t : m_strip->physicalTerminal())
			{
				//Get the good offset according to how many level have the current physical terminal
				const QVector<QSharedPointer<AbstractRealTerminalInterface>> real_terminal_vector{physical_t->realTerminals()};
				const auto real_t_count{real_terminal_vector.size()};
				const auto offset_{4 - real_t_count};

				//Loop over real terminals
				for (auto i=0 ; i<real_t_count ; ++i)
				{
					const auto index_ = offset_ + i;
					if (index_ >= 4) {
						break;
					}

					width_ += m_pattern->m_terminal_rect[index_].width();
				}
			}
		}

		return width_;
	}

	return 0;
}

} //End namespace TerminalStripDrawer
