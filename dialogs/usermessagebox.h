#ifndef USERMESSAGEBOX_H
#define USERMESSAGEBOX_H

#include "framelessdialog.h"

namespace Ui {
class UserMessageBox;
}

class UserMessageBox : public FramelessDialog
{
    Q_OBJECT

public:
    enum ButtonType
    {
        NoButton           = 0x00000000,
        Ok                 = 0x00000400,
        Cancel           = 0x00000800
    };

    enum NoticeType
    {
        Info,
        Warn,
        Ques,
        Fail
    };
public:
    explicit UserMessageBox(QWidget *parent = nullptr);
    ~UserMessageBox() override;

    static ButtonType information(QWidget *parent, const QString &title,
                                  const QString& text,
                                  ButtonType button0, ButtonType button1 = NoButton);

    inline static ButtonType information(QWidget *parent, const QString &title,
                               const QString& text) {
        return information(parent, title, text, Ok, NoButton);
    }

    static ButtonType question(QWidget *parent, const QString &title,
                               const QString& text,
                               ButtonType button0,
                               ButtonType button1);

    inline static ButtonType question(QWidget *parent, const QString &title,
                               const QString& text) {
        return question(parent, title, text, Ok, Cancel);
    }

    static ButtonType warning(QWidget *parent, const QString &title,
                              const QString& text,
                              ButtonType button0,
                              ButtonType button1,
                              const QString &button0Text = "",
                              const QString &button1Text = "");

    inline static ButtonType warning(QWidget *parent, const QString &title,
                              const QString& text)
    {
        return warning(parent, title, text, Ok, NoButton);
    }

    static ButtonType critical(QWidget *parent, const QString &title,
                               const QString& text,
                               ButtonType button0, ButtonType button1);

    inline static ButtonType critical(QWidget *parent, const QString &title,
                               const QString& text) {
        return critical(parent, title, text, Ok, NoButton);
    }

    void setTitle(const QString &title);

    void setText(const QString &text);

    void setButton1Hidden(bool hidden);

    void setButton0Name(const QString &buttonTitle);

    void setButton1Name(const QString &buttonTitle);

private:
    void paintEvent(QPaintEvent *) override;

    static ButtonType showMessageBox(QWidget *parent, const QString &title,
                    const QString& text,
                    ButtonType button0, ButtonType button1,
                    NoticeType = NoticeType::Info,
                    const QString &button0Title = "", const QString &button1Title = "");
private:
    Ui::UserMessageBox *ui;
};

#endif // USERMESSAGEBOX_H
