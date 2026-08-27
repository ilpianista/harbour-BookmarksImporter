#pragma once

#include <QList>
#include <QObject>
#include <QQmlListProperty>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <QVariantMap>

class BookmarkNode : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString title READ title CONSTANT)
    Q_PROPERTY(QString pathTitle READ pathTitle CONSTANT)
    Q_PROPERTY(QString url READ url CONSTANT)
    Q_PROPERTY(QString icon READ icon CONSTANT)
    Q_PROPERTY(bool isFolder READ isFolder CONSTANT)
    Q_PROPERTY(int depth READ depth CONSTANT)
    Q_PROPERTY(bool expanded READ isExpanded WRITE setExpanded NOTIFY expandedChanged)
    Q_PROPERTY(int checkState READ checkState WRITE setCheckState NOTIFY checkStateChanged)
    Q_PROPERTY(QQmlListProperty<BookmarkNode> childNodes READ childNodes CONSTANT)

public:
    enum CheckState { Unchecked = 0, Partial = 1, Checked = 2 };
    Q_ENUM(CheckState)

    explicit BookmarkNode(QObject *parent = nullptr);

    QString title() const;
    void setTitle(const QString &title);

    QString pathTitle() const;
    void setPathTitle(const QString &pathTitle);

    QString url() const;
    void setUrl(const QString &url);

    QString icon() const;
    void setIcon(const QString &icon);

    bool isFolder() const;
    void setIsFolder(bool isFolder);

    int depth() const;
    void setDepth(int depth);

    bool isExpanded() const;
    void setExpanded(bool expanded);

    int checkState() const;
    void setCheckState(int state);

    QQmlListProperty<BookmarkNode> childNodes();

    void appendChild(BookmarkNode *child);
    QList<BookmarkNode *> childList() const;
    void setParentNode(BookmarkNode *parent);
    bool pruneEmptyFolders();

signals:
    void expandedChanged();
    void checkStateChanged();

private:
    void cascade(int state);
    void updateAncestors();
    void recomputeState();

    QString m_title;
    QString m_pathTitle;
    QString m_url;
    QString m_icon;
    bool m_isFolder = false;
    int m_depth = 0;
    bool m_expanded = true;
    int m_checkState = Checked;
    QList<BookmarkNode *> m_children;
    BookmarkNode *m_parent = nullptr;
};

class BookmarkImporter : public QObject
{
    Q_OBJECT

public:
    explicit BookmarkImporter(QObject *parent = nullptr);

    Q_INVOKABLE QVariant parseFile(const QString &filePath);
    Q_INVOKABLE QVariantMap importSelected();

private:
    struct Bookmark
    {
        QString title;
        QString url;
        QString icon;
    };

    QList<BookmarkNode *> parseNodes(const QString &html);
    QString bookmarksFilePath() const;
    void backupBookmarks(const QString &bookmarksFile) const;
    void collectChecked(const QList<BookmarkNode *> &nodes, QList<Bookmark> &out) const;

    QList<BookmarkNode *> m_nodes;
};
