#include <QtCore>
#include <QTimer>
#include <QCoreApplication>
#include <QCommandLineParser>
#include <QtGlobal>
#include "siilihaitool.h"

// Output qInfo's in stdout:
void messageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    Q_UNUSED(context);
    Q_UNUSED(type);
    // QTextStream outstream(type == QtInfoMsg ? stdout : stderr, QIODevice::WriteOnly);
    QTextStream outstream(stdout, QIODevice::WriteOnly);
    outstream << msg << endl;
}

int main(int argc, char *argv[])
{
    qInstallMessageHandler(messageOutput);
    QCoreApplication a(argc, argv);
    QCoreApplication::setApplicationName("siilihai-tool");
    QCoreApplication::setApplicationVersion("1.0");

    QCommandLineParser parser;
    parser.setApplicationDescription("Siilihai command line tool");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("command",
                                 QCoreApplication::translate("main", "One of: list-forums, get-forum, probe, list-groups, list-threads, list-messages, update-forum"));
    QCommandLineOption forumId(QStringList() << "forumid",
                               QCoreApplication::translate("main", "Forum id"),
                               QCoreApplication::translate("main", "integer value"));
    parser.addOption(forumId);
    QCommandLineOption forumUrl(QStringList() << "url",
                                QCoreApplication::translate("main", "Forum URL"),
                                QCoreApplication::translate("main", "string value"));
    parser.addOption(forumUrl);
    QCommandLineOption groupId(QStringList() << "group",
                               QCoreApplication::translate("main", "Group ID"),
                               QCoreApplication::translate("main", "string value"));
    parser.addOption(groupId);
    QCommandLineOption threadId(QStringList() << "thread",
                                QCoreApplication::translate("main", "Thread ID"),
                                QCoreApplication::translate("main", "string value"));
    parser.addOption(threadId);
    QCommandLineOption noServerOption(QStringList() << "noserver", "Don't ask server when probing");
    parser.addOption(noServerOption);
    parser.process(a);
    const QStringList args = parser.positionalArguments();

    if(args.isEmpty()) {
        qInfo() << "This is the siilihai command line test tool. Try --help for commands.";
        return 0;
    }
    SiilihaiTool tool;
    tool.setNoServer(parser.isSet(noServerOption));

    if(parser.isSet(forumUrl)) tool.setForumUrl(QUrl(parser.value(forumUrl)));
    if(parser.isSet(forumId)) tool.setForumId(parser.value(forumId).toInt());

    bool quit = true;
    if(args[0] == "list-forums") {
        quit = false;
        tool.listForums();
    } else if(args[0] == "get-forum") {
        if(parser.isSet(forumId)) {
            quit = false;
            tool.getForum(parser.value(forumId).toInt());
        } else {
            qWarning() << "Enter forum id to get";
        }
    } else if(args[0] == "probe") {
        quit = false;
        tool.probe();
    } else if(args[0] == "list-groups") {
        quit = false;
        tool.listGroups();
    } else if(args[0] == "list-threads") {
        if(parser.isSet(groupId)) {
            quit = false;
            tool.listThreads(parser.value(groupId));
        } else {
            qWarning() << "Enter group id to list threads with --group";
        }
    } else if(args[0] == "list-messages") {
        if(parser.isSet(groupId) && parser.isSet(threadId)) {
            quit = false;
            tool.listMessages(parser.value(groupId), parser.value(threadId));
        } else {
            qWarning() << "Enter group id and thread id to list messages with --group and --thread";
        }
    } else if(args[0] == "update-forum") {
        if(parser.isSet(forumId) || parser.isSet(forumUrl)) {
            quit = false;
            tool.updateForum();
        } else {
            qWarning() << "Enter forum id or url to update";
        }
    }
    if(quit) {
        return -1;
    } else {
        a.exec();
        return tool.success() ? 0 : -1;
    }
}
