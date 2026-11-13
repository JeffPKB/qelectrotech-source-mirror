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
#include "elementinfowidget.h"

#include "../diagram.h"
#include "../qetapp.h"
#include "../qetgraphicsitem/element.h"
#include "../qetinformation.h"
#include "../ui_elementinfowidget.h"
#include "../undocommand/changeelementinformationcommand.h"
#include "elementinfopartwidget.h"
#include "../qeticons.h"

#include <QApplication>
#include <QClipboard>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMimeData>
#include <QPushButton>

namespace {
		//Format of the informations of an element in the clipboard
	const QString INFO_MIME_TYPE{QStringLiteral("application/x-qet-element-informations")};

		//The label and the formula belong to each element: never copied
	bool isCopiedKey(const QString &key) {
		return key != QETInformation::ELMT_LABEL && key != QETInformation::ELMT_FORMULA;
	}
}

/**
	@brief ElementInfoWidget::ElementInfoWidget
	Constructor
	@param elmt element to edit information
	@param parent parent widget
*/
ElementInfoWidget::ElementInfoWidget(Element *elmt, QWidget *parent) :
	AbstractElementPropertiesEditorWidget(parent),
	ui(new Ui::ElementInfoWidget),
	m_first_activation (false)
{
	ui->setupUi(this);
	setElement(elmt);
}

/**
	@brief ElementInfoWidget::~ElementInfoWidget
	Destructor
*/
ElementInfoWidget::~ElementInfoWidget()
{
	qDeleteAll(m_eipw_list);
	delete ui;
}

/**
	@brief ElementInfoWidget::setElement
	Set element to be the edited element
	@param element
*/
void ElementInfoWidget::setElement(Element *element)
{
	if (m_element == element) return;

	if (m_element)
		disconnect(m_element.data(), &Element::elementInfoChange, this, &ElementInfoWidget::elementInfoChange);

	m_element = element;
	updateUi();

	const auto formula_info_widget = infoPartWidgetForKey(QETInformation::ELMT_FORMULA);
	const auto label_info_widget = infoPartWidgetForKey(QETInformation::ELMT_LABEL);

	if (formula_info_widget && label_info_widget)
	{
		if (formula_info_widget->text().isEmpty())
			label_info_widget->setEnabled(true);
		else
			label_info_widget->setDisabled(true);

		connect(formula_info_widget, &ElementInfoPartWidget::textChanged, this, [label_info_widget](const QString text)
		{
			label_info_widget->setEnabled(text.isEmpty()? true : false);
		});
	}

	connect(m_element.data(), &Element::elementInfoChange, this, &ElementInfoWidget::elementInfoChange);
}

/**
	@brief ElementInfoWidget::apply
	Apply the new information with a new undo command (got with method associatedUndo)
	pushed to the stack of element project.
*/
void ElementInfoWidget::apply()
{
	if (auto undo = associatedUndo())
		m_element -> diagram() -> undoStack().push(undo);
}

/**
	@brief ElementInfoWidget::associatedUndo
	If the edited info is different of the actual element info,
	return a QUndoCommand with the change.
	If no change return nullptr;
	@return
*/
QUndoCommand* ElementInfoWidget::associatedUndo() const
{
	const auto new_info = currentInfo();
	const auto old_info = m_element -> elementInformations();

	if (old_info != new_info)
		return (new ChangeElementInformationCommand(m_element, old_info, new_info));

	return nullptr;
}

/**
	@brief ElementInfoWidget::setLiveEdit
	@param live_edit true : enable the live edit mode, false disable
	@return always true;
*/
bool ElementInfoWidget::setLiveEdit(bool live_edit)
{
	if (m_live_edit == live_edit) return true;
	m_live_edit = live_edit;

	if (m_live_edit)
		enableLiveEdit();
	else
		disableLiveEdit();

	return true;
}

/**
	@brief ElementInfoWidget::event
	Reimplemented from QWidget::event
	Only give focus to the first line edit at first activation.
	After send the event to QWidget.
	@param event
	@return
*/
bool ElementInfoWidget::event(QEvent *event)
{
	if (m_first_activation)
	{
		if (event -> type() == QEvent::WindowActivate || event -> type() == QEvent::Show)
		{
			QTimer::singleShot(250, this, SLOT(firstActivated()));
			m_first_activation = false;
		}
	}
	return(QWidget::event(event));
}

/**
	@brief ElementInfoWidget::enableLiveEdit
	Enable the live edit mode
*/
void ElementInfoWidget::enableLiveEdit()
{
	for (ElementInfoPartWidget *eipw : m_eipw_list)
		connect(eipw, &ElementInfoPartWidget::textChanged, this, &ElementInfoWidget::apply);
}

/**
	@brief ElementInfoWidget::disableLiveEdit
	disable the live edit mode
*/
void ElementInfoWidget::disableLiveEdit()
{
	for (ElementInfoPartWidget *eipw : m_eipw_list)
		disconnect(eipw, &ElementInfoPartWidget::textChanged, this, &ElementInfoWidget::apply);
}

/**
	@brief ElementInfoWidget::buildInterface
	Build the widget
*/
void ElementInfoWidget::buildInterface()
{
	QStringList keys;
	if (m_element.data()->elementData().m_type == ElementData::Terminal) {
		keys = QETInformation::terminalElementInfoKeys();
	 } else {
		keys = QETInformation::elementInfoKeys();
	}

		//Copy / paste all the informations (except label and formula) from
		//an element to another one
	auto buttons_layout_{new QHBoxLayout};
	auto copy_button_{new QPushButton(QET::Icons::EditCopy, tr("Copier les informations"), this)};
	auto paste_button_{new QPushButton(QET::Icons::EditPaste, tr("Coller les informations"), this)};
	copy_button_->setToolTip(tr("Copie toutes les informations de cet élément (installation, "
								"localisation, commentaire, fonction...), sauf le label et la formule"));
	paste_button_->setToolTip(tr("Remplace les informations de cet élément par celles copiées, "
								 "sauf le label et la formule"));
	connect(copy_button_, &QPushButton::clicked, this, &ElementInfoWidget::copyInformations);
	connect(paste_button_, &QPushButton::clicked, this, &ElementInfoWidget::pasteInformations);
	buttons_layout_->addWidget(copy_button_);
	buttons_layout_->addWidget(paste_button_);
	ui->scroll_vlayout->addLayout(buttons_layout_);

	for (auto str : keys)
	{
		ElementInfoPartWidget *eipw = new ElementInfoPartWidget(str, QETInformation::translatedInfoKey(str), this);
		ui->scroll_vlayout->addWidget(eipw);
		m_eipw_list << eipw;
	}

	ui->scroll_vlayout->addStretch();
}

/**
	@brief ElementInfoWidget::infoPartWidgetForKey
	@param key
	@return the ElementInfoPartWidget with key key,
	if not found return nullptr;
*/
ElementInfoPartWidget *ElementInfoWidget::infoPartWidgetForKey(const QString &key) const
{
	for (const auto &eipw : qAsConst(m_eipw_list))
	{
		if (eipw->key() == key)
			return eipw;
	}

	return nullptr;
}

/**
	@brief ElementInfoWidget::updateUi
	fill information fetch in m_element_info to the
	corresponding line edit
*/
void ElementInfoWidget::updateUi()
{
	if (!m_ui_builded) {
		buildInterface();
		m_ui_builded = true;
	}
		//We disable live edit to avoid wrong undo when we fill the line edit with new text
	if (m_live_edit) disableLiveEdit();

	const auto element_info{m_element->elementInformations()};
	
	for (ElementInfoPartWidget *eipw : m_eipw_list) {
		eipw -> setText (element_info[eipw->key()].toString());
	}

	if (m_live_edit) {
		enableLiveEdit();
	}
}

/**
	@brief ElementInfoWidget::currentInfo
	@return the info currently edited
*/
DiagramContext ElementInfoWidget::currentInfo() const
{
	DiagramContext info_;

	for (const auto &eipw : qAsConst(m_eipw_list))
	{

			//add value only if they're something to store
		if (!eipw->text().isEmpty())
		{
			QString txt{eipw->text()};
				//remove line feed and carriage return
			txt.remove(QStringLiteral("\r"));
			txt.remove(QStringLiteral("\n"));
			info_.addValue(eipw->key(), txt);
		}
	}

		//Keep the informations this widget doesn't edit (the cable of
		//a terminal set in the terminal strip editor), else they are lost
	if (m_element)
	{
		const auto element_info{m_element->elementInformations()};
		for (const auto &key : element_info.keys())
		{
			if (key.startsWith(QStringLiteral("terminal_cable")) &&
				!info_.contains(key)) {
				info_.addValue(key, element_info.value(key));
			}
		}
	}

	return info_;
}

/**
	@brief ElementInfoWidget::firstActivated
	Slot activated when this widget is show.
	Set the focus to the first line edit provided by this widget
*/
void ElementInfoWidget::firstActivated()
{
	m_eipw_list.first() -> setFocusTolineEdit();
}

/**
	@brief ElementInfoWidget::elementInfoChange
	This slot is called when m_element::elementInformation change.
*/
void ElementInfoWidget::elementInfoChange()
{
	auto elmt_info = m_element->elementInformations();
	auto current_info = currentInfo();

		//If both info have a formula, we remove the label
		//value before compare the equality, because the
		//label of the information returned by the element
		//can be different of the current label because
		//updated by the element to reflect the actual
		//displayed label according the current formula.
	if (current_info.contains(QETInformation::ELMT_FORMULA) &&
		elmt_info.contains(QETInformation::ELMT_FORMULA))
	{
		elmt_info.remove(QETInformation::ELMT_LABEL);
		current_info.remove((QETInformation::ELMT_LABEL ));
	}

	if(current_info != elmt_info)
		updateUi();
}

/**
	@brief ElementInfoWidget::copyInformations
	Copy in the clipboard all the informations currently edited, except the
	label and the formula which belong to each element. The empty
	informations are copied too, so the paste gives the same informations.
*/
void ElementInfoWidget::copyInformations()
{
	QJsonObject json_;
	QStringList text_;
	for (const auto &eipw : qAsConst(m_eipw_list))
	{
		if (!isCopiedKey(eipw->key())) {
			continue;
		}
		json_.insert(eipw->key(), eipw->text());
		if (!eipw->text().isEmpty()) {
			text_ << QETInformation::translatedInfoKey(eipw->key()) + QStringLiteral(" : ") + eipw->text();
		}
	}

	auto mime_data_{new QMimeData};
	mime_data_->setData(INFO_MIME_TYPE, QJsonDocument(json_).toJson(QJsonDocument::Compact));
	mime_data_->setText(text_.join(QLatin1Char('\n')));
	QApplication::clipboard()->setMimeData(mime_data_);
}

/**
	@brief ElementInfoWidget::pasteInformations
	Replace the informations currently edited by the ones copied with
	copyInformations, except the label and the formula.
	In live edit, the paste is one undo command.
*/
void ElementInfoWidget::pasteInformations()
{
	const auto mime_data_{QApplication::clipboard()->mimeData()};
	if (!mime_data_ || !mime_data_->hasFormat(INFO_MIME_TYPE)) {
		return;
	}
	const auto json_{QJsonDocument::fromJson(mime_data_->data(INFO_MIME_TYPE)).object()};

	if (m_live_edit) disableLiveEdit();
	for (const auto &eipw : qAsConst(m_eipw_list))
	{
		if (isCopiedKey(eipw->key()) && json_.contains(eipw->key())) {
			eipw->setText(json_.value(eipw->key()).toString());
		}
	}
	if (m_live_edit)
	{
		enableLiveEdit();
		apply();
	}
}
