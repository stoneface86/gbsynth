#include "WaveGraph.hpp"

#include <QPainter>

constexpr int STEP_X = 12;
constexpr int STEP_Y = 12;

constexpr int PLOT_WIDTH = STEP_X * 32;
constexpr int PLOT_HEIGHT = STEP_Y * 16;


WaveGraph::WaveGraph(QWidget *parent) :
    mDragging(false),
    mCurX(0),
    mCurY(0),
    mPlotAxisColor(0x8E, 0x8E, 0x8E),
    mPlotGridColor(0x20, 0x20, 0x20),
    mPlotLineColor(0xF0, 0xF0, 0xF0),
    mPlotSampleColor(0xFF, 0xFF, 0xFF),
    mPlotRect(0, 0, PLOT_WIDTH, PLOT_HEIGHT),
    mData(nullptr),
    QFrame(parent)
{
    calcGraph();
    setMouseTracking(true);
    setFixedSize(400, 200);
}

void WaveGraph::setData(uint8_t *_data) {
    mData = _data;
}


void WaveGraph::paintEvent(QPaintEvent *evt) {
    (void)evt;

    QPainter painter(this);

    auto r = rect();

    painter.fillRect(r, QColorConstants::Black);
    

    int const xaxis = mPlotRect.left();
    int const yaxis = mPlotRect.bottom();

    painter.setPen(mPlotAxisColor);
    painter.drawLine(xaxis, yaxis, mPlotRect.right(), yaxis);

    painter.setPen(mPlotGridColor);
    int yline = yaxis;
    for (int i = 0; i != 15; ++i) {
        yline -= STEP_Y;
        painter.drawLine(xaxis, yline, mPlotRect.right(), yline);
    }

    QBrush brush(mPlotLineColor);
    painter.setBrush(brush);

    if (mData == nullptr) {
        return;
    }
    
    int x = xaxis + 1;
    for (int i = 0; i != 32; ++i) {

        int y = ((16 - mData[i]) * STEP_Y) + mPlotRect.top();
        painter.drawRect(x, y, STEP_X - 2, yaxis - y - 1);

        x += STEP_X;
    }

}

void WaveGraph::mousePressEvent(QMouseEvent *evt) {
    if (evt->button() == Qt::LeftButton) {
        mDragging = true;
        mData[mCurX] = mCurY;
        emit sampleChanged(QPoint(mCurX, mCurY));
        repaint();
    }
}

void WaveGraph::mouseReleaseEvent(QMouseEvent *evt) {
    if (evt->button() == Qt::LeftButton) {
        mDragging = false;
    }
}


void WaveGraph::mouseMoveEvent(QMouseEvent *evt) {

    uint8_t newX, newY;

    auto mx = evt->x();
    auto my = evt->y();

    if (mx < mPlotRect.x()) {
        newX = 0;
    } else if (mx > mPlotRect.right()) {
        newX = 31;
    } else {
        newX = static_cast<uint8_t>((mx - mPlotRect.x()) / STEP_X);
    }

    if (my < mPlotRect.y()) {
        newY = 0xF;
    } else if (my > mPlotRect.bottom()) {
        newY = 0;
    } else {
        newY = 0xF - static_cast<uint8_t>((my - mPlotRect.y() - (STEP_Y / 2)) / STEP_Y);
    }

    if (mCurX != newX || mCurY != newY) {
        mCurX = newX;
        mCurY = newY;
        emit coordsTextChanged(QString("(%1, %2)").arg(QString::number(newX), QString::number(newY)));
        if (mDragging) {
            if (mData[mCurX] != mCurY) {
                mData[mCurX] = mCurY;
                emit sampleChanged(QPoint(mCurX, mCurY));
                repaint();
            }
        }
    }

}

void WaveGraph::leaveEvent(QEvent *evt) {
    (void)evt;
    emit coordsTextChanged("");
}

void WaveGraph::resizeEvent(QResizeEvent *evt) {
    (void)evt;
    calcGraph();
    repaint();
}


void WaveGraph::calcGraph() {
    // re-center plot rectangle
    mPlotRect.moveCenter(rect().center());
}

