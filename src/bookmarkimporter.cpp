#include "bookmarkimporter.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QSet>
#include <QStandardPaths>
#include <QTextStream>
#include <QVariantList>
#include <QtAlgorithms>

namespace {
int childCount(QQmlListProperty<BookmarkNode> *list)
{
    return static_cast<BookmarkNode *>(list->object)->childList().count();
}

BookmarkNode *childAt(QQmlListProperty<BookmarkNode> *list, int index)
{
    return static_cast<BookmarkNode *>(list->object)->childList().at(index);
}

void childAppend(QQmlListProperty<BookmarkNode> *list, BookmarkNode *item)
{
    static_cast<BookmarkNode *>(list->object)->appendChild(item);
}
} // namespace

BookmarkNode::BookmarkNode(QObject *parent)
    : QObject(parent)
{}

QString BookmarkNode::title() const
{
    return m_title;
}

void BookmarkNode::setTitle(const QString &title)
{
    m_title = title;
}

QString BookmarkNode::pathTitle() const
{
    return m_pathTitle;
}

void BookmarkNode::setPathTitle(const QString &pathTitle)
{
    m_pathTitle = pathTitle;
}

QString BookmarkNode::url() const
{
    return m_url;
}

void BookmarkNode::setUrl(const QString &url)
{
    m_url = url;
}

QString BookmarkNode::icon() const
{
    return m_icon;
}

void BookmarkNode::setIcon(const QString &icon)
{
    m_icon = icon;
}

bool BookmarkNode::isFolder() const
{
    return m_isFolder;
}

void BookmarkNode::setIsFolder(bool isFolder)
{
    m_isFolder = isFolder;
}

int BookmarkNode::depth() const
{
    return m_depth;
}

void BookmarkNode::setDepth(int depth)
{
    m_depth = depth;
}

bool BookmarkNode::isExpanded() const
{
    return m_expanded;
}

void BookmarkNode::setExpanded(bool expanded)
{
    if (m_expanded != expanded) {
        m_expanded = expanded;
        emit expandedChanged();
    }
}

int BookmarkNode::checkState() const
{
    return m_checkState;
}

void BookmarkNode::setCheckState(int state)
{
    if (state != Checked && state != Unchecked) {
        return;
    }
    if (state == m_checkState) {
        return;
    }
    cascade(state);
    updateAncestors();
}

QQmlListProperty<BookmarkNode> BookmarkNode::childNodes()
{
    return QQmlListProperty<BookmarkNode>(this, nullptr, childAppend, childCount, childAt, nullptr);
}

void BookmarkNode::appendChild(BookmarkNode *child)
{
    m_children.append(child);
}

QList<BookmarkNode *> BookmarkNode::childList() const
{
    return m_children;
}

void BookmarkNode::setParentNode(BookmarkNode *parent)
{
    m_parent = parent;
}

void BookmarkNode::cascade(int state)
{
    m_checkState = state;
    if (m_isFolder) {
        for (BookmarkNode *child : m_children) {
            child->cascade(state);
        }
    }
    emit checkStateChanged();
}

void BookmarkNode::updateAncestors()
{
    BookmarkNode *node = m_parent;
    while (node) {
        node->recomputeState();
        emit node->checkStateChanged();
        node = node->m_parent;
    }
}

bool BookmarkNode::pruneEmptyFolders()
{
    if (!m_isFolder) {
        return true;
    }

    const QList<BookmarkNode *> children = m_children;
    for (BookmarkNode *child : children) {
        if (!child->pruneEmptyFolders()) {
            m_children.removeAll(child);
            delete child;
        }
    }

    return !m_children.isEmpty();
}

void BookmarkNode::recomputeState()
{
    if (m_children.isEmpty()) {
        return;
    }

    bool anyChecked = false;
    bool anyNotChecked = false;
    for (BookmarkNode *child : m_children) {
        if (child->m_checkState != Unchecked) {
            anyChecked = true;
        }
        if (child->m_checkState != Checked) {
            anyNotChecked = true;
        }
    }
    m_checkState = (anyChecked && anyNotChecked) ? Partial : (anyChecked ? Checked : Unchecked);
}

BookmarkImporter::BookmarkImporter(QObject *parent)
    : QObject(parent)
{}

QVariant BookmarkImporter::parseFile(const QString &filePath)
{
    qDeleteAll(m_nodes);
    m_nodes.clear();

    QFile htmlFile(filePath);
    if (!htmlFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QVariantMap error;
        error.insert(QStringLiteral("error"), tr("Cannot open %1").arg(filePath));
        return error;
    }

    QTextStream in(&htmlFile);
    in.setCodec("UTF-8");
    const QString html = in.readAll();
    htmlFile.close();

    m_nodes = parseNodes(html);

    QVariantList list;
    for (BookmarkNode *node : m_nodes) {
        list.append(QVariant::fromValue(static_cast<QObject *>(node)));
    }
    return list;
}

QVariantMap BookmarkImporter::importSelected()
{
    QVariantMap result;
    result.insert(QStringLiteral("imported"), 0);
    result.insert(QStringLiteral("skipped"), 0);

    QList<Bookmark> selected;
    collectChecked(m_nodes, selected);

    const QString bookmarksFile = bookmarksFilePath();
    if (bookmarksFile.isEmpty()) {
        result.insert(QStringLiteral("error"), tr("Cannot locate the browser data directory"));
        return result;
    }

    const QString dataDir = QFileInfo(bookmarksFile).absolutePath();
    QDir dir(dataDir);
    if (!dir.exists() && !dir.mkpath(dataDir)) {
        result.insert(QStringLiteral("error"), tr("Cannot create %1").arg(dataDir));
        return result;
    }

    backupBookmarks(bookmarksFile);

    QJsonArray array;
    QSet<QString> existingUrls;

    QFile existing(bookmarksFile);
    if (existing.open(QIODevice::ReadOnly | QIODevice::Text)) {
        const QJsonDocument doc = QJsonDocument::fromJson(existing.readAll());
        existing.close();
        if (doc.isArray()) {
            array = doc.array();
            for (const QJsonValue &value : array) {
                if (value.isObject()) {
                    existingUrls.insert(value.toObject().value(QStringLiteral("url")).toString());
                }
            }
        }
    }

    int imported = 0;
    int skipped = 0;

    for (const Bookmark &bookmark : selected) {
        if (bookmark.url.isEmpty() || existingUrls.contains(bookmark.url)) {
            ++skipped;
            continue;
        }

        QJsonObject item;
        item.insert(QStringLiteral("url"), bookmark.url);
        item.insert(QStringLiteral("title"), bookmark.title);
        item.insert(QStringLiteral("favicon"),
                    bookmark.icon.isEmpty() ? QStringLiteral("icon-launcher-bookmark")
                                            : bookmark.icon);
        item.insert(QStringLiteral("hasTouchIcon"), !bookmark.icon.isEmpty());
        array.append(item);
        existingUrls.insert(bookmark.url);
        ++imported;
    }

    QFile out(bookmarksFile);
    if (!out.open(QIODevice::WriteOnly | QIODevice::Text)) {
        result.insert(QStringLiteral("error"), tr("Cannot write %1").arg(bookmarksFile));
        return result;
    }

    QTextStream stream(&out);
    stream.setCodec("UTF-8");
    stream << QJsonDocument(array).toJson();
    out.close();

    result.insert(QStringLiteral("imported"), imported);
    result.insert(QStringLiteral("skipped"), skipped);
    return result;
}

QList<BookmarkNode *> BookmarkImporter::parseNodes(const QString &html)
{
    QList<BookmarkNode *> roots;

    const QRegularExpression tokenRe(QStringLiteral(
                                         "(<h3\\b[^>]*>.*?</h3>)|(<a\\b[^>]*>.*?</a>)|(</dl>)"),
                                     QRegularExpression::CaseInsensitiveOption
                                         | QRegularExpression::DotMatchesEverythingOption);
    const QRegularExpression hrefRe(QStringLiteral("href\\s*=\\s*\"([^\"]*)\""),
                                    QRegularExpression::CaseInsensitiveOption);
    const QRegularExpression iconRe(QStringLiteral("icon\\s*=\\s*\"([^\"]*)\""),
                                    QRegularExpression::CaseInsensitiveOption);
    const QRegularExpression tagRe(QStringLiteral("<[^>]*>"));

    const auto stripTags = [&tagRe](const QString &text) {
        QString out = text;
        out.remove(tagRe);
        return out.trimmed();
    };

    QList<BookmarkNode *> stack;

    const auto fullTitle = [&stack](const QString &name) {
        if (!stack.isEmpty()) {
            return stack.last()->pathTitle() + QStringLiteral(" / ") + name;
        }
        return name;
    };

    const auto addNode = [&](BookmarkNode *node, const QString &name) {
        node->setTitle(name);
        node->setPathTitle(fullTitle(name));
        node->setDepth(stack.count());
        if (!stack.isEmpty()) {
            node->setParentNode(stack.last());
            stack.last()->appendChild(node);
        } else {
            roots.append(node);
        }
    };

    QRegularExpressionMatchIterator it = tokenRe.globalMatch(html);
    while (it.hasNext()) {
        const QRegularExpressionMatch match = it.next();

        if (match.capturedStart(1) != -1) {
            const QString name = stripTags(match.captured(1));
            if (!name.isEmpty()) {
                BookmarkNode *folder = new BookmarkNode(this);
                folder->setIsFolder(true);
                addNode(folder, name);
                stack.append(folder);
            }
        } else if (match.capturedStart(3) != -1) {
            if (!stack.isEmpty()) {
                stack.removeLast();
            }
        } else {
            const QString tag = match.captured(2);

            const QRegularExpressionMatch hrefMatch = hrefRe.match(tag);
            if (!hrefMatch.hasMatch()) {
                continue;
            }

            const QString url = hrefMatch.captured(1);
            if (url.startsWith(QStringLiteral("place:"), Qt::CaseInsensitive)) {
                continue;
            }

            QString icon;
            const QRegularExpressionMatch iconMatch = iconRe.match(tag);
            if (iconMatch.hasMatch()) {
                icon = iconMatch.captured(1);
            }

            const QString title = stripTags(tag);
            if (title.isEmpty()) {
                continue;
            }

            BookmarkNode *bookmark = new BookmarkNode(this);
            bookmark->setIsFolder(false);
            bookmark->setUrl(url);
            bookmark->setIcon(icon);
            addNode(bookmark, title);
        }
    }

    QList<BookmarkNode *> pruned;
    for (BookmarkNode *root : roots) {
        if (root->pruneEmptyFolders()) {
            pruned.append(root);
        } else {
            delete root;
        }
    }

    return pruned;
}

QString BookmarkImporter::bookmarksFilePath() const
{
    const QString dataLocation = QStandardPaths::writableLocation(
        QStandardPaths::GenericDataLocation);
    if (dataLocation.isEmpty()) {
        return QString();
    }
    return dataLocation + QStringLiteral("/org.sailfishos/browser/bookmarks.json");
}

void BookmarkImporter::backupBookmarks(const QString &bookmarksFile) const
{
    if (!QFile::exists(bookmarksFile)) {
        return;
    }

    const QString backupsDir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)
                               + QStringLiteral("/backups");
    QDir dir(backupsDir);
    if (!dir.exists() && !dir.mkpath(backupsDir)) {
        return;
    }

    const QString fileName = QStringLiteral("bookmarks-%1.json")
                                 .arg(QDateTime::currentDateTime().toString(
                                     QStringLiteral("yyyyMMdd-HHmmss")));
    QFile::copy(bookmarksFile, backupsDir + QStringLiteral("/") + fileName);
}

void BookmarkImporter::collectChecked(const QList<BookmarkNode *> &nodes, QList<Bookmark> &out) const
{
    for (BookmarkNode *node : nodes) {
        if (node->isFolder()) {
            collectChecked(node->childList(), out);
        } else if (node->checkState() == BookmarkNode::Checked) {
            Bookmark bookmark;
            bookmark.title = node->pathTitle();
            bookmark.url = node->url();
            bookmark.icon = node->icon();
            out.append(bookmark);
        }
    }
}
