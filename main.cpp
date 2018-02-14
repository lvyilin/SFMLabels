#include <QCoreApplication>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QDirIterator>
#include <QFileInfo>
#include <QVariant>
#include <QDebug>
#include <QTextStream>
#include <QTextCodec>

void dict_import(QSqlQuery &q, const QString &path, int lvl, int parent);
void sql_exec(QSqlQuery &q, QString name, int level, int parent, bool is_leaf, QString type);
int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "SFM");
    db.setDatabaseName("sfmlabel.db");
    db.open();
    QSqlQuery query(db);
    query.exec("create table labels("
               "id integer primary key autoincrement NOT NULL,"
               "name varchar(255) NOT NULL,"
               "level int,"
               "parent int,"
               "is_leaf bool NOT NULL,"
               "type varchar(20) NOT NULL"
               ")");

    QDirIterator it("F:\\sogou_dicts_chinese"
                    , QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot
                    , QDirIterator::Subdirectories
                   );
    while (it.hasNext())
    {
        QString dirStr = it.next();
        QFileInfo dir(dirStr);
        QDir dir2(dirStr);
        qDebug() << dirStr;
        int lvl = 0;
        QString type;

        if (dir.isDir())
        {

            QDir thisDir = dir2;
            QDir sogouPath("F:\\sogou_dicts_chinese");
            QDir fPath("F:\\");
            while (thisDir != sogouPath)
            {
                if (thisDir == fPath)
                    break;
                ++lvl;
                thisDir.cdUp();
            }

            if (lvl == 0)
                a.exit(1);

            QString name = dir2.dirName();
            type = "field";
            int parent = 0;
            if (lvl == 1)
                parent = 0;
            else
            {
                dir2.cdUp();
                QString n = dir2.dirName();
                query.exec(QString("select id from labels where name=\"%1\"").arg(n));
                if (query.next())
                {
                    parent = query.value(0).toString().toInt();
                }
                else
                    qDebug() << "something wrong";
            }
            sql_exec(query, name, lvl, parent, false, type);
        }

        else if (dir.isFile())
        {
            int parent = 0;
            QDir thisDir = dir.dir();
            query.exec(QString("select id,level from labels where name=\"%1\"").arg(thisDir.dirName()));
            if (query.next())
            {
                parent = query.value(0).toString().toInt();
                lvl = query.value(1).toString().toInt();
                lvl++;
            }
            else
                qDebug() << "something wrong";
            dict_import(query, dirStr, lvl, parent);
        }
        else;

    }
    return a.exec();
}

void dict_import(QSqlQuery &q, const QString &path, int lvl, int parent)
{
    qDebug() << path;
    QFile f(path);
    if (!f.open(QFile::ReadOnly | QFile::Text))
        qDebug() << "something wrong";

    QTextStream in(&f);
    in.setCodec("UTF-8");
    while (!in.atEnd())
    {
        QString name = in.readLine();
        sql_exec(q, name, lvl, parent, true, "keyword");
    }
    qDebug() << "import done: " << path;
    f.close();
}

void sql_exec(QSqlQuery &q, QString name, int level, int parent, bool is_leaf, QString type)
{
    qDebug() << name << " " << level << " " << parent << " " << is_leaf << " " << type;
    if (parent == 0)
    {
        q.prepare("insert into labels(name,level,is_leaf,type) values(:name, :level, :is_leaf, :type)");
    }
    else
    {
        q.prepare("insert into labels(name,level,parent,is_leaf,type) values(:name, :level, :parent, :is_leaf, :type)");
    }
    q.bindValue(":name", name);
    q.bindValue(":level", level);
    if (parent != 0)
        q.bindValue(":parent", parent);
    q.bindValue(":is_leaf", (int)is_leaf);
    q.bindValue(":type", type);

    q.exec();
}
