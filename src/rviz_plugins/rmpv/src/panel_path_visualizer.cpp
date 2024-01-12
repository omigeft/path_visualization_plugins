/***********************************************************
*
* @file: panel_path_visualizer.h
* @breif: Contains panel of path visualizer class
* @author: Wu Maojia, Yang Haodong
* @update: 2024-1-12
* @version: 1.0
*
* Copyright (c) 2024， Yang Haodong, Wu Maojia
* All rights reserved.
* --------------------------------------------------------
*
**********************************************************/
#include "include/panel_path_visualizer.h"

namespace rmpv
{
/**
 * @brief Construct a new PanelPathVisualizer object
 */
PanelPathVisualizer::PanelPathVisualizer(rviz::Panel* parent, Ui::RMPV* ui)
  : parent_(parent), ui_(ui), core_(new CorePathVisualizer)
{
  core_->setupROS();
  setupUi();
}

/**
* @brief Destroy the PanelPathVisualizer object
 */
PanelPathVisualizer::~PanelPathVisualizer()
{
  if (ui_)
    delete ui_;
  if (core_)
    delete core_;
  if (table_model_)
    delete table_model_;
}

/**
* @brief User interface parameters initialization
 */
void PanelPathVisualizer::setupUi()
{
  table_model_ = new QStandardItemModel(parent_);
  table_header_ = QStringList({ "Select", "Planner", "Start", "Goal", "Length", "Turning Angle (rad)", "Color", "Remove" });
  table_model_->setHorizontalHeaderLabels(table_header_);
  ui_->tableView_path_list->setModel(table_model_);
  ui_->tableView_path_list->setItemDelegateForColumn(0, &selectDelegate_);
  _updateTableView();

  for (const auto& p_name : core_->planner_list_)
    ui_->comboBox_path_add_planner_global->addItem(QString::fromStdString(p_name));

  _onValueChanged();

  connect(core_, SIGNAL(valueChanged()), this, SLOT(_onValueChanged()));
  connect(ui_->pushButton_path_add_add, SIGNAL(clicked()), this, SLOT(_onClicked()));
  connect(ui_->pushButton_path_files_load, SIGNAL(clicked()), this, SLOT(_onClicked()));
  connect(ui_->pushButton_path_files_save, SIGNAL(clicked()), this, SLOT(_onClicked()));
  connect(ui_->lineEdit_path_add_start_x, SIGNAL(editingFinished()), this, SLOT(_onEditingFinished()));
  connect(ui_->lineEdit_path_add_start_y, SIGNAL(editingFinished()), this, SLOT(_onEditingFinished()));
  connect(ui_->lineEdit_path_add_start_yaw, SIGNAL(editingFinished()), this, SLOT(_onEditingFinished()));
  connect(ui_->lineEdit_path_add_goal_x, SIGNAL(editingFinished()), this, SLOT(_onEditingFinished()));
  connect(ui_->lineEdit_path_add_goal_y, SIGNAL(editingFinished()), this, SLOT(_onEditingFinished()));
  connect(ui_->lineEdit_path_add_goal_yaw, SIGNAL(editingFinished()), this, SLOT(_onEditingFinished()));
  connect(&selectDelegate_, SIGNAL(selectChanged(const int&, const bool&)), this, SLOT(_onSelectChanged(const int&, const bool&)));
}

/**
*  @brief if clicked signal from pushButton is received, call this slot function
 */
void PanelPathVisualizer::_onClicked()
{
  QPushButton* senderPushButton = qobject_cast<QPushButton*>(sender());

  if (senderPushButton)
  {
    // get the button name
    QString senderName = senderPushButton->objectName();

    // regular expression to match button names
    QRegularExpression re_remove("pushButton_path_list_remove_(\\d+)");
    QRegularExpressionMatch match_remove = re_remove.match(senderName);

    if (senderName == QString::fromUtf8("pushButton_path_add_add"))
    {
      core_->addPath(ui_->comboBox_path_add_planner_global->currentText());
      _updateTableView();
    }
    else if (senderName == QString::fromUtf8("pushButton_path_files_load"))
    {
      QString open_dir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
      QStringList open_files = QFileDialog::getOpenFileNames(parent_, QStringLiteral("Select Path Files"), open_dir,
                                                             "JSON Files(*.json)", nullptr, QFileDialog::DontResolveSymlinks);
      for (const auto& open_file : open_files)
        core_->loadPaths(open_file);
      _updateTableView();
    }
    else if (senderName == QString::fromUtf8("pushButton_path_files_save"))
    {
      QString save_dir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + QString("/path.json");
      QString save_file = QFileDialog::getSaveFileName(parent_, QStringLiteral("Save Path File"), save_dir,
                                                       "JSON Files(*.json)", nullptr, QFileDialog::DontResolveSymlinks);
      if (!save_file.isEmpty())
        core_->savePaths(save_file);
    }
    else if (match_remove.hasMatch())
    {
      QString capturedText = match_remove.captured(1);
      bool ok = false;
      int index = capturedText.toInt(&ok);
      if (ok)
      {
        core_->removePath(index);
        _updateTableView();
      }
      else
      {
        ROS_ERROR("Failed to get the path index to remove!");
        return;
      };
    }
    else
    {
      ROS_ERROR("Unknown signal sender QPushButton.");
      return;
    }
  }
  else
    ROS_ERROR("Failed to get signal sender QPushButton.");
}

/**
*  @brief if editing finished signal from lineEdit is received, call this slot function
 */
void PanelPathVisualizer::_onEditingFinished()
{
  // get the signal sender
  QLineEdit* senderLineEdit = qobject_cast<QLineEdit*>(sender());

  if (senderLineEdit)
  {
    QString senderName = senderLineEdit->objectName();
    QString text = senderLineEdit->text();
    bool ok;
    double value = text.toDouble(&ok);
    double* valueToChange;

    if (senderName == QString::fromUtf8("lineEdit_path_add_start_x"))
      valueToChange = &(core_->start_.x);
    else if (senderName == QString::fromUtf8("lineEdit_path_add_start_y"))
      valueToChange = &(core_->start_.y);
    else if (senderName == QString::fromUtf8("lineEdit_path_add_start_yaw"))
      valueToChange = &(core_->start_.yaw);
    else if (senderName == QString::fromUtf8("lineEdit_path_add_goal_x"))
      valueToChange = &(core_->goal_.x);
    else if (senderName == QString::fromUtf8("lineEdit_path_add_goal_y"))
      valueToChange = &(core_->goal_.y);
    else if (senderName == QString::fromUtf8("lineEdit_path_add_goal_yaw"))
      valueToChange = &(core_->goal_.yaw);
    else
    {
      ROS_ERROR("Unknown signal sender QLineEdit.");
      return;
    }

    if (ok)
    {
      *valueToChange = value;
      core_->refresh_poses();
    }
    senderLineEdit->setText(QString::number(*valueToChange, 'f', 3));
  }
  else
    ROS_ERROR("Failed to get signal sender QLineEdit.");
}

/**
 *  @brief if color changed signal from colorEditor is received, call this slot function
 *  @param index  row index of path
 *  @param color  color of path
 */
void PanelPathVisualizer::_onColorChanged(const int &index, const QColor &color)
{
  core_->setPathColor(index, color);
}

/**
 *  @brief if select changed signal from selectDelegate is received, call this slot function
 *  @param index  row index of path
 *  @param checked  if path is selected
 */
void PanelPathVisualizer::_onSelectChanged(const int &index, const bool &checked)
{
  core_->setPathSelectStatus(index, checked);
}

/**
*  @brief if value changed signal from core is received, call this slot function
 */
void PanelPathVisualizer::_onValueChanged()
{
  ui_->lineEdit_path_add_start_x->setText(QString::number(core_->start_.x, 'f', 3));
  ui_->lineEdit_path_add_start_y->setText(QString::number(core_->start_.y, 'f', 3));
  ui_->lineEdit_path_add_start_yaw->setText(QString::number(core_->start_.yaw, 'f', 3));
  ui_->lineEdit_path_add_goal_x->setText(QString::number(core_->goal_.x, 'f', 3));
  ui_->lineEdit_path_add_goal_y->setText(QString::number(core_->goal_.y, 'f', 3));
  ui_->lineEdit_path_add_goal_yaw->setText(QString::number(core_->goal_.yaw, 'f', 3));
}

/**
 *  @brief update the table_model_ to update the table view of Path List
 */
void PanelPathVisualizer::_updateTableView()
{
  // initialize table model
  table_model_->clear();
  table_model_->setHorizontalHeaderLabels(table_header_);
  table_model_->setHeaderData(0, Qt::Horizontal, "Only the selected paths will be displayed or saved.", Qt::ToolTipRole);
  table_model_->setHeaderData(1, Qt::Horizontal, "Planner used to plan the path", Qt::ToolTipRole);
  table_model_->setHeaderData(2, Qt::Horizontal, "Start pose of the path", Qt::ToolTipRole);
  table_model_->setHeaderData(3, Qt::Horizontal, "Goal pose of the path", Qt::ToolTipRole);
  table_model_->setHeaderData(4, Qt::Horizontal, "Total length of the path", Qt::ToolTipRole);
  table_model_->setHeaderData(5, Qt::Horizontal, "Total turning angle of the path", Qt::ToolTipRole);
  table_model_->setHeaderData(6, Qt::Horizontal, "Color of the path displayed in rviz", Qt::ToolTipRole);
  table_model_->setHeaderData(7, Qt::Horizontal, "Remove the path from path list", Qt::ToolTipRole);
  table_model_->setRowCount(core_->path_list_->size());

  int row = 0;
  for (const auto& info : *(core_->path_list_->getListPtr()))
  {
    // 0th column: select status
    table_model_->setData(table_model_->index(row, 0), info.getData(PathInfo::selectStatus), Qt::UserRole);

    // 1st column: planner name
    table_model_->setItem(row, 1, new QStandardItem(info.getData(PathInfo::plannerName).toString()));

    // 2nd column: start pose
    table_model_->setItem(row, 2, new QStandardItem(QString("(%1,%2)")
                                                        .arg(QString::number(info.getData(PathInfo::startPoseX).toDouble(), 'f', 3))
                                                        .arg(QString::number(info.getData(PathInfo::startPoseY).toDouble(), 'f', 3))
                                                        .arg(QString::number(info.getData(PathInfo::startPoseYaw).toDouble(), 'f', 3))
                                                        ));

    // 3rd column: goal pose
    table_model_->setItem(row, 3, new QStandardItem(QString("(%1,%2)")
                                                        .arg(QString::number(info.getData(PathInfo::goalPoseX).toDouble(), 'f', 3))
                                                        .arg(QString::number(info.getData(PathInfo::goalPoseY).toDouble(), 'f', 3))
                                                        .arg(QString::number(info.getData(PathInfo::goalPoseYaw).toDouble(), 'f', 3))
                                                        ));

    // 4th column: path length
    table_model_->setItem(row, 4, new QStandardItem(QString::number(info.getData(PathInfo::pathLength).toDouble(), 'f', 3)));

    // 5th column: path turning angle
    double turning_angle_rad = info.getData(PathInfo::turningAngle).toDouble();
    table_model_->setItem(row, 5, new QStandardItem(QString::number(turning_angle_rad, 'f', 3)));

    // 6th column: color
    ColorEditor* colorEditor_list_color = new ColorEditor(row, info.getData(PathInfo::pathColor).value<QColor>(), parent_);
    ui_->tableView_path_list->setIndexWidget(table_model_->index(row, 6), colorEditor_list_color);
    connect(colorEditor_list_color, SIGNAL(colorChanged(const int&, const QColor&)), this,
            SLOT(_onColorChanged(const int&, const QColor&)));

    // 7th column: remove button
    QPushButton* pushButton_list_remove = new QPushButton(parent_);
    pushButton_list_remove->setObjectName(QString::fromUtf8("pushButton_path_list_remove_%1").arg(QString::number(row)));
    pushButton_list_remove->setIcon(QIcon(":/icons/cross.png"));
    pushButton_list_remove->setIconSize(QSize(20, 20));
    ui_->tableView_path_list->setIndexWidget(table_model_->index(row, 7), pushButton_list_remove);
    connect(pushButton_list_remove, SIGNAL(clicked()), this, SLOT(_onClicked()));

    ++row;
  }
  ui_->tableView_path_list->resizeColumnsToContents();
  ui_->tableView_path_list->resizeRowsToContents();
}
}  // namespace rmpv