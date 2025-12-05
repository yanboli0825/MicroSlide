// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef CONNECTION_H
#define CONNECTION_H

#include <QMessageBox>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>

/*
    This file defines a helper function to open a connection to an
    in-memory SQLITE database and to create a test table.

    If you want to use another database, simply modify the code
    below. All the examples in this directory use this function to
    connect to a database.
*/
//! [0]
static bool createDataBase()
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(":memory:");
    if (!db.open()) {
        QMessageBox::critical(nullptr, QObject::tr("Cannot open database"),
            QObject::tr("Unable to establish a database connection.\n"
                        "This example needs SQLite support. Please read "
                        "the Qt SQL driver documentation for information how "
                        "to build it.\n\n"
                        "Click Cancel to exit."), QMessageBox::Cancel);
        return false;
    }

    QSqlQuery query;
    query.exec("create table person (id int primary key, "
               "firstname varchar(20), lastname varchar(20))");
    query.exec("insert into person values(101, 'Danny', 'Young')");
    query.exec("insert into person values(102, 'Christine', 'Holand')");
    query.exec("insert into person values(103, 'Lars', 'Gordon')");
    query.exec("insert into person values(104, 'Roberto', 'Robitaille')");
    query.exec("insert into person values(105, 'Maria', 'Papadopoulos')");

    query.exec("create table items (id int primary key,"
                                             "imagefile int,"
                                             "itemtype varchar(20),"
                                             "description varchar(100))");
    query.exec("insert into items "
               "values(0, 0, 'Qt',"
               "'Qt is a full development framework with tools designed to "
               "streamline the creation of stunning applications and  "
               "amazing user interfaces for desktop, embedded and mobile "
               "platforms.')");
    query.exec("insert into items "
               "values(1, 1, 'Qt Quick',"
               "'Qt Quick is a collection of techniques designed to help "
               "developers create intuitive, modern-looking, and fluid "
               "user interfaces using a CSS & JavaScript like language.')");
    query.exec("insert into items "
               "values(2, 2, 'Qt Creator',"
               "'Qt Creator is a powerful cross-platform integrated "
               "development environment (IDE), including UI design tools "
               "and on-device debugging.')");
    query.exec("insert into items "
               "values(3, 3, 'Qt Project',"
               "'The Qt Project governs the open source development of Qt, "
               "allowing anyone wanting to contribute to join the effort "
               "through a meritocratic structure of approvers and "
               "maintainers.')");

    query.exec("create table images (itemid int, file varchar(20))");
    query.exec("insert into images values(0, 'images/qt-logo.png')");
    query.exec("insert into images values(1, 'images/qt-quick.png')");
    query.exec("insert into images values(2, 'images/qt-creator.png')");
    query.exec("insert into images values(3, 'images/qt-project.png')");

    return true;
}
//! [0]


//wll use
//使用自定义 connectionName 创建连接
bool createConnectionByName(const QString &connectionName, const QString &db_name){

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
    // 数据库连接需要设置的信息
    // db.setHostName("127.0.0.1"); // 数据库服务器IP，我用的是本地电脑
    // db.setPort(3306);// 端口号
    QString _name = db_name + ".db";
    db.setDatabaseName(_name);
    //设置用户名和密码
    db.setUserName("HPEC");
    db.setPassword("123456");

    // 连接数据库判断
    bool ok = db.open();

    if (ok){
        qDebug() << "database connect is ok";
        return true;
    } else {
        QMessageBox::critical(0, "Cannot open database",
                              "Unable to establish a database connection", QMessageBox::Cancel);
        return false;
        //qDebug() << "database connect is fail";
    }
}


static bool updateDiagnosisResult(int patientId, const QString& newResult,QSqlDatabase db) {

    QSqlQuery query(db);
    query.prepare("UPDATE patients SET diagnosis_result = :result WHERE patient_id = :id");
    query.bindValue(":result", newResult);
    query.bindValue(":id", patientId);

    if (!query.exec()) {
        QMessageBox::critical(nullptr, "Update Error", query.lastError().text());
        return false;
    }

    return true;
}



// 使用自定义 connectionName 获取连接
QSqlDatabase getConnectionByName(const QString &connectionName) {
    // 获取数据库连接
    return QSqlDatabase::database(connectionName);
}



/*
 * 功能描述：数据增操作
 * 向数据库中插入一条数据记录，名称绑定的方式实现
 * @param QSqlDatabase：数据库连接
 * @param id:用户id
 * @param name:用户名
 */

// void insertUserNameGenderAge(QSqlDatabase& db,const int& id,const QString &name,
//                              const QString &gender, const int &age) {
//     QSqlQuery query(db);
//     query.prepare("INSERT INTO patients (id, name, gender, age) VALUES (:id, :name, :gender, :age);");
//     query.bindValue(":id", id);
//     query.bindValue(":name", name);
//     query.bindValue(":gender", gender);
//     query.bindValue(":age", age);
//     if (!query.exec()) {
//         qDebug() << query.lastError();
//     }
// }

void insertUserNameGenderAge(QSqlDatabase& db,const QString &name,
                    const QString &gender, const int &age) {
    QSqlQuery query(db);
    query.prepare("INSERT INTO patients (name, gender, age) VALUES (:name, :gender, :age);");
    query.bindValue(":name", name);
    query.bindValue(":gender", gender);
    query.bindValue(":age", age);
    if (!query.exec()) {
        qDebug() << query.lastError();
    }
}

/*
 * 功能描述：数据删操作
 * 从数据库中删除一条数据，名称绑定的方式实现
 * @param QSqlDatabase：数据库连接
 * @param name:用户名
 */
void deleteUserNameGenderAge(QSqlDatabase db,const QString &name) {
    QSqlQuery query(db);
    query.prepare("DELETE FROM patients WHERE name=:name");
    query.bindValue(":name", name);
    query.exec();
}

/*
 * 功能描述：更新数据
 * 修改传入的 id 的 name
 * @param QSqlDatabase：数据库连接
 * @param id:用户id
 * @param name:用户名
 */
void updateUser(QSqlDatabase db,const int &id,const QString &name) {
    QSqlQuery query(db);
    query.prepare("update patients set name=:name WHERE id=:id");
    query.bindValue(":id", id);
    query.bindValue(":name", name);
    query.exec();
}

/*
 * 功能描述：数据查操作，查询所有数据
 * 执行SQL语句的方式，查询所有的用户数据记录
 * @param QSqlDatabase：数据库连接
 */
void queryAllUser(QSqlDatabase db) {
    QString sql = "SELECT id, name,gender,age,"
                  "diag_date,pathology_result FROM patients" ; // 组装sql语句
    QSqlQuery query(db);                               // [1] 传入数据库连接
    query.exec(sql);                                   // [2] 执行sql语句
    while (query.next()) {                             // [3] 遍历查询结果
        qDebug() << QString("Id: %1, name: %2, gender: %3,"
                            "age: %4,diag_date: %5,"
                            "pathology_result: %6.")
                        .arg(query.value("id").toInt())
                        .arg(query.value("name").toString())
                        .arg(query.value("gender").toString())
                        .arg(query.value("age").toInt())
                        .arg(query.value("diag_date").toString())
                        .arg(query.value("pathology_result").toString());
    }
}


/*
 * 功能描述：查询一条数据记录
 * 数据查操作，SQL语句的方式实现
 * @param QSqlDatabase：数据库连接
 * @param name:用户名
 */
void selectQueryUser(QSqlDatabase db,const QString &name) {
    QString sql = "SELECT * FROM patients WHERE name='" + name + "'";
    QSqlQuery query(db);    // [1] 传入数据库连接
    query.exec(sql);        // [2] 执行sql语句
    while (query.next()) {  // [3] 遍历查询结果
        qDebug() << QString("Id: %1, name: %2, gender: %3,"
                            "age: %4,diag_date: %5,"
                            "pathology_result: %6.")
                        .arg(query.value("id").toInt())
                        .arg(query.value("name").toString())
                        .arg(query.value("gender").toString())
                        .arg(query.value("age").toInt())
                        .arg(query.value("diag_date").toString())
                        .arg(query.value("pathology_result").toString());
    }
}

/*
 * 功能描述：查询一条数据记录
 * 数据查操作，名称绑定的方式实现
 * @param QSqlDatabase：数据库连接
 * @param name:用户名
 */
void preparedQueryUser(QSqlDatabase db,const QString &name) {
    QString sql = "SELECT * FROM patients WHERE name=:name";
    QSqlQuery query(db);                    // [1] 传入数据库连接
    query.prepare(sql);                     // [2] 使用名称绑定的方式解析 SQL 语句
    query.bindValue(":name", name); // [3] 把占位符替换为传入的参数
    query.exec();                           // [4] 执行数据库操作
    while (query.next()) {                  // [5] 遍历查询结果
        qDebug() << QString("Id: %1, name: %2, gender: %3,"
                            "age: %4,diag_date: %5,"
                            "pathology_result: %6.")
                        .arg(query.value("id").toInt())
                        .arg(query.value("name").toString())
                        .arg(query.value("gender").toString())
                        .arg(query.value("age").toInt())
                        .arg(query.value("diag_date").toString())
                        .arg(query.value("pathology_result").toString());
    }
}

void db_run(){
    //创建数据库连接
    createConnectionByName("firstConnect","test");
    QSqlDatabase db = getConnectionByName("firstConnect");
    QSqlQuery query(db);

    //创建一个表格
    query.exec("CREATE TABLE IF NOT EXISTS patients "
               "(id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,"
               "name VARCHAR(50) DEFAULT 'unknown', "
               "gender CHAR(1) DEFAULT '男', "
               "age INT DEFAULT 50, "
               "diag_date DATETIME DEFAULT CURRENT_TIMESTAMP, "
               "pathology_result TEXT DEFAULT 'unknown')");
    //插入内容
    insertUserNameGenderAge(db,"被试一","男",50);
    insertUserNameGenderAge(db,"被试二","男",46);
    insertUserNameGenderAge(db,"被试三","男",48);
    insertUserNameGenderAge(db,"被试四","女",46);

    // queryAllUser(db);
}



#endif
