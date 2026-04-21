//
// Created by 58226 on 2023/11/21.
//

#include "recordingwindow.h"
#include "dialogs/ui_recordingwindow.h"
#include "Fluent/FluentCard.h"
#include "Fluent/FluentButton.h"
#include "Fluent/FluentFlowLayout.h"
#include "Fluent/FluentIconButton.h"
#include "Fluent/FluentLabel.h"
#include "Fluent/FluentNavigationView.h"
#include "Fluent/FluentScrollArea.h"
#include "Fluent/FluentSlider.h"
#include "Fluent/FluentStyle.h"
#include "Fluent/FluentTheme.h"
#include "Fluent/FluentWidget.h"
#include "dialogs/preview/videorenderer.h"
#include "QtAVPlayer/qavaudiooutput.h"
#include "QtAVPlayer/qavplayer.h"
#include "QtAVPlayer/qavvideoframe.h"
#include <QFileDialog>
#include <QFrame>
#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <functional>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QListView>
#include <QLineEdit>
#include <QMenu>
#include <QMouseEvent>
#include <QPropertyAnimation>
#include <QResizeEvent>
#include <QSignalBlocker>
#include <QVariant>
#include <QScreen>
#include <QSplitter>
#include <QStackedWidget>
#include <QStackedLayout>
#include <QStandardItemModel>
#include <QStringListModel>
#include <QStyledItemDelegate>
#include <QTcpSocket>
#include <QUrl>
#include <QVBoxLayout>
#ifdef _WIN32
#else
#include <X11/Xlib.h>
#endif
#include <QThread>
#include "usermessagebox.h"
#include "backgroundwindow.h"
#include "countdowndialog.h"
#include "optionnalchain.h"
#include "signalproxy.h"
#define SET_POP_VIEW(styledItem) \
    if (!qobject_cast<Fluent::FluentComboBox*>(ui->styledItem)) \
    { \
        ui->styledItem->setView(new QListView(ui->styledItem)); \
    }


enum ScreenArea
{
    DeskTop = 0,
    Customized = 1
};

const QString ScreenAreaStr[2] = {u8"全屏", u8"自定义"};

using NavigationItem = Fluent::FluentNavigationItem;

constexpr auto kOverviewMirrorButtonProperty = "overviewMirrorButton";
constexpr auto kOverviewMirrorPrimaryProperty = "overviewMirrorPrimary";
constexpr auto kOverviewExpandedProperty = "overviewExpanded";
constexpr auto kOverviewPendingRestoreOrderProperty = "overviewPendingRestoreOrder";

NavigationItem makeNavigationItem(const QString& key, const QString& text, const ushort glyphCode)
{
    NavigationItem item;
    item.key = key;
    item.text = text;
    item.iconGlyph = QString(QChar(glyphCode));
    item.iconFontFamily = QStringLiteral("Segoe Fluent Icons");
    return item;
}

void detachWidgetFromLayout(QLayout* parentLayout, QWidget* widget)
{
    if (parentLayout == nullptr || widget == nullptr)
    {
        return;
    }

    parentLayout->removeWidget(widget);
}

void detachLayoutFromLayout(QLayout* parentLayout, QLayout* childLayout)
{
    if (parentLayout == nullptr || childLayout == nullptr)
    {
        return;
    }

    for (int index = 0; index < parentLayout->count(); ++index)
    {
        if (parentLayout->itemAt(index) == childLayout)
        {
            parentLayout->takeAt(index);
            return;
        }
    }
}

void resetRowMargins(QLayout* layout)
{
    if (layout == nullptr)
    {
        return;
    }

    layout->setContentsMargins(0, 0, 0, 0);
}

QWidget* rebuildOverviewContentLayout(QWidget* contentWidget,
                                     QVBoxLayout* rootLayout,
                                     QLayout* actionLayout,
                                     const QList<QLayout*>& controlLayouts,
                                     const int horizontalSpacing = 14,
                                     const int controlSpacing = 8)
{
    if (contentWidget == nullptr || rootLayout == nullptr || actionLayout == nullptr)
    {
        return contentWidget;
    }

    detachLayoutFromLayout(rootLayout, actionLayout);
    for (auto* controlLayout : controlLayouts)
    {
        detachLayoutFromLayout(rootLayout, controlLayout);
    }

    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    auto* compactHost = new QWidget(contentWidget);
    compactHost->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    auto* compactLayout = new QHBoxLayout(compactHost);
    compactLayout->setContentsMargins(0, 0, 0, 0);
    compactLayout->setSpacing(horizontalSpacing);

    auto* actionColumn = new QVBoxLayout();
    actionColumn->setContentsMargins(0, 0, 0, 0);
    actionColumn->setSpacing(0);
    actionColumn->addLayout(actionLayout);
    actionColumn->addStretch(1);
    compactLayout->addLayout(actionColumn);

    auto* controlsHost = new QWidget(compactHost);
    controlsHost->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    auto* controlsLayout = new QVBoxLayout(controlsHost);
    controlsLayout->setContentsMargins(0, 0, 0, 0);
    controlsLayout->setSpacing(controlSpacing);

    for (auto* controlLayout : controlLayouts)
    {
        if (controlLayout != nullptr)
        {
            controlsLayout->addLayout(controlLayout);
        }
    }

    compactLayout->addWidget(controlsHost, 1, Qt::AlignTop);
    rootLayout->addWidget(compactHost);
    return contentWidget;
}

class OverviewCardClickFilter : public QObject
{
public:
    explicit OverviewCardClickFilter(const std::function<void()>& onClick, QObject* parent = nullptr)
        : QObject(parent)
        , m_onClick(onClick)
    {
    }

protected:
    bool eventFilter(QObject* watched, QEvent* event) override
    {
        if (event->type() == QEvent::MouseButtonRelease)
        {
            auto* mouseEvent = static_cast<QMouseEvent*>(event);
            if (mouseEvent->button() == Qt::LeftButton && m_onClick)
            {
                m_onClick();
                return true;
            }
        }

        return QObject::eventFilter(watched, event);
    }

private:
    std::function<void()> m_onClick;
};

QWidget* createPageIntro(const QString& title,
                         const QString& hint,
                         QWidget* parent)
{
    auto* introHost = new QWidget(parent);
    auto* introLayout = new QVBoxLayout(introHost);
    introLayout->setContentsMargins(4, 0, 4, 4);
    introLayout->setSpacing(4);

    auto* titleLabel = new Fluent::FluentLabel(title, introHost);
    titleLabel->setStyleSheet("font-size: 28px; font-weight: 700;");
    introLayout->addWidget(titleLabel);

    if (!hint.isEmpty())
    {
        auto* hintLabel = new Fluent::FluentLabel(hint, introHost);
        hintLabel->setWordWrap(true);
        hintLabel->setStyleSheet("font-size: 12px; opacity: 0.82;");
        introLayout->addWidget(hintLabel);
    }

    return introHost;
}

void syncOverviewMirrorButton(QPushButton* source)
{
    if (source == nullptr)
    {
        return;
    }

    auto* mirrorObject = source->property(kOverviewMirrorButtonProperty).value<QObject*>();
    auto* mirrorButton = qobject_cast<QPushButton*>(mirrorObject);
    if (mirrorButton == nullptr)
    {
        return;
    }

    mirrorButton->setStyleSheet(QString());
    mirrorButton->setText(source->text());
    mirrorButton->setIcon(source->icon());
    mirrorButton->setIconSize(source->iconSize());
    mirrorButton->setToolTip(source->toolTip());
    mirrorButton->setEnabled(source->isEnabled());
    mirrorButton->setFlat(false);

    if (auto* fluentMirror = qobject_cast<Fluent::FluentButton*>(mirrorButton))
    {
        fluentMirror->setPrimary(source->property(kOverviewMirrorPrimaryProperty).toBool());
    }
}

void syncOverviewMirrorButtons(Ui::RecordingWindow* ui)
{
    if (ui == nullptr)
    {
        return;
    }

    syncOverviewMirrorButton(ui->RecordingButton);
    syncOverviewMirrorButton(ui->areaButton);
    syncOverviewMirrorButton(ui->micphoneButton);
    syncOverviewMirrorButton(ui->playerButton);
}

Fluent::FluentCard* createSectionCard(const QString& title,
                                      const QString& subtitle,
                                      QWidget* content,
                                      QWidget* parent,
                                      const int minimumWidth = 0,
                                      const bool fullRow = false)
{
    auto* card = new Fluent::FluentCard(parent);
    auto* cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(18, 16, 18, 18);
    cardLayout->setSpacing(12);

    auto* titleLabel = new Fluent::FluentLabel(title, card);
    titleLabel->setStyleSheet("font-size: 14px; font-weight: 650;");
    cardLayout->addWidget(titleLabel);

    if (!subtitle.isEmpty())
    {
        auto* subtitleLabel = new Fluent::FluentLabel(subtitle, card);
        subtitleLabel->setWordWrap(true);
        subtitleLabel->setStyleSheet("font-size: 12px; opacity: 0.78;");
        cardLayout->addWidget(subtitleLabel);
    }

    if (content != nullptr)
    {
        content->setParent(card);
        cardLayout->addWidget(content);
        content->show();
    }

    if (minimumWidth > 0)
    {
        card->setMinimumWidth(minimumWidth);
    }
    card->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);

    if (fullRow)
    {
        card->setProperty(Fluent::FluentFlowLayout::kFullRowProperty, true);
    }

    return card;
}

struct ExpandableOverviewCard
{
    Fluent::FluentCard* card = nullptr;
    QWidget* summaryHost = nullptr;
    QPushButton* summaryActionButton = nullptr;
    QWidget* bodyHost = nullptr;
    QPropertyAnimation* bodyAnimation = nullptr;
    QList<QWidget*> clickTargets;
};

ExpandableOverviewCard createExpandableOverviewCard(const QString& title,
                                                    QWidget* content,
                                                    QPushButton* sourceActionButton,
                                                    const QSize& summaryButtonSize,
                                                    const QSize& summaryIconSize,
                                                    const bool primaryAction,
                                                    QWidget* parent)
{
    ExpandableOverviewCard parts;
    parts.card = new Fluent::FluentCard(parent);
    parts.card->setMinimumWidth(140);
    parts.card->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    parts.card->setProperty(Fluent::FluentFlowLayout::kFullRowProperty, false);
    parts.card->setProperty(Fluent::FluentFlowLayout::kDisableAnimationProperty, true);
    parts.card->setProperty(kOverviewExpandedProperty, false);
    parts.card->setProperty(kOverviewPendingRestoreOrderProperty, false);

    auto* cardLayout = new QVBoxLayout(parts.card);
    cardLayout->setContentsMargins(14, 12, 14, 14);
    cardLayout->setSpacing(8);

    auto* titleLabel = new Fluent::FluentLabel(title, parts.card);
    titleLabel->setStyleSheet("font-size: 14px; font-weight: 650;");
    cardLayout->addWidget(titleLabel);

    parts.summaryHost = new QWidget(parts.card);
    parts.summaryHost->setFixedHeight(summaryButtonSize.height() + 8);
    auto* summaryLayout = new QVBoxLayout(parts.summaryHost);
    summaryLayout->setContentsMargins(0, 4, 0, 2);
    summaryLayout->setSpacing(0);

    auto* summaryActionButton = new Fluent::FluentIconButton(parts.summaryHost);
    summaryActionButton->setFixedSize(summaryButtonSize);
    summaryActionButton->setButtonExtent(summaryButtonSize.width());
    summaryActionButton->setIconSize(summaryIconSize);
    summaryActionButton->setFocusPolicy(Qt::NoFocus);
    summaryLayout->addWidget(summaryActionButton, 0, Qt::AlignHCenter);
    parts.summaryActionButton = summaryActionButton;
    cardLayout->addWidget(parts.summaryHost);

    if (sourceActionButton != nullptr)
    {
        sourceActionButton->setProperty(kOverviewMirrorButtonProperty,
                                        QVariant::fromValue(static_cast<QObject*>(summaryActionButton)));
        sourceActionButton->setProperty(kOverviewMirrorPrimaryProperty, primaryAction);
        syncOverviewMirrorButton(sourceActionButton);
        QObject::connect(summaryActionButton, &QPushButton::clicked, sourceActionButton, &QPushButton::click);
        sourceActionButton->show();
    }

    parts.bodyHost = new QWidget(parts.card);
    auto* bodyLayout = new QVBoxLayout(parts.bodyHost);
    bodyLayout->setContentsMargins(0, 8, 0, 0);
    bodyLayout->setSpacing(0);

    if (content != nullptr)
    {
        content->setParent(parts.bodyHost);
        bodyLayout->addWidget(content);
        content->show();
    }

    parts.bodyHost->setMaximumHeight(0);
    parts.bodyHost->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    cardLayout->addWidget(parts.bodyHost);

    parts.bodyAnimation = new QPropertyAnimation(parts.bodyHost, "maximumHeight", parts.card);
    parts.bodyAnimation->setDuration(180);
    parts.bodyAnimation->setEasingCurve(QEasingCurve::InOutCubic);

    parts.clickTargets = {parts.card, titleLabel, parts.summaryHost};
    return parts;
}

QWidget* createNavigationHeader(QWidget* parent)
{
    auto* headerHost = new QWidget(parent);
    auto* headerLayout = new QVBoxLayout(headerHost);
    headerLayout->setContentsMargins(4, 4, 4, 8);
    headerLayout->setSpacing(0);

    auto* titleLabel = new Fluent::FluentLabel(u8"录制工作台", headerHost);
    titleLabel->setStyleSheet("font-size: 16px; font-weight: 700;");

    headerLayout->addWidget(titleLabel);
    return headerHost;
}

QString iconPath(const QString& iconName)
{
    return QString(":/icons/images/%1.svg").arg(iconName);
}

void syncCountDownControls(Ui::RecordingWindow* ui)
{
    if (ui == nullptr)
    {
        return;
    }

    const bool enabled = ui->countDownCheckBox->isChecked() && ui->countDownCheckBox->isEnabled();
    ui->countDownEdit->setEnabled(enabled);
    ui->label_10->setEnabled(enabled);
}

void configureFluentButton(QPushButton* button,
                           const QString& text,
                           const QString& iconName,
                           const QSize& iconSize,
                           const bool primary)
{
    if (button == nullptr)
    {
        return;
    }

    button->setText(text);
    button->setIcon(iconName.isEmpty() ? QIcon() : QIcon(iconPath(iconName)));
    button->setStyleSheet(QString());
    if (iconSize.isValid())
    {
        button->setIconSize(iconSize);
    }
    button->setAutoDefault(false);
    button->setDefault(false);
    button->setFlat(false);

    if (auto* fluentButton = qobject_cast<Fluent::FluentButton*>(button))
    {
        fluentButton->setPrimary(primary);
    }

    syncOverviewMirrorButton(button);
}

void configureTileButton(QPushButton* button, const QString& iconName)
{
    configureFluentButton(button, QString(), iconName, QSize(54, 54), false);
}

void configureRecordingButton(QPushButton* button, const QString& iconName)
{
    configureFluentButton(button, QString(), iconName, QSize(44, 44), true);
}

QString recordingWindowStyleSheet()
{
    const auto& colors = Fluent::ThemeManager::instance().colors();

    return QString(
        "QWidget#centralwidget,"
        "QWidget#AreaWidgets,"
        "QWidget#ScreenAreaWidget,"
        "QWidget#MicphoneWidget,"
        "QWidget#PlayerWidget,"
        "QWidget#RecordingWidget,"
        "QWidget#SettingExpandWidget,"
        "QWidget#SettingWidget,"
        "QFrame#placeholder,"
        "QFrame#contentFrame {"
        "  background: transparent;"
        "  border: none;"
        "}"
        "QFrame#titleFrame {"
        "  background: transparent;"
        "  border: none;"
        "}"
        "QFrame#line,"
        "#line_1,"
        "#line_2,"
        "#line_3,"
        "#line_4 {"
        "  background-color: %1;"
        "}"
        "QLabel {"
        "  color: %2;"
        "  font-size: 12px;"
        "  font-family: Microsoft YaHei UI;"
        "}"
        "QLabel#statusLabel {"
        "  color: %3;"
        "  font-size: 10px;"
        "}"
        "QLabel#unloginNoticeLabel {"
        "  color: %4;"
        "  font-size: 10px;"
        "}"
        "QCheckBox#settingExpander {"
        "  color: %2;"
        "  font-size: 12px;"
        "  font-family: Microsoft YaHei UI;"
        "}")
        .arg(colors.hover.name())
        .arg(colors.text.name())
        .arg(colors.subText.name())
        .arg(colors.error.name());
}

QString recordingPreviewPageStyleSheet()
{
    const auto& colors = Fluent::ThemeManager::instance().colors();
    const bool dark = Fluent::ThemeManager::instance().themeMode() == Fluent::ThemeManager::ThemeMode::Dark;
    const QColor controlBackground = dark
                                         ? Fluent::Style::mix(colors.surface, colors.hover, 0.32)
                                         : Fluent::Style::mix(colors.surface, colors.hover, 0.88);
    const QColor controlBorder = dark
                                     ? Fluent::Style::mix(colors.border, colors.hover, 0.25)
                                     : Fluent::Style::mix(colors.border, colors.hover, 0.55);
    const QColor historyItemBackground = dark
                                             ? Fluent::Style::mix(colors.surface, colors.hover, 0.12)
                                             : Fluent::Style::mix(colors.surface, colors.hover, 0.52);
    const QColor historyItemSelected = dark
                                           ? Fluent::Style::mix(colors.accent, colors.surface, 0.18)
                                           : Fluent::Style::mix(colors.accent, colors.surface, 0.12);

    return QString(
        "QFrame#previewSurfaceFrame {"
        "  background: #000000;"
        "  border: 1px solid %1;"
        "  border-radius: 12px;"
        "}"
        "QWidget#previewControlPanel {"
        "  background: %2;"
        "  border: 1px solid %3;"
        "  border-radius: 12px;"
        "}"
        "QLabel#previewEmptyStateLabel {"
        "  color: %4;"
        "  font-size: 16px;"
        "  font-weight: 600;"
        "}"
        "QListWidget#previewHistoryList {"
        "  background: transparent;"
        "  border: none;"
        "  outline: 0;"
        "}"
        "QListWidget#previewHistoryList::item {"
        "  background: %5;"
        "  color: %6;"
        "  border: 1px solid %3;"
        "  border-radius: 10px;"
        "  padding: 10px 12px;"
        "  margin: 0 0 8px 0;"
        "}"
        "QListWidget#previewHistoryList::item:selected {"
        "  background: %7;"
        "  color: %8;"
        "  border-color: %9;"
        "}")
        .arg(colors.border.name())
        .arg(controlBackground.name())
        .arg(controlBorder.name())
        .arg(colors.subText.name())
        .arg(historyItemBackground.name())
        .arg(colors.text.name())
        .arg(historyItemSelected.name())
        .arg(colors.text.name())
        .arg(colors.accent.name());
}

void configurePreviewIconButton(QPushButton* button,
                                const QString& iconName,
                                const QString& toolTip,
                                const QSize& iconSize,
                                const bool checkable = false)
{
    if (button == nullptr)
    {
        return;
    }

    button->setStyleSheet(QString());
    button->setText(QString());
    button->setToolTip(toolTip);
    button->setIcon(QIcon(iconPath(iconName)));
    button->setIconSize(iconSize);
    button->setCheckable(checkable);
    button->setCursor(Qt::PointingHandCursor);

    if (auto* iconButton = qobject_cast<Fluent::FluentIconButton*>(button))
    {
        iconButton->setButtonExtent(button->width());
    }
}

void configurePreviewActionButton(QPushButton* button, const QString& text, const QString& iconName, const bool primary)
{
    if (button == nullptr)
    {
        return;
    }

    button->setStyleSheet(QString());
    button->setText(text);
    button->setIcon(iconName.isEmpty() ? QIcon() : QIcon(iconPath(iconName)));
    button->setIconSize(QSize(16, 16));
    button->setCursor(Qt::PointingHandCursor);

    if (auto* fluentButton = qobject_cast<Fluent::FluentButton*>(button))
    {
        fluentButton->setPrimary(primary);
    }
}

class RecordingPreviewPage final : public Fluent::FluentWidget
{
public:
    explicit RecordingPreviewPage(QWidget* parent = nullptr)
        : Fluent::FluentWidget(parent)
    {
        setObjectName("recordingPreviewPage");
        setBackgroundRole(Fluent::FluentWidget::BackgroundRole::Transparent);

        auto* rootLayout = new QVBoxLayout(this);
        rootLayout->setContentsMargins(0, 0, 0, 12);
        rootLayout->setSpacing(14);

        auto* headerRow = new QWidget(this);
        auto* headerLayout = new QHBoxLayout(headerRow);
        headerLayout->setContentsMargins(4, 0, 4, 0);
        headerLayout->setSpacing(12);

        auto* titleLabel = new Fluent::FluentLabel(u8"录屏预览", headerRow);
        titleLabel->setStyleSheet("font-size: 28px; font-weight: 700;");
        headerLayout->addWidget(titleLabel, 1);

        m_historyToggleButton = new Fluent::FluentButton(u8"历史录屏", headerRow);
        configurePreviewActionButton(m_historyToggleButton, u8"历史录屏", QString(), false);
        headerLayout->addWidget(m_historyToggleButton, 0, Qt::AlignRight | Qt::AlignVCenter);
        rootLayout->addWidget(headerRow);

        auto* contentRow = new QHBoxLayout();
        contentRow->setContentsMargins(0, 0, 0, 0);
        contentRow->setSpacing(0);
        rootLayout->addLayout(contentRow, 1);

        auto* historyBody = new QWidget(this);
        auto* historyLayout = new QVBoxLayout(historyBody);
        historyLayout->setContentsMargins(0, 0, 0, 0);
        historyLayout->setSpacing(10);

        m_historySummaryLabel = new Fluent::FluentLabel(u8"当前目录暂无录屏", historyBody);
        m_historySummaryLabel->setStyleSheet("font-size: 12px; opacity: 0.78;");
        historyLayout->addWidget(m_historySummaryLabel);

        m_historyList = new QListWidget(historyBody);
        m_historyList->setObjectName("previewHistoryList");
        m_historyList->setFrameShape(QFrame::NoFrame);
        m_historyList->setSelectionMode(QAbstractItemView::SingleSelection);
        m_historyList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        m_historyList->setWordWrap(true);
        m_historyList->setMinimumHeight(180);
        m_historyList->setMaximumHeight(240);
        historyLayout->addWidget(m_historyList, 1);

        auto* refreshButton = new Fluent::FluentButton(u8"刷新列表", historyBody);
        configurePreviewActionButton(refreshButton, u8"刷新列表", QString(), false);
        historyLayout->addWidget(refreshButton, 0, Qt::AlignLeft);

        m_historyCard = createSectionCard(u8"历史录屏", QString(), historyBody, this, 0, true);
        m_historyCard->setMaximumWidth(380);
        m_historyCard->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);

        m_historyDockHost = new QWidget(this);
        m_historyDockHost->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
        auto* historyDockLayout = new QVBoxLayout(m_historyDockHost);
        historyDockLayout->setContentsMargins(14, 0, 0, 0);
        historyDockLayout->setSpacing(0);
        historyDockLayout->addWidget(m_historyCard, 0, Qt::AlignTop);
        historyDockLayout->addStretch(1);
        m_historyDockHost->hide();
        m_historyDockHost->setMaximumWidth(0);

        m_historyWidthAnimation = new QPropertyAnimation(m_historyDockHost, "maximumWidth", this);
        m_historyWidthAnimation->setDuration(220);
        m_historyWidthAnimation->setEasingCurve(QEasingCurve::OutCubic);
        connect(m_historyWidthAnimation, &QPropertyAnimation::valueChanged, this, [this](const QVariant&) {
            updateGeometry();
            if (parentWidget() != nullptr)
            {
                parentWidget()->updateGeometry();
            }
        });
        connect(m_historyWidthAnimation, &QPropertyAnimation::finished, this, [this]() {
            if (!m_historyVisible && m_historyDockHost != nullptr)
            {
                m_historyDockHost->hide();
                m_historyDockHost->setMaximumWidth(0);
            }

            updateGeometry();
        });

        auto* previewBody = new QWidget(this);
        auto* previewLayout = new QVBoxLayout(previewBody);
        previewLayout->setContentsMargins(0, 0, 0, 0);
        previewLayout->setSpacing(12);

        m_currentFileLabel = new Fluent::FluentLabel(u8"未选择录屏", previewBody);
        m_currentFileLabel->setWordWrap(true);
        m_currentFileLabel->setStyleSheet("font-size: 12px; opacity: 0.82;");
        previewLayout->addWidget(m_currentFileLabel);

        auto* previewSurfaceFrame = new QFrame(previewBody);
        previewSurfaceFrame->setObjectName("previewSurfaceFrame");
        previewSurfaceFrame->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        auto* previewSurfaceLayout = new QVBoxLayout(previewSurfaceFrame);
        previewSurfaceLayout->setContentsMargins(0, 0, 0, 0);

        auto* previewStack = new QStackedLayout();
        previewSurfaceLayout->addLayout(previewStack);

        m_previewWidget = new VideoWidget(previewSurfaceFrame);
        m_previewWidget->setAspectRatioMode(Qt::KeepAspectRatio);
        m_previewWidget->setFullScreen(false);
        previewStack->addWidget(m_previewWidget);

        m_emptyStateLabel = new Fluent::FluentLabel(u8"点击右上角历史录屏后选择文件开始预览", previewSurfaceFrame);
        m_emptyStateLabel->setObjectName("previewEmptyStateLabel");
        m_emptyStateLabel->setAlignment(Qt::AlignCenter);
        previewStack->addWidget(m_emptyStateLabel);
        previewStack->setCurrentWidget(m_emptyStateLabel);
        m_previewStack = previewStack;

        previewLayout->addWidget(previewSurfaceFrame, 1);

        auto* controlPanel = new QWidget(previewBody);
        controlPanel->setObjectName("previewControlPanel");
        auto* controlPanelLayout = new QVBoxLayout(controlPanel);
        controlPanelLayout->setContentsMargins(14, 12, 14, 12);
        controlPanelLayout->setSpacing(10);

        auto* progressRow = new QWidget(controlPanel);
        auto* progressRowLayout = new QHBoxLayout(progressRow);
        progressRowLayout->setContentsMargins(0, 0, 0, 0);
        progressRowLayout->setSpacing(8);

        m_positionLabel = new Fluent::FluentLabel(u8"00:00:00", progressRow);
        progressRowLayout->addWidget(m_positionLabel);
        m_slider = new Fluent::FluentSlider(Qt::Horizontal, progressRow);
        progressRowLayout->addWidget(m_slider, 1);
        m_durationLabel = new Fluent::FluentLabel(u8"00:00:00", progressRow);
        progressRowLayout->addWidget(m_durationLabel);
        controlPanelLayout->addWidget(progressRow);

        auto* actionRow = new QWidget(controlPanel);
        auto* actionRowLayout = new QHBoxLayout(actionRow);
        actionRowLayout->setContentsMargins(0, 0, 0, 0);
        actionRowLayout->setSpacing(8);

        m_prevFrameButton = new Fluent::FluentIconButton(actionRow);
        m_prevFrameButton->setFixedSize(30, 30);
        configurePreviewIconButton(m_prevFrameButton, "preFrame", u8"上一帧", QSize(12, 16));
        actionRowLayout->addWidget(m_prevFrameButton);

        m_playButton = new Fluent::FluentIconButton(actionRow);
        m_playButton->setFixedSize(30, 30);
        configurePreviewIconButton(m_playButton, "play", u8"播放/暂停", QSize(16, 16), true);
        actionRowLayout->addWidget(m_playButton);

        m_nextFrameButton = new Fluent::FluentIconButton(actionRow);
        m_nextFrameButton->setFixedSize(30, 30);
        configurePreviewIconButton(m_nextFrameButton, "nextFrame", u8"下一帧", QSize(12, 16));
        actionRowLayout->addWidget(m_nextFrameButton);

        actionRowLayout->addStretch(1);

        m_openFolderButton = new Fluent::FluentButton(u8"打开文件夹", actionRow);
        configurePreviewActionButton(m_openFolderButton, u8"打开文件夹", "folder", true);
        actionRowLayout->addWidget(m_openFolderButton);
        controlPanelLayout->addWidget(actionRow);

        previewLayout->addWidget(controlPanel);

        auto* previewCard = createSectionCard(u8"当前预览", QString(), previewBody, this, 0, true);
        previewCard->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        contentRow->addWidget(previewCard, 1);
        contentRow->addWidget(m_historyDockHost, 0, Qt::AlignTop);

        m_player = new QAVPlayer;
        m_audioOutput = new QAVAudioOutput;
        m_audioOutput->setVolume(1.0);
        m_renderer = new VideoRenderer;
        m_renderer->setParent(this);
        m_mediaObject = new MediaObject(m_renderer, this);
        m_previewWidget->setMediaObject(m_mediaObject);

        const auto updatePlayButtonIcon = [this](const bool playing) {
            m_playButton->setIcon(QIcon(iconPath(playing ? "pause" : "play")));
            m_playButton->setChecked(playing);
        };
        updatePlayButtonIcon(false);

        connect(&Fluent::ThemeManager::instance(), &Fluent::ThemeManager::themeChanged, this, [this]() {
            setStyleSheet(recordingPreviewPageStyleSheet());
        });
        setStyleSheet(recordingPreviewPageStyleSheet());

        connect(refreshButton, &QPushButton::clicked, this, [this]() {
            refreshHistory();
        });
        connect(m_historyToggleButton, &QPushButton::clicked, this, [this]() {
            setHistoryVisible(!m_historyVisible);
        });
        connect(m_historyList, &QListWidget::currentItemChanged, this, [this](QListWidgetItem* current, QListWidgetItem*) {
            if (current == nullptr)
            {
                return;
            }

            loadRecording(current->data(Qt::UserRole).toString(), true);
        });

        connect(m_slider, &QSlider::valueChanged, this, [this](int value) {
            if (m_isSelfInvokeSliderChange)
            {
                return;
            }

            m_isSelfInvokeSliderChange = true;
            seek(value);
            m_isSelfInvokeSliderChange = false;
        });
        connect(m_slider, &QSlider::sliderPressed, this, [this]() {
            m_ignoreTimerUpdate = true;
        });
        connect(m_slider, &QSlider::sliderReleased, this, [this]() {
            m_ignoreTimerUpdate = false;
        });

        connect(m_playButton, &QPushButton::clicked, this, [this, updatePlayButtonIcon]() {
            if (m_currentFile.isEmpty())
            {
                return;
            }

            if (m_player->state() == QAVPlayer::PlayingState)
            {
                m_player->pause();
                updatePlayButtonIcon(false);
            }
            else
            {
                m_player->play();
                updatePlayButtonIcon(true);
            }
        });
        connect(m_prevFrameButton, &QPushButton::clicked, this, [this]() {
            if (m_currentFile.isEmpty())
            {
                return;
            }
            m_player->paused(m_player->position());
            m_player->stepBackward();
        });
        connect(m_nextFrameButton, &QPushButton::clicked, this, [this]() {
            if (m_currentFile.isEmpty())
            {
                return;
            }
            m_player->paused(m_player->position());
            m_player->stepForward();
        });
        connect(m_openFolderButton, &QPushButton::clicked, this, [this]() {
            openPreviewFolder();
        });

        connect(m_player, &QAVPlayer::audioFrame, this, [&](const QAVAudioFrame& frame) {
            if (m_audioOutput != nullptr)
            {
                m_audioOutput->play(frame);
            }
        }, Qt::DirectConnection);

        connect(m_player, &QAVPlayer::videoFrame, this, [&](const QAVVideoFrame& frame) {
            if (m_renderer == nullptr || m_renderer->m_surface == nullptr || m_onResize)
            {
                return;
            }

            QVideoFrame videoFrame = frame.convertTo(AVPixelFormat::AV_PIX_FMT_RGB32);
            if (!videoFrame.isValid())
            {
                return;
            }

            if (!m_renderer->m_surface->isActive())
            {
                QVideoSurfaceFormat format(videoFrame.size(), videoFrame.pixelFormat(), videoFrame.handleType());
                if (!m_renderer->m_surface->start(format))
                {
                    return;
                }
            }
            else if (m_renderer->m_surface->surfaceFormat().frameSize() != videoFrame.size())
            {
                m_renderer->m_surface->stop();
                QVideoSurfaceFormat format(videoFrame.size(), videoFrame.pixelFormat(), videoFrame.handleType());
                if (!m_renderer->m_surface->start(format))
                {
                    return;
                }
            }

            if (m_renderer->m_surface->isActive())
            {
                m_renderer->m_surface->present(videoFrame);
            }
        }, Qt::DirectConnection);

        connect(m_player, &QAVPlayer::durationChanged, this, [this](const qint64 duration) {
            m_slider->setMaximum(duration);
            m_durationLabel->setText(formatTime(duration));
        });

        const auto updatePosition = [this](const qint64 position) {
            if (m_slider->isSliderDown() || m_isSelfInvokeSliderChange)
            {
                return;
            }

            m_isSelfInvokeSliderChange = true;
            m_slider->setSliderPosition(position);
            m_positionLabel->setText(formatTime(position));
            m_isSelfInvokeSliderChange = false;
        };

        connect(m_player, &QAVPlayer::played, this, updatePosition);
        connect(m_player, &QAVPlayer::paused, this, updatePosition);
        connect(m_player, &QAVPlayer::stepped, this, updatePosition);
        connect(m_player, &QAVPlayer::seeked, this, updatePosition);
        connect(m_player, &QAVPlayer::errorOccurred, this, [](const QAVPlayer::Error error, const QString& text) {
            qDebug() << "preview media play error:" << error << text;
        });

        m_timer.setInterval(100);
        connect(&m_timer, &QTimer::timeout, this, [this, updatePosition]() {
            if (m_ignoreTimerUpdate || m_currentFile.isEmpty())
            {
                return;
            }

            updatePosition(m_player->position());
        });

        setHistoryVisible(false, false);
        clearPreview();
    }

    ~RecordingPreviewPage() override
    {
        m_timer.stop();
        if (m_player != nullptr)
        {
            m_player->stop();
        }
        if (m_audioOutput != nullptr)
        {
            m_audioOutput->stop();
        }
        if (m_renderer != nullptr && m_renderer->m_surface != nullptr && m_renderer->m_surface->isActive())
        {
            m_renderer->m_surface->stop();
        }

        delete m_audioOutput;
        delete m_player;
    }

    void setRecordingDirectory(const QString& directory)
    {
        m_recordingDirectory = directory;
    }

    void refreshHistory(const QString& preferredFile = QString())
    {
        const QString effectiveDirectory = m_recordingDirectory.isEmpty()
                                               ? QFileInfo(preferredFile).absolutePath()
                                               : m_recordingDirectory;
        const QDir dir(effectiveDirectory);

        if (!dir.exists())
        {
            m_historySummaryLabel->setText(u8"当前目录暂无录屏");
            clearPreview();
            QSignalBlocker blocker(m_historyList);
            m_historyList->clear();
            return;
        }

        m_recordingDirectory = dir.absolutePath();

        const QFileInfoList fileInfos = dir.entryInfoList(QStringList() << "*.mp4", QDir::Files | QDir::Readable, QDir::Time);
        m_historySummaryLabel->setText(fileInfos.isEmpty()
                                           ? u8"当前目录暂无录屏"
                                           : QString(u8"共 %1 个录屏文件").arg(fileInfos.size()));

        QSignalBlocker blocker(m_historyList);
        m_historyList->clear();

        int preferredIndex = -1;
        const QString preferredPath = preferredFile.isEmpty() ? QFileInfo(m_currentFile).absoluteFilePath() : QFileInfo(preferredFile).absoluteFilePath();

        for (int index = 0; index < fileInfos.size(); ++index)
        {
            const auto& fileInfo = fileInfos.at(index);
            auto* item = new QListWidgetItem(QString("%1\n%2")
                                                .arg(fileInfo.completeBaseName(), fileInfo.lastModified().toString("yyyy-MM-dd hh:mm:ss")));
            item->setData(Qt::UserRole, fileInfo.absoluteFilePath());
            item->setToolTip(fileInfo.absoluteFilePath());
            item->setSizeHint(QSize(0, 52));
            m_historyList->addItem(item);

            if (!preferredPath.isEmpty() && QFileInfo(preferredPath).absoluteFilePath() == fileInfo.absoluteFilePath())
            {
                preferredIndex = index;
            }
        }

        blocker.unblock();

        if (m_historyList->count() == 0)
        {
            clearPreview();
            return;
        }

        const int targetIndex = preferredIndex >= 0 ? preferredIndex : 0;
        m_historyList->setCurrentRow(targetIndex);
    }

    void selectRecording(const QString& file)
    {
        if (file.isEmpty())
        {
            return;
        }

        const QFileInfo fileInfo(file);
        if (!fileInfo.absolutePath().isEmpty())
        {
            m_recordingDirectory = fileInfo.absolutePath();
        }

        refreshHistory(fileInfo.absoluteFilePath());
    }

    void setHistoryVisible(const bool visible, const bool animated = true)
    {
        if (m_historyCard == nullptr || m_historyDockHost == nullptr || m_historyWidthAnimation == nullptr)
        {
            m_historyVisible = visible;
            updateHistoryToggleButton();
            return;
        }

        m_historyVisible = visible;
        updateHistoryToggleButton();
        const int targetWidth = historyPanelWidth();
        m_historyWidthAnimation->stop();

        if (!animated)
        {
            m_historyDockHost->setVisible(visible);
            m_historyDockHost->setMaximumWidth(visible ? targetWidth : 0);
            updateGeometry();
            return;
        }

        const int currentWidth = qMax(m_historyDockHost->width(), m_historyDockHost->maximumWidth());
        if (visible)
        {
            m_historyDockHost->show();
        }

        m_historyWidthAnimation->setStartValue(visible ? currentWidth : qMax(currentWidth, targetWidth));
        m_historyWidthAnimation->setEndValue(visible ? targetWidth : 0);

        m_historyWidthAnimation->start();

        updateGeometry();
    }

    int historyPanelWidth() const
    {
        if (m_historyCard == nullptr)
        {
            return 0;
        }

        const int availableWidth = qMax(320, width() - 40);
        return qMin(availableWidth, 380);
    }

    void updateHistoryToggleButton()
    {
        if (m_historyToggleButton == nullptr)
        {
            return;
        }

        configurePreviewActionButton(m_historyToggleButton,
                                     m_historyVisible ? u8"收起历史" : u8"历史录屏",
                                     QString(),
                                     m_historyVisible);
    }

    void syncHistoryPanelAfterResize()
    {
        if (m_historyDockHost == nullptr)
        {
            return;
        }

        m_historyDockHost->setMaximumWidth(m_historyVisible ? historyPanelWidth() : 0);

        updateGeometry();
    }

protected:
    void resizeEvent(QResizeEvent* event) override
    {
        m_onResize = true;
        Fluent::FluentWidget::resizeEvent(event);

        if (m_renderer != nullptr && m_renderer->m_surface != nullptr && m_renderer->m_surface->isActive())
        {
            m_renderer->m_surface->stop();
        }

        syncHistoryPanelAfterResize();

        m_onResize = false;
    }

private:
    QString formatTime(const qint64 duration) const
    {
        int s = static_cast<int>(duration / 1000);
        const int m = s / 60;
        const int h = m / 60;
        return QString::asprintf("%02d:%02d:%02d", h, m % 60, s % 60);
    }

    void clearPreview()
    {
        m_timer.stop();
        m_currentFile.clear();
        if (m_player != nullptr)
        {
            m_player->stop();
        }
        if (m_audioOutput != nullptr)
        {
            m_audioOutput->stop();
        }
        m_slider->setMaximum(0);
        m_slider->setValue(0);
        m_positionLabel->setText(u8"00:00:00");
        m_durationLabel->setText(u8"00:00:00");
        m_currentFileLabel->setText(u8"未选择录屏");
        m_openFolderButton->setEnabled(false);
        m_playButton->setChecked(false);
        m_playButton->setIcon(QIcon(iconPath("play")));
        m_previewStack->setCurrentWidget(m_emptyStateLabel);
    }

    void loadRecording(const QString& file, const bool autoPlay)
    {
        if (file.isEmpty() || !QFileInfo::exists(file))
        {
            clearPreview();
            return;
        }

        if (m_currentFile == QFileInfo(file).absoluteFilePath())
        {
            return;
        }

        clearPreview();
        m_currentFile = QFileInfo(file).absoluteFilePath();
        m_currentFileLabel->setText(QFileInfo(file).fileName());
        m_openFolderButton->setEnabled(true);
        m_previewStack->setCurrentWidget(m_previewWidget);

        m_player->setSource(m_currentFile);
        m_player->setSynced(true);
        m_player->setInputOptions(QMap<QString, QString>{{"fastseek", "1"}, {"accurate_seek", "1"}, {"threads", "4"}});

        if (autoPlay)
        {
            m_player->play();
            m_playButton->setChecked(true);
            m_playButton->setIcon(QIcon(iconPath("pause")));
        }

        m_timer.start();
    }

    void seek(const qint64 position)
    {
        if (m_currentFile.isEmpty())
        {
            return;
        }

        m_playButton->setChecked(false);
        m_playButton->setIcon(QIcon(iconPath("play")));
        m_player->pause();
        m_player->seek(position);
    }

    void openPreviewFolder()
    {
        if (m_currentFile.isEmpty())
        {
            return;
        }

        const QString folderPath = QFileInfo(m_currentFile).absolutePath();
        if (folderPath.isEmpty() || !QFileInfo::exists(folderPath))
        {
            UserMessageBox::warning(this, u8"错误", u8"无法定位录屏文件所在文件夹");
            return;
        }

        if (!QDesktopServices::openUrl(QUrl::fromLocalFile(folderPath)))
        {
            UserMessageBox::warning(this, u8"错误", u8"无法打开录屏文件所在文件夹");
        }
    }

private:
    Fluent::FluentCard* m_historyCard = nullptr;
    QWidget* m_historyDockHost = nullptr;
    QListWidget* m_historyList = nullptr;
    Fluent::FluentLabel* m_historySummaryLabel = nullptr;
    Fluent::FluentButton* m_historyToggleButton = nullptr;
    Fluent::FluentLabel* m_currentFileLabel = nullptr;
    Fluent::FluentLabel* m_positionLabel = nullptr;
    Fluent::FluentLabel* m_durationLabel = nullptr;
    Fluent::FluentLabel* m_emptyStateLabel = nullptr;
    QStackedLayout* m_previewStack = nullptr;
    VideoWidget* m_previewWidget = nullptr;
    Fluent::FluentSlider* m_slider = nullptr;
    Fluent::FluentIconButton* m_prevFrameButton = nullptr;
    Fluent::FluentIconButton* m_playButton = nullptr;
    Fluent::FluentIconButton* m_nextFrameButton = nullptr;
    Fluent::FluentButton* m_openFolderButton = nullptr;
    QAVPlayer* m_player = nullptr;
    QAVAudioOutput* m_audioOutput = nullptr;
    VideoRenderer* m_renderer = nullptr;
    MediaObject* m_mediaObject = nullptr;
    QPropertyAnimation* m_historyWidthAnimation = nullptr;
    QTimer m_timer;
    QString m_recordingDirectory;
    QString m_currentFile;
    bool m_historyVisible = false;
    bool m_isSelfInvokeSliderChange = false;
    bool m_ignoreTimerUpdate = false;
    bool m_onResize = false;
};

bool isValidFilePath(const QString& filePath)
{
    QRegularExpression re("[*?<>|]");
    return !filePath.contains(re);
}


RecordingWindow::RecordingWindow(const QString& url, const QString& token, int port, QWidget* parent) :
    Fluent::FluentMainWindow(parent)
    , ui(new Ui::RecordingWindow)
    , m_obs(QSharedPointer<ObsWrapper>(new ObsWrapper()))
    , m_recordHotKey(new QHotkey(this))
    , m_pauseHotKey(new QHotkey(this))
    , m_url(url)
    , m_token(token)
    , m_socketPort(port)
{
    ui->setupUi(this);

    ui->countDownLabel->setCursor(Qt::PointingHandCursor);
    ui->countDownLabel->installEventFilter(this);
    connect(ui->countDownCheckBox, &QCheckBox::toggled, this, [this](bool) {
        syncCountDownControls(ui);
    });
    syncCountDownControls(ui);

    configureTileButton(ui->areaButton, "fullscreen");
    configureTileButton(ui->micphoneButton, "micphone");
    configureTileButton(ui->playerButton, "player");
    configureRecordingButton(ui->RecordingButton, "start");
    configureFluentButton(ui->pathSelectionButton, ui->pathSelectionButton->text(), QString(), QSize(), false);
    setupModernLayout();

    setupFluentWindowChrome();
}

RecordingWindow::~RecordingWindow()
{
    if (m_server)
    {
        m_server->close();
    }
    if (m_miniWindow)
    {
        m_miniWindow->close();
        delete m_miniWindow;
    }

    if (m_backgroundWindow)
    {
        m_backgroundWindow->close();
        delete m_backgroundWindow;
    }

    if (m_test)
    {
        m_test->close();
        delete m_test;
    }

    delete ui;
}

void RecordingWindow::setupModernLayout()
{
    setMinimumSize(820, 600);
    setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
    resize(880, 620);
    ui->unloginNoticeLabel->setVisible(false);
    detachWidgetFromLayout(ui->verticalLayout_2, ui->AreaWidgets);
    detachWidgetFromLayout(ui->verticalLayout_2, ui->SettingExpandWidget);
    detachWidgetFromLayout(ui->verticalLayout_2, ui->SettingWidget);

    detachWidgetFromLayout(ui->horizontalLayout_2, ui->ScreenAreaWidget);
    detachWidgetFromLayout(ui->horizontalLayout_2, ui->MicphoneWidget);
    detachWidgetFromLayout(ui->horizontalLayout_2, ui->PlayerWidget);
    detachWidgetFromLayout(ui->horizontalLayout_2, ui->RecordingWidget);

    detachWidgetFromLayout(ui->horizontalLayout_3, ui->settingExpander);

    detachLayoutFromLayout(ui->verticalLayout_6, ui->horizontalLayout_7);
    detachLayoutFromLayout(ui->verticalLayout_6, ui->horizontalLayout_10);
    detachLayoutFromLayout(ui->verticalLayout_6, ui->horizontalLayout_12);
    detachLayoutFromLayout(ui->verticalLayout_6, ui->horizontalLayout_14);
    detachLayoutFromLayout(ui->verticalLayout_6, ui->horizontalLayout_11);
    detachLayoutFromLayout(ui->verticalLayout_6, ui->horizontalLayout_17);
    detachLayoutFromLayout(ui->verticalLayout_6, ui->horizontalLayout_18);
    detachLayoutFromLayout(ui->verticalLayout_6, ui->horizontalLayout_19);

    resetRowMargins(ui->horizontalLayout_7);
    resetRowMargins(ui->horizontalLayout_10);
    resetRowMargins(ui->horizontalLayout_12);
    resetRowMargins(ui->horizontalLayout_14);
    resetRowMargins(ui->horizontalLayout_11);
    resetRowMargins(ui->horizontalLayout_17);
    resetRowMargins(ui->horizontalLayout_18);
    resetRowMargins(ui->horizontalLayout_19);
    resetRowMargins(ui->horizontalLayout_6);
    resetRowMargins(ui->horizontalLayout_9);
    resetRowMargins(ui->horizontalLayout_15);
    resetRowMargins(ui->horizontalLayout_20);

    rebuildOverviewContentLayout(ui->ScreenAreaWidget,
                                 ui->verticalLayout_3,
                                 ui->horizontalLayout_5,
                                 {ui->horizontalLayout_6, ui->horizontalLayout_4});
    rebuildOverviewContentLayout(ui->MicphoneWidget,
                                 ui->verticalLayout_9,
                                 ui->horizontalLayout_8,
                                 {ui->verticalLayout_4, ui->horizontalLayout_9});
    rebuildOverviewContentLayout(ui->PlayerWidget,
                                 ui->verticalLayout_8,
                                 ui->horizontalLayout_13,
                                 {ui->verticalLayout_5, ui->horizontalLayout_15});
    rebuildOverviewContentLayout(ui->RecordingWidget,
                                 ui->verticalLayout_7,
                                 ui->horizontalLayout_21,
                                 {ui->horizontalLayout_22, ui->horizontalLayout_20});

    ui->verticalLayout_4->setContentsMargins(0, 0, 0, 0);
    ui->verticalLayout_5->setContentsMargins(0, 0, 0, 0);

    ui->ScreenAreaWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    ui->MicphoneWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    ui->PlayerWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    ui->RecordingWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    ui->AreaWidgets->hide();
    ui->SettingWidget->hide();
    ui->SettingExpandWidget->hide();
    ui->settingExpander->setText(u8"隐藏详细设置");
    ui->settingExpander->setChecked(true);

    ui->verticalLayout_2->setContentsMargins(0, 0, 0, 0);
    ui->verticalLayout_2->setSpacing(0);

    auto* shell = new Fluent::FluentWidget(ui->contentFrame);
    shell->setObjectName("recordingShell");
    shell->setBackgroundRole(Fluent::FluentWidget::BackgroundRole::Transparent);

    auto* shellLayout = new QHBoxLayout(shell);
    shellLayout->setContentsMargins(12, 12, 12, 12);
    shellLayout->setSpacing(12);

    m_navigationView = new Fluent::FluentNavigationView(shell);
    m_navigationView->setExpandedWidth(188);
    m_navigationView->setCompactWidth(48);
    m_navigationView->setAutoCollapseWidth(860);
    m_navigationView->setPaneTitle(u8"工作台");
    m_navigationView->setHeaderWidget(createNavigationHeader(m_navigationView));
    m_navigationView->addItem(makeNavigationItem("overview", u8"概览", 0xE80F));
    m_navigationView->addFooterItem(makeNavigationItem("preview", u8"录屏预览", 0xE8B2));
    m_navigationView->addFooterItem(makeNavigationItem("settings", u8"录制参数", 0xE713));

    m_contentStack = new QStackedWidget(shell);
    m_contentStack->setObjectName("recordingContentStack");
    m_contentStack->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    auto configureScrollArea = [](Fluent::FluentScrollArea* area) {
        area->setWidgetResizable(true);
        area->setFrameShape(QFrame::NoFrame);
        area->setOverlayScrollBarsEnabled(true);
        area->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    };

    auto* overviewScrollArea = new Fluent::FluentScrollArea(shell);
    configureScrollArea(overviewScrollArea);

    auto* overviewContent = new Fluent::FluentWidget(overviewScrollArea);
    overviewContent->setObjectName("overviewContent");
    overviewContent->setBackgroundRole(Fluent::FluentWidget::BackgroundRole::Transparent);
    overviewScrollArea->setWidget(overviewContent);

    auto* overviewLayout = new QVBoxLayout(overviewContent);
    overviewLayout->setContentsMargins(0, 0, 0, 12);
    overviewLayout->setSpacing(12);
    overviewLayout->addWidget(createPageIntro(u8"概览", QString(), overviewContent));

    auto* overviewTilesHost = new QWidget(overviewContent);
    auto* overviewRootLayout = new QVBoxLayout(overviewTilesHost);
    overviewRootLayout->setContentsMargins(0, 0, 0, 0);
    overviewRootLayout->setSpacing(12);

    auto* overviewCollapsedHost = new QWidget(overviewTilesHost);
    auto* overviewCollapsedLayout = new Fluent::FluentFlowLayout(overviewCollapsedHost, 0, 12, 12);
    overviewCollapsedLayout->setUniformItemWidthEnabled(true);
    overviewCollapsedLayout->setMinimumItemWidth(140);
    overviewCollapsedLayout->setAnimationEnabled(false);
    overviewRootLayout->addWidget(overviewCollapsedHost);

    auto* overviewExpandedHost = new QWidget(overviewTilesHost);
    overviewExpandedHost->hide();
    auto* overviewExpandedLayout = new QHBoxLayout(overviewExpandedHost);
    overviewExpandedLayout->setContentsMargins(0, 0, 0, 0);
    overviewExpandedLayout->setSpacing(12);

    auto* overviewExpandedPrimaryHost = new QWidget(overviewExpandedHost);
    auto* overviewExpandedPrimaryLayout = new QVBoxLayout(overviewExpandedPrimaryHost);
    overviewExpandedPrimaryLayout->setContentsMargins(0, 0, 0, 0);
    overviewExpandedPrimaryLayout->setSpacing(0);
    overviewExpandedLayout->addWidget(overviewExpandedPrimaryHost, 1);

    auto* overviewExpandedSecondaryHost = new QWidget(overviewExpandedHost);
    overviewExpandedSecondaryHost->setMinimumWidth(208);
    overviewExpandedSecondaryHost->setMaximumWidth(240);
    auto* overviewExpandedSecondaryLayout = new QVBoxLayout(overviewExpandedSecondaryHost);
    overviewExpandedSecondaryLayout->setContentsMargins(0, 0, 0, 0);
    overviewExpandedSecondaryLayout->setSpacing(12);
    overviewExpandedLayout->addWidget(overviewExpandedSecondaryHost);

    overviewRootLayout->addWidget(overviewExpandedHost);

    const auto recordingOverviewCard = createExpandableOverviewCard(u8"开始录制",
                                                                    ui->RecordingWidget,
                                                                    ui->RecordingButton,
                                                                    QSize(60, 60),
                                                                    QSize(44, 44),
                                                                    true,
                                                                    overviewTilesHost);
    const auto areaOverviewCard = createExpandableOverviewCard(u8"录制区域",
                                                               ui->ScreenAreaWidget,
                                                               ui->areaButton,
                                                               QSize(60, 60),
                                                               QSize(54, 54),
                                                               false,
                                                               overviewTilesHost);
    const auto micOverviewCard = createExpandableOverviewCard(u8"麦克风",
                                                              ui->MicphoneWidget,
                                                              ui->micphoneButton,
                                                              QSize(60, 60),
                                                              QSize(54, 54),
                                                              false,
                                                              overviewTilesHost);
    const auto playerOverviewCard = createExpandableOverviewCard(u8"系统声音",
                                                                 ui->PlayerWidget,
                                                                 ui->playerButton,
                                                                 QSize(60, 60),
                                                                 QSize(54, 54),
                                                                 false,
                                                                 overviewTilesHost);

    const QList<ExpandableOverviewCard> overviewCards {
        recordingOverviewCard,
        areaOverviewCard,
        micOverviewCard,
        playerOverviewCard
    };
    const QList<Fluent::FluentCard*> overviewDefaultOrder {
        recordingOverviewCard.card,
        areaOverviewCard.card,
        micOverviewCard.card,
        playerOverviewCard.card
    };

    const auto clearOverviewLayout = [](QLayout* layout) {
        while (layout != nullptr && layout->count() > 0)
        {
            delete layout->takeAt(0);
        }
    };

    const auto syncOverviewLayoutGeometry = [overviewCollapsedLayout, overviewTilesHost, overviewContent, overviewScrollArea]() {
        overviewCollapsedLayout->invalidate();
        overviewCollapsedLayout->activate();
        overviewTilesHost->updateGeometry();
        overviewTilesHost->adjustSize();
        overviewContent->updateGeometry();

        if (auto* widget = overviewScrollArea->widget(); widget != nullptr)
        {
            widget->updateGeometry();
            widget->adjustSize();
        }
    };

    const auto showCollapsedOverview = [overviewDefaultOrder,
                                        overviewCollapsedHost,
                                        overviewCollapsedLayout,
                                        overviewExpandedHost,
                                        overviewExpandedPrimaryLayout,
                                        overviewExpandedSecondaryLayout,
                                        clearOverviewLayout,
                                        syncOverviewLayoutGeometry]() {
        clearOverviewLayout(overviewCollapsedLayout);
        clearOverviewLayout(overviewExpandedPrimaryLayout);
        clearOverviewLayout(overviewExpandedSecondaryLayout);
        overviewExpandedHost->hide();
        overviewCollapsedHost->show();

        for (auto* card : overviewDefaultOrder)
        {
            overviewCollapsedLayout->addWidget(card);
        }

        syncOverviewLayoutGeometry();
    };

    const auto showExpandedOverview = [overviewDefaultOrder,
                                       overviewCollapsedLayout,
                                       overviewCollapsedHost,
                                       overviewExpandedHost,
                                       overviewExpandedPrimaryLayout,
                                       overviewExpandedSecondaryLayout,
                                       clearOverviewLayout,
                                       syncOverviewLayoutGeometry](Fluent::FluentCard* selectedCard) {
        clearOverviewLayout(overviewCollapsedLayout);
        clearOverviewLayout(overviewExpandedPrimaryLayout);
        clearOverviewLayout(overviewExpandedSecondaryLayout);
        overviewCollapsedHost->hide();
        overviewExpandedHost->show();

        overviewExpandedPrimaryLayout->addWidget(selectedCard);
        overviewExpandedPrimaryLayout->addStretch(1);

        for (auto* card : overviewDefaultOrder)
        {
            if (card != selectedCard)
            {
                overviewExpandedSecondaryLayout->addWidget(card);
            }
        }

        overviewExpandedSecondaryLayout->addStretch(1);

        syncOverviewLayoutGeometry();
    };

    const auto collapseCardImmediately = [](const ExpandableOverviewCard& cardParts) {
        cardParts.bodyAnimation->stop();
        cardParts.card->setProperty(kOverviewExpandedProperty, false);
        cardParts.summaryHost->show();
        cardParts.bodyHost->setMaximumHeight(0);
        cardParts.card->updateGeometry();
    };

    const auto expandCardAnimated = [syncOverviewLayoutGeometry](const ExpandableOverviewCard& cardParts) {
        cardParts.bodyAnimation->stop();
        cardParts.card->setProperty(kOverviewExpandedProperty, true);
        cardParts.summaryHost->hide();

        const int targetHeight = qMax(cardParts.bodyHost->layout()->sizeHint().height(), cardParts.bodyHost->sizeHint().height());
        cardParts.bodyHost->setMaximumHeight(0);
        cardParts.bodyAnimation->setStartValue(0);
        cardParts.bodyAnimation->setEndValue(targetHeight);
        cardParts.bodyAnimation->start();
        syncOverviewLayoutGeometry();
    };

    const auto collapseCardAnimated = [syncOverviewLayoutGeometry](const ExpandableOverviewCard& cardParts) {
        cardParts.bodyAnimation->stop();
        cardParts.card->setProperty(kOverviewExpandedProperty, false);
        cardParts.summaryHost->show();

        const int startHeight = qMax(cardParts.bodyHost->height(), cardParts.bodyHost->layout()->sizeHint().height());
        cardParts.bodyHost->setMaximumHeight(startHeight);
        cardParts.bodyAnimation->setStartValue(startHeight);
        cardParts.bodyAnimation->setEndValue(0);
        cardParts.bodyAnimation->start();
        syncOverviewLayoutGeometry();
    };

    for (const auto& cardParts : overviewCards)
    {
        QObject::connect(cardParts.bodyAnimation, &QPropertyAnimation::valueChanged, cardParts.card,
                         [card = cardParts.card, syncOverviewLayoutGeometry](const QVariant&) {
                             card->updateGeometry();
                             syncOverviewLayoutGeometry();
                         });
        QObject::connect(cardParts.bodyAnimation, &QPropertyAnimation::finished, cardParts.card,
                         [cardParts, syncOverviewLayoutGeometry]() {
                             if (cardParts.card->property(kOverviewExpandedProperty).toBool())
                             {
                                 cardParts.bodyHost->setMaximumHeight(QWIDGETSIZE_MAX);
                             }

                             syncOverviewLayoutGeometry();
                         });
    }

    const auto setExpandedOverviewCard = [overviewCards,
                                          showCollapsedOverview,
                                          showExpandedOverview,
                                          syncOverviewLayoutGeometry,
                                          collapseCardImmediately,
                                          expandCardAnimated,
                                          collapseCardAnimated](Fluent::FluentCard* expandedCard) {
        Fluent::FluentCard* currentExpandedCard = nullptr;
        for (const auto& cardParts : overviewCards)
        {
            if (cardParts.card->property(kOverviewExpandedProperty).toBool())
            {
                currentExpandedCard = cardParts.card;
                break;
            }
        }

        if (expandedCard == nullptr)
        {
            if (currentExpandedCard == nullptr)
            {
                return;
            }

            for (const auto& cardParts : overviewCards)
            {
                if (cardParts.card == currentExpandedCard)
                {
                    collapseCardAnimated(cardParts);
                    showCollapsedOverview();
                    return;
                }
            }
        }

        for (const auto& cardParts : overviewCards)
        {
            if (cardParts.card != expandedCard)
            {
                collapseCardImmediately(cardParts);
            }
        }

        showExpandedOverview(expandedCard);

        for (const auto& cardParts : overviewCards)
        {
            const bool expanded = cardParts.card == expandedCard;
            if (!expanded)
            {
                cardParts.card->setProperty(kOverviewExpandedProperty, false);
            }
        }

        for (const auto& cardParts : overviewCards)
        {
            if (cardParts.card == expandedCard)
            {
                expandCardAnimated(cardParts);
                break;
            }
        }

        syncOverviewLayoutGeometry();
    };

    for (const auto& cardParts : overviewCards)
    {
        auto* clickFilter = new OverviewCardClickFilter([setExpandedOverviewCard, card = cardParts.card]() {
            setExpandedOverviewCard(card->property(kOverviewExpandedProperty).toBool() ? nullptr : card);
        }, cardParts.card);

        for (auto* clickTarget : cardParts.clickTargets)
        {
            clickTarget->installEventFilter(clickFilter);
            clickTarget->setCursor(Qt::PointingHandCursor);
        }
    }

    for (const auto& cardParts : overviewCards)
    {
        collapseCardImmediately(cardParts);
    }
    showCollapsedOverview();
    syncOverviewMirrorButtons(ui);
    overviewLayout->addWidget(overviewTilesHost);
    overviewLayout->addStretch(1);

    m_previewPage = new RecordingPreviewPage(shell);
    m_previewPage->setRecordingDirectory(ui->savePathEdit->text());
    m_previewPage->refreshHistory();
    connect(ui->savePathEdit, &QLineEdit::textChanged, this, [this](const QString& path) {
        if (m_previewPage != nullptr)
        {
            m_previewPage->setRecordingDirectory(path);
        }
    });

    auto* settingsScrollArea = new Fluent::FluentScrollArea(shell);
    configureScrollArea(settingsScrollArea);

    auto* settingsContent = new Fluent::FluentWidget(settingsScrollArea);
    settingsContent->setObjectName("settingsContent");
    settingsContent->setBackgroundRole(Fluent::FluentWidget::BackgroundRole::Transparent);
    settingsScrollArea->setWidget(settingsContent);

    auto* settingsLayout = new QVBoxLayout(settingsContent);
    settingsLayout->setContentsMargins(0, 0, 0, 12);
    settingsLayout->setSpacing(14);
    settingsLayout->addWidget(createPageIntro(u8"录制参数", u8"展开后直接修改输出、快捷键和计时。", settingsContent));

    const int settingsLabelWidth = 88;
    for (auto* label : {ui->label_2, ui->label_4, ui->label_5, ui->label_6, ui->label_7, ui->label_8, ui->label_11, ui->countDownLabel})
    {
        label->setMinimumWidth(settingsLabelWidth);
        label->setMaximumWidth(settingsLabelWidth);
        label->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    }

    auto* toggleCard = createSectionCard(u8"详细配置", u8"展开后编辑输出、快捷键和计时。", nullptr, settingsContent, 0, true);
    auto* toggleCardLayout = static_cast<QVBoxLayout*>(toggleCard->layout());
    auto* toggleRow = new QWidget(toggleCard);
    auto* toggleRowLayout = new QHBoxLayout(toggleRow);
    toggleRowLayout->setContentsMargins(0, 0, 0, 0);
    toggleRowLayout->setSpacing(10);

    toggleRowLayout->addStretch(1);
    toggleRowLayout->addWidget(ui->settingExpander, 0, Qt::AlignRight | Qt::AlignVCenter);
    toggleCardLayout->addWidget(toggleRow);
    settingsLayout->addWidget(toggleCard);

    m_settingsPanelHost = new Fluent::FluentWidget(settingsContent);
    m_settingsPanelHost->setObjectName("recordingSettingsPanelHost");
    //m_settingsPanelHost->setBackgroundRole(QPalette::ColorRole::Background);
    m_settingsPanelHost->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);

    auto* settingsCardsLayout = new QVBoxLayout(m_settingsPanelHost);
    settingsCardsLayout->setContentsMargins(0, 0, 0, 0);
    settingsCardsLayout->setSpacing(14);

    auto* outputFields = new QWidget(m_settingsPanelHost);
    auto* outputFieldsLayout = new QVBoxLayout(outputFields);
    outputFieldsLayout->setContentsMargins(0, 0, 0, 0);
    outputFieldsLayout->setSpacing(10);

    auto* outputNameRow = new QWidget(outputFields);
    auto* outputNameRowLayout = new QHBoxLayout(outputNameRow);
    outputNameRowLayout->setContentsMargins(0, 0, 0, 0);
    outputNameRowLayout->setSpacing(10);
    outputNameRowLayout->addWidget(ui->label_2);
    outputNameRowLayout->addWidget(ui->nameEdit, 1);
    outputFieldsLayout->addWidget(outputNameRow);

    auto* bitrateRow = new QWidget(outputFields);
    auto* bitrateRowLayout = new QHBoxLayout(bitrateRow);
    bitrateRowLayout->setContentsMargins(0, 0, 0, 0);
    bitrateRowLayout->setSpacing(10);
    bitrateRowLayout->addWidget(ui->label_4);
    bitrateRowLayout->addWidget(ui->bitrateComboBox, 1);
    outputFieldsLayout->addWidget(bitrateRow);

    auto* frameRateRow = new QWidget(outputFields);
    auto* frameRateRowLayout = new QHBoxLayout(frameRateRow);
    frameRateRowLayout->setContentsMargins(0, 0, 0, 0);
    frameRateRowLayout->setSpacing(10);
    frameRateRowLayout->addWidget(ui->label_5);
    frameRateRowLayout->addWidget(ui->frameRateComboBox, 1);
    outputFieldsLayout->addWidget(frameRateRow);

    auto* savePathRow = new QWidget(outputFields);
    auto* savePathRowLayout = new QHBoxLayout(savePathRow);
    savePathRowLayout->setContentsMargins(0, 0, 0, 0);
    savePathRowLayout->setSpacing(10);
    savePathRowLayout->addWidget(ui->label_6);
    savePathRowLayout->addWidget(ui->savePathEdit, 1);
    savePathRowLayout->addWidget(ui->pathSelectionButton);
    outputFieldsLayout->addWidget(savePathRow);

    auto* outputCard = createSectionCard(u8"输出与文件", u8"修改文件名、码率、帧率和保存位置。", outputFields, m_settingsPanelHost, 0, true);
    auto* outputCardLayout = static_cast<QVBoxLayout*>(outputCard->layout());
    outputCardLayout->setSpacing(12);
    settingsCardsLayout->addWidget(outputCard);

    auto* shortcutFields = new QWidget(m_settingsPanelHost);
    auto* shortcutFieldsLayout = new QVBoxLayout(shortcutFields);
    shortcutFieldsLayout->setContentsMargins(0, 0, 0, 0);
    shortcutFieldsLayout->setSpacing(10);

    auto* startShortcutRow = new QWidget(shortcutFields);
    auto* startShortcutRowLayout = new QHBoxLayout(startShortcutRow);
    startShortcutRowLayout->setContentsMargins(0, 0, 0, 0);
    startShortcutRowLayout->setSpacing(10);
    startShortcutRowLayout->addWidget(ui->label_7);
    startShortcutRowLayout->addWidget(ui->StartShortCut, 1);
    shortcutFieldsLayout->addWidget(startShortcutRow);

    auto* pauseShortcutRow = new QWidget(shortcutFields);
    auto* pauseShortcutRowLayout = new QHBoxLayout(pauseShortcutRow);
    pauseShortcutRowLayout->setContentsMargins(0, 0, 0, 0);
    pauseShortcutRowLayout->setSpacing(10);
    pauseShortcutRowLayout->addWidget(ui->label_8);
    pauseShortcutRowLayout->addWidget(ui->PauseShortCut, 1);
    shortcutFieldsLayout->addWidget(pauseShortcutRow);

    auto* shortcutCard = createSectionCard(u8"快捷键", u8"录制前先设置启停和暂停热键。", shortcutFields, m_settingsPanelHost, 0, true);
    auto* shortcutCardLayout = static_cast<QVBoxLayout*>(shortcutCard->layout());
    shortcutCardLayout->setSpacing(12);
    settingsCardsLayout->addWidget(shortcutCard);

    auto* timerFields = new QWidget(m_settingsPanelHost);
    auto* timerFieldsLayout = new QVBoxLayout(timerFields);
    timerFieldsLayout->setContentsMargins(0, 0, 0, 0);
    timerFieldsLayout->setSpacing(10);

    auto* durationRow = new QWidget(timerFields);
    auto* durationRowLayout = new QHBoxLayout(durationRow);
    durationRowLayout->setContentsMargins(0, 0, 0, 0);
    durationRowLayout->setSpacing(10);
    durationRowLayout->addWidget(ui->label_11);
    durationRowLayout->addWidget(ui->m_minuteSpinBox);
    durationRowLayout->addWidget(ui->label_12);
    durationRowLayout->addWidget(ui->m_secondSpinBox);
    durationRowLayout->addWidget(ui->label_13);
    durationRowLayout->addStretch(1);
    timerFieldsLayout->addWidget(durationRow);

    auto* countDownRow = new QWidget(timerFields);
    auto* countDownRowLayout = new QHBoxLayout(countDownRow);
    countDownRowLayout->setContentsMargins(0, 0, 0, 0);
    countDownRowLayout->setSpacing(10);
    countDownRowLayout->addWidget(ui->countDownLabel);
    countDownRowLayout->addWidget(ui->countDownCheckBox);
    countDownRowLayout->addWidget(ui->countDownEdit, 1);
    countDownRowLayout->addWidget(ui->label_10);
    timerFieldsLayout->addWidget(countDownRow);

    auto* timerCard = createSectionCard(u8"计时与倒计时", u8"设置自动停止和开录倒计时。", timerFields, m_settingsPanelHost, 0, true);
    auto* timerCardLayout = static_cast<QVBoxLayout*>(timerCard->layout());
    timerCardLayout->setSpacing(12);
    settingsCardsLayout->addWidget(timerCard);

    settingsLayout->addWidget(m_settingsPanelHost);
    settingsLayout->addStretch(1);

    m_contentStack->addWidget(overviewScrollArea);
    m_contentStack->addWidget(m_previewPage);
    m_contentStack->addWidget(settingsScrollArea);

    shellLayout->addWidget(m_navigationView);
    shellLayout->addWidget(m_contentStack, 1);
    ui->verticalLayout_2->addWidget(shell);

    connect(m_navigationView, &Fluent::FluentNavigationView::selectedKeyChanged, this, &RecordingWindow::switchContentPage);
    m_navigationView->setSelectedKey("overview");
    switchContentPage("overview");
    syncSettingsVisibility(ui->settingExpander->isChecked());
}

void RecordingWindow::syncSettingsVisibility(bool expanded)
{
    if (m_settingsPanelHost != nullptr)
    {
        m_settingsPanelHost->setVisible(expanded);
        m_settingsPanelHost->updateGeometry();

        if (auto* parentWidget = m_settingsPanelHost->parentWidget(); parentWidget != nullptr)
        {
            parentWidget->updateGeometry();
            parentWidget->adjustSize();
        }
    }

    ui->settingExpander->setText(expanded ? u8"隐藏详细设置" : u8"显示详细设置");

    if (m_contentStack != nullptr)
    {
        m_contentStack->currentWidget()->updateGeometry();
    }
}

void RecordingWindow::switchContentPage(const QString& key)
{
    if (m_contentStack == nullptr)
    {
        return;
    }

    if (key == "preview")
    {
        m_contentStack->setCurrentIndex(1);
        if (m_previewPage != nullptr)
        {
            m_previewPage->setRecordingDirectory(ui->savePathEdit->text());
            m_previewPage->refreshHistory();
        }

        if (m_backgroundWindow != nullptr)
        {
            m_backgroundWindow->hide();
        }
        return;
    }

    if (key == "settings")
    {
        m_contentStack->setCurrentIndex(2);
        return;
    }

    m_contentStack->setCurrentIndex(0);

    if (!m_isRecordingStarted && m_backgroundWindow != nullptr)
    {
        m_backgroundWindow->show();
    }
}

void RecordingWindow::setupFluentWindowChrome()
{
    setWindowIcon(QIcon(iconPath("recording")));
    setWindowTitle(u8"录制工作台");
    setFluentTitleBarTitle(u8"录制工作台");
    setFluentTitleBarIcon(QIcon(iconPath("recording")));
    setFluentWindowButtons(Fluent::FluentMainWindow::MinimizeButton | Fluent::FluentMainWindow::CloseButton);
    setFluentResizeEnabled(true);

    ui->titleFrame->hide();
    setStyleSheet(recordingWindowStyleSheet());

    auto* titleBarActionHost = new QWidget(this);
    auto* titleBarActionLayout = new QHBoxLayout(titleBarActionHost);
    titleBarActionLayout->setContentsMargins(0, 0, 0, 0);
    titleBarActionLayout->setSpacing(8);

    auto* themeToggleButton = new Fluent::FluentIconButton(titleBarActionHost);
    themeToggleButton->setFixedSize(30, 30);
    themeToggleButton->setButtonExtent(30);
    themeToggleButton->setIconSize(QSize(16, 16));
    titleBarActionLayout->addWidget(themeToggleButton);

    auto* miniModeButton = new Fluent::FluentButton(u8"迷你窗", titleBarActionHost);
    miniModeButton->setFixedSize(88, 28);
    miniModeButton->setToolTip(u8"切换到微型窗口");
    configureFluentButton(miniModeButton, u8"迷你窗", "minimize", QSize(16, 16), false);
    titleBarActionLayout->addWidget(miniModeButton);
    setFluentTitleBarRightWidget(titleBarActionHost);

    const auto updateThemeChrome = [this, themeToggleButton]() {
        const bool isDark = Fluent::ThemeManager::instance().themeMode() == Fluent::ThemeManager::ThemeMode::Dark;
        configureFluentButton(themeToggleButton,
                              QString(),
                              isDark ? "theme_light" : "theme_dark",
                              QSize(16, 16),
                              false);
        themeToggleButton->setToolTip(isDark ? u8"切换到浅色主题" : u8"切换到深色主题");
        setStyleSheet(recordingWindowStyleSheet());
    };

    updateThemeChrome();

    connect(themeToggleButton, &QPushButton::clicked, this, []() {
        auto& themeManager = Fluent::ThemeManager::instance();
        themeManager.setThemeMode(themeManager.themeMode() == Fluent::ThemeManager::ThemeMode::Dark
                                      ? Fluent::ThemeManager::ThemeMode::Light
                                      : Fluent::ThemeManager::ThemeMode::Dark);
    });
    connect(&Fluent::ThemeManager::instance(), &Fluent::ThemeManager::themeChanged, this, updateThemeChrome);

    connect(miniModeButton, &QPushButton::clicked, this, [this]() {
        this->hide();
        if (m_miniWindow != nullptr)
        {
            m_miniWindow->show();
            m_miniWindow->raise();
            m_miniWindow->activateWindow();
        }
    });
}

bool RecordingWindow::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == ui->countDownLabel && event != nullptr && event->type() == QEvent::MouseButtonRelease)
    {
        const auto* mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->button() == Qt::LeftButton && ui->countDownCheckBox->isEnabled())
        {
            ui->countDownCheckBox->toggle();
            return true;
        }
    }

    return Fluent::FluentMainWindow::eventFilter(watched, event);
}

QScreen* RecordingWindow::findScreen() const
{
    auto curScreens = QGuiApplication::screens();
#ifdef _WIN32
    auto resolutionStr = ui->screenDeviceComboBox->currentText().split("@").last();
    for (int i = 0; i < curScreens.count(); i++)
    {
        auto curScreen = curScreens[i];
        auto curTL = curScreen->geometry().topLeft();

        auto actualResolutionStr = resolutionStr.split("(").first();
        auto xy = actualResolutionStr.split(",");
        bool parseLOk = false;
        bool parseTOk = false;
        auto left = xy.first().toInt(&parseLOk);
        auto top = xy.last().toInt(&parseTOk);
        if (!parseLOk || !parseTOk)
        {
            continue;
        }
        if (curTL.x() == left && curTL.y() == top)
        {
            return curScreen;
        }
    }
    return curScreens.at(ui->screenDeviceComboBox->currentIndex());
#else
    return ExtensionMethods::SourcesExtension<QScreen*>::firstOf(curScreens,[&](const QScreen* curSc)
    {
        return ui->screenDeviceComboBox->currentText().contains(curSc->name());
    },nullptr);
#endif
}


int RecordingWindow::calculateNameSimilarity(const QString& a, const QString& b) const
{
    // 简单的字符串包含匹配
    if (a.contains(b) || b.contains(a))
        return 100; // 完全包含
    return 0; // 不包含
}

void RecordingWindow::resetVideo()
{
    auto curScreen = findScreen();
    qreal devicePixelRatio = curScreen->devicePixelRatio();
    QRect cropRect = isFullScreenMode()
                         ? (QRect{0, 0, curScreen->geometry().width(), curScreen->geometry().height()})
                         : (QRect{m_startPos, m_endPos});
    auto curRect = QRect{m_startPos, m_endPos};
    auto left = m_startPos.x() * devicePixelRatio;
    auto right = (curScreen->geometry().width() - m_endPos.x()) * devicePixelRatio;
    auto top = m_startPos.y() * devicePixelRatio;
    auto bottom = (curScreen->geometry().height() - m_endPos.y()) * devicePixelRatio;
    m_obs->updateRecItem(ui->screenDeviceComboBox->currentText().toUtf8().data(),
                         REC_DESKTOP, !isFullScreenMode(), left, right, top, bottom);
    if (OBS_VIDEO_SUCCESS != m_obs->resetVideo(cropRect.width() * devicePixelRatio,
                                               cropRect.height() * devicePixelRatio,
                                               cropRect.width() * devicePixelRatio,
                                               cropRect.height() * devicePixelRatio,
                                               ui->frameRateComboBox->currentText().remove("FPS").toInt()))
    {
        qWarning() << u8"reset video failed!";
        UserMessageBox::warning(this, u8"警告", u8"输出重置失败!");
    }
}

#pragma region initilize
void RecordingWindow::init(const int defaultPort)
{
    if (m_alreadyInited)
        return;

    auto firstScreen = QGuiApplication::primaryScreen();
    auto pix = devicePixelRatio();

    //init obs
    if (!m_obs->initObs(firstScreen->geometry().width() * pix,
                        firstScreen->geometry().height() * pix, 60))
    {
        qWarning() << "init obs failed!";
        UserMessageBox::warning(this, u8"警告", u8"初始化OBS失败！");
        exit(-1);
    }

    //miniWinodw
    initMiniWindow();

    //backgroundWindow
    initBackgroundWindow();

    //layout
    ui->settingExpander->setChecked(true);
    syncSettingsVisibility(true);
    connect(ui->settingExpander, &QCheckBox::stateChanged, this, [this](int status)
    {
        syncSettingsVisibility(status != 0);
    });

    //init config
    initConfig();

    //init server for communicate between different process
    setupPort(defaultPort < 0 ? m_config.tcpPort : defaultPort);


    //display status when recording like time\audio wave etc...
    initRecordingStatus();

    //get desktops
    m_obs->addSceneSource(REC_DESKTOP);
    m_obs->searchRecTargets(REC_DESKTOP);

    //get micphone and players
    m_obs->searchPlayerDevice();
    m_obs->searchMicDevice();

    //button status
    connect(m_obs.get(), &ObsWrapper::recordStatusChanged, this, [&](int status)
    {
        m_backgroundWindow->hide();
        if (status == RecordingStatus::Recording)
        {
            m_timer.start();
            configureRecordingButton(ui->RecordingButton, "stop_recoding");
            ui->statusLabel->setText("");
            ui->ScreenAreaWidget->setDisabled(true);
            ui->MicphoneWidget->setDisabled(true);
            ui->PlayerWidget->setDisabled(true);
            ui->settingExpander->setDisabled(true);
            if (m_settingsPanelHost != nullptr)
            {
                m_settingsPanelHost->setDisabled(true);
            }
            ui->screenDeviceComboBox->setDisabled(true);
            m_isPauseNow = false;
            syncOverviewMirrorButtons(ui);
            emit SignalProxy::instance()->recodingStarted();
        }
        else if (status == RecordingStatus::Paused)
        {
            m_timer.stop();
            configureRecordingButton(ui->RecordingButton, "stop_recoding");
            ui->statusLabel->setText(u8"暂停中...");
            ui->ScreenAreaWidget->setDisabled(true);
            ui->MicphoneWidget->setDisabled(true);
            ui->PlayerWidget->setDisabled(true);
            ui->settingExpander->setDisabled(true);
            if (m_settingsPanelHost != nullptr)
            {
                m_settingsPanelHost->setDisabled(true);
            }
            ui->screenDeviceComboBox->setDisabled(true);
            m_isPauseNow = true;
            syncOverviewMirrorButtons(ui);
            emit SignalProxy::instance()->recodingPaused();
        }
        else
        {
            m_timer.stop();
            m_seconds = 0;
            configureRecordingButton(ui->RecordingButton, "start");
            ui->statusLabel->setText("");
            ui->ScreenAreaWidget->setDisabled(false);
            ui->MicphoneWidget->setDisabled(false);
            ui->PlayerWidget->setDisabled(false);
            ui->settingExpander->setDisabled(false);
            if (m_settingsPanelHost != nullptr)
            {
                m_settingsPanelHost->setDisabled(false);
            }
            ui->screenDeviceComboBox->setDisabled(false);
            if (m_server != nullptr) //to notice other process
            {
                QTcpSocket* socket = nullptr;
                do
                {
                    socket = m_server->nextPendingConnection();
                    if (socket != nullptr && socket->isValid())
                    {
                        QString data = ui->savePathEdit->text() + "/" + ui->nameEdit->text() + ".mp4";
                        socket->write(data.toUtf8().data());
                        socket->flush();
                    }
                }
                while (socket != nullptr);
            }
            m_isPauseNow = false;
            m_backgroundWindow->show();
            syncOverviewMirrorButtons(ui);
            emit SignalProxy::instance()->recodingStoped();
        }
    });
    configureRecordingButton(ui->RecordingButton, "start");

    //button connection
    initFunctionalControls();

    //hot keys
    initHotKeys();

    //debug preview window
    initDebugWindow();


    //signalproxy
    initSignalProxy();

    m_hasFirstInit = true;
}

void RecordingWindow::initSignalProxy()
{
    connect(SignalProxy::instance(),&SignalProxy::requestStartRecording,this,&RecordingWindow::startRecord);
    connect(SignalProxy::instance(),&SignalProxy::requestPauseRecording,this,&RecordingWindow::pauseRecord);
    connect(SignalProxy::instance(),&SignalProxy::requestStopRecording,this,&RecordingWindow::stopRecord);
}

void RecordingWindow::initMiniWindow()
{
    //miniwindow
    m_miniWindow = new MinimizedRecordingWindow(m_obs, nullptr);
    connect(m_miniWindow, &MinimizedRecordingWindow::onRecover, this, [&]()
    {
        m_miniWindow->hide();
        this->show();
        this->setFocus();
    });
    connect(m_miniWindow, &MinimizedRecordingWindow::closed, this, [&]()
    {
        m_miniWindow->hide();
        this->showNormal();
    });
    connect(m_miniWindow, &MinimizedRecordingWindow::onRecordAct, this,
[&]()
    {
      m_miniWindow->showMinimized();
      startRecord();
    });
    connect(m_miniWindow, &MinimizedRecordingWindow::onPauseAct, this, &RecordingWindow::pauseRecord);
}

void RecordingWindow::initBackgroundWindow()
{
    //backgroundWindow
    m_backgroundWindow = new BackgroundWindow(true, nullptr, QGuiApplication::primaryScreen());
    connect(m_backgroundWindow, &BackgroundWindow::areaChanged, this, [&](
            int x1, int y1, int x2, int y2)
            {
                auto curScreen = findScreen();
                qreal devicePixelRatio = curScreen->devicePixelRatio();
                m_startPos = {x1, y1};
                m_endPos = {x2, y2};
                auto curRect = QRect{m_startPos, m_endPos};
                ui->areaWidthEdit->blockSignals(true);
                ui->areaHeightEdit->blockSignals(true);
                ui->areaWidthEdit->setText(QString::number(curRect.width() * devicePixelRatio));
                ui->areaHeightEdit->setText(QString::number(curRect.height() * devicePixelRatio));
                ui->areaWidthEdit->blockSignals(false);
                ui->areaHeightEdit->blockSignals(false);
                resetVideo();
            });

    connect(m_backgroundWindow, &BackgroundWindow::requestHideWindow, this, [&]()
    {
        if (!isFullScreenMode())
        {
            this->showMinimized();
        }
    });


    connect(m_backgroundWindow, &BackgroundWindow::requestToFullScreenMode, this, [&]()
    {
        ui->areaComboBox->setCurrentIndex(0);
    });
}

void RecordingWindow::initConfig()
{
    if (m_config.isDefault())
    {
        m_config.frameRateInUse = "25FPS";
        m_config.bitRateInUse = "4MB";
        m_config.savePath = QApplication::applicationDirPath();
        m_config.startRecordShortCut = QKeySequence::fromString("F7").toString();
        m_config.pauseRecordShortCut = QKeySequence::fromString("F8").toString();
        m_config.countDownEnable = false;
        m_config.countDownSeconds = 3;
        m_config.stopRecInterval = 0;
        m_config.tcpPort = 29989;
        m_config.showPreviewWindow = false;
        m_config.bitRatesPresets = {"2MB", "4MB", "8MB"};
        m_config.frameRatePresets = {"25FPS", "30FPS", "50FPS", "60FPS"};
        m_config.writeJson();
    }
    ui->nameEdit->setText(QDateTime::currentDateTime().toString("yyyy.MM.dd-hh.mm.ss"));
    SET_POP_VIEW(bitrateComboBox)
    ui->bitrateComboBox->setModel(new QStringListModel(m_config.bitRatesPresets, this));
    ui->bitrateComboBox->setCurrentText(m_config.bitRateInUse);
    SET_POP_VIEW(frameRateComboBox)
    ui->frameRateComboBox->setModel(new QStringListModel(m_config.frameRatePresets, this));
    ui->frameRateComboBox->setCurrentText(m_config.frameRateInUse);
    ui->savePathEdit->setText(m_config.savePath);
    ui->StartShortCut->setKeySequence(m_config.startRecordShortCut);
    ui->PauseShortCut->setKeySequence(m_config.pauseRecordShortCut);
    m_recordHotKey->setShortcut(ui->StartShortCut->keySequence(), true);
    m_pauseHotKey->setShortcut(ui->PauseShortCut->keySequence(), true);
    ui->countDownCheckBox->setChecked(m_config.countDownEnable);
    auto* validator = new QIntValidator(1, INT_MAX, this);
    ui->countDownEdit->setValidator(validator);

    ui->countDownEdit->setText(QString::number(m_config.countDownSeconds));

    auto minute = m_config.stopRecInterval / 60;
    auto sec = m_config.stopRecInterval%60;
    ui->m_minuteSpinBox->setValue(minute);
    ui->m_secondSpinBox->setValue(sec);
}

void RecordingWindow::initRecordingStatus()
{
    //timer for display recording status
    m_seconds = 0;
    m_timer.setInterval(1000);
    connect(&m_timer, &QTimer::timeout, this, [&]()
    {

        ++m_seconds;
        int hours = m_seconds / 3600;
        int mins = (m_seconds - hours * 3600) / 60;
        int secs = m_seconds - hours * 3600 - mins * 60;
        QString formattedTime = QString("%1:%2:%3").arg(hours, 4, 10, QLatin1Char('0'))
                                                   .arg(mins, 2, 10, QLatin1Char('0'))
                                                   .arg(secs, 2, 10, QLatin1Char('0'));
        ui->statusLabel->setText(QString(formattedTime));
        if (m_stopTimeInterval!=0 && m_seconds>=m_stopTimeInterval)
        {
            qDebug()<<u8"record stopped by timer";
            stopRecord();
        }
    });
    //widgets connection
    //audio signal display
    connect(m_obs.get(), &ObsWrapper::micVolumeDataChange, ui->micphoneVolume, &VolumeControl::setLevels,
            Qt::QueuedConnection);
    connect(m_obs.get(), &ObsWrapper::playerVolumeDataChange, ui->playerVolume, &VolumeControl::setLevels,
            Qt::QueuedConnection);
    int audioChannel = m_obs->audioChannel();
    ui->micphoneVolume->setChannelCount(m_obs->micChannelCount());
    ui->micphoneVolume->setAudioChannel(audioChannel);
    ui->micphoneVolume->setInverting(true);
    ui->playerVolume->setChannelCount(m_obs->playerChannelCount());
    ui->playerVolume->setAudioChannel(audioChannel);
    ui->playerVolume->setInverting(true);
}

void RecordingWindow::initDebugWindow()
{
    if (m_config.showPreviewWindow)
    {
        m_test = new testwindow(m_obs);
        m_test->show();
        m_test->createDisplayer();
    }
    m_showCaptureKey = new QHotkey(QKeySequence("Alt+S"), true);
    connect(m_showCaptureKey, &QHotkey::activated, this, [&]()
    {
        if (m_test != nullptr)
        {
            m_test->close();
            delete m_test;
        }
        m_test = new testwindow(m_obs);
        m_test->show();
        m_test->createDisplayer();
    });
    m_obs->recMicAudio(true, "default");
    m_obs->recPlayerAudio(true, "default");
    m_alreadyInited = true;
}

void RecordingWindow::initFunctionalControls()
{

    //area Init
    SET_POP_VIEW(areaComboBox)
    ui->areaComboBox->setModel(new QStringListModel({ScreenAreaStr[DeskTop], ScreenAreaStr[Customized]}, this));
    //capture type changed
    connect(ui->areaComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [&](int curIndex)
    {
        configureTileButton(ui->areaButton, isFullScreenMode() ? "fullscreen" : "areas");
        ui->areaWidthEdit->setDisabled(isFullScreenMode());
        ui->areaHeightEdit->setDisabled(isFullScreenMode());
        auto curScreen = findScreen();
        if (isFullScreenMode())
        {
            ui->areaWidthEdit->blockSignals(true);
            ui->areaHeightEdit->blockSignals(true);
            qreal devicePixelRatio = curScreen->devicePixelRatio();
            auto WValidator = new QIntValidator(0, curScreen->geometry().width() * devicePixelRatio, this);
            auto HValidator = new QIntValidator(0, curScreen->geometry().height() * devicePixelRatio, this);
            ui->areaWidthEdit->setText(QString::number(curScreen->geometry().width() * devicePixelRatio));
            ui->areaWidthEdit->setValidator(WValidator);
            ui->areaHeightEdit->setText(QString::number(curScreen->geometry().height() * devicePixelRatio));
            ui->areaHeightEdit->setValidator(HValidator);
            ui->areaWidthEdit->blockSignals(false);
            ui->areaHeightEdit->blockSignals(false);
            resetVideo();
        }
        QMetaObject::invokeMethod(this, "rebuildBackgroundWindow");
        QMetaObject::invokeMethod(this, "rebuildBackgroundWindow");
    });


    //screen mode
    SET_POP_VIEW(screenDeviceComboBox)
    QStandardItemModel* curScModel = new QStandardItemModel(this);
    auto curScItems = m_obs->getRecTargets();
    for (int i = 0; i < curScItems.size(); ++i)
    {
        QStandardItem* item = new QStandardItem(curScItems.at(i));;
        item->setToolTip(curScItems.at(i));
        curScModel->appendRow(item);
    }
    ui->screenDeviceComboBox->setModel(curScModel);

    //screen comboBox changed
    connect(ui->screenDeviceComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            [&](int curIndex)
            {
                auto curScreen = findScreen();
                ui->areaWidthEdit->blockSignals(true);
                ui->areaHeightEdit->blockSignals(true);
                qreal devicePixelRatio = curScreen->devicePixelRatio();
                auto WValidator = new QIntValidator(0, curScreen->geometry().width() * devicePixelRatio, this);
                auto HValidator = new QIntValidator(0, curScreen->geometry().height() * devicePixelRatio, this);
                ui->areaWidthEdit->setText(QString::number(curScreen->geometry().width() * devicePixelRatio));
                ui->areaWidthEdit->setValidator(WValidator);
                ui->areaHeightEdit->setText(QString::number(curScreen->geometry().height() * devicePixelRatio));
                ui->areaHeightEdit->setValidator(HValidator);
                ui->areaWidthEdit->blockSignals(false);
                ui->areaHeightEdit->blockSignals(false);
                QMetaObject::invokeMethod(this, "rebuildBackgroundWindow");
                resetVideo();
            });
    ui->screenDeviceComboBox->setCurrentIndex(0);
    ui->areaComboBox->setCurrentIndex(0);
    ui->areaWidthEdit->blockSignals(true);
    ui->areaHeightEdit->blockSignals(true);
    ui->areaWidthEdit->setText(QString::number(QGuiApplication::primaryScreen()->geometry().width() * QGuiApplication::primaryScreen()->devicePixelRatio()));
    ui->areaHeightEdit->setText(QString::number(QGuiApplication::primaryScreen()->geometry().height() * QGuiApplication::primaryScreen()->devicePixelRatio()));
    rebuildBackgroundWindow();
    resetVideo();
    ui->areaWidthEdit->blockSignals(false);
    ui->areaHeightEdit->blockSignals(false);


    //Sound device name
    SET_POP_VIEW(micphoneDeviceComboBox)
    QStandardItemModel* curMicModel = new QStandardItemModel(this);
    auto curMicItems = m_obs->micphoneDeviceName();
    for (int i = 0; i < curMicItems.size(); ++i)
    {
        QStandardItem* item = new QStandardItem(curMicItems.at(i));;
        item->setToolTip(curMicItems.at(i));
        curMicModel->appendRow(item);
    }
    ui->micphoneDeviceComboBox->setModel(curMicModel);
    ui->micphoneDeviceComboBox->setCurrentIndex(0);
    connect(ui->micphoneDeviceComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [&]()
    {
        auto curId = m_obs->micDeviceId(ui->micphoneDeviceComboBox->currentText());
        m_obs->resetMicphoneVolumeLevelCallback(curId);
    });


    //player device name
    SET_POP_VIEW(playerDeviceComboBox)
    QStandardItemModel* curPlayerModel = new QStandardItemModel(this);
    auto curPlayerItems = m_obs->playerDeviceName();
    for (int i = 0; i < curPlayerItems.size(); ++i)
    {
        QStandardItem* item = new QStandardItem(curPlayerItems.at(i));;
        item->setToolTip(curPlayerItems.at(i));
        curPlayerModel->appendRow(item);
    }
    ui->playerDeviceComboBox->setModel(curPlayerModel);
    ui->playerDeviceComboBox->setCurrentIndex(0);
    connect(ui->playerDeviceComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [&]()
    {
        auto curId = m_obs->playerDeviceId(ui->playerDeviceComboBox->currentText());
        m_obs->resetPlayerVolumeLevelCallback(curId);
    });

    //area size line edit
    auto setSizeAct = [&]()
    {
        if (isFullScreenMode())
            return;
        auto sc = findScreen();
        qreal devicePixelRatio = sc->devicePixelRatio();
        m_backgroundWindow->setSize(ui->areaWidthEdit->text().toInt() / devicePixelRatio,
                                    ui->areaHeightEdit->text().toInt() / devicePixelRatio);
    };

    connect(ui->areaWidthEdit, &QLineEdit::textEdited, this, [&, setSizeAct]()
{
    setSizeAct();
});
    connect(ui->areaHeightEdit, &QLineEdit::textEdited, this, [&, setSizeAct]()
    {
        setSizeAct();
    });

    //region set
    connect(ui->areaButton, &QPushButton::clicked, this, [&]()
    {
        if (isFullScreenMode())
            return;
        auto sc = findScreen();
        m_backgroundWindow->resetSelectionPos(sc->geometry().width(), sc->geometry().height());
    });
    //save path
    connect(ui->pathSelectionButton, &QPushButton::clicked, this, [&]()
    {
        auto curStr = QFileDialog::getExistingDirectory(this, tr("Open Directory"),
                                                        ui->savePathEdit->text().isEmpty()
                                                            ? "."
                                                            : ui->savePathEdit->text());
        if (!curStr.isEmpty())
        {
            ui->savePathEdit->setText(curStr);
        }
    });
    //recording button
    connect(ui->RecordingButton, &QPushButton::clicked, this, [&]()
    {
        startRecord();
    });
    //micphone button
    connect(ui->micphoneButton, &QPushButton::clicked, this, [&]()
    {
        m_isMicphoneEnable = !m_isMicphoneEnable;
        ui->micphoneLabel->setText(m_isMicphoneEnable ? "" : u8"已禁用");
        ui->micphoneVolume->setVisible(m_isMicphoneEnable);
        auto curId = m_obs->micDeviceId(ui->micphoneDeviceComboBox->currentText());
        m_obs->recMicAudio(m_isMicphoneEnable, curId);
        configureTileButton(ui->micphoneButton, m_isMicphoneEnable ? "micphone" : "micphone_mute");
    });
    //player button
    connect(ui->playerButton, &QPushButton::clicked, this, [&]()
    {
        m_isPlayerEnable = !m_isPlayerEnable;
        ui->playerLabel->setText(m_isPlayerEnable ? "" : u8"已禁用");
        ui->playerVolume->setVisible(m_isPlayerEnable);
        auto curId = m_obs->playerDeviceId(ui->playerDeviceComboBox->currentText());
        m_obs->recPlayerAudio(m_isPlayerEnable, curId);
        configureTileButton(ui->playerButton, m_isPlayerEnable ? "player" : "player_mute");
    });
}

void RecordingWindow::initHotKeys()
{
    //hotkey
    connect(ui->PauseShortCut, &QKeySequenceEdit::keySequenceChanged,
        this, [&](const QKeySequence& s)
    {
        m_pauseHotKey->setRegistered(false);
        delete m_pauseHotKey;
        m_pauseHotKey = new QHotkey(s, true);
        connect(m_pauseHotKey, &QHotkey::activated, this, &RecordingWindow::pauseRecord);
    });
    connect(ui->StartShortCut, &QKeySequenceEdit::keySequenceChanged, this,
        [&](const QKeySequence& s)
    {
        m_recordHotKey->setRegistered(false);
        delete m_recordHotKey;
        m_recordHotKey = new QHotkey(s, true);
        connect(m_recordHotKey, &QHotkey::activated, this, &RecordingWindow::startRecord);
    });
    connect(m_pauseHotKey, &QHotkey::activated, this, &RecordingWindow::pauseRecord);
    connect(m_recordHotKey, &QHotkey::activated, this, &RecordingWindow::startRecord);
}

#pragma endregion

bool RecordingWindow::isFullScreenMode() const
{
    return ui->areaComboBox->currentIndex() == 0;
}

void RecordingWindow::setupPort(int port)
{
    if (m_server != nullptr)
    {
        m_server->close();
        m_server->disconnect();
        m_server->deleteLater();
        m_server = nullptr;
    }
    m_server = new QTcpServer(this);
    m_server->setMaxPendingConnections(50);

    auto originPort = port;
    bool startTCPSuccess = false;
    do
    {
        startTCPSuccess = m_server->listen(QHostAddress::Any, port);
        if (!startTCPSuccess && port < 65535)
        {
            port++;
        }
    }
    while (!startTCPSuccess);

    if (!startTCPSuccess && port >= 65535)
    {
        qWarning() << "tcp server start listen at [" << originPort << "] failed!";
        UserMessageBox::warning(this, u8"警告",
                                u8"服务监听端口：" + QString::number(originPort) + u8"失败！");
        qApp->exit(-1);
    }
    else if (startTCPSuccess && port != originPort)
    {
        qWarning() << "tcp server replace [" << originPort << "] with [" << port << "]";
        UserMessageBox::information(this, u8"通知",
                                    u8"原服务监听端口：" + QString::number(originPort) + u8"不可用！已替换为：" + QString::number(port));
    }
    qDebug() << "tcp server start listen at [" << port << "]";
}

void RecordingWindow::closeEvent(QCloseEvent* event)
{
    if (m_isRecordingStarted)
    {
        const auto result = UserMessageBox::question(this, u8"提示", u8"当前正在录制，确认退出？");
        if (result != UserMessageBox::ButtonType::Ok)
        {
            event->ignore();
            return;
        }

        stopRecord();
    }

    m_obs->release();
    Fluent::FluentMainWindow::closeEvent(event);
}

void RecordingWindow::showEvent(QShowEvent* event)
{
    Fluent::FluentMainWindow::showEvent(event);

    if (!m_hasFirstInit)
    {
        init(m_socketPort);
    }
}

void RecordingWindow::startRecord()
{
    if (!m_isRecordingStarted)
    {
        auto sc = findScreen();

        qreal devicePixelRatio = sc->devicePixelRatio();
        auto left = m_startPos.x() * devicePixelRatio;
        auto right = (sc->geometry().width() - m_endPos.x()) * devicePixelRatio;
        auto top = m_startPos.y() * devicePixelRatio;
        auto bottom = (sc->geometry().height() - m_endPos.y()) * devicePixelRatio;

        QRect cropRect = isFullScreenMode()
                             ? (QRect{0, 0, sc->geometry().width(), sc->geometry().height()})
                             : (QRect{m_startPos, m_endPos});

        qDebug() << "in screen:[" << sc->name() << " ]with crop rect:" << cropRect;

        auto fps = ui->frameRateComboBox->currentText().replace(QString("FPS"), QString("")).toInt();
        auto bitRate = ui->bitrateComboBox->currentText().replace(QString("MB"), QString("")).toInt();

        if (OBS_VIDEO_SUCCESS != m_obs->resetVideo(cropRect.width() * devicePixelRatio,
                                                   cropRect.height() * devicePixelRatio,
                                                   cropRect.width() * devicePixelRatio,
                                                   cropRect.height() * devicePixelRatio, fps))
        {
            qDebug() << u8"reset video failed!";
            UserMessageBox::warning(this, u8"警告", u8"输出重置失败!");
            return;
        }

        if (ui->savePathEdit->text().isEmpty())
        {
            UserMessageBox::warning(this, u8"警告", u8"当前存储路径为空!");
            return;
        }
        auto dir = QDir(ui->savePathEdit->text());

        if (!dir.exists())
        {
            dir.mkpath(ui->savePathEdit->text());
        }
        if (!dir.exists())
        {
            qDebug() << u8"save path" << ui->savePathEdit->text() << "invalid!";
            UserMessageBox::warning(this, u8"警告", u8"当前存储路径不合法!");
            return;
        }
        m_stopTimeInterval = ui->m_minuteSpinBox->value() * 60;
        m_stopTimeInterval += ui->m_secondSpinBox->value();

        qDebug()<<"current recording time interval:"<<m_stopTimeInterval;

        auto fileName = ui->savePathEdit->text() + "/" + ui->nameEdit->text();

        if (!isValidFilePath(fileName))
        {
            qDebug() << u8"name " << ui->nameEdit->text() << "invalid!";
            UserMessageBox::warning(this, u8"警告", u8"文件名称或路径非法!");
            return;
        }
        m_currentRecordingFile = fileName + ".mp4";
        m_obs->setupFFmpeg(fileName,
                           cropRect.width() * devicePixelRatio,
                           cropRect.height() * devicePixelRatio,
                           fps, bitRate);

        saveConfig();

        this->showMinimized();
        if (ui->countDownCheckBox->isChecked()) //with count down dialog to start
        {
            if (m_countDownDialog != nullptr)
            {
                delete m_countDownDialog;
                m_countDownDialog = nullptr;
            }
            //count down dialog
            m_countDownDialog = new CountDownDialog(ui->countDownEdit->text().toInt(), m_obs, sc, nullptr);
            m_countDownDialog->show();
        }
        else // else, start immediately
        {
            auto res = m_obs->startRecording();
            qDebug() << "start with status:" << res;
            if (0 != res)
            {
                qDebug() << u8"start recording" << fileName + ".mp4" << "falied!";
                UserMessageBox::warning(this, u8"警告", u8"开始录屏失败!");
                return;
            }
        }
        m_isRecordingStarted = true;
    }
    else
    {
        stopRecord();
    }
}

void RecordingWindow::pauseRecord()
{
    if (m_obs->isRecordingStart())
    {
        if (m_seconds < 1) //too short cause crash
        {
            UserMessageBox::warning(this, u8"警告", u8"录制时长过短，请稍后再试");
            return;
        }
        m_obs->pauseRecording();
    }
}

void RecordingWindow::stopRecord()
{
    m_stopTimeInterval = 0;
    if (this->isHidden()&& m_miniWindow)
    {
        m_miniWindow->showNormal();
        m_miniWindow->raise();
        m_miniWindow->activateWindow();
    }
    else
    {
        this->showNormal();
        this->raise();
        this->activateWindow();
    }

    if (m_obs->isRecordingStart())
    {
        if (m_seconds < 3) //too short cause crash
        {
            UserMessageBox::warning(this, u8"警告", u8"录制时长过短，请稍后再试");
            return;
        }
        m_isRecordingStarted = false;
        auto res = m_obs->stopRecording();
        qDebug() << "stop with status:" << res;
        //rename current recording
        ui->nameEdit->setText(QDateTime::currentDateTime().toString("yyyy.MM.dd-hh.mm.ss"));

        invokePreviewWindow({m_currentRecordingFile});
    }

    if (!m_url.isEmpty() && !m_token.isEmpty())
    {
        ui->statusLabel->setText(u8"已设置在线登录信息");
    }
}

void RecordingWindow::saveConfig()
{
    //m_config.nameOfRecord = ui->nameEdit->text();

    m_config.frameRateInUse = ui->frameRateComboBox->currentText();
    m_config.bitRateInUse = ui->bitrateComboBox->currentText();
    m_config.savePath = ui->savePathEdit->text();
    m_config.startRecordShortCut = ui->StartShortCut->keySequence().toString();
    m_config.pauseRecordShortCut = ui->PauseShortCut->keySequence().toString();
    m_config.countDownEnable = ui->countDownCheckBox->isChecked();
    m_config.stopRecInterval = m_stopTimeInterval;
    m_config.countDownSeconds = ui->countDownEdit->text().toInt();
    m_config.writeJson();
}


void RecordingWindow::rebuildBackgroundWindow()
{
    if (!isFullScreenMode())
    {
        this->showMinimized();
    }
    auto curScreen = findScreen();
    qDebug() << "get Screen" << curScreen->name();
    m_backgroundWindow->resetStatus(isFullScreenMode(), curScreen);
    m_backgroundWindow->showFullScreen();
}


bool RecordingWindow::checkUpdate(const QString& serverVersion)
{
    qDebug() << "current version:" << QCoreApplication::applicationVersion() << "server version:" << serverVersion;
    // 将版本号字符串按"."拆分为数字列表
    QStringList parts1 = QCoreApplication::applicationVersion().split('.');
    QStringList parts2 = serverVersion.split('.');

    // 获取较长的版本列表长度
    int maxLength = qMax(parts1.size(), parts2.size());

    for (int i = 0; i < maxLength; ++i)
    {
        // 获取当前位的数字，如果缺失，则默认值为0
        int num1 = (i < parts1.size()) ? parts1[i].toInt() : 0;
        int num2 = (i < parts2.size()) ? parts2[i].toInt() : 0;

        // 比较当前数字
        if (num1 < num2)
        {
            return true; // version1 < version2
        }
    }
    // 如果所有数字都相等，则版本号相等
    return false;
}

void RecordingWindow::invokePreviewWindow(const QString& file)
{
    if (m_previewPage == nullptr || m_navigationView == nullptr)
    {
        return;
    }

    const QFileInfo fileInfo(file);
    const QString previewDirectory = !fileInfo.absolutePath().isEmpty()
                                         ? fileInfo.absolutePath()
                                         : ui->savePathEdit->text();

    m_previewPage->setRecordingDirectory(previewDirectory);
    m_previewPage->refreshHistory(file);

    if (!file.isEmpty() && QFileInfo::exists(file))
    {
        m_previewPage->selectRecording(file);
    }

    m_navigationView->setSelectedKey("preview");
    switchContentPage("preview");

    if (file.isEmpty() || !QFile::exists(file))
    {
        QTimer::singleShot(300, this, [this, file]() {
            if (m_previewPage == nullptr)
            {
                return;
            }

            m_previewPage->refreshHistory(file);
            if (!file.isEmpty() && QFileInfo::exists(file))
            {
                m_previewPage->selectRecording(file);
            }
        });
    }
}

