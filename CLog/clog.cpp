#include "clog.h"

#include <QApplication>
#include <QDomDocument>

#include <QDate>
#include <QFile>
#include <QUdpSocket>
#include <QDir>

#include <QDebug>

#define MAX_BUFF_LEN 1024

CLog::CLog(QObject *parent):
    QObject(parent)
{
    init();
}

CLog::~CLog()
{
    close();
}

void CLog::init()
{
    memset(&m_logConfig, 0, sizeof(LogConfig));
    m_list.empty();
    //解析失败
    if (!parseXml("config/log.xml"))
    {
       //默认
        m_logConfig.mode = LOG_MODE_FILE;
        m_logConfig.debugMode = 1;

        //路径
        QString path;
        QString logFileNamePath = QCoreApplication::applicationDirPath();
        path = logFileNamePath;
        if(path.lastIndexOf("//",path.length()-1) == -1)
        {
            path.append('/').append("log/");
        }
        else
        {
            path.append("log/");
        }
        //路径存在或者创建成功
        if (!isCreateDir(path))
        {
            path = logFileNamePath;
        }

        setFileNamePath(path);
    }
    //启动线程
    m_workThread = new LogThread(this);
    connect(m_workThread, SIGNAL(finished()), SLOT(deleteLater()));
    m_workThread->start();
}

// 获得日志类的实例
CLog* CLog::getInstance()
{
    static CLog logObject;
    return &logObject;
}

// 日志打印接口，参数可变
void CLog::writeLog(LOG_LEVEL_TYPE level, const char* _file_, int _line_, const char* format,...)
{
     //检查输入
     if (format == NULL || level >= LOG_LEVEL_MAX)
         return;

     //文件存在不需要再创建，不存在则创建log文件
     if (!isFileExist())
     {
         if (!isCreateFile())
         {
            return;
         }
     }

     //写入文件的内容
     QString msg;

     //debug 模式  输出的信息有文件名与行号
     if (m_logConfig.debugMode == 0)
     {
         msg += "File=" + QString(QLatin1String(_file_)) + ",";
         msg += "Line=" + QString::number(_line_, 10) + ",";
     }

     switch (level)
     {
     case LOG_LEVEL_ERROR:
         msg += QString("err_msg:");
         break;
     case LOG_LEVEL_DEBUG:

         msg += QString("debug_msg:");
         break;
     default:
         break;
     }

     //格式化
     char buff[MAX_BUFF_LEN+1] = {0};
     va_list arg;
     va_start(arg, format);
     vsnprintf(buff, MAX_BUFF_LEN, format, arg);
     va_end (arg);

     msg += buff;
     push(msg);
}


//写链表信息
bool CLog::writeList(void)
{
    bool ret = false;
    QString msg;
    QString str = pop();
    if (str.isEmpty())
        return false;
    //日期
    msg = "[" + QDateTime::currentDateTime().toString("hh:mm:ss").trimmed() + "]";
    msg += str;

    //写入文件
    switch(m_logConfig.mode)
    {
    case LOG_MODE_FILE:
        ret = writeLogFile(msg);
        break;
    case LOG_MODE_FILEANDSOCKET:
        ret = writeLogFile(msg);
        ret = ret | writeLogSocket(msg);
        break;
     default:
        break;
    }

    return ret;
}

//只写文件
bool CLog::writeLogFile(const QString &msg)
{
    QString fileName = QString(QLatin1String(m_logConfig.outFilePath));
    QFile file(fileName);
    //追加模式写文件
    if (!file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text))
    {
        return false;
    }

    QTextStream out(&file);
    out<<msg<<endl;
    file.close();
    return true;
}

//只写Socket
bool CLog::writeLogSocket(const QString &msg)
{
    bool ret;
    QString ipAddr = QString(QLatin1String(m_logConfig.ip));
     //UDP 发送数据
    QUdpSocket udpSocket;
    QByteArray arry = msg.toUtf8();
    qint64 len = udpSocket.writeDatagram(arry, arry.size(), QHostAddress(ipAddr), m_logConfig.port);
    if (len == -1)
        ret = false;

    return ret;
}


//解析配置文件
bool CLog::parseXml(const char* configxml)
{

    //读取配置文件
    QFile file(configxml);
    if (!file.open(QIODevice::ReadOnly))
        return false;
    QDomDocument doc;
    if (!doc.setContent(&file, false))
    {
        file.close();
        return false;
    }
    file.close();
    QDomElement root = doc.documentElement();
    if (root.tagName() != "LogConfig")
    {
        return false;
    }

    QDomNode child = root.firstChild();
    while (!child.isNull()) //解析LogConfig字段下内容
    {
        if (child.toElement().tagName() == "logMode")
        {
           m_logConfig.mode = child.toElement().text().toInt();
        }
        else if(child.toElement().tagName() == "debugMode")
        {
            m_logConfig.debugMode = child.toElement().text().toInt();
        }
        else if(child.toElement().tagName() == "path")
        {
            QString path = child.toElement().text();
            if(path.isEmpty())
            {
                path = QString("/log");
            }
            //路径存在或者创建成功
            if (!isCreateDir(path))
            {
                QString logFileNamePath = QCoreApplication::applicationDirPath();
                path = logFileNamePath;
                if(path.lastIndexOf("//",path.length()-1) == -1)
                {
                    path.append('/').append("log/");
                }
                else
                {
                    path.append("log/");
                }
                //路径存在或者创建成功
                if (!isCreateDir(path))
                {
                    path = logFileNamePath;
                }
            }
            setFileNamePath(path);
        }
        else if(child.toElement().tagName() == "server")
        {
            QDomNode childServer = child.toElement().firstChild();
            while (!childServer.isNull()) //解析server字段下内容
            {
                if (childServer.toElement().tagName() == "ip")
                {
                    QByteArray str = childServer.toElement().text().toUtf8();
                    strcpy (m_logConfig.ip, str.data());
                }
                else if(childServer.toElement().tagName() == "port")
                {
                    m_logConfig.port = childServer.toElement().text().toInt();
                }
                childServer = childServer.nextSibling();
            }
        }

        child = child.nextSibling();
    }

    return true;
}


//设置日记的文件名和路径
void CLog::setFileNamePath(QString &logFileNamePath)
{
    QString fileName = QDateTime::currentDateTime().toString("yyyy-MM-dd");  //日期
    fileName.append(".log");

    if(logFileNamePath.lastIndexOf("//",logFileNamePath.length()-1) == -1)
    {
        logFileNamePath.append('/').append(fileName);
    }
    else
    {
        logFileNamePath.append(fileName);
    }

    QByteArray str = logFileNamePath.toUtf8();
    strcpy (m_logConfig.outFilePath , str.data());
}


//判断文件是不是存在
bool CLog::isFileExist(void)
{
    bool ret;
    QString fileName = QString(QLatin1String(m_logConfig.outFilePath));
    QFile logFile(fileName);
    ret = logFile.exists();
    return ret;
}

//创建文件是否成功
bool CLog::isCreateFile(void)
{
    bool ret;
    QString fileName = QString(QLatin1String(m_logConfig.outFilePath));
    QFile logFile(fileName);
    ret = logFile.open(QIODevice::WriteOnly);
    //文件创建失败
    if (!ret)
    {
        QString fileName = QDateTime::currentDateTime().toString("yyyy-MM-dd");  //日期
        fileName.append(".log");
        logFile.setFileName(fileName);
        ret = logFile.open(QIODevice::WriteOnly);
    }
    logFile.close();
    return ret;
}

//判断路径是否存在
bool CLog::isCreateDir(const QString& dir)
{
    bool ret = true;
    QDir path(dir);
    if (!path.exists())
    {
        ret = path.mkpath(dir);
    }
    return ret;
}

 //压入链表
void CLog::push(const QString &msg)
{
    m_list.push_back(msg);
}

//出表
QString CLog::pop(void)
{
    QString str;

    str.clear();
    if (!m_list.isEmpty())
    {
        str = m_list.takeFirst();
    }
    return str;
}


//关闭日志
void CLog::close()
{
    m_workThread->stop();
}


LogThread::LogThread(QObject *parent) :
    QThread(parent),
    m_stop(false)
{

}


void LogThread::run()
{
    bool ret;
    while (!m_stop)
    {
        m_mutex.lock();
        ret = CLog::getInstance()->writeList();

        if (ret)
        {
            msleep(10);
        }
        else
        {
            msleep(20);
        }
        m_mutex.unlock();
    }

    //写完列表中所有log信息
    while(CLog::getInstance()->writeList());
}

void LogThread::stop(void)
{
    m_mutex.lock();
    m_stop = true;
    m_mutex.unlock();
    wait();
}
