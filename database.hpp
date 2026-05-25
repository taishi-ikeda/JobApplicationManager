#ifndef INCLUDE_HEAR_PARALLEL_MANAGER_HPP
#define INCLUDE_HEAR_PARALLEL_MANAGER_HPP
#include <iostream>
#include <string>
#include <QFileInfo>
#include <vector>
#include <QFile>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QUrl>
#include <map>

namespace application_gui
{
  class database
  {
  public:
    class application
    {
    public:
      enum class submit
      {
        email,
        mail,
        web
      };
      class item
      {
      public:
        enum class Status
        {
          Preparing,
          Finish
        };
        enum class Type
        {
          Optional,
          Mandatory
        };
        item(const std::string &name)
        {
          name_ = name;
        }
        void set_status_preparing()
        {
          status_ = Status::Preparing;
        }
        void set_status_finish()
        {
          status_ = Status::Finish;
        }
        Type get_type() const
        {
          return type_;
        }
        Status get_status() const
        {
          return status_;
        }
        std::string get_name() const
        {
          return name_;
        }
        void set_type_optional()
        {
          type_ = Type::Optional;
        }
        void set_type_mandatory()
        {
          type_ = Type::Mandatory;
        }
        std::string name_;
        Status status_;
        Type type_;
        QFileInfo file_info_;
      };

    public:
      application(const std::string &institution_name,
                  const int year_deadline,
                  const int month_deadline,
                  const int day_deadline,
                  const submit sub,
                  const QUrl &url,
                  const QDir &dir,
                  const std::string &note = "",
                  const std::vector<std::string> &required_docs = {})
      {
        institution_name_ = institution_name;
        year_deadline_ = year_deadline;
        month_deadline_ = month_deadline;
        day_deadline_ = day_deadline;
        submit_ = sub;
        url_ = url;
        dir_ = dir;
        note_ = note;
        for (const auto &doc : required_docs)
        {
          item it(doc);
          it.set_status_preparing();
          it.set_type_mandatory();
          item_list_.push_back(it);
        }
      }
      void set_year_deadline(const int y)
      {
        year_deadline_ = y;
      }
      void set_month_deadline(const int m)
      {
        month_deadline_ = m;
      }
      void set_day_deadline(const int d)
      {
        day_deadline_ = d;
      }
      int get_year_deadline() const { return year_deadline_; }
      int get_month_deadline() const { return month_deadline_; }
      int get_day_deadline() const { return day_deadline_; }
      int get_hash_deadline() const
      {
        return year_deadline_ * 366 + month_deadline_ * 12 + day_deadline_;
      }
      void set_institution_name(const std::string &name)
      {
        institution_name_ = name;
      }
      std::string get_institution_name() const
      {
        return institution_name_;
      }
      submit get_submit() const
      {
        return submit_;
      }
      const std::vector<item> &get_item_list() const
      {
        return item_list_;
      }
      void append_item(const item &it)
      {
        item_list_.push_back(it);
      }
      std::string get_directory_path() const
      {
        return dir_.absolutePath().toStdString();
      }
      std::string get_url() const
      {
        return url_.toString().toStdString();
      }
      std::string get_note() const
      {
        return note_;
      }

    private:
      std::string institution_name_;
      std::vector<item> item_list_;
      int year_deadline_;
      int month_deadline_;
      int day_deadline_;
      submit submit_;
      QUrl url_;
      QDir dir_;
      std::string note_;
    };

  public:
    static database &get()
    {
      static database db;
      return db;
    }
    void initialize()
    {
      application_list_.clear();
    }
    void add_application(application &app)
    {
      const int day = app.get_day_deadline();
      const int month = app.get_month_deadline();
      const int year = app.get_year_deadline();

      const int date_hash = day + 31 * (month + 12 * year);
      if (application_list_.count(date_hash) == 0)
      {
        std::vector<application> app_vec;
        app_vec.push_back(app);
        application_list_[date_hash] = app_vec;
      }
      else
      {
        std::vector<application> &app_vec = application_list_[date_hash];
        for (size_t i = 0; i < app_vec.size(); ++i)
        {
          const application &existing_app = app_vec[i];
          if (existing_app.get_institution_name() == app.get_institution_name())
          {
            std::cerr << "Error: An application with the same institution name already exists for the same deadline." << std::endl;
            return;
          }
        }
        application_list_[date_hash].push_back(app);
      }
    }
    void remove_application(const int date_hash, const std::string &institution_name)
    {
      if (application_list_.count(date_hash) == 0)
      {
        std::cerr << "Error: No applications found for the given deadline." << std::endl;
        return;
      }
      std::vector<application> &app_vec = application_list_[date_hash];
      for (size_t i = 0; i < app_vec.size(); ++i)
      {
        const application &existing_app = app_vec[i];
        if (existing_app.get_institution_name() == institution_name)
        {
          app_vec.erase(app_vec.begin() + i);
          if (app_vec.empty())
          {
            application_list_.erase(date_hash);
          }
          return;
        }
      }
      std::cerr << "Error: No application found with the given institution name for the specified deadline." << std::endl;
    }
    std::map<int, std::vector<application>> &get_application_list()
    {
      return application_list_;
    }
    void saveData(const std::string &filename)
    {
      QJsonArray jsonArray;
      for (auto &pair : application_list_)
      {
        const auto &app_vec = pair.second;
        for (size_t i = 0; i < app_vec.size(); ++i)
        {
          const auto &app = app_vec[i];
          QJsonObject jsonObj;
          jsonObj["institution_name"] = QString::fromStdString(app.get_institution_name());
          jsonObj["year_deadline"] = app.get_year_deadline();
          jsonObj["month_deadline"] = app.get_month_deadline();
          jsonObj["day_deadline"] = app.get_day_deadline();
          jsonObj["submit"] = static_cast<int>(app.get_submit());
          jsonObj["url"] = QString::fromStdString(app.get_url());
          jsonObj["directory_path"] = QString::fromStdString(app.get_directory_path());
          const auto &item_list = app.get_item_list();
          QJsonArray jsonItemArray;
          for (size_t j = 0; j < item_list.size(); ++j)
          {
            const auto &item = item_list[j];
            QJsonObject jsonItemObj;
            jsonItemObj["name"] = QString::fromStdString(item.name_);
            jsonItemObj["status"] = static_cast<int>(item.get_status());
            jsonItemObj["type"] = static_cast<int>(item.get_type());
            jsonItemObj["file_info"] = item.file_info_.absoluteFilePath();
            jsonItemArray.append(jsonItemObj);
          }
          jsonObj["items"] = jsonItemArray;
          jsonArray.append(jsonObj);
        }
      }

      QJsonDocument jsonDoc(jsonArray);
      QFile file(QString::fromStdString(filename));
      if (!file.open(QIODevice::WriteOnly))
      {
        std::cerr << "Could not open file for writing: " << filename << std::endl;
        return;
      }
      file.write(jsonDoc.toJson());
    }
    void loadData(const std::string &filename)
    {
      QFile file(QString::fromStdString(filename));
      if (!file.open(QIODevice::ReadOnly))
      {
        std::cerr << "Could not open file for reading: " << filename << std::endl;
        return;
      }
      QByteArray data = file.readAll();
      QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
      if (!jsonDoc.isArray())
      {
        std::cerr << "Invalid JSON format: expected an array" << std::endl;
        return;
      }
      QJsonArray jsonArray = jsonDoc.array();
      application_list_.clear();
      for (const auto &jsonValue : jsonArray)
      {
        if (!jsonValue.isObject())
        {
          std::cerr << "Invalid JSON format: expected an object" << std::endl;
          continue;
        }
        QJsonObject jsonObj = jsonValue.toObject();
        std::string institution_name = jsonObj["institution_name"].toString().toStdString();
        int year_deadline = jsonObj["year_deadline"].toInt();
        int month_deadline = jsonObj["month_deadline"].toInt();
        int day_deadline = jsonObj["day_deadline"].toInt();
        std::string directory_path = jsonObj["directory_path"].toString().toStdString();
        std::string url = jsonObj["url"].toString().toStdString();
        application::submit submit_method = static_cast<application::submit>(jsonObj["submit"].toInt());
        application app(institution_name,
                        year_deadline,
                        month_deadline,
                        day_deadline,
                        submit_method,
                        QUrl(QString::fromStdString(url)),
                        QDir(QString::fromStdString(directory_path)));
        QJsonArray jsonItemArray = jsonObj["items"].toArray();
        for (const auto &itemValue : jsonItemArray)
        {
          if (!itemValue.isObject())
          {
            std::cerr << "Invalid JSON format: expected an object" << std::endl;
            continue;
          }
          QJsonObject jsonItemObj = itemValue.toObject();
          std::string item_name = jsonItemObj["name"].toString().toStdString();
          application::item item(item_name);
          item.set_status_preparing();
          item.set_type_optional();
          item.file_info_ = QFileInfo(jsonItemObj["file_info"].toString());
          app.append_item(item);
        }
        add_application(app);
      }
    }

  private:
    std::map<int, std::vector<application>> application_list_;
  };
}
#endif
