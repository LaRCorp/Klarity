#pragma once

#include <QAbstractListModel>
#include <QQmlEngine>
#include <QVector>

class LogModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    enum LogRoles {
        TimestampRole = Qt::UserRole + 1,
        SourceRole,
        LevelRole,
        TextRole
    };
    Q_ENUM(LogRoles)

    struct LogItem {
        QString timestamp;
        QString source;
        QString level; // "info", "warn", "error"
        QString text;
    };

    static LogModel *instance();
    static LogModel *create(QQmlEngine *qmlEngine, QJSEngine *jsEngine);

    explicit LogModel(QObject *parent = nullptr);
    ~LogModel() override = default;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void append(const QString &source, const QString &level, const QString &text);
    Q_INVOKABLE void clear();
    Q_INVOKABLE QString allText() const;

private:
    static LogModel *s_instance;
    QVector<LogItem> m_entries;
    static constexpr int MaxEntries = 5000;
};
