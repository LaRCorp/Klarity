#include "LogModel.h"

#include <QDateTime>

LogModel *LogModel::s_instance = nullptr;

LogModel *LogModel::instance()
{
    if (!s_instance) {
        s_instance = new LogModel();
    }
    return s_instance;
}

LogModel *LogModel::create(QQmlEngine *qmlEngine, QJSEngine *jsEngine)
{
    Q_UNUSED(qmlEngine)
    Q_UNUSED(jsEngine)
    return instance();
}

LogModel::LogModel(QObject *parent)
    : QAbstractListModel(parent)
{
    if (!s_instance) {
        s_instance = this;
    }
    m_entries.reserve(100);
}

int LogModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return static_cast<int>(m_entries.size());
}

QVariant LogModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_entries.size()) {
        return {};
    }

    const auto &item = m_entries.at(index.row());
    switch (role) {
    case TimestampRole:
        return item.timestamp;
    case SourceRole:
        return item.source;
    case LevelRole:
        return item.level;
    case TextRole:
        return item.text;
    default:
        return {};
    }
}

QHash<int, QByteArray> LogModel::roleNames() const
{
    return {
        {TimestampRole, "timestamp"},
        {SourceRole, "source"},
        {LevelRole, "level"},
        {TextRole, "text"}
    };
}

void LogModel::append(const QString &source, const QString &level, const QString &text)
{
    const QString timestamp = QDateTime::currentDateTime().toString(QStringLiteral("hh:mm:ss.zzz"));

    if (m_entries.size() >= MaxEntries) {
        beginRemoveRows(QModelIndex(), 0, 0);
        m_entries.removeFirst();
        endRemoveRows();
    }

    const int newRow = static_cast<int>(m_entries.size());
    beginInsertRows(QModelIndex(), newRow, newRow);
    m_entries.append({timestamp, source, level, text});
    endInsertRows();
}

void LogModel::clear()
{
    if (m_entries.isEmpty()) {
        return;
    }
    beginResetModel();
    m_entries.clear();
    endResetModel();
}

QString LogModel::allText() const
{
    QString result;
    result.reserve(m_entries.size() * 80);
    for (const auto &item : m_entries) {
        result += QStringLiteral("[%1] [%2] [%3]: %4\n")
                      .arg(item.timestamp, item.level.toUpper(), item.source, item.text);
    }
    return result;
}
