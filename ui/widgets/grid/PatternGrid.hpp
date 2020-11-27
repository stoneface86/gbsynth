
#pragma once

#include "model/OrderModel.hpp"

#include "widgets/grid/PatternColors.hpp"

#include <QWidget>
#include <QPaintEvent>
#include <QString>
#include <QBitmap>

#include <array>
#include <cstdint>
#include <tuple>
#include <vector>

class PatternGrid : public QWidget {

    Q_OBJECT

public:

    explicit PatternGrid(OrderModel &model, QWidget *parent = nullptr);
    ~PatternGrid() = default;

    // Settings

    //void apply(); // applies current settings

    int row() const;

signals:

    // emitted when the user changes the current pattern
    void cursorPatternChanged(int patternNo);

    // emitted when the user selects a different track via keyboard or mouse
    void cursorTrackChanged(int track);

    void cursorColumnChanged(int column);

    // emitted when the user changes the current row via keyboard, scroll wheel or mouse
    void cursorRowChanged(int row);

public slots:

    void setCursorTrack(int track);
    void setCursorColumn(int column);
    void setCursorRow(int row);

    void moveCursorRow(int amount);
    void moveCursorColumn(int amount);


protected:

    void leaveEvent(QEvent *evt) override;

    void mouseMoveEvent(QMouseEvent *evt) override;

    void mousePressEvent(QMouseEvent *evt) override;

    void mouseReleaseEvent(QMouseEvent *evt) override;

    void paintEvent(QPaintEvent *evt) override;

    void resizeEvent(QResizeEvent *evt) override;

private:

    //
    // Hover state when the mouse is not over any track header
    //
    static constexpr int HOVER_NONE = -1;

    //
    // Called when appearance settings have changed, recalculates metrics and redraws
    // all rows.
    //
    void appearanceChanged();

    //
    // Calculates the number of rows we can draw on this widget
    //
    unsigned getVisibleRows();

    //
    // Calculates the x position of the given column
    //
    int columnLocation(int column);

    //
    // Converts translated mouse coordinates on the grid to a row and column coordinate
    //
    void getCursorFromMouse(int x, int y, unsigned &outRow, unsigned &outCol);

    //
    // Paints rows using the given painter. Row indices range form 0 to mVisibleRows.
    // Row 0 is the first visible row on the widget.
    // 0 <= rowStart < rowEnd <= mVisibleRows
    //
    void paintRows(QPainter &painter, int rowStart, int rowEnd);

    //
    // Utility method paints lines between tracks using the given painter
    // The pen is not set, do so before calling this method.
    //
    void paintLines(QPainter &painter, int height);

    //
    // Scrolls displayed rows and paints new ones. If rows >= mVisibleRows then
    // no scrolling will occur and the entire display will be repainted
    //
    void scroll(int rows);

    void setTrackHover(int track);

    //
    // Schedules a repaint for just the grid portion of the widget
    //
    void updateGrid();

    //
    // Schedules a repaint for just the header portion of the widget
    //
    void updateHeader();

    //
    // Schedules a full repaint for the widget.
    //
    void updateAll();

    enum ColumnType {
        COLUMN_NOTE,

        // high is the upper nibble (bits 4-7)
        // low is the lower nibble (bits 0-3)

        COLUMN_INSTRUMENT_HIGH,
        COLUMN_INSTRUMENT_LOW,

        COLUMN_EFFECT1_TYPE,
        COLUMN_EFFECT1_ARG_HIGH,
        COLUMN_EFFECT1_ARG_LOW,

        COLUMN_EFFECT2_TYPE,
        COLUMN_EFFECT2_ARG_HIGH,
        COLUMN_EFFECT2_ARG_LOW,

        COLUMN_EFFECT3_TYPE,
        COLUMN_EFFECT3_ARG_HIGH,
        COLUMN_EFFECT3_ARG_LOW

    };

    enum class PaintChoice {
        both,           // repaint the header and grid
        header,         // just the header
        grid            // just the grid
    };

    static constexpr int ROWNO_CELLS = 4; // 4 cells for row numbers

    static constexpr int TRACK_CELLS = 20;
    static constexpr int TRACK_COLUMNS = 12;

    // total columns on the grid (excludes rowno)
    static constexpr int COLUMNS = TRACK_COLUMNS * 4;

    static constexpr int HEADER_HEIGHT = 32;
    static constexpr int HEADER_FONT_WIDTH = 7;
    static constexpr int HEADER_FONT_HEIGHT = 11;

    // converts a column index -> cell index
    static uint8_t TRACK_CELL_MAP[TRACK_CELLS];
    // converts a cell index -> column index
    static uint8_t TRACK_COLUMN_MAP[TRACK_COLUMNS];


    OrderModel &mModel;

    // display image, rows get painted here when needed as opposed to every
    // paintEvent
    QPixmap mDisplay;

    // 1bpp font 7x11, used for the Header
    QBitmap mHeaderFont;
    // header drawing is cached here, similar to the grid, to speed up painting
    QPixmap mHeaderPixmap;

    ColorTable mColorTable;

    // if true all rows will be redrawn on next paint event
    bool mRepaintImage;

    // determines what to paint (optimization)
    PaintChoice mPaintChoice;

    int mCursorRow;
    int mCursorCol;
    int mCursorPattern;    // the current pattern

    int mPatterns;
    int mPatternSize;

    bool mSelecting;

    // translation x coordinate for centering the grid
    int mDisplayXpos;

    // header stuff
    int mTrackHover;
    int mTrackFlags;


    // variables here are dependent on appearance settings
    // treat them as constants, only appearanceChanged() can modify them

    // metrics
    unsigned mRowHeight; // height of a row in pixels, including padding
    //unsigned mRowHeightHalf;
    //unsigned mCenter;
    unsigned mCharWidth; // width of a character
    unsigned mVisibleRows; // number of rows visible on the widget
    // mVisibleRows * mRowHeight is always >= height()

    // width, in pixels, of a track
    int mTrackWidth; // = TRACK_CELLS * mCharWidth

    // width, in pixels of the row number column
    int mRownoWidth; // = 4 * mCharWidth


};
