#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>
#include <QProcess>
#include <QTimer>
#include <QMimeDatabase>

class SearchManager : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QVariantList results READ results NOTIFY resultsChanged)
    Q_PROPERTY(int resultCount READ resultCount NOTIFY resultCountChanged)
    Q_PROPERTY(bool isSearching READ isSearching NOTIFY isSearchingChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)
    Q_PROPERTY(QStringList searchHistory READ searchHistory NOTIFY searchHistoryChanged)
    Q_PROPERTY(bool hasFd READ hasFd CONSTANT)
    Q_PROPERTY(bool hasRg READ hasRg CONSTANT)

public:
    explicit SearchManager(QObject *parent = nullptr);
    ~SearchManager() override;

    QVariantList results() const { return m_results; }
    int resultCount() const { return m_results.size(); }
    bool isSearching() const { return m_isSearching; }
    QString statusMessage() const { return m_statusMessage; }
    QStringList searchHistory() const { return m_searchHistory; }
    bool hasFd() const { return m_hasFd; }
    bool hasRg() const { return m_hasRg; }

    Q_INVOKABLE void search(const QString &query,
                            const QString &scope = QStringLiteral("home"),
                            const QString &typeFilter = QStringLiteral("all"),
                            int maxDepth = 6,
                            bool searchContent = false,
                            bool includeHidden = false);
    Q_INVOKABLE void cancelSearch();
    Q_INVOKABLE void openFile(const QString &path);
    Q_INVOKABLE void openContainingFolder(const QString &path);
    Q_INVOKABLE void copyToClipboard(const QString &text);
    Q_INVOKABLE void commitSearchHistory(const QString &query);
    Q_INVOKABLE void clearHistory();
    Q_INVOKABLE QStringList getSuggestions(const QString &partialQuery) const;

signals:
    void resultsChanged();
    void resultCountChanged();
    void isSearchingChanged();
    void statusMessageChanged();
    void searchHistoryChanged();
    void operationFinished(bool success, const QString &message);

private slots:
    void onReadyReadStandardOutput();
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void flushPendingResults();

private:
    void parseAndExecuteQuery(const QString &query,
                              const QString &scope,
                              const QString &typeFilter,
                              int maxDepth,
                              bool searchContent,
                              bool includeHidden);
    void addResultPath(const QString &path);
    void saveHistory(const QString &query);
    void loadHistory();
    static QString formatSize(qint64 bytes);

    QProcess *m_process = nullptr;
    QByteArray m_outputBuffer;
    QVariantList m_results;
    QVariantList m_pendingResults;
    QTimer *m_batchTimer = nullptr;
    QTimer *m_historyTimer = nullptr;
    QString m_pendingHistoryQuery;

    bool m_isSearching = false;
    QString m_statusMessage;
    QStringList m_searchHistory;
    bool m_hasFd = false;
    bool m_hasRg = false;
    QMimeDatabase m_mimeDb;

    int m_maxResultsLimit = 1000;
};
