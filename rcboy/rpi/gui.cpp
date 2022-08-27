#include <QFrame>

#include "gui.h"

int GUI::exec(int argc, char * argv[]) {
    QApplication app(argc, argv);
    QCursor cursor(Qt::BlankCursor);
    QApplication::setOverrideCursor(cursor);
    QApplication::changeOverrideCursor(cursor);    
    GUI w{};
    w.show();
    return app.exec();
}

GUI::GUI(QWidget *parent):
    QMainWindow{parent},
    headerView_{new QGraphicsView{this}},
    pageView_{new QGraphicsView{this}},
    footerView_{new QGraphicsView{this}},
    header_{new Header{}},
    footer_{new Footer{}} {

    setFixedSize(QSize{320,240});
    headerView_->setGeometry(QRect{0,0,320,32});
    headerView_->setFrameStyle(QFrame::NoFrame);
    pageView_->setGeometry(QRect{0,32,320,176});
    pageView_->setFrameStyle(QFrame::NoFrame);
    footerView_->setGeometry(QRect{0,208,320,32});
    footerView_->setFrameStyle(QFrame::NoFrame);
    headerView_->setScene(header_);
    footerView_->setScene(footer_);

}

GUI::~GUI() {
    delete header_;
    delete footer_;
}
