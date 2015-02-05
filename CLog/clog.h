#ifndef CLOG_H
#define CLOG_H


#include <QString>
#include <QMutex>
#include <QList>
#include <QThread>
#include <QObject>
#include <QWaitCondition>

class LogThread;

//日志等级
enum LOG_LEVEL_TYPE
{
    LOG_LEVEL_ERROR = 0,        //错误信息
    LOG_LEVEL_DEBUG,            //debug信息
    LOG_LEVEL_MAX
};


#define log(level, format, ...) CLog::getInstance()->writeLog(level, __FILE__, __LINE__, format, __VA_ARGS__)


#define IP_LEN_MAX  16     //ip地址最长长度
#define FILE_PATH_LEN 256  //文件路径最长长度


//日志类
class CLog : public QObject
{
    Q_OBJECT
private:
    CLog(QObject *parent = 0);
    ~CLog();
public:
    //日志输出模式
    enum LOG_MODE_TYPE
    {
        LOG_MODE_FILE = 0,      //只写文件
        LOG_MODE_FILEANDSOCKET, //既写文件也写socket
        LOG_MODE_MAX
    };

    //日志配置文件内容
    typedef struct LogConfig
    {
        uint         mode;       //日志模式
        uint         debugMode;  //调试模式
        uint         port;       //服务器端口号
        char         ip[IP_LEN_MAX];            //服务器ip地址
        char         outFilePath[FILE_PATH_LEN];//日记文件输出路径
    }LogConfig;

public:
    static CLog *getInstance();// 获得日志类的实例,单例模式
    void writeLog(LOG_LEVEL_TYPE level, const char* _file_, int _line_,const char* format,...);// 日志打印接口，参数可变

    bool writeList(void);  //写信息
public slots:
    void close();
private:
    void init();
    bool parseXml(const char* configxml);   //解析配置文件
    void setFileNamePath(QString &logFileNamePath);//设置日记的文件名和路径
    bool isFileExist(void);                 //判断文件是不是存在
    bool isCreateFile(void);                //创建文件是否成功
    bool isCreateDir(const QString& dir);   //判断路径是否存在

    bool writeLogFile(const QString &msg);  //只写文件
    bool writeLogSocket(const QString &msg);//只写Socket

    void push(const QString &msg);  //压入链表
    QString pop(void);              //出表
private:
    LogConfig m_logConfig;      //config文件内容
    QList<QString> m_list;      //链表
    LogThread *m_workThread;   //线程    
};




//工作线程
class LogThread : public QThread
{
    Q_OBJECT
public:
    explicit LogThread(QObject *parent = 0);
signals:

public slots:

public:
    void run(void);
    void stop(void);
private:
    bool m_stop;
    QMutex  m_mutex;            //互斥量
    QWaitCondition m_cond;
};

#endif // CLOG_H
