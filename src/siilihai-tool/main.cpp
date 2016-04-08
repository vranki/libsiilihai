#include <QtCore>
#include <QTimer>
#include <QCoreApplication>
#include <QCommandLineParser>
#include "siilihaitool.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    QCoreApplication::setApplicationName("siilihai-tool");
    QCoreApplication::setApplicationVersion("1.0");

    QCommandLineParser parser;
    parser.setApplicationDescription("Siilihai command line tool");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("command", QCoreApplication::translate("main", "Command to execute"));
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

    SiilihaiTool tool;
    if(args.size() < 1) {
        qDebug() << "This is the siilihai command line test tool. Try --help for commands.";
        return 0;
    }
    tool.setNoServer(parser.isSet("noserver"));
    bool quit = false;
    if(args[0] == "list-forums") {
        tool.listForums();
    } else if(args[0] == "get-forum") {
        if(parser.isSet(forumId)) {
            tool.getForum(parser.value(forumId).toInt());
        } else {
            qWarning() << "Enter forum id to get";
            quit = true;
        }
    } else if(args[0] == "probe") {
        if(parser.isSet(forumUrl)) {
            tool.probe(parser.value(forumUrl));
        } else {
            qWarning() << "Enter forum URL to probe with --url";
            quit = true;
        }
    } else if(args[0] == "list-groups") {
        if(parser.isSet(forumUrl)) {
            tool.listGroups(parser.value(forumUrl));
        } else {
            qWarning() << "Enter forum URL to list groups from with  --url";
            quit = true;
        }
    } else if(args[0] == "list-threads") {
        if(parser.isSet(forumUrl) && parser.isSet(groupId)) {
            tool.listThreads(parser.value(forumUrl), parser.value(groupId));
        } else {
            qWarning() << "Enter forum URL and group id to list threads with  --url and --group";
            quit = true;
        }
    } else if(args[0] == "list-messages") {
        if(parser.isSet(forumUrl) && parser.isSet(groupId) && parser.isSet(threadId)) {
            tool.listMessages(parser.value(forumUrl), parser.value(groupId), parser.value(threadId));
        } else {
            qWarning() << "Enter forum URL, group id and thread id to list messages with --url, --group and --thread";
            quit = true;
        }
    }
    return quit ? -1 : a.exec();
}
