#include <rviz_vln_panel/vln_status_panel.h>

#include <pluginlib/class_list_macros.h>

#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QDockWidget>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QLabel>
#include <QMainWindow>
#include <QSizePolicy>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

namespace
{

QLabel* makeSectionTitle(const QString& text, QWidget* parent)
{
  QLabel* label = new QLabel(text, parent);
  label->setStyleSheet(
      "color: #334155; font-size: 13px; font-weight: 700;");
  return label;
}

QFrame* makeStatusCard(
    const QString& title,
    const QString& initial_value,
    QLabel** value_label,
    QWidget* parent)
{
  QFrame* card = new QFrame(parent);
  card->setFrameShape(QFrame::NoFrame);
  card->setStyleSheet(
      "QFrame { background: #f8fafc; border: 1px solid #dbe3ec; "
      "border-radius: 8px; }");

  QVBoxLayout* layout = new QVBoxLayout(card);
  layout->setContentsMargins(14, 10, 14, 10);
  layout->setSpacing(5);

  QLabel* title_label = makeSectionTitle(title, card);
  *value_label = new QLabel(initial_value, card);
  (*value_label)->setAlignment(Qt::AlignCenter);
  (*value_label)->setMinimumHeight(40);
  (*value_label)->setStyleSheet(
      "background: transparent; border: none; color: #0f172a; "
      "font-size: 23px; font-weight: 800;");

  layout->addWidget(title_label);
  layout->addWidget(*value_label, 1);
  return card;
}

QWidget* makeLegendItem(
    const QString& text,
    const QString& color,
    const QString& shape,
    QWidget* parent)
{
  QWidget* item = new QWidget(parent);
  QHBoxLayout* layout = new QHBoxLayout(item);
  layout->setContentsMargins(0, 2, 6, 2);
  layout->setSpacing(7);

  QFrame* swatch = new QFrame(item);
  QString style = "background: " + color + "; border: 1px solid #64748b;";
  if (shape == "line")
  {
    swatch->setFixedSize(28, 5);
    style += " border-radius: 2px;";
  }
  else
  {
    swatch->setFixedSize(18, 18);
    style += shape == "circle" ? " border-radius: 9px;" : " border-radius: 2px;";
  }
  swatch->setStyleSheet(style);

  QLabel* label = new QLabel(text, item);
  label->setStyleSheet("color: #1e293b; font-size: 12px;");
  label->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

  layout->addWidget(swatch, 0, Qt::AlignVCenter);
  layout->addWidget(label, 1, Qt::AlignVCenter);
  return item;
}

}  // namespace

namespace rviz_vln_panel
{

VlnStatusPanel::VlnStatusPanel(QWidget* parent)
  : rviz::Panel(parent)
  , instruction_value_(nullptr)
  , step_value_(nullptr)
  , action_value_(nullptr)
  , step_count_(0)
  , dock_attempts_(0)
{
  setMinimumSize(720, 245);
  setStyleSheet("rviz_vln_panel--VlnStatusPanel { background: #eef2f7; }");

  QVBoxLayout* root_layout = new QVBoxLayout(this);
  root_layout->setContentsMargins(14, 12, 14, 12);
  root_layout->setSpacing(10);

  QFrame* instruction_card = new QFrame(this);
  instruction_card->setStyleSheet(
      "QFrame { background: #ffffff; border: 1px solid #dbe3ec; "
      "border-radius: 8px; }");
  QVBoxLayout* instruction_layout = new QVBoxLayout(instruction_card);
  instruction_layout->setContentsMargins(14, 10, 14, 10);
  instruction_layout->setSpacing(6);
  instruction_layout->addWidget(makeSectionTitle("VLN INSTRUCTION", instruction_card));

  instruction_value_ = new QLabel(
      QString::fromUtf8("等待 /vln/instruction ..."), instruction_card);
  instruction_value_->setWordWrap(true);
  instruction_value_->setAlignment(Qt::AlignLeft | Qt::AlignTop);
  instruction_value_->setTextInteractionFlags(Qt::TextSelectableByMouse);
  instruction_value_->setMinimumHeight(42);
  instruction_value_->setStyleSheet(
      "background: transparent; border: none; color: #0f172a; "
      "font-size: 16px; font-weight: 650;");
  instruction_layout->addWidget(instruction_value_);
  root_layout->addWidget(instruction_card);

  QHBoxLayout* status_layout = new QHBoxLayout();
  status_layout->setContentsMargins(0, 0, 0, 0);
  status_layout->setSpacing(10);
  status_layout->addWidget(
      makeStatusCard(QString::fromUtf8("当前步数"), "0", &step_value_, this), 1);
  status_layout->addWidget(
      makeStatusCard("CURRENT ACTION", "WAITING", &action_value_, this), 2);
  root_layout->addLayout(status_layout);

  QFrame* legend_card = new QFrame(this);
  legend_card->setStyleSheet(
      "QFrame { background: #ffffff; border: 1px solid #dbe3ec; "
      "border-radius: 8px; }");
  QVBoxLayout* legend_layout = new QVBoxLayout(legend_card);
  legend_layout->setContentsMargins(14, 8, 14, 8);
  legend_layout->setSpacing(5);
  legend_layout->addWidget(
      makeSectionTitle(QString::fromUtf8("地图与导航图例"), legend_card));

  QGridLayout* legend_grid = new QGridLayout();
  legend_grid->setContentsMargins(0, 0, 0, 0);
  legend_grid->setHorizontalSpacing(12);
  legend_grid->setVerticalSpacing(3);
  legend_grid->addWidget(
      makeLegendItem(QString::fromUtf8("自由区域"), "#ffffff", "square", legend_card), 0, 0);
  legend_grid->addWidget(
      makeLegendItem(QString::fromUtf8("障碍物"), "#111827", "square", legend_card), 0, 1);
  legend_grid->addWidget(
      makeLegendItem(QString::fromUtf8("未知区域"), "#808080", "square", legend_card), 0, 2);
  legend_grid->addWidget(
      makeLegendItem(QString::fromUtf8("起点"), "#0d59ff", "circle", legend_card), 0, 3);
  legend_grid->addWidget(
      makeLegendItem(QString::fromUtf8("中间航点"), "#19d926", "square", legend_card), 1, 0);
  legend_grid->addWidget(
      makeLegendItem(QString::fromUtf8("终点"), "#ff140d", "circle", legend_card), 1, 1);
  legend_grid->addWidget(
      makeLegendItem(QString::fromUtf8("航点连线"), "#19d926", "line", legend_card), 1, 2);
  legend_grid->addWidget(
      makeLegendItem(QString::fromUtf8("当前轨迹"), "#1f45b5", "line", legend_card), 1, 3);
  legend_layout->addLayout(legend_grid);
  root_layout->addWidget(legend_card);

  connect(
      this,
      &VlnStatusPanel::instructionMessage,
      this,
      &VlnStatusPanel::updateInstruction,
      Qt::QueuedConnection);
  connect(
      this,
      &VlnStatusPanel::actionMessage,
      this,
      &VlnStatusPanel::updateAction,
      Qt::QueuedConnection);

  instruction_subscriber_ = node_handle_.subscribe(
      "/vln/instruction", 1, &VlnStatusPanel::instructionCallback, this);
  action_subscriber_ = node_handle_.subscribe(
      "/vln/action", 10, &VlnStatusPanel::actionCallback, this);

  setActionStyle(QString());
  QTimer::singleShot(0, this, &VlnStatusPanel::dockAtBottom);
}

void VlnStatusPanel::dockAtBottom()
{
  QDockWidget* dock_widget = nullptr;
  QWidget* ancestor = parentWidget();
  while (ancestor != nullptr)
  {
    dock_widget = qobject_cast<QDockWidget*>(ancestor);
    if (dock_widget != nullptr)
    {
      break;
    }
    ancestor = ancestor->parentWidget();
  }

  QMainWindow* main_window =
      dock_widget == nullptr ? nullptr : qobject_cast<QMainWindow*>(dock_widget->window());
  if (dock_widget != nullptr && main_window != nullptr)
  {
    main_window->addDockWidget(Qt::BottomDockWidgetArea, dock_widget);
    dock_widget->setMinimumWidth(720);
    dock_widget->setMinimumHeight(245);
    return;
  }

  ++dock_attempts_;
  if (dock_attempts_ < 20)
  {
    QTimer::singleShot(100, this, &VlnStatusPanel::dockAtBottom);
  }
}

void VlnStatusPanel::instructionCallback(const std_msgs::String::ConstPtr& message)
{
  Q_EMIT instructionMessage(QString::fromStdString(message->data));
}

void VlnStatusPanel::actionCallback(const std_msgs::String::ConstPtr& message)
{
  Q_EMIT actionMessage(QString::fromStdString(message->data));
}

void VlnStatusPanel::updateInstruction(const QString& instruction)
{
  const QString clean_instruction = instruction.trimmed();
  instruction_value_->setText(
      clean_instruction.isEmpty() ? QString::fromUtf8("当前指令为空") : clean_instruction);
}

void VlnStatusPanel::updateAction(const QString& payload)
{
  const QString clean_payload = payload.trimmed();
  if (clean_payload.isEmpty())
  {
    action_value_->setText("INVALID ACTION");
    action_value_->setStyleSheet(
        "background: #b91c1c; border: none; border-radius: 6px; "
        "color: white; font-size: 19px; font-weight: 800;");
    return;
  }

  QString action = clean_payload;
  if (clean_payload.startsWith('{'))
  {
    QJsonParseError parse_error;
    const QJsonDocument document =
        QJsonDocument::fromJson(clean_payload.toUtf8(), &parse_error);
    if (parse_error.error != QJsonParseError::NoError || !document.isObject())
    {
      action_value_->setText("INVALID ACTION");
      action_value_->setStyleSheet(
          "background: #b91c1c; border: none; border-radius: 6px; "
          "color: white; font-size: 19px; font-weight: 800;");
      return;
    }
    action = document.object().value("action").toString().trimmed();
  }

  action = action.toUpper();
  if (action.isEmpty())
  {
    action_value_->setText("INVALID ACTION");
    action_value_->setStyleSheet(
        "background: #b91c1c; border: none; border-radius: 6px; "
        "color: white; font-size: 19px; font-weight: 800;");
    return;
  }

  ++step_count_;
  step_value_->setText(QString::number(step_count_));
  action_value_->setText(action);
  setActionStyle(action);
}

void VlnStatusPanel::setActionStyle(const QString& action)
{
  QString color = "#64748b";
  if (action == "MOVE_FORWARD")
  {
    color = "#2563eb";
  }
  else if (action == "TURN_LEFT" || action == "TURN_RIGHT")
  {
    color = "#d97706";
  }
  else if (action == "STOP")
  {
    color = "#dc2626";
  }

  action_value_->setStyleSheet(
      "background: " + color +
      "; border: none; border-radius: 6px; color: white; "
      "font-size: 19px; font-weight: 800;");
}

}  // namespace rviz_vln_panel

PLUGINLIB_EXPORT_CLASS(rviz_vln_panel::VlnStatusPanel, rviz::Panel)
