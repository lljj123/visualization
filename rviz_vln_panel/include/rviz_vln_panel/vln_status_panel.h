#ifndef RVIZ_VLN_PANEL_VLN_STATUS_PANEL_H
#define RVIZ_VLN_PANEL_VLN_STATUS_PANEL_H

#include <ros/ros.h>
#include <rviz/panel.h>
#include <std_msgs/String.h>

#include <QString>

class QLabel;
class QPlainTextEdit;

namespace rviz_vln_panel
{

class VlnStatusPanel : public rviz::Panel
{
  Q_OBJECT

public:
  explicit VlnStatusPanel(QWidget* parent = nullptr);

Q_SIGNALS:
  void actionMessage(const QString& payload);

private Q_SLOTS:
  void dockAtBottom();
  void updateAction(const QString& payload);

private:
  void actionCallback(const std_msgs::String::ConstPtr& message);

  ros::NodeHandle node_handle_;
  ros::Subscriber action_subscriber_;

  QPlainTextEdit* instruction_editor_;
  QLabel* step_value_;
  QLabel* action_value_;
  unsigned long step_count_;
  int dock_attempts_;
};

}  // namespace rviz_vln_panel

#endif  // RVIZ_VLN_PANEL_VLN_STATUS_PANEL_H
