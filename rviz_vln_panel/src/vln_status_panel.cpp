#include <rviz_vln_panel/vln_status_panel.h>

#include <pluginlib/class_list_macros.h>

#include <QFrame>
#include <QHBoxLayout>
#include <QDockWidget>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QLabel>
#include <QMainWindow>
#include <QPlainTextEdit>
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
      "color: #172554; font-size: 16px; font-weight: 700;");
  return label;
}

QFrame* makeCard(QWidget* parent)
{
  QFrame* card = new QFrame(parent);
  card->setObjectName("vlnCard");
  card->setFrameShape(QFrame::NoFrame);
  card->setStyleSheet(
      "QFrame#vlnCard { background: #ffffff; border: 1px solid #dbe5f0; "
      "border-radius: 10px; }");
  return card;
}

QFrame* makeStatusCard(
    const QString& title,
    const QString& initial_value,
    QLabel** value_label,
    QWidget* parent)
{
  QFrame* card = makeCard(parent);

  QVBoxLayout* layout = new QVBoxLayout(card);
  layout->setContentsMargins(16, 14, 16, 14);
  layout->setSpacing(10);

  QLabel* title_label = makeSectionTitle(title, card);
  *value_label = new QLabel(initial_value, card);
  (*value_label)->setAlignment(Qt::AlignCenter);
  (*value_label)->setMinimumHeight(100);
  (*value_label)->setStyleSheet(
      "background: #f5f8fc; border: 1px solid #e5edf6; border-radius: 8px; "
      "color: #0f172a; font-size: 25px; font-weight: 800;");

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
  item->setMinimumHeight(33);
  item->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  QHBoxLayout* layout = new QHBoxLayout(item);
  layout->setContentsMargins(0, 2, 2, 2);
  layout->setSpacing(9);

  QFrame* swatch = new QFrame(item);
  QString style = "background: " + color + "; border: 1px solid #64748b;";
  if (shape == "line")
  {
    swatch->setFixedSize(34, 6);
    style += " border-radius: 2px;";
  }
  else
  {
    swatch->setFixedSize(22, 22);
    style += shape == "circle" ? " border-radius: 11px;" : " border-radius: 3px;";
  }
  swatch->setStyleSheet(style);

  QWidget* swatch_slot = new QWidget(item);
  swatch_slot->setFixedSize(38, 26);
  QHBoxLayout* swatch_layout = new QHBoxLayout(swatch_slot);
  swatch_layout->setContentsMargins(0, 0, 0, 0);
  swatch_layout->addWidget(swatch, 0, Qt::AlignCenter);

  QLabel* label = new QLabel(text, item);
  label->setStyleSheet(
      "color: #172554; font-size: 15px; font-weight: 600;");
  label->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

  layout->addWidget(swatch_slot, 0, Qt::AlignVCenter);
  layout->addWidget(label, 1, Qt::AlignVCenter);
  return item;
}

}  // namespace

namespace rviz_vln_panel
{

VlnStatusPanel::VlnStatusPanel(QWidget* parent)
  : rviz::Panel(parent)
  , instruction_editor_(nullptr)
  , step_value_(nullptr)
  , action_value_(nullptr)
  , step_count_(0)
  , dock_attempts_(0)
{
  setMinimumSize(980, 240);
  setStyleSheet("rviz_vln_panel--VlnStatusPanel { background: #eef2f7; }");

  QHBoxLayout* root_layout = new QHBoxLayout(this);
  root_layout->setContentsMargins(12, 12, 12, 12);
  root_layout->setSpacing(12);

  QFrame* instruction_card = makeCard(this);
  QVBoxLayout* instruction_layout = new QVBoxLayout(instruction_card);
  instruction_layout->setContentsMargins(16, 14, 16, 14);
  instruction_layout->setSpacing(10);
  QLabel* instruction_title = makeSectionTitle("INSTRUCTION", instruction_card);
  instruction_title->setStyleSheet(
      "color: #0f172a; font-size: 18px; font-weight: 800;");
  instruction_layout->addWidget(instruction_title);

  instruction_editor_ = new QPlainTextEdit(instruction_card);
  instruction_editor_->setPlaceholderText(
      QString::fromUtf8("在这里输入导航指令…"));
  instruction_editor_->setMinimumHeight(100);
  instruction_editor_->setStyleSheet(
      "QPlainTextEdit { background: #f5f8fc; border: 1px solid #e5edf6; "
      "border-radius: 8px; padding: 12px; color: #0f172a; "
      "font-size: 17px; font-weight: 600; selection-background-color: #bfdbfe; }");
  instruction_layout->addWidget(instruction_editor_, 1);
  root_layout->addWidget(instruction_card, 7);

  QFrame* step_card =
      makeStatusCard(QString::fromUtf8("当前步数"), "0", &step_value_, this);
  step_value_->setStyleSheet(
      "background: #f5f8fc; border: 1px solid #e5edf6; border-radius: 8px; "
      "color: #1267e5; font-size: 42px; font-weight: 800;");
  root_layout->addWidget(step_card, 2);

  QFrame* action_card =
      makeStatusCard("ACTION", "WAITING", &action_value_, this);
  root_layout->addWidget(action_card, 3);

  QFrame* legend_card = makeCard(this);
  legend_card->setMinimumWidth(430);
  QVBoxLayout* legend_layout = new QVBoxLayout(legend_card);
  legend_layout->setContentsMargins(14, 12, 14, 12);
  legend_layout->setSpacing(0);

  QVBoxLayout* left_legend = new QVBoxLayout();
  left_legend->setContentsMargins(0, 0, 0, 0);
  left_legend->setSpacing(1);
  left_legend->addWidget(
      makeLegendItem(QString::fromUtf8("自由区域"), "#ffffff", "square", legend_card));
  left_legend->addWidget(
      makeLegendItem(QString::fromUtf8("障碍物"), "#111827", "square", legend_card));
  left_legend->addWidget(
      makeLegendItem(QString::fromUtf8("未知区域"), "#808080", "square", legend_card));
  left_legend->addWidget(
      makeLegendItem(QString::fromUtf8("起点"), "#0d59ff", "circle", legend_card));

  QVBoxLayout* right_legend = new QVBoxLayout();
  right_legend->setContentsMargins(0, 0, 0, 0);
  right_legend->setSpacing(1);
  right_legend->addWidget(
      makeLegendItem(QString::fromUtf8("中间航点"), "#19d926", "square", legend_card));
  right_legend->addWidget(
      makeLegendItem(QString::fromUtf8("终点"), "#ff140d", "circle", legend_card));
  right_legend->addWidget(
      makeLegendItem(QString::fromUtf8("航点连线"), "#19d926", "line", legend_card));
  right_legend->addWidget(
      makeLegendItem(QString::fromUtf8("当前轨迹"), "#1f45b5", "line", legend_card));

  QFrame* legend_divider = new QFrame(legend_card);
  legend_divider->setFrameShape(QFrame::VLine);
  legend_divider->setFrameShadow(QFrame::Plain);
  legend_divider->setStyleSheet("color: #dbe5f0;");

  QHBoxLayout* legend_columns = new QHBoxLayout();
  legend_columns->setContentsMargins(0, 0, 0, 0);
  legend_columns->setSpacing(14);
  legend_columns->addLayout(left_legend, 1);
  legend_columns->addWidget(legend_divider);
  legend_columns->addLayout(right_legend, 1);
  legend_layout->addStretch(1);
  legend_layout->addLayout(legend_columns, 1);
  legend_layout->addStretch(1);
  root_layout->addWidget(legend_card, 3);

  connect(
      this,
      &VlnStatusPanel::actionMessage,
      this,
      &VlnStatusPanel::updateAction,
      Qt::QueuedConnection);

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
    dock_widget->setMinimumWidth(980);
    dock_widget->setMinimumHeight(240);
    return;
  }

  ++dock_attempts_;
  if (dock_attempts_ < 20)
  {
    QTimer::singleShot(100, this, &VlnStatusPanel::dockAtBottom);
  }
}

void VlnStatusPanel::actionCallback(const std_msgs::String::ConstPtr& message)
{
  Q_EMIT actionMessage(QString::fromStdString(message->data));
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
