/* This file is part of libSiilihai.

    libSiilihai is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    libSiilihai is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with libSiilihai.  If not, see <http://www.gnu.org/licenses/>. */

#include "libsiilihaitests.h"
#include "siilihai/tapatalk/forumsubscriptiontapatalk.h"
#include "siilihai/forumdata/forumgroup.h"
#include "siilihai/forumdata/forumthread.h"
#include "siilihai/forumdata/forummessage.h"

LibSiilihaiTests::LibSiilihaiTests(QObject *parent) :
    QObject(parent), fdb(parent) {
}

LibSiilihaiTests::~LibSiilihaiTests() {
}

void LibSiilihaiTests::runTests() {
    runForumDatabaseTests();
    return;

    QString
            html =
            "<body><test>foo%%foo</test>\n\tkekekee<num>123&x=y</num><test>keke</test>kekekee<num>666</num></body>";
    QString pattern = "<test>%a<%i<num>%B<";

    PatternMatcher pm(this);
    qDebug() << "begin test section:";
    QString kek;
    /*
        QVERIFY(pm.isTag(kek="%a"));
        QVERIFY(!pm.isTag(kek="%%"));
        QVERIFY(!pm.isTag(kek="%"));
        QVERIFY(!pm.isTag(kek=""));
        QVERIFY(!pm.isTag(kek="zttyztzyzy"));
        QVERIFY(!pm.isTag(kek="a%"));
        QVERIFY(pm.isNumberTag(kek="%A"));
        QVERIFY(!pm.isNumberTag(kek="%!"));
        QVERIFY(!pm.isNumberTag(kek="%9"));
        QVERIFY(pm.numberize(kek="aazhzh6j6xj6!¤!#¤#\"")=="6");
        QVERIFY(pm.numberize(kek="#&#/#%&/#%/#%/#/#%")=="");
        QVERIFY(pm.numberize(kek="123456789")=="123456789");
        QVERIFY(pm.numberize(kek="123&456789")=="123");
        QVERIFY(pm.tokenizePattern("%a%a").length()==0);
        QVERIFY(pm.tokenizePattern("%ax%a").length()==0);
        QVERIFY(pm.tokenizePattern("x%ax%ax").length()==5);
        */
    qDebug() << "end test section";
    /*
         QList<QString> patternTokens = pm.tokenizePattern(pattern);
         qDebug() << "Pattern " << pattern << "\n tokenized:";
         if (patternTokens.length() == 0) {
         qDebug() << "invalid pattern!!";
         } else {
         for (int i = 0; i < patternTokens.length(); i++) {
         qDebug() << "\t" << patternTokens[i];
         }
         }
         */
    pm.setPattern(pattern);
    QList<QHash<QString, QString> > matches = pm.findMatches(html);
    for (int i = 0; i < matches.length(); i++) {
        qDebug() << "Match:";
        QHash<QString, QString> matchHash = matches[i];
        QHashIterator<QString, QString> iter(matchHash);
        while (iter.hasNext()) {
            iter.next();
            qDebug() << "\t" << iter.key() << ": " << iter.value();
        }
    }

    /*
         pattern = "<test>%a<%i<num>%%%B<";
         pm.findMatches(html, pattern);
         pattern = "<test>%a<%i\n<num>\n%%%B<%";
         pm.findMatches(html, pattern);
         */

}

void LibSiilihaiTests::runForumDatabaseTests() {
    QString dbfile = "siilihai-test.db";
    qDebug() << "Hello, using database " << dbfile;
    fdb.openDatabase(dbfile, false);

    ForumSubscriptionTapaTalk fs(this, false);
    fs.setAlias("Test Subscription");
    fs.setLatestMessages(11);
    fs.setLatestThreads(11);
    Q_ASSERT(fdb.addSubscription(&fs));
    ForumGroup fg(&fs, false);
    fg.setId("G666");
    fg.setName("Test group");
    fg.setSubscribed(true);
    fs.addGroup(&fg);
    ForumThread ft(&fg, false);
    ft.setId("T1000");
    ft.setName("Test Thread");
    ft.setOrdernum(0);
    fg.addThread(&ft);
    ForumMessage fm(&ft);
    fm.setId("M000");
    fm.setName("Test Message");
    fm.setAuthor("Tester");
    fm.setRead(false);
    ft.addMessage(&fm);

    fdb.checkSanity();

    Q_ASSERT(fdb.size() == 1);
    Q_ASSERT(fg.size()==1);
    Q_ASSERT(ft.size()==1);
    Q_ASSERT(ft.unreadCount()==1);

    qDebug() << "===Listing db contents====";

    foreach (ForumSubscription* fs, fdb.values()) {
        qDebug() << "FS: " << fs->toString() << " @ " << fs;
        foreach (ForumGroup* fg, fs->values()) {
            qDebug() << "\tFG: " << fg->toString() << " @ " << fg;
            foreach (ForumThread* ft, fg->values()) {
                qDebug() << "\t\tFT: " << ft->toString() << " @ " << ft;
                foreach (ForumMessage* fm, ft->values()) {
                    qDebug() << "\t\t\tFM: " << fm->toString() << " @ " << fm;
                }
            }
        }
    }
    qDebug() << "=== End list ====";

    fm.setRead(true);

    Q_ASSERT(ft.unreadCount()==0);
    Q_ASSERT(fs.unreadCount()==0);

    fm.setRead(false);

    Q_ASSERT(ft.unreadCount()==1);
    Q_ASSERT(fs.unreadCount()==1);

    fg.setSubscribed(false);

    Q_ASSERT(ft.unreadCount()==0);
    Q_ASSERT(fs.unreadCount()==0);

    ft.removeMessage(&fm);

    Q_ASSERT(ft.unreadCount()==0);
    Q_ASSERT(fs.unreadCount()==0);

    qDebug() << "=== Deleting forums ====";
    foreach(ForumSubscription *fs, fdb.values()) {
        fdb.deleteSubscription(fs);
    }
    qDebug() << "=== End delete ====";
}


void LibSiilihaiTests::groupFound(ForumGroup *grp) {
    qDebug() << Q_FUNC_INFO;
}
