#pragma once
#include <QApplication>
#include <QMainWindow>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsTextItem>

/** RCBoy's launcher main window. 
 */
class GUI : public QMainWindow {
    Q_OBJECT
public:
    class Header;
    class Footer;

    static int exec(int argc, char * argv[]);

private:
    explicit GUI(QWidget *parent = nullptr);

    ~GUI() override;

    QGraphicsView * headerView_;
    QGraphicsView * pageView_;
    QGraphicsView * footerView_;

    Header * header_;
    Footer * footer_;

}; // GUI

/** GUI Header 
 
    Displays the status bar with information about charging, battery levels, signal strength, volume, etc. 
 */
class GUI::Header : public QGraphicsScene {
    Q_OBJECT
public:
    explicit Header() {
        setBackgroundBrush(Qt::black);
        //setForegroundPen(Qt::white);
        setSceneRect(QRectF{0,0,320,32});
        addText("Hello world!")->setDefaultTextColor(Qt::white);
    }
}; 

/** GUI Footer
 
    The footer displays hints for current controls such as 
 */
class GUI::Footer : public QGraphicsScene {
    Q_OBJECT
public:
    explicit Footer() {
        setBackgroundBrush(Qt::black);
        setSceneRect(QRectF{0,0,320,32});
        addEllipse(QRectF{8,8,16,16}, QPen{Qt::red}, QBrush{Qt::red});
    }
}; 


