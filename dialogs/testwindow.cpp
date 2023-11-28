//
// Created by Type3Limit on 2023/11/28.
//

// You may need to build the project (run Qt uic code generator) to get "ui_testwindow.h" resolved

#include "testwindow.h"

#include "obswrapper.h"
#include "ui_testwindow.h"
#include "qoperationhelper.h"




testwindow::testwindow(const QSharedPointer<ObsWrapper>& obs, QWidget *parent) :
m_obs(obs),QDialog(parent), ui(new Ui::testwindow) {
    ui->setupUi(this);
    setWindowFlags(Qt::SubWindow);
    setWindowIcon(QIcon(QString(":/icons/images/recording.svg")));
    setWindowTitle(u8"采集预览");
}

testwindow::~testwindow() {
    obs_source_dec_showing(m_obs->captureSource);
    delete ui;
}

void testwindow::createDisplayer()
{
    QSize size = GetPixelSize(this);
    gs_init_data info = {};
    info.cx = size.width();
    info.cy = size.height();
    info.format = GS_BGRA;
    info.zsformat = GS_ZS_NONE;

    if (!QTToGSWindow(windowHandle(), info.window))
        return;
    obs_source_inc_showing(m_obs->captureSource);
    m_displayer = obs_display_create(&info, 0xFF4C4C4C);
    obs_display_add_draw_callback(m_displayer,
                  testwindow::DrawPreview,
                  this);
}

void testwindow::resizeEvent(QResizeEvent* event)
{
    QSize size = GetPixelSize(this);
    obs_display_resize(m_displayer,size.width(), size.height());
    QDialog::resizeEvent(event);
}


void testwindow::DrawPreview(void *data, uint32_t cx, uint32_t cy)
{
    testwindow *window = static_cast<testwindow *>(data);

    if (!window->m_obs->captureSource)
        return;

    uint32_t sourceCX = max(obs_source_get_width(window->m_obs->captureSource), 1u);
    uint32_t sourceCY = max(obs_source_get_height(window->m_obs->captureSource), 1u);

    int x, y;
    int newCX, newCY;
    float scale;

    GetScaleAndCenterPos(sourceCX, sourceCY, cx, cy, x, y, scale);

    newCX = int(scale * float(sourceCX));
    newCY = int(scale * float(sourceCY));

    gs_viewport_push();
    gs_projection_push();
    const bool previous = gs_set_linear_srgb(true);

    gs_ortho(0.0f, float(sourceCX), 0.0f, float(sourceCY), -100.0f, 100.0f);
    gs_set_viewport(x, y, newCX, newCY);
    obs_source_video_render(window->m_obs->captureSource);

    gs_set_linear_srgb(previous);
    gs_projection_pop();
    gs_viewport_pop();
}
