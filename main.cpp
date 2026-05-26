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
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFileInfo>
#include "database.hpp"
#include "form_dialog.hpp"

namespace application_gui
{

    class JobApplicationTracker : public QWidget
    {
    public:
        JobApplicationTracker(QWidget *parent = nullptr) : QWidget(parent)
        {

            auto &database = database::get();
            database.initialize();

            auto *mainLayout = new QVBoxLayout(this);

            auto *titleLabel = new QLabel("<h2>公募・選考進捗管理ダッシュボード</h2>", this);
            mainLayout->addWidget(titleLabel);

            auto *dataBtnGroupBox = new QGroupBox("データ管理", this);
            auto *dataBtnLayout = new QHBoxLayout(dataBtnGroupBox);
            btnform_ = new QPushButton("新規公募情報の登録", this);
            btnLoad_ = new QPushButton("データを読み込む", this);
            btnSave_ = new QPushButton("データを保存する", this);
            btnOverride_ = new QPushButton("データを上書き保存", this);
            dataBtnLayout->addWidget(btnform_);
            dataBtnLayout->addWidget(btnLoad_);
            dataBtnLayout->addWidget(btnSave_);
            dataBtnLayout->addWidget(btnOverride_);
            mainLayout->addWidget(dataBtnGroupBox);

            btnHidePastApp = new QPushButton("過去の公募を非表示", this);
            mainLayout->addWidget(btnHidePastApp);
            btnHidePastApp->setCheckable(true);
            btnHidePastApp->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

            selectedDataFile_ = new QLabel("保存先データ：未選択", this);
            mainLayout->addWidget(selectedDataFile_);

            tableWidget_ = new QTableWidget(0, 9, this);
            tableWidget_->setHorizontalHeaderLabels({"応募先",
                                                     "締切日",
                                                     "応募方法",
                                                     "必要提出書類",
                                                     "フォルダを開く",
                                                     "URL",
                                                     "進捗率", "", ""});

            // tableWidget_->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
            //  tableWidget_->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
            //  tableWidget_->horizontalHeader()->setSectionResizeMode(6, QHeaderView::ResizeToContents);
            tableWidget_->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
            tableWidget_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
            mainLayout->addWidget(tableWidget_);

            // this->insertSampleData();
            this->updateTableWidget();
            connect(btnform_, &QPushButton::clicked, this, &JobApplicationTracker::openFormDialog);
            connect(btnLoad_, &QPushButton::clicked, this, &JobApplicationTracker::onLoadButtonClicked);
            connect(btnSave_, &QPushButton::clicked, this, &JobApplicationTracker::onSaveButtonClicked);
            connect(btnOverride_, &QPushButton::clicked, this, &JobApplicationTracker::onOverideButtonClicked);
            connect(btnHidePastApp, &QPushButton::toggled, this, &JobApplicationTracker::onHidePastApplicationsToggled);
            resize(1600, 800);
        }

    private:
        QTableWidget *tableWidget_;
        QPushButton *btnLoad_;
        QPushButton *btnSave_;
        QPushButton *btnOverride_;
        QPushButton *btnform_;
        QPushButton *btnHidePastApp;
        QLabel *selectedDataFile_;
        QString selectedDataFilePath_;

        void insertSampleData()
        {
            auto &database = database::get();
            {
                QDate date = QDate::currentDate().addDays(5);
                QUrl url("https://www.kurims.kyoto-u.ac.jp/ja/research/job.html");
                QDir dir("C:/Users/username/Documents/JobApplications/Kurims");
                database::application app = database::application("○▲大学 国際数理データサイエンス分野",
                                                                  date.year(),
                                                                  date.month(),
                                                                  date.day(),
                                                                  database::application::submit::mail,
                                                                  url,
                                                                  dir);
                database.add_application(app);
            }

            {
                QDate date = QDate::currentDate().addDays(30);
                QUrl url("https://www.abc-research.com/careers");
                QDir dir("C:/Users/username/Documents/JobApplications/ABCResearch");
                database::application app = database::application("未来テクノロジー株式会社 R&D部門",
                                                                  date.year(),
                                                                  date.month(),
                                                                  date.day(),
                                                                  database::application::submit::email,
                                                                  url,
                                                                  dir);
                database.add_application(app);
            }
        }

        void updateTableWidget()
        {
            auto &database = database::get();
            tableWidget_->setRowCount(0);
            auto &app_list = database.get_application_list();

            for (auto &pair : app_list)
            {
                const std::vector<database::application> &app_vec = pair.second;
                for (size_t i = 0; i < app_vec.size(); ++i)
                {
                    const auto &app = app_vec[i];
                    addApplicationData(app);
                }
            }
        }

        void onHidePastApplicationsToggled(const bool checked)
        {
            if (checked)
            {
                for (int i = 0; i < tableWidget_->rowCount(); ++i)
                {
                    QTableWidgetItem *item = tableWidget_->item(i, 1);
                    if (item)
                    {
                        QDate deadline = QDate::fromString(item->text(), "yyyy-MM-dd");
                        if (deadline < QDate::currentDate())
                        {
                            tableWidget_->hideRow(i);
                        }
                    }
                }
            }
            else
            {
                for (int i = 0; i < tableWidget_->rowCount(); ++i)
                {
                    tableWidget_->showRow(i);
                }
            }
        }

        void addApplicationData(const database::application &app)
        {
            const QString &institution = QString::fromStdString(app.get_institution_name());
            const QDate deadline = QDate(app.get_year_deadline(),
                                         app.get_month_deadline(),
                                         app.get_day_deadline());
            QString method = "メール";
            if (app.get_submit() == database::application::submit::mail)
            {
                method = "郵送";
            }
            else if (app.get_submit() == database::application::submit::web)
            {
                method = "web応募";
            }
            int row = tableWidget_->rowCount();
            tableWidget_->insertRow(row);

            auto *deleteBtn = new QPushButton("削除", this);
            tableWidget_->setCellWidget(row, 7, deleteBtn);
            connect(deleteBtn, &QPushButton::clicked, [this, row]()
                    {
            this->tableWidget_->removeRow(row);
            updateDataBase(); });

            auto *editBtn = new QPushButton("編集", this);
            tableWidget_->setCellWidget(row, 8, editBtn);
            connect(editBtn, &QPushButton::clicked, [app, this]()
                    {
            const int day = app.get_day_deadline();
            const int month = app.get_month_deadline();
            const int year = app.get_year_deadline();
            const int hash_date = day + 31*(month + 12*year);
            auto& database = database::get();
            database.remove_application(hash_date, app.get_institution_name()); 
            formDialog dialog(app);
            if (dialog.exec() == QDialog::Accepted) {
                this->updateTableWidget(); 
            } });

            // 1. 応募先
            auto *itemInst = new QTableWidgetItem(institution);
            itemInst->setFlags(itemInst->flags() & ~Qt::ItemIsEditable);
            tableWidget_->setItem(row, 0, itemInst);
            itemInst->setToolTip(QString::fromStdString(app.get_note()));

            // 2. 締切日
            auto *itemDate = new QTableWidgetItem(deadline.toString("yyyy-MM-dd"));
            itemDate->setFlags(itemDate->flags() & ~Qt::ItemIsEditable);
            qint64 daysToDeadline = QDate::currentDate().daysTo(deadline);
            if (daysToDeadline >= 0)
            {
                itemDate->setBackground(QColor("#f8d7da"));
                itemDate->setForeground(QColor("#721c24"));
                itemDate->setFont(QFont("Arial", -1, QFont::Bold));
            }
            tableWidget_->setItem(row, 1, itemDate);

            // 3. 応募方法
            auto *itemMethod = new QTableWidgetItem(method);
            if (method.contains("郵送"))
            {
                itemMethod->setForeground(QColor("#e67e22"));
            }
            itemMethod->setFlags(itemMethod->flags() & ~Qt::ItemIsEditable);
            tableWidget_->setItem(row, 2, itemMethod);

            // 4. 必要提出書類（セルの中に複数のチェックボックスを配置）
            auto *scrollWidget = new QWidget(this);
            auto *boxLayout = new QHBoxLayout(scrollWidget);
            boxLayout->setContentsMargins(5, 2, 5, 2);

            QStringList docs;
            const std::vector<database::application::item> &item_list = app.get_item_list();
            for (const auto &item : item_list)
            {
                docs.append(QString::fromStdString(item.name_));
            }

            for (const QString &doc : docs)
            {
                auto *cb = new QCheckBox(doc, scrollWidget);
                boxLayout->addWidget(cb);
                connect(cb, &QCheckBox::checkStateChanged, [row, this]()
                        { this->updateProgress(row); });
            }
            tableWidget_->setCellWidget(row, 3, scrollWidget);
            tableWidget_->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
            tableWidget_->resizeColumnToContents(3);

            // 4. フォルダを開くボタン
            auto *btnOpenFolder = new QPushButton("開く", this);
            QIcon folderIcon = style()->standardIcon(QStyle::SP_DirOpenIcon);
            btnOpenFolder->setIcon(folderIcon);
            tableWidget_->setCellWidget(row, 4, btnOpenFolder);
            connect(btnOpenFolder, &QPushButton::clicked, [this, app]()
                    {
            QString folderPath = QString::fromStdString(app.get_directory_path());
            if (folderPath.isEmpty()) {
                QMessageBox::information(this, "フォルダ情報なし", "この公募には関連フォルダの情報がありません。");
                return;
            }
            QDesktopServices::openUrl(QUrl::fromLocalFile(folderPath)); });

            // 5. URLを開くボタン
            auto *btnUrl = new QPushButton(QString::fromStdString(app.get_url()), this);
            connect(btnUrl, &QPushButton::clicked, [this, app]()
                    {
            QString urlStr = QString::fromStdString(app.get_url());
            if (urlStr.isEmpty()) {
                QMessageBox::information(this, "URL情報なし", "この公募にはURLの情報がありません。");
                return;
            }
            QDesktopServices::openUrl(QUrl(urlStr)); });
            tableWidget_->setCellWidget(row, 5, btnUrl);

            // 6. 進捗率の初期表示
            auto *itemProgress = new QTableWidgetItem("0 %");
            itemProgress->setTextAlignment(Qt::AlignCenter);
            itemProgress->setFlags(itemProgress->flags() & ~Qt::ItemIsEditable);
            tableWidget_->setItem(row, 6, itemProgress);

            updateProgress(row);
        }

        void updateProgress(int row)
        {
            auto *scrollWidget = tableWidget_->cellWidget(row, 3);
            if (!scrollWidget)
                return;

            QList<QCheckBox *> checkBoxes = scrollWidget->findChildren<QCheckBox *>();
            int total = checkBoxes.size();
            int checkedCount = 0;

            for (auto *cb : checkBoxes)
            {
                if (cb->isChecked())
                {
                    checkedCount++;
                }
            }

            int percent = (total > 0) ? (checkedCount * 100 / total) : 0;

            QTableWidgetItem *item = tableWidget_->item(row, 6);
            if (item)
            {
                item->setText(QString("%1 %").arg(percent));
                if (percent == 100)
                {
                    item->setBackground(QColor("#d4edda")); // 完了したら緑
                    item->setForeground(QColor("#155724"));
                }
                else
                {
                    item->setBackground(Qt::white); // それ以外は白
                    item->setForeground(Qt::black);
                    // item->setBackground(Qt::noStyle().background());
                    // item->setForeground(Qt::noStyle().foreground());
                }
            }
        }

        void onLoadButtonClicked()
        {
            QString filename = QFileDialog::getOpenFileName(this, "データファイルを選択", "", "JSON Files (*.json)");
            if (!filename.isEmpty())
            {
                auto &database = database::get();
                database.loadData(filename.toStdString());
                updateTableWidget();
                selectedDataFilePath_ = filename;
                selectedDataFile_->setText(QString("保存先データ：%1").arg(selectedDataFilePath_));
            }
        }

        void onSaveButtonClicked()
        {
            QString filename = QFileDialog::getSaveFileName(this, "データファイルを保存", "", "JSON Files (*.json)");
            if (!filename.isEmpty())
            {
                if (filename.endsWith(".json", Qt::CaseInsensitive) == false)
                {
                    filename += ".json";
                }
                if (filename.endsWith(".json", Qt::CaseInsensitive) == false)
                {
                    QMessageBox::warning(this, "保存エラー", "ファイル名は.jsonで終わる必要があります。");
                    return;
                }
                auto &database = database::get();
                database.saveData(filename.toStdString());
                selectedDataFilePath_ = filename;
                selectedDataFile_->setText(QString("保存先データ：%1").arg(selectedDataFilePath_));
            }
        }

        void onOverideButtonClicked()
        {
            if (selectedDataFilePath_.isEmpty())
            {
                QMessageBox::warning(this, "保存エラー", "保存先データが選択されていません。");
                return;
            }
            auto &database = database::get();
            database.saveData(selectedDataFilePath_.toStdString());
        }

        void updateDataBase()
        {
            auto &database = database::get();
            database.initialize();

            int rowCount = tableWidget_->rowCount();
            for (int row = 0; row < rowCount; ++row)
            {
                QString institution = tableWidget_->item(row, 0)->text();
                QDate deadline = QDate::fromString(tableWidget_->item(row, 1)->text(), "yyyy-MM-dd");
                QString methodStr = tableWidget_->item(row, 2)->text();
                database::application::submit submit_method = database::application::submit::email;
                if (methodStr.contains("郵送"))
                {
                    submit_method = database::application::submit::mail;
                }
                else if (methodStr.contains("Web応募"))
                {
                    submit_method = database::application::submit::web;
                }
                QUrl url;
                QDir dir;
                database::application app(institution.toStdString(),
                                          deadline.year(),
                                          deadline.month(),
                                          deadline.day(),
                                          submit_method,
                                          url,
                                          dir);
                database.add_application(app);
            }
        }

        void openFormDialog()
        {
            formDialog dialog(this);
            if (dialog.exec() == QDialog::Accepted)
            {
                updateTableWidget();
            }
        }
    };
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    application_gui::JobApplicationTracker window;
    window.setWindowTitle("公募プロセス・トラッカー");
    window.show();

    return app.exec();
}
