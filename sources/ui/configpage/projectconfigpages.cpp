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
#include "../../diagram.h"
#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QDialog>
#include <QCheckBox>
#include <QDateEdit>
#include <QHBoxLayout>
#include <QPushButton>
#include <QSettings>
#include <QMessageBox>
#include "projectconfigpages.h"

#include "../autoNum/numerotationcontext.h"
#include "../autoNum/ui/autonumberingmanagementw.h"
#include "../autoNum/ui/folioautonumbering.h"
#include "../autoNum/ui/formulaautonumberingw.h"
#include "../autoNum/ui/selectautonumw.h"
#include "../qeticons.h"
#include "../qetproject.h"
#include "../borderpropertieswidget.h"
#include "../conductorpropertieswidget.h"
#include "../diagramcontextwidget.h"
#include "../reportpropertiewidget.h"
#include "../titleblockpropertieswidget.h"
#include "../xrefpropertieswidget.h"

//#include "ui_autonumberingmanagementw.h"

#include <QtWidgets>

/**
	Constructor
	@param project Project this page is editing.
	@param parent Parent QWidget
*/
ProjectConfigPage::ProjectConfigPage(QETProject *project, QWidget *parent) :
	ConfigPage(parent),
	m_project(project)
{
}

/**
	Destructor
*/
ProjectConfigPage::~ProjectConfigPage()
{
}

/**
	@return the project being edited by this page
*/
QETProject *ProjectConfigPage::project() const
{
	return(m_project);
}

/**
	@brief ProjectConfigPage::setProject
	Set \a new_project as the project being edited by this page.
	@param new_project : True to read values from the project
	into widgets before setting them read only accordingly,
	false otherwise. Defaults to true.
	@param read_values
	@return the former project
*/
QETProject *ProjectConfigPage::setProject(QETProject *new_project,
					  bool read_values) {
	if (new_project == m_project) return(m_project);
	
	QETProject *former_project = m_project;
	m_project = new_project;
	if (m_project && read_values) {
		readValuesFromProject();
		adjustReadOnly();
	}
	return(former_project);
}

/**
	Apply the configuration after user input
*/
void ProjectConfigPage::applyConf()
{
	if (!m_project || m_project -> isReadOnly()) return;
	applyProjectConf();
}

/**
	Initialize the page by calling initWidgets() and initLayout(). Also call
	readValuesFromProject() and adjustReadOnly() if a non-zero project has been
	set. Typically, you should call this function in your subclass constructor.
*/
void ProjectConfigPage::init()
{
	initWidgets();
	initLayout();
	if (m_project) {
		readValuesFromProject();
		adjustReadOnly();
	}
}

//######################################################################################//

/**
	Constructor
	@param project Project this page is editing.
	@param parent Parent QWidget
*/
ProjectMainConfigPage::ProjectMainConfigPage(QETProject *project, QWidget *parent) :
	ProjectConfigPage(project, parent)
{
	init();
}

/**
	Destructor
*/
ProjectMainConfigPage::~ProjectMainConfigPage()
{
}

/**
	@return the title for this page
*/
QString ProjectMainConfigPage::title() const
{
	return(tr("Général", "configuration page title"));
}

/**
	@return the icon for this page
*/
QIcon ProjectMainConfigPage::icon() const
{
	return(QET::Icons::Settings);
}

/**
	Apply the configuration after user input
*/
void ProjectMainConfigPage::applyProjectConf()
{
	bool modified_project = false;
	
	QString new_title = title_value_ -> text();
	if (m_project -> title() != new_title) {
		m_project -> setTitle(new_title);
		modified_project = true;
	}
	
	DiagramContext new_properties = project_variables_ -> context();
	if (m_project -> projectProperties() != new_properties) {
		const auto old_properties{m_project -> projectProperties()};
		const auto folios_count_{m_project -> diagrams().count()};

			//Ask before changing the existing folios: "No" keeps their old
			//values (written in their own title block), only the new
			//folios get the new ones
		bool apply_to_folios{true};
		if (folios_count_ > 0)
		{
			apply_to_folios = QMessageBox::question(
					this,
					tr("Variables du projet"),
					tr("Appliquer les nouvelles valeurs des variables aux %n folio(s) existant(s) ?\n\n"
					   "Non : les folios existants gardent leurs valeurs actuelles, "
					   "seuls les nouveaux folios prendront les nouvelles.",
					   "", folios_count_),
					QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) == QMessageBox::Yes;
		}

		if (!apply_to_folios) {
			m_project -> keepVariablesOnFolios(old_properties, new_properties);
		}
		m_project -> setProjectProperties(new_properties);
		modified_project = true;
		if (apply_to_folios) {
				//A folio with a custom field of the same name would keep
				//its old value (it overrides the project variable)
			m_project -> applyVariablesToFolios(old_properties, new_properties);
		}
	}
	if (modified_project) {
		m_project -> setModified(true);
	}
}

/**
	@return the project title entered by the user
*/
QString ProjectMainConfigPage::projectTitle() const
{
	return(title_value_ -> text());
}

/**
	Initialize widgets displayed by the page.
*/
void ProjectMainConfigPage::initWidgets()
{
	title_label_ = new QLabel(tr("Titre du projet :", "label when configuring"));
	title_value_ = new QLineEdit();
	title_information_ = new QLabel(tr("Ce titre sera disponible pour tous les folios de ce projet en tant que %projecttitle.", "informative label"));
	project_variables_label_ = new QLabel(
		tr(
			"Vous pouvez définir ci-dessous des propriétés personnalisées qui seront disponibles pour tous les folios de ce projet (typiquement pour les cartouches).",
			 "informative label"
		)
	);
	project_variables_label_ -> setWordWrap(true);
	project_variables_ = new DiagramContextWidget();
	project_variables_ -> setContext(DiagramContext());

	m_save_defaults_button = new QPushButton(tr("Mémoriser pour les nouveaux projets"));
	m_save_defaults_button -> setToolTip(
		tr("Les variables ci-dessus et le cartouche par défaut de ce projet seront "
		   "repris automatiquement à la création de chaque nouveau projet."));
	connect(m_save_defaults_button, &QPushButton::clicked, this, &ProjectMainConfigPage::saveAsDefaults);

	m_load_defaults_button = new QPushButton(tr("Reprendre les valeurs mémorisées"));
	m_load_defaults_button -> setToolTip(
		tr("Ajoute les variables mémorisées qui manquent à ce projet et remplit "
		   "celles qui sont vides. Les valeurs déjà remplies sont gardées."));
	connect(m_load_defaults_button, &QPushButton::clicked, this, &ProjectMainConfigPage::loadDefaults);

		//Date of all the folios in one go
	m_folios_date = new QDateEdit(QDate::currentDate());
	m_folios_date -> setCalendarPopup(true);
	m_folios_date -> setDisplayFormat(QStringLiteral("dd/MM/yyyy"));
	m_today_button = new QPushButton(tr("Aujourd'hui"));
	connect(m_today_button, &QPushButton::clicked, this, [this]() {
		m_folios_date -> setDate(QDate::currentDate());
	});
	m_restart_button = new QPushButton(tr("Repartir de ce projet..."));
	m_restart_button -> setToolTip(
		tr("Pour une nouvelle machine à partir de ce projet : date du jour, "
		   "tableau des indices vidé, indice de révision remis à zéro..."));
	connect(m_restart_button, &QPushButton::clicked, this, [this]() {
		if (restartFromProject(m_project, this)) {
			readValuesFromProject();
			emit projectRestarted();
		}
	});
	m_apply_date_button = new QPushButton(tr("Appliquer à tous les folios"));
	m_apply_date_button -> setToolTip(
		tr("Met cette date dans le cartouche de tous les folios du projet et "
		   "dans le cartouche par défaut (nouveaux folios)."));
	connect(m_apply_date_button, &QPushButton::clicked, this, &ProjectMainConfigPage::applyDateToAllFolios);
}

/**
	@brief ProjectMainConfigPage::applyDateToAllFolios
	Write the chosen date in the title block of all the folios, after a
	confirmation.
*/
void ProjectMainConfigPage::applyDateToAllFolios()
{
	const auto date_{m_folios_date -> date()};
	const auto count_{m_project -> diagrams().count()};
	if (QMessageBox::question(
			this,
			tr("Date des folios"),
			tr("Mettre la date du %1 dans le cartouche des %n folio(s) du projet ?",
			   "", count_).arg(date_.toString(QStringLiteral("dd/MM/yyyy"))),
			QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) != QMessageBox::Yes) {
		return;
	}
	m_project -> setDateOnAllFolios(date_);
}

/**
	@brief ProjectMainConfigPage::loadDefaults
	Add to the variables edited the kept ones which are missing, and fill
	the empty ones. The values already filled are kept.
*/
void ProjectMainConfigPage::loadDefaults()
{
	auto context_{project_variables_ -> context()};
	const auto defaults_{QETProject::newProjectVariables()};
	for (const auto &key : defaults_.keys())
	{
		if (!context_.contains(key) || context_.value(key).toString().isEmpty()) {
			context_.addValue(key, defaults_.value(key));
		}
	}
	project_variables_ -> setContext(context_);
}

/**
	@brief ProjectMainConfigPage::saveAsDefaults
	Keep the project variables currently edited and the default title
	block of the project: a new project starts with them, instead of empty.
*/
void ProjectMainConfigPage::saveAsDefaults()
{
	QSettings settings;
	settings.remove(QStringLiteral("newproject/variables"));
	project_variables_ -> context().toSettings(settings, QStringLiteral("newproject/variables"));
	m_project -> defaultTitleBlockProperties().toSettings(settings, QStringLiteral("diagrameditor/default"));

	QMessageBox::information(
		this,
		tr("Nouveaux projets"),
		tr("Les variables et le cartouche par défaut sont mémorisés : "
		   "ils seront repris à la création de chaque nouveau projet.\n\n"
		   "Si vous venez de modifier le cartouche par défaut dans l'onglet "
		   "« Nouveau folio », validez d'abord les propriétés du projet, puis "
		   "revenez cliquer sur ce bouton."));
}

/**
	Initialize the layout of this page.
*/
void ProjectMainConfigPage::initLayout()
{
	QVBoxLayout *main_layout0 = new QVBoxLayout();
	QHBoxLayout *title_layout0 = new QHBoxLayout();
	title_layout0 -> addWidget(title_label_);
	title_layout0 -> addWidget(title_value_);
	main_layout0 -> addLayout(title_layout0);
	main_layout0 -> addWidget(title_information_);
	main_layout0 -> addSpacing(10);
	main_layout0 -> addWidget(project_variables_label_);
	main_layout0 -> addWidget(project_variables_);
	main_layout0 -> addSpacing(10);
	QHBoxLayout *date_layout = new QHBoxLayout();
	date_layout -> addWidget(new QLabel(tr("Date des folios :")));
	date_layout -> addWidget(m_folios_date);
	date_layout -> addWidget(m_today_button);
	date_layout -> addWidget(m_apply_date_button);
	date_layout -> addStretch();
	main_layout0 -> addLayout(date_layout);

	QHBoxLayout *defaults_layout = new QHBoxLayout();
	defaults_layout -> addWidget(m_restart_button);
	defaults_layout -> addStretch();
	defaults_layout -> addWidget(m_load_defaults_button);
	defaults_layout -> addWidget(m_save_defaults_button);
	main_layout0 -> addLayout(defaults_layout);
	setLayout(main_layout0);
	this -> setMinimumWidth(680);

}

/**
	Read properties from the edited project then fill widgets with them.
*/
void ProjectMainConfigPage::readValuesFromProject()
{
	title_value_ -> setText(m_project -> title());
	project_variables_ -> setContext(m_project -> projectProperties());
}

/**
	Set the content of this page read only if the project is read only,
	editable if the project is editable.
*/
void ProjectMainConfigPage::adjustReadOnly()
{
	bool is_read_only = m_project -> isReadOnly();
	title_value_ -> setReadOnly(is_read_only);
}

//######################################################################################//

/**
	@brief ProjectAutoNumConfigPage::ProjectAutoNumConfigPage
	Default constructor
	@param project : project to edit
	@param parent : parent widget
*/
ProjectAutoNumConfigPage::ProjectAutoNumConfigPage (QETProject *project,
						    QWidget *parent) :
	ProjectConfigPage(project, parent)
{
	initWidgets();
	buildConnections();
	readValuesFromProject();
}

/**
	@brief ProjectAutoNumConfigPage::title
	Title of this config page
	@return
*/
QString ProjectAutoNumConfigPage::title() const
{
	return tr("Numérotation auto");
}

/**
	@brief ProjectAutoNumConfigPage::icon
	Icon of this config pafe
	@return
*/
QIcon ProjectAutoNumConfigPage::icon() const
{
	return QIcon (QET::Icons::AutoNum);
}

/**
	@brief ProjectAutoNumConfigPage::applyProjectConf
*/
void ProjectAutoNumConfigPage::applyProjectConf()
{}

/**
	@brief ProjectAutoNumConfigPage::initWidgets
	Init some widget of this page
*/
void ProjectAutoNumConfigPage::initWidgets()
{
	QTabWidget *tab_widget = new QTabWidget(this);
	
		//Management tab
	m_amw = new AutoNumberingManagementW(project());
	tab_widget->addTab(m_amw, tr("Management"));
	
		//Conductor tab
	m_saw_conductor = new SelectAutonumW(1);
	tab_widget->addTab(m_saw_conductor, tr("Conducteurs"));
	
		//Element tab
	m_saw_element = new SelectAutonumW(0);
	tab_widget->addTab(m_saw_element, tr("Eléments"));
	
		//Folio Tab
	m_saw_folio = new SelectAutonumW(2);
	tab_widget->addTab(m_saw_folio, tr("Folios"));
	
		//AutoNumbering Tab
	m_faw = new FolioAutonumberingW(project());
	tab_widget->addTab(m_faw, tr("Numérotation auto des folios"));
	
	QHBoxLayout *main_layout = new QHBoxLayout();
	main_layout->addWidget(tab_widget);
	setLayout(main_layout);
}

/**
	@brief ProjectAutoNumConfigPage::readValuesFromProject
	Read value stored on project, and update display
*/
void ProjectAutoNumConfigPage::readValuesFromProject()
{
		//Conductor Tab
	const QStringList strlc(m_project->conductorAutoNum().keys());
	m_saw_conductor->contextComboBox()->addItems(strlc);
	
		//Element Tab
	const QStringList strle(m_project->elementAutoNum().keys());
	m_saw_element->contextComboBox()->addItems(strle);
	
		//Folio Tab
	const QStringList strlf(m_project->folioAutoNum().keys());
	m_saw_folio->contextComboBox()->addItems(strlf);
	
		//Folio AutoNumbering Tab
	m_faw->setContext(m_project->folioAutoNum().keys());
}

/**
	@brief ProjectAutoNumConfigPage::adjustReadOnly
	set this config page disable if project is read only
*/
void ProjectAutoNumConfigPage::adjustReadOnly()
{
}

/**
	@brief ProjectAutoNumConfigPage::buildConnections
	setup some connections
*/
void ProjectAutoNumConfigPage::buildConnections()
{
		//Management Tab
	connect (m_amw, SIGNAL(applyPressed()), this, SLOT(applyManagement()));

		//Conductor Tab
	connect(m_saw_conductor, &SelectAutonumW::applyPressed,  this, &ProjectAutoNumConfigPage::saveContextConductor);
	connect(m_saw_conductor, &SelectAutonumW::removeClicked, this, &ProjectAutoNumConfigPage::removeContextConductor);
	connect(m_saw_conductor->contextComboBox(), SIGNAL(currentIndexChanged(QString)), this, SLOT(updateContextConductor(QString)));

		//Element Tab
	connect(m_saw_element, &SelectAutonumW::applyPressed,  this, &ProjectAutoNumConfigPage::saveContextElement);
	connect(m_saw_element, &SelectAutonumW::removeClicked, this, &ProjectAutoNumConfigPage::removeContextElement);
	connect(m_saw_element->contextComboBox(), SIGNAL(currentIndexChanged(QString)), this, SLOT(updateContextElement(QString)));

		//Folio Tab
	connect(m_saw_folio, &SelectAutonumW::applyPressed,  this, &ProjectAutoNumConfigPage::saveContextFolio);
	connect(m_saw_folio, &SelectAutonumW::removeClicked, this, &ProjectAutoNumConfigPage::removeContextFolio);
	connect(m_saw_folio->contextComboBox(), SIGNAL(currentIndexChanged(QString)), this, SLOT(updateContextFolio(QString)));

		//	Auto Folio Numbering
	connect (m_faw, SIGNAL (applyPressed()),				 this, SLOT (applyAutoNum()));
}

/**
	@brief ProjectAutoNumConfigPage::updateContext_conductor
	Display the current selected context for conductor
	@param str : key of context stored in project
*/
void ProjectAutoNumConfigPage::updateContextConductor(const QString& str) {
	if (str == tr("Nom de la nouvelle numérotation")) m_saw_conductor -> setContext(NumerotationContext());
	else m_saw_conductor ->setContext(m_project->conductorAutoNum(str));
}

/**
	@brief ProjectAutoNumConfigPage::updateContext_folio
	Display the current selected context for folio
	@param str : key of context stored in project
*/
void ProjectAutoNumConfigPage::updateContextFolio(const QString& str) {
	if (str == tr("Nom de la nouvelle numérotation")) m_saw_folio -> setContext(NumerotationContext());
	else m_saw_folio ->setContext(m_project->folioAutoNum(str));
}

/**
	@brief ProjectAutoNumConfigPage::updateContextElement
	Display the current selected context for element
	@param str : key of context stored in project
*/
void ProjectAutoNumConfigPage::updateContextElement(const QString& str)
{
	if (str == tr("Nom de la nouvelle numérotation"))
	{
		m_saw_element->setContext(NumerotationContext());
	}
	else
	{
		m_saw_element->setContext(m_project->elementAutoNum(str));
	}
}

/**
	@brief ProjectAutoNumConfigPage::saveContextElement
	Save the current displayed Element formula in project
*/
void ProjectAutoNumConfigPage::saveContextElement()
{
		// If the text is the default text "Name of new numerotation" save the edited context
		// With the the name "No name"
	if (m_saw_element->contextComboBox()->currentText() == tr("Nom de la nouvelle numérotation"))
	{
		QString title(tr("Sans nom"));

		m_project->addElementAutoNum (title, m_saw_element -> toNumContext());
		m_project->setCurrrentElementAutonum(title);
		m_saw_element->contextComboBox()->addItem(tr("Sans nom"));
	}
		// If the text isn't yet to the autonum of the project, add this new item to the combo box.
	else if ( !m_project -> elementAutoNum().keys().contains( m_saw_element->contextComboBox()->currentText()))
	{
		m_project->addElementAutoNum(m_saw_element->contextComboBox()->currentText(), m_saw_element->toNumContext());
		m_project->setCurrrentElementAutonum(m_saw_element->contextComboBox()->currentText());
		m_saw_element->contextComboBox()->addItem(m_saw_element->contextComboBox()->currentText());
	}
		// Else, the text already exist in the autonum of the project, just update the context
	else
	{
		m_project->addElementAutoNum (m_saw_element->contextComboBox() -> currentText(), m_saw_element -> toNumContext());
		m_project->setCurrrentElementAutonum(m_saw_element->contextComboBox()->currentText());
	}
}

/**
	@brief ProjectAutoNumConfigPage::removeContextElement
	Remove from project the current element numerotation context
*/
void ProjectAutoNumConfigPage::removeContextElement()
{
		//if default text, return
	if (m_saw_element->contextComboBox()->currentText() == tr("Nom de la nouvelle numérotation"))
		return;
	m_project->removeElementAutoNum (m_saw_element->contextComboBox()->currentText());
	m_saw_element->contextComboBox()->removeItem (m_saw_element->contextComboBox()->currentIndex());
}

/**
	@brief ProjectAutoNumConfigPage::saveContext_conductor
	Save the current displayed conductor context in project
*/
void ProjectAutoNumConfigPage::saveContextConductor()
{
		// If the text is the default text "Name of new numerotation" save the edited context
		// With the the name "No name"
	if (m_saw_conductor->contextComboBox()-> currentText() == tr("Nom de la nouvelle numérotation"))
	{
		m_project->addConductorAutoNum (tr("Sans nom"), m_saw_conductor -> toNumContext());
		project()->setCurrentConductorAutoNum(tr("Sans nom"));
		m_saw_conductor->contextComboBox()-> addItem(tr("Sans nom"));
	}
	// If the text isn't yet to the autonum of the project, add this new item to the combo box.
	else if ( !m_project -> conductorAutoNum().keys().contains( m_saw_conductor->contextComboBox()->currentText()))
	{
		project()->addConductorAutoNum(m_saw_conductor->contextComboBox()->currentText(), m_saw_conductor->toNumContext());
		project()->setCurrentConductorAutoNum(m_saw_conductor->contextComboBox()->currentText());
		m_saw_conductor->contextComboBox()-> addItem(m_saw_conductor->contextComboBox()->currentText());
	}
	// Else, the text already exist in the autonum of the project, just update the context
	else
	{
		project()->setCurrentConductorAutoNum(m_saw_conductor->contextComboBox()->currentText());
		m_project->addConductorAutoNum (m_saw_conductor->contextComboBox()-> currentText(), m_saw_conductor -> toNumContext());
	}
	project()->conductorAutoNumAdded();
}

/**
	@brief ProjectAutoNumConfigPage::saveContext_folio
	Save the current displayed folio context in project
*/
void ProjectAutoNumConfigPage::saveContextFolio()
{
	// If the text is the default text "Name of new numerotation" save the edited context
	// With the the name "No name"
	if (m_saw_folio->contextComboBox() -> currentText() == tr("Nom de la nouvelle numérotation")) {
		m_project->addFolioAutoNum (tr("Sans nom"), m_saw_folio -> toNumContext());
		m_saw_folio->contextComboBox() -> addItem(tr("Sans nom"));
	}
	// If the text isn't yet to the autonum of the project, add this new item to the combo box.
	else if ( !m_project -> folioAutoNum().keys().contains( m_saw_folio->contextComboBox()->currentText())) {
		project()->addFolioAutoNum(m_saw_folio->contextComboBox()->currentText(), m_saw_folio->toNumContext());
		m_saw_folio->contextComboBox() -> addItem(m_saw_folio->contextComboBox()->currentText());
	}
	// Else, the text already exist in the autonum of the project, just update the context
	else {
		m_project->addFolioAutoNum (m_saw_folio->contextComboBox() -> currentText(), m_saw_folio -> toNumContext());
	}
	project()->folioAutoNumAdded();
}

/**
	@brief ProjectAutoNumConfigPage::applyAutoNum
	Apply auto folio numbering, New Folios or Selected Folios
*/
void ProjectAutoNumConfigPage::applyAutoNum()
{

	if (m_faw->newFolios){
		int foliosRemaining = m_faw->newFoliosNumber();
		emit (saveCurrentTbp());
		emit (setAutoNum(m_faw->autoNumSelected()));
		while (foliosRemaining > 0){
			project()->autoFolioNumberingNewFolios();
			foliosRemaining = foliosRemaining-1;
		}
		emit (loadSavedTbp());
	}
	else{
		QString autoNum   = m_faw->autoNumSelected();
		int fromFolio = m_faw->fromFolio();
		int toFolio   = m_faw->toFolio();
		m_project->autoFolioNumberingSelectedFolios(fromFolio,toFolio,autoNum);
	}
}

/**
	@brief ProjectAutoNumConfigPage::applyAutoManagement
	Apply Management Options in Selected Folios
*/
void ProjectAutoNumConfigPage::applyManagement()
{
	//	int from;
	//	int to;
	//	//Apply to Entire Project
	//	if (m_amw->ui->m_apply_project_rb->isChecked()) {
	//		from = 0;
	//		to = project()->diagrams().size() - 1;
	//	}
	//	//Apply to selected Folios
	//	else {
	//		from =
	//m_amw->ui->m_from_folios_cb->itemData(m_amw->ui->m_from_folios_cb->currentIndex()).toInt();
	//		to =
	//m_amw->ui->m_to_folios_cb->itemData(m_amw->ui->m_to_folios_cb->currentIndex()).toInt();
	//	}

	//	//Conductor Autonumbering Update Policy
	//	//Allow Both Existent and New Conductors
	//	if (m_amw->ui->m_both_conductor_rb->isChecked()) {
	//		//Unfreeze Existent and New Conductors
	//		project()->freezeExistentConductorLabel(false, from,to);
	//		project()->freezeNewConductorLabel(false, from,to);
	//		project()->setFreezeNewConductors(false);
	//	}
	//	//Allow Only New
	//	else if (m_amw->ui->m_new_conductor_rb->isChecked()) {
	//		//Freeze Existent and Unfreeze New Conductors
	//		project()->freezeExistentConductorLabel(true, from,to);
	//		project()->freezeNewConductorLabel(false, from,to);
	//		project()->setFreezeNewConductors(false);
	//	}
	//	//Allow Only Existent
	//	else if (m_amw->ui->m_existent_conductor_rb->isChecked()) {
	//		//Freeze Existent and Unfreeze New Conductors
	//		project()->freezeExistentConductorLabel(false, from,to);
	//		project()->freezeNewConductorLabel(true, from,to);
	//		project()->setFreezeNewConductors(true);
	//	}
	//	//Disable
	//	else if (m_amw->ui->m_disable_conductor_rb->isChecked()) {
	//		//Freeze Existent and New Elements, Set Freeze Element Project Wide
	//		project()->freezeExistentConductorLabel(true, from,to);
	//		project()->freezeNewConductorLabel(true, from,to);
	//		project()->setFreezeNewConductors(true);
	//	}

	//	//Element Autonumbering Update Policy
	//	//Allow Both Existent and New Elements
	//	if (m_amw->ui->m_both_element_rb->isChecked()) {
	//		//Unfreeze Existent and New Elements
	//		project()->freezeExistentElementLabel(false, from,to);
	//		project()->freezeNewElementLabel(false, from,to);
	//		project()->setFreezeNewElements(false);
	//	}
	//	//Allow Only New
	//	else if (m_amw->ui->m_new_element_rb->isChecked()) {
	//		//Freeze Existent and Unfreeze New Elements
	//		project()->freezeExistentElementLabel(true, from,to);
	//		project()->freezeNewElementLabel(false, from,to);
	//		project()->setFreezeNewElements(false);
	//	}
	//	//Allow Only Existent
	//	else if (m_amw->ui->m_existent_element_rb->isChecked()) {
	//		//Freeze New and Unfreeze Existent Elements, Set Freeze Element
	//Project Wide 		project()->freezeExistentElementLabel(false, from,to);
	//		project()->freezeNewElementLabel(true, from,to);
	//		project()->setFreezeNewElements(true);
	//	}
	//	//Disable
	//	else if (m_amw->ui->m_disable_element_rb->isChecked()) {
	//		//Freeze Existent and New Elements, Set Freeze Element Project Wide
	//		project()->freezeExistentElementLabel(true, from,to);
	//		project()->freezeNewElementLabel(true, from,to);
	//		project()->setFreezeNewElements(true);
	//	}

	//	//Folio Autonumbering Status
	//	if (m_amw->ui->m_both_folio_rb->isChecked()) {

	//	}
	//	else if (m_amw->ui->m_new_folio_rb->isChecked()) {

	//	}
	//	else if (m_amw->ui->m_existent_folio_rb->isChecked()) {

	//	}
	//	else if (m_amw->ui->m_disable_folio_rb->isChecked()) {

	//	}
}

/**
	@brief ProjectAutoNumConfigPage::removeContext
	Remove from project the current conductor numerotation context
*/
void ProjectAutoNumConfigPage::removeContextConductor()
{
	//if default text, return
	if ( m_saw_conductor->contextComboBox()-> currentText() == tr("Nom de la nouvelle numérotation") ) return;
	m_project -> removeConductorAutoNum (m_saw_conductor->contextComboBox()-> currentText() );
	m_saw_conductor->contextComboBox()-> removeItem (m_saw_conductor->contextComboBox()-> currentIndex() );
	project()->conductorAutoNumRemoved();
}

/**
	@brief ProjectAutoNumConfigPage::removeContext_folio
	Remove from project the current folio numerotation context
*/
void ProjectAutoNumConfigPage::removeContextFolio()
{
	//if default text, return
	if ( m_saw_folio->contextComboBox() -> currentText() == tr("Nom de la nouvelle numérotation") ) return;
	m_project -> removeFolioAutoNum (m_saw_folio->contextComboBox() -> currentText() );
	m_saw_folio->contextComboBox() -> removeItem (m_saw_folio->contextComboBox() -> currentIndex() );
	project()->folioAutoNumRemoved();
}

/**
	@brief ProjectAutoNumConfigPage::changeToTab
	@param i index
	Change to Selected Tab
*/
void ProjectAutoNumConfigPage::changeToTab(int i)
{
	qDebug()<<"Q_UNUSED"<<i;
}

/**
	@brief ProjectMainConfigPage::restartFromProject
	Start a new machine from an existing project: ask what to reset (date
	of the folios, revision table A to G, revision index of the folios,
	machine and document numbers) and do it.
	@return true if something was reset
*/
bool ProjectMainConfigPage::restartFromProject(QETProject *project, QWidget *parent)
{
	if (!project) {
		return false;
	}

	QDialog dialog_(parent);
	dialog_.setWindowTitle(tr("Repartir de ce projet"));
	auto layout_{new QVBoxLayout(&dialog_)};
	auto info_{new QLabel(tr("Pour une nouvelle machine à partir de ce projet, remettre à zéro :"))};
	info_ -> setWordWrap(true);
	layout_ -> addWidget(info_);

	auto date_cb_{new QCheckBox(tr("Date de tous les folios :"))};
	date_cb_ -> setChecked(true);
	auto date_edit_{new QDateEdit(QDate::currentDate())};
	date_edit_ -> setCalendarPopup(true);
	date_edit_ -> setDisplayFormat(QStringLiteral("dd/MM/yyyy"));
	auto date_layout_{new QHBoxLayout};
	date_layout_ -> addWidget(date_cb_);
	date_layout_ -> addWidget(date_edit_);
	date_layout_ -> addStretch();
	layout_ -> addLayout(date_layout_);

	auto revisions_cb_{new QCheckBox(tr("Vider le tableau des indices (A à G : modif, date, dessiné, vérifié)"))};
	revisions_cb_ -> setChecked(true);
	layout_ -> addWidget(revisions_cb_);
	auto indexrev_cb_{new QCheckBox(tr("Remettre l'indice de révision des folios à zéro (vide)"))};
	indexrev_cb_ -> setChecked(true);
	layout_ -> addWidget(indexrev_cb_);
	auto numbers_cb_{new QCheckBox(tr("Vider le numéro de machine et le numéro de document"))};
	numbers_cb_ -> setChecked(true);
	layout_ -> addWidget(numbers_cb_);

	auto buttons_{new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel)};
	connect(buttons_, &QDialogButtonBox::accepted, &dialog_, &QDialog::accept);
	connect(buttons_, &QDialogButtonBox::rejected, &dialog_, &QDialog::reject);
	layout_ -> addWidget(buttons_);

	if (dialog_.exec() != QDialog::Accepted) {
		return false;
	}

		//Project variables
	const auto old_variables_{project -> projectProperties()};
	auto new_variables_{old_variables_};
	QStringList cleared_;
	if (revisions_cb_ -> isChecked()) {
		for (const auto &index : {"a", "b", "c", "d", "e", "f", "g"}) {
			for (const auto &field : {"modif", "date", "dessine", "verifie"}) {
				cleared_ << QString(index) + QLatin1Char('-') + QString(field);
			}
		}
	}
	if (numbers_cb_ -> isChecked()) {
		cleared_ << QStringLiteral("nummachine") << QStringLiteral("document-n");
	}
	for (const auto &key : qAsConst(cleared_)) {
		if (new_variables_.contains(key)) {
			new_variables_.addValue(key, QString());
		}
	}
	if (new_variables_ != old_variables_) {
		project -> setProjectProperties(new_variables_);
		project -> applyVariablesToFolios(old_variables_, new_variables_);
	}

		//Revision index of the folios and of the default title block
	if (indexrev_cb_ -> isChecked())
	{
		auto default_tbp_{project -> defaultTitleBlockProperties()};
		default_tbp_.indexrev.clear();
		project -> setDefaultTitleBlockProperties(default_tbp_);
		for (const auto &diagram_ : project -> diagrams())
		{
			auto tbp_{diagram_ -> border_and_titleblock.exportTitleBlock()};
			tbp_.indexrev.clear();
			diagram_ -> border_and_titleblock.importTitleBlock(tbp_);
		}
	}

	if (date_cb_ -> isChecked()) {
		project -> setDateOnAllFolios(date_edit_ -> date());
	}

	project -> setModified(true);
	return true;
}
