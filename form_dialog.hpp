#include <QApplication>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTableWidget>
#include <QHeaderView>
#include <QLabel>
#include <QDateEdit>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QGroupBox>
#include <QCheckBox>
#include <QMessageBox>
#include <QDate>
#include <QFileDialog>
#include <QStyle>
#include <QIcon>
#include <QDesktopServices>
#include <QUrl>
#include <QInputDialog>
#include <QTextEdit>
#include "database.hpp"

namespace application_gui {

    class formDialog : public QDialog {
    public:
      formDialog(const database::application& app, QWidget *parent = nullptr) : QDialog(parent) {
        setWindowTitle("公募情報の編集");
        setWindowTitle("新規公募情報の登録");
        auto *mainLayout = new QVBoxLayout(this);

        this->resize(400, 300);

        editInstitution = new QLineEdit(this);
        /*editInstitution->setPlaceholderText(app.get_institution_name().empty() ? "応募先（例: ○○大学 / ○○社）" :
            QString::fromStdString(app.get_institution_name()));*/
        editInstitution->setText(QString::fromStdString(app.get_institution_name()));
        mainLayout->addWidget(editInstitution);

        dateEditDeadline = new QDateEdit(QDate(app.get_year_deadline(), app.get_month_deadline(), app.get_day_deadline()), this);
        dateEditDeadline->setCalendarPopup(true); // カレンダーから選択可能にする

        auto *deadlineLayout = new QHBoxLayout();
        deadlineLayout->addWidget(new QLabel("締切:", this));
        deadlineLayout->addWidget(dateEditDeadline);
        mainLayout->addLayout(deadlineLayout);

        comboMethod = new QComboBox(this);
        comboMethod->addItems({"メール",
                                "郵送",
                                "Web応募"});
        comboMethod->setCurrentIndex(static_cast<int>(app.get_submit()));
        mainLayout->addWidget(comboMethod);

        auto* dirLayout = new QHBoxLayout();
        btnDirectory = new QPushButton("フォルダ選択", this);
        editDirectory = new QLineEdit(this);
        editDirectory->setText(QString::fromStdString(app.get_directory_path()));
        editDirectory->setPlaceholderText("フォルダのパス");
        dirLayout->addWidget(btnDirectory);
        dirLayout->addWidget(editDirectory);
        mainLayout->addLayout(dirLayout);
        connect(btnDirectory, &QPushButton::clicked, [this]() {
          QString dir = QFileDialog::getExistingDirectory(this, "関連フォルダを選択");
          if (!dir.isEmpty()) {
            editDirectory->setText(dir);
            }
        });
            
        // urlを入力するためのIcon付きのボタンとテキストフィールドを追加
        auto* urlLayout = new QHBoxLayout();
        btnUrl = new QPushButton("URL入力", this);
        editUrl = new QLineEdit(this);
        editUrl->setText(QString::fromStdString(app.get_url()));
        editUrl->setPlaceholderText("公募情報のURL");
        urlLayout->addWidget(btnUrl);
        urlLayout->addWidget(editUrl);
        mainLayout->addLayout(urlLayout);
        connect(btnUrl, &QPushButton::clicked, [this]() {
            bool ok;
            QString url = QInputDialog::getText(this, "URL入力", "公募情報のURLを入力してください:", QLineEdit::Normal, "", &ok);
            if (ok && !url.isEmpty()) {
                editUrl->setText(url);
            }
        });

        //必要な書類を選択
        docsGroupBox = new QGroupBox("必要提出書類", this);
        docsLayout = new QVBoxLayout(docsGroupBox);
        btnAddDoc = new QPushButton("書類を追加", this);
        docsLayout->addWidget(btnAddDoc);
        connect(btnAddDoc, &QPushButton::clicked, this, &formDialog::onAddButtonClicked);
        for(const auto& doc : app.get_item_list()) {
            QHBoxLayout* docLayout = new QHBoxLayout();
            QComboBox* docBox = new QComboBox(this);
            docBox->addItems({"履歴書", "職務経歴書", "成績証明書", "研究業績リスト", "推薦状"});
            docBox->setEditable(true); // ユーザーが自由に入力できるようにする
            docBox->setCurrentText(QString::fromStdString(doc.get_name()));
            QPushButton* btnDeleteDoc = new QPushButton("削除", this);
            docLayout->addWidget(docBox);
            docLayout->addWidget(btnDeleteDoc);
            connect(btnDeleteDoc, &QPushButton::clicked, [docLayout, docBox, btnDeleteDoc]() {
                // 書類の削除処理
                docBox->deleteLater();
                btnDeleteDoc->deleteLater();
                docLayout->deleteLater();
            });
            docsLayout->addLayout(docLayout);
        }

        mainLayout->addWidget(docsGroupBox);

        editNotes = new QTextEdit(this);
        editNotes->setPlaceholderText("備考欄（例: 連絡先や必要書類など）");
        editNotes->setText(QString::fromStdString(app.get_notes()));
        mainLayout->addWidget(editNotes);

        auto *btnEdit = new QPushButton("編集", this);
        mainLayout->addWidget(btnEdit);
        connect(btnEdit, &QPushButton::clicked, [this]() {
            if (editInstitution->text().trimmed().isEmpty()) {
                QMessageBox::warning(this, "入力エラー", "応募先を入力してください。");
                return;
            }
            if (!editDirectory->text().trimmed().isEmpty() && !QDir(editDirectory->text()).exists()) {
                if(QMessageBox::question(this, "入力エラー", "指定されたフォルダが存在しません。新しいフォルダを作成しますか？", QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
                    QDir().mkpath(editDirectory->text());
                } else {
                    QMessageBox::warning(this, "入力エラー", "指定されたフォルダが存在しません。");
                    return;
                }
            }
            // 編集ボタンがクリックされたときの処理を実装
            QString institution = editInstitution->text();
            QDate deadline = dateEditDeadline->date();
            QString methodStr = comboMethod->currentText();
            database::application::submit submit_method = database::application::submit::email;
            if (methodStr.contains("郵送")) {
                submit_method = database::application::submit::mail;
            } else if (methodStr.contains("Web応募")) {
                submit_method = database::application::submit::web;
            }
            // フォルダパスとURLはここでは仮の値を入れる
            QUrl url(editUrl->text());
            QDir dir(editDirectory->text());
            QString note = editNotes->toPlainText();
            std::vector<std::string> required_docs;
            for (int i = 0; i < docsLayout->count(); ++i) {
                QLayoutItem* item = docsLayout->itemAt(i);
                if(item->layout() == nullptr) {
                    continue; // レイアウトでないアイテムはスキップ
                } 
                QHBoxLayout* docLayout = qobject_cast<QHBoxLayout*>(item->layout());
                if (docLayout) {
                    QComboBox* docBox = qobject_cast<QComboBox*>(docLayout->itemAt(0)->widget());
                    if (docBox) {
                        QString docName = docBox->currentText();
                        if (!docName.isEmpty()) {
                            required_docs.push_back(docName.toStdString());
                        }
                    }
                }
            }
            database::application app(institution.toStdString(),
                                      deadline.year(),
                                      deadline.month(),
                                      deadline.day(),
                                      submit_method,
                                      url,
                                      dir,
                                      note.toStdString(),
                                      required_docs);
            auto& database = database::get();
            database.add_application(app);
            accept(); // ダイアログを閉じる
        });

        auto *btnCancel = new QPushButton("キャンセル", this);
        mainLayout->addWidget(btnCancel);
        connect(btnCancel, &QPushButton::clicked, [this]() {
            reject(); // ダイアログを閉じる
        });
      }
      formDialog(QWidget *parent = nullptr) : QDialog(parent) {
        setWindowTitle("新規公募情報の登録");
        auto *mainLayout = new QVBoxLayout(this);

        this->resize(400, 300);

        editInstitution = new QLineEdit(this);
        editInstitution->setPlaceholderText("応募先（例: ○○大学 / ○○社）");
        mainLayout->addWidget(editInstitution);

        dateEditDeadline = new QDateEdit(QDate::currentDate(), this);
        dateEditDeadline->setCalendarPopup(true); // カレンダーから選択可能にする

        auto *deadlineLayout = new QHBoxLayout();
        deadlineLayout->addWidget(new QLabel("締切:", this));
        deadlineLayout->addWidget(dateEditDeadline);
        mainLayout->addLayout(deadlineLayout);

        comboMethod = new QComboBox(this);
        comboMethod->addItems({"メール",
                                "郵送",
                                "Web応募"});
        mainLayout->addWidget(comboMethod);

        auto* dirLayout = new QHBoxLayout();
        btnDirectory = new QPushButton("フォルダ選択", this);
        editDirectory = new QLineEdit(this);
        editDirectory->setPlaceholderText("フォルダのパス");
        dirLayout->addWidget(btnDirectory);
        dirLayout->addWidget(editDirectory);
        mainLayout->addLayout(dirLayout);
        connect(btnDirectory, &QPushButton::clicked, [this]() {
          QString dir = QFileDialog::getExistingDirectory(this, "関連フォルダを選択");
          if (!dir.isEmpty()) {
            editDirectory->setText(dir);
            }
        });
            
        // urlを入力するためのIcon付きのボタンとテキストフィールドを追加
        auto* urlLayout = new QHBoxLayout();
        btnUrl = new QPushButton("URL入力", this);
        editUrl = new QLineEdit(this);
        editUrl->setPlaceholderText("公募情報のURL");
        urlLayout->addWidget(btnUrl);
        urlLayout->addWidget(editUrl);
        mainLayout->addLayout(urlLayout);
        connect(btnUrl, &QPushButton::clicked, [this]() {
            bool ok;
            QString url = QInputDialog::getText(this, "URL入力", "公募情報のURLを入力してください:", QLineEdit::Normal, "", &ok);
            if (ok && !url.isEmpty()) {
                editUrl->setText(url);
            }
        });

        //必要な書類を選択
        docsGroupBox = new QGroupBox("必要提出書類", this);
        docsLayout = new QVBoxLayout(docsGroupBox);
        btnAddDoc = new QPushButton("書類を追加", this);
        docsLayout->addWidget(btnAddDoc);
        connect(btnAddDoc, &QPushButton::clicked, this, &formDialog::onAddButtonClicked);
        mainLayout->addWidget(docsGroupBox);

        editNotes = new QTextEdit(this);
        editNotes->setPlaceholderText("備考欄（例: 連絡先や必要書類など）");
        mainLayout->addWidget(editNotes);

        auto *btnAdd = new QPushButton("追加", this);
        mainLayout->addWidget(btnAdd);
        connect(btnAdd, &QPushButton::clicked, [this]() {
             if (editInstitution->text().trimmed().isEmpty()) {
                QMessageBox::warning(this, "入力エラー", "応募先を入力してください。");
                return;
            }
             if (!editDirectory->text().trimmed().isEmpty() && !QDir(editDirectory->text()).exists()) {
                if(QMessageBox::question(this, "入力エラー", "指定されたフォルダが存在しません。新しいフォルダを作成しますか？", QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
                    QDir().mkpath(editDirectory->text());
                } else {
                    QMessageBox::warning(this, "入力エラー", "指定されたフォルダが存在しません。");
                    return;
                }
            }
            // 追加ボタンがクリックされたときの処理を実装
            QString institution = editInstitution->text();
            QDate deadline = dateEditDeadline->date();
            QString methodStr = comboMethod->currentText();
            database::application::submit submit_method = database::application::submit::email;
            if (methodStr.contains("郵送")) {
                submit_method = database::application::submit::mail;
            } else if (methodStr.contains("Web応募")) {
                submit_method = database::application::submit::web;
            }
            // フォルダパスとURLはここでは仮の値を入れる
            QUrl url;
            QDir dir;
            QString note = editNotes->toPlainText();
            std::vector<std::string> required_docs;
            for (int i = 0; i < docsLayout->count(); ++i) {
                QLayoutItem* item = docsLayout->itemAt(i);
                if(item->layout() == nullptr) {
                    continue; // レイアウトでないアイテムはスキップ
                } 
                QHBoxLayout* docLayout = qobject_cast<QHBoxLayout*>(item->layout());
                if (docLayout) {
                    QComboBox* docBox = qobject_cast<QComboBox*>(docLayout->itemAt(0)->widget());
                    if (docBox) {
                        QString docName = docBox->currentText();
                        if (!docName.isEmpty()) {
                            required_docs.push_back(docName.toStdString());
                        }
                    }
                }
            }
            if(required_docs.empty()) {
                QMessageBox::warning(this, "入力エラー", "必要な書類を選択してください。");
                return;
            }
            database::application app(institution.toStdString(),
                                      deadline.year(),
                                      deadline.month(),
                                      deadline.day(),
                                      submit_method,
                                      url,
                                      dir,
                                      note.toStdString(),
                                      required_docs);
            auto& database = database::get();
            database.add_application(app);
            accept(); // ダイアログを閉じる
        });

        auto *btnCancel = new QPushButton("キャンセル", this);
        mainLayout->addWidget(btnCancel);
        connect(btnCancel, &QPushButton::clicked, [this]() {
            reject(); // ダイアログを閉じる
        });


      }

      void onAddButtonClicked(){
        QHBoxLayout* addDocLayout = new QHBoxLayout();
        QPushButton* btnDeleteDoc = new QPushButton("削除", this);
        QComboBox* docBox = new QComboBox(this);
        docBox->addItems({"履歴書", "職務経歴書", "成績証明書", "研究業績リスト", "推薦状"});
        docBox->setEditable(true); // ユーザーが自由に入力できるようにする
        addDocLayout->addWidget(docBox);
        addDocLayout->addWidget(btnDeleteDoc);
        connect(btnDeleteDoc, &QPushButton::clicked, [addDocLayout, docBox, btnDeleteDoc]() {
            // 書類の削除処理
            docBox->deleteLater();
            btnDeleteDoc->deleteLater();
            addDocLayout->deleteLater();
        });
        docsLayout->addLayout(addDocLayout);
      }

      QLineEdit *editInstitution;
      QDateEdit *dateEditDeadline;
      QComboBox *comboMethod;
      QPushButton *btnDirectory;
      QLineEdit *editDirectory;
      QPushButton *btnUrl;
      QLineEdit *editUrl;
      QTextEdit *editNotes;
      QGroupBox *docsGroupBox;
      QVBoxLayout *docsLayout;
      QPushButton *btnAddDoc;

    };
}
